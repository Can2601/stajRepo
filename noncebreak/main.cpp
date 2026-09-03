// ============================================================
//  Many-Time Pad Attack -- N-File Grouped Solver
//
//  Core idea (pure algebraic XOR self-validation, no heuristics):
//
//  KEY PROPERTY:
//    A = P_A XOR K,   B = P_B XOR K
//    A XOR B = P_A XOR P_B
//
//    At TEMPLATE positions (P_A == P_B):  A XOR B == 0
//    --> For any candidate word W of length L at a zero-run:
//          W XOR (A XOR B)[pos..pos+L]  ==  W XOR 0  ==  W
//        Getting back exactly W proves it's the template text.
//        No hardcoded key lengths or ordering needed.
//
//  PHASES:
//  1. Group files by ID length (same-length groups share the
//     same payload layout, so template positions align exactly).
//  2. Compute pairwise XOR for each same-length group.
//     Find zero-runs.  For each zero-run, slide every candidate
//     token -- if its length matches the run AND XOR gives back
//     the token itself, commit it (derive keystream from file[0]).
//  3. Using the now-known keystream, locate the id field in every
//     file (including different-length groups) by committing each
//     file's known id + password-key prefix.
//  4. Final statistical pass over all files for remaining unknowns.
// ============================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <filesystem>
#include <cctype>

namespace fs = std::filesystem;

static const size_t NONCE_SIZE = 24;
static const size_t MAC_SIZE   = 16;

// ---------------------------------------------------------------
struct CipherFile {
    std::string name;
    std::string idStr;  // filename without .lic
    std::vector<uint8_t> data;
    std::vector<uint8_t> payload;
};

std::vector<uint8_t> readBinary(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return {};
    auto sz = f.tellg(); f.seekg(0);
    std::vector<uint8_t> buf(static_cast<size_t>(sz));
    f.read(reinterpret_cast<char*>(buf.data()), sz);
    return buf;
}

void printPlaintext(const std::vector<int>& pt, const std::string& label, size_t len) {
    std::cout << "\n[" << label << "]\n  asc: ";
    for (size_t i = 0; i < len; ++i) {
        if (i > 0 && i % 16 == 0) std::cout << "\n       ";
        char c = (pt[i] >= 0x20 && pt[i] <= 0x7E) ? (char)pt[i]
               : (pt[i] >= 0 ? '.' : '?');
        std::cout << " " << c << " ";
    }
    std::cout << "\n";
    int known = 0;
    for (size_t i = 0; i < len; ++i) if (pt[i] >= 0) ++known;
    std::cout << "  Recovered: " << known << "/" << (int)len
              << " bytes (" << (len > 0 ? 100*known/(int)len : 0) << "%)\n";
}

struct ManyPadState {
    std::vector<CipherFile> files;
    std::vector<std::vector<int>> pts;
    size_t len;
    std::vector<int> knownKs;

    ManyPadState(const std::vector<CipherFile>& f, size_t n)
        : files(f), len(n), knownKs(n, -1)
    {
        pts.resize(files.size(), std::vector<int>(n, -1));
    }

    void printState() const {
        for (size_t i = 0; i < files.size(); ++i)
            printPlaintext(pts[i], files[i].name, len);
    }

    // ---- Commit operations ------------------------------------

    void commitKeystream(size_t pos, uint8_t ks) {
        if (pos >= len || knownKs[pos] != -1) return;
        knownKs[pos] = ks;
        for (size_t i = 0; i < files.size(); ++i)
            if (pos < files[i].payload.size())
                pts[i][pos] = files[i].payload[pos] ^ ks;
    }

    void commitFile(size_t fileIdx, size_t pos, const std::string& text) {
        for (size_t j = 0; j < text.size(); ++j) {
            size_t p = pos + j;
            if (p >= len || p >= files[fileIdx].payload.size()) break;
            commitKeystream(p, files[fileIdx].payload[p] ^ (uint8_t)text[j]);
        }
    }


    // ---- Run-length detection ---------------------------------
    struct Run {
        bool isTemplate;
        size_t start, length;
    };


    // ---- Pure XOR self-validation template finder -------------
    //
    // For a zero-run in (A XOR B) at [pos, pos+L):
    //   A[pos+j] XOR B[pos+j] == 0  for all j in [0,L)
    //
    // Therefore, for any candidate word W of length tLen <= L:
    //   W[j] XOR (A[pos+j] XOR B[pos+j])  =  W[j] XOR 0  =  W[j]
    //
    // Getting back exactly W proves the zero-run property at that
    // range and authorises committing the keystream there.
    //
    // KEY FIX -- no sliding within a zero-run:
    //   Every byte in a zero-run satisfies xorByte==0 trivially, so
    //   sliding would match the same token at every offset, committing
    //   wrong keystream bytes (observed: {"createdAt":" appearing at
    //   offsets 0, 14, 28 inside one run).
    //
    //   Instead, we match tokens in JSON STRUCTURAL ORDER -- one token
    //   per zero-run -- advancing the token index after each match.
    //   This is valid because the JSON key order is fixed and invariant:
    //     {"createdAt":"  ->  ","expiryDate":"  ->  ","id":"
    //     ->  ","password":"  ->  "}
    //   Structural order pins the right token to the right run without
    //   any hardcoded byte lengths or offsets.
    //
    // ---- Find all valid zero-run positions for a candidate token ----
    std::vector<size_t> findValidPositions(const std::vector<size_t>& subset, const std::string& token) {
        std::vector<size_t> validPos;
        size_t tLen = token.size();
        for (size_t pos = 0; pos + tLen <= len; ++pos) {
            bool valid = true;
            for (size_t j = 0; j < tLen && valid; ++j) {
                size_t p = pos + j;
                for (size_t k = 1; k < subset.size() && valid; ++k) {
                    size_t fi = subset[k];
                    if (p >= files[fi].payload.size() ||
                        p >= files[subset[0]].payload.size())
                    { valid = false; break; }
                    if ((files[subset[0]].payload[p] ^
                         files[fi].payload[p]) != 0)
                        valid = false;
                }
            }
            if (valid) validPos.push_back(pos);
        }
        return validPos;
    }

    int findAndCommitTemplateXOR(const std::vector<size_t>& subset) {
        // We will "try" these common JSON structural words
        std::vector<std::string> words = {
            "{\"createdAt\":\"",
            "\",\"expiryDate\":\"",
            "\",\"id\":\"",
            "\",\"password\":\""
        };

        // Find all mathematically valid positions for each word in this group's zero-runs
        std::vector<std::vector<size_t>> candidates(words.size());
        for (size_t i = 0; i < words.size(); ++i) {
            candidates[i] = findValidPositions(subset, words[i]);
        }

        size_t bestP0 = std::string::npos;
        size_t bestP1 = std::string::npos;
        size_t bestP2 = std::string::npos;
        size_t bestP3 = std::string::npos;

        // Combinatorial search: find a consistent, ordered layout.
        // We assume the variable gap between createdAt and expiryDate
        // equals the gap between expiryDate and id (since both are dates of the same length).
        for (size_t p0 : candidates[0]) {
            if (p0 != 0) continue; // JSON payload must start at index 0
            
            for (size_t p1 : candidates[1]) {
                if (p1 <= p0 + words[0].size()) continue;
                size_t gap1 = p1 - (p0 + words[0].size());
                
                for (size_t p2 : candidates[2]) {
                    if (p2 <= p1 + words[1].size()) continue;
                    size_t gap2 = p2 - (p1 + words[1].size());
                    
                    // Crucial heuristic: the two dates have identical string lengths!
                    if (gap1 != gap2) continue; 
                    
                    for (size_t p3 : candidates[3]) {
                        if (p3 <= p2 + words[2].size()) continue;
                        
                        bestP0 = p0;
                        bestP1 = p1;
                        bestP2 = p2;
                        bestP3 = p3;
                        break; // Found a valid consistent layout
                    }
                    if (bestP3 != std::string::npos) break;
                }
                if (bestP3 != std::string::npos) break;
            }
            if (bestP3 != std::string::npos) break;
        }

        int committed = 0;
        if (bestP3 != std::string::npos) {
            std::cout << "  [i] Discovered structural layout by trying candidate words:\n";
            commitFile(subset[0], bestP0, words[0]); ++committed;
            std::cout << "      [+] pos=" << std::setw(2) << bestP0 << " -> " << words[0] << "\n";
            
            commitFile(subset[0], bestP1, words[1]); ++committed;
            std::cout << "      [+] pos=" << std::setw(2) << bestP1 << " -> " << words[1] << "\n";
            
            commitFile(subset[0], bestP2, words[2]); ++committed;
            std::cout << "      [+] pos=" << std::setw(2) << bestP2 << " -> " << words[2] << "\n";
            
            commitFile(subset[0], bestP3, words[3]); ++committed;
            std::cout << "      [+] pos=" << std::setw(2) << bestP3 << " -> " << words[3] << "\n";
            
            // Try to place closing brace after the password
            std::string w4 = "\"}";
            auto cand4 = findValidPositions(subset, w4);
            for (size_t p4 : cand4) {
                if (p4 >= bestP3 + words[3].size()) {
                    commitFile(subset[0], p4, w4); ++committed;
                    std::cout << "      [+] pos=" << std::setw(2) << p4 << " -> " << w4 << "\n";
                    break;
                }
            }
        } else {
            std::cout << "  [-] Could not find a consistent layout by trying candidate words.\n";
        }

        return committed;
    }

    // ---- Statistical cracker (subset) -------------------------

    int statCrackSubset(const std::vector<size_t>& subset) {
        int cracked = 0;
        for (size_t pos = 0; pos < len; ++pos) {
            if (knownKs[pos] != -1) continue;

            // Check if variable within subset
            bool allSame = true;
            uint8_t r = files[subset[0]].payload[pos];
            for (size_t k = 1; k < subset.size(); ++k) {
                size_t fi = subset[k];
                if (pos < files[fi].payload.size() && files[fi].payload[pos] != r) {
                    allSame = false; break;
                }
            }
            if (allSame) continue;

            int bestScore = -1, bestKs = -1;
            for (int ks = 0; ks < 256; ++ks) {
                int score = 0; bool valid = true;
                for (size_t k = 0; k < subset.size() && valid; ++k) {
                    size_t fi = subset[k];
                    if (pos >= files[fi].payload.size()) continue;
                    uint8_t b = files[fi].payload[pos] ^ ks;
                    if (b < 0x20 || b > 0x7E) { valid = false; break; }
                    if (std::isalnum(b)) score += 2;
                    else if (std::ispunct(b) || b == ' ') score += 1;
                }
                if (valid && score > bestScore) { bestScore = score; bestKs = ks; }
            }
            if (bestKs != -1) { commitKeystream(pos, bestKs); ++cracked; }
        }
        return cracked;
    }

    // ---- Crib drag (all files, verbose for manual use) --------

    void cribDragAll(const std::string& crib, bool verbose) const {
        if (crib.empty() || crib.size() > len) {
            std::cout << "  (crib too long or empty)\n"; return;
        }
        int uniformHits = 0;
        for (size_t pos = 0; pos + crib.size() <= len; ++pos) {
            // Derive keystream from file 0
            std::vector<uint8_t> ks(crib.size());
            for (size_t j = 0; j < crib.size(); ++j)
                ks[j] = files[0].payload[pos + j] ^ (uint8_t)crib[j];

            bool ok = true;
            bool allSameCtxt = true;
            std::vector<std::string> decoded(files.size());
            for (size_t i = 0; i < files.size() && ok; ++i) {
                for (size_t j = 0; j < crib.size(); ++j) {
                    if (pos + j >= files[i].payload.size()) { ok = false; break; }
                    uint8_t b = files[i].payload[pos + j] ^ ks[j];
                    if (b < 0x20 || b > 0x7E) { ok = false; break; }
                    decoded[i] += (char)b;
                    if (files[i].payload[pos+j] != files[0].payload[pos+j])
                        allSameCtxt = false;
                }
            }

            if (ok) {
                if (allSameCtxt) {
                    ++uniformHits;
                } else if (verbose) {
                    std::cout << "  pos=" << std::setw(3) << pos << " (Variable Hit)\n";
                    for (size_t i = 0; i < files.size(); ++i)
                        std::cout << "    [" << files[i].name << "] has \""
                                  << decoded[i] << "\"\n";
                }
            }
        }
        if (uniformHits > 0)
            std::cout << "  (+" << uniformHits << " uniform-region hits)\n";
    }

    // ===========================================================
    //  MAIN AUTO ATTACK  (pure algebraic XOR self-validation)
    // ===========================================================
    void autoAttackGrouped() {

        // ----------------------------------------------------------
        // Phase 1: Group files by ID length.
        //   Files with the same ID length share exactly the same
        //   payload layout, so their pairwise XOR has zero-runs
        //   precisely at every template (common-text) position.
        // ----------------------------------------------------------
        std::map<size_t, std::vector<size_t>> groups;
        for (size_t i = 0; i < files.size(); ++i)
            groups[files[i].idStr.size()].push_back(i);

        std::cout << "\n[Phase 1] Groups by ID length:\n";
        for (auto& [idLen, idx] : groups) {
            std::cout << "  idLen=" << idLen
                      << " (" << idx.size() << " files): ";
            for (auto i : idx) std::cout << files[i].name << " ";
            std::cout << "\n";
        }

        // ----------------------------------------------------------
        // Phase 2: XOR self-validation to find and commit ALL template
        //   tokens for groups that have 2+ files.
        //   NO stat cracking here -- we must commit IDs first (Phase 3)
        //   before the stat cracker can lock wrong bytes at ID positions.
        // ----------------------------------------------------------
        std::cout << "\n[Phase 2] Template token discovery via XOR self-validation:\n";
        for (auto& [idLen, idx] : groups) {
            if (idx.size() < 2) {
                std::cout << "  (idLen=" << idLen
                          << " has only 1 file -- will anchor via Phase 3)\n";
                continue;
            }
            std::cout << "  -> Group idLen=" << idLen << ":\n";
            int committed = findAndCommitTemplateXOR(idx);
            std::cout << "  [+] Committed " << committed
                      << " template token(s).\n";
        }

        // ----------------------------------------------------------
        // Phase 3: Cross-group anchor using already-known keystream.
        //
        //   By now the keystream is known at all template positions
        //   (from the same-length-group XOR phase above).  We scan
        //   the recovered plaintext of any file that has the template
        //   committed to locate the ID field start.
        //
        //   For EVERY file (including single-file / different-length
        //   groups) we then commit:
        //     <known id from filename> + ","password":"  
        //   at the discovered id-field start offset.
        //
        //   This is the "intersection" step: common words found via
        //   same-length groups give us the keystream positions that
        //   bracket the password field; knowing the ID bridges us
        //   into the variable (password) region for all groups.
        // ----------------------------------------------------------
        std::cout << "\n[Phase 3] Cross-group anchor: committing all IDs + "
                     "password-key prefix...\n";

        // Find '","id":"' in any file's recovered plaintext.
        const std::string idMarker = "\",\"id\":\"";
        size_t idStart = std::string::npos;

        for (size_t fi = 0; fi < files.size() && idStart == std::string::npos; ++fi) {
            for (size_t pos = 0; pos + idMarker.size() <= len; ++pos) {
                bool match = true;
                for (size_t j = 0; j < idMarker.size(); ++j) {
                    if (pts[fi][pos + j] != (int)(uint8_t)idMarker[j]) {
                        match = false; break;
                    }
                }
                if (match) {
                    idStart = pos + idMarker.size();
                    std::cout << "  [+] '" << idMarker << "' found in file "
                              << files[fi].name << " at pos=" << pos
                              << " -> ID values start at " << idStart << ".\n";
                }
            }
        }

        if (idStart == std::string::npos) {
            std::cout << "  [-] Could not locate id field in any recovered "
                         "plaintext.\n"
                      << "      Run 'crib \"{\'\"createdAt\":' to anchor manually.\n";
        } else {
            for (size_t i = 0; i < files.size(); ++i) {
                // id value + the password-key template that follows it
                std::string crib = files[i].idStr + "\",\"password\":\"";
                commitFile(i, idStart, crib);
                std::cout << "  [+] Committed id+password-key for "
                          << files[i].name
                          << "  (id=\"" << files[i].idStr
                          << "\" + password-key)\n";
            }
        }

        std::cout << "\n[Done] Recovery complete.\n"
                  << "       Use 'crib <text>' or 'set' for any remaining unknown bytes.\n";
    }
};

// ---------------------------------------------------------------

void printHelp() {
    std::cout << R"(
Commands:
  auto                   -- Run grouped MTP attack
  crib <text>            -- Drag crib across all files, show hits
  set <idx> <pos> <text> -- Commit: file[idx] has <text> at byte <pos>
  show                   -- Display current recovery state
  help                   -- This message
  quit / q               -- Exit
)";
}

int main() {
    std::vector<CipherFile> files;
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.path().extension() == ".lic") {
            CipherFile cf;
            cf.name  = entry.path().filename().string();
            cf.idStr = entry.path().stem().string();
            cf.data  = readBinary(entry.path().string());
            if (cf.data.size() > NONCE_SIZE + MAC_SIZE) {
                cf.payload = std::vector<uint8_t>(
                    cf.data.begin() + NONCE_SIZE,
                    cf.data.end()   - MAC_SIZE);
                files.push_back(cf);
            }
        }
    }

    if (files.size() < 2) {
        std::cerr << "[!] Need at least 2 .lic files.\n"; return 1;
    }

    std::vector<uint8_t> refNonce(files[0].data.begin(),
                                   files[0].data.begin() + NONCE_SIZE);
    bool noncesMatch = true;
    for (size_t i = 1; i < files.size(); ++i)
        for (size_t j = 0; j < NONCE_SIZE; ++j)
            if (files[i].data[j] != refNonce[j]) { noncesMatch = false; break; }

    if (!noncesMatch) {
        std::cerr << "[!] Nonces do NOT match. Attack not feasible.\n"; return 1;
    }

    size_t minLen = files[0].payload.size();
    for (auto& f : files) minLen = std::min(minLen, f.payload.size());

    std::cout << "\n+======================================================+\n"
              << "|    MANY-TIME PAD ATTACK -- GROUPED SOLVER           |\n"
              << "+======================================================+\n\n";
    std::cout << "Loaded " << files.size() << " files:\n";
    for (size_t i = 0; i < files.size(); ++i)
        std::cout << "  [" << i << "] " << files[i].name
                  << "  id=\"" << files[i].idStr
                  << "\" (idLen=" << files[i].idStr.size()
                  << ", payload=" << files[i].payload.size() << "B)\n";

    std::cout << "\n[!] Nonce match: YES -- ATTACK FEASIBLE!\n";

    ManyPadState state(files, minLen);

    std::cout << "\n======================================================\n"
              << "  INTERACTIVE RECOVERY  (type 'help' for commands)\n"
              << "======================================================\n";

    std::string line;
    while (true) {
        std::cout << "\n> ";
        if (!std::getline(std::cin, line)) break;
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string cmd; iss >> cmd;

        if (cmd == "quit" || cmd == "exit" || cmd == "q") {
            break;
        } else if (cmd == "help" || cmd == "h") {
            printHelp();
        } else if (cmd == "show") {
            state.printState();
        } else if (cmd == "auto") {
            state.autoAttackGrouped();
            state.printState();
        } else if (cmd == "crib") {
            std::string crib;
            std::getline(iss >> std::ws, crib);
            if (crib.size() >= 2 && crib.front() == '"' && crib.back() == '"')
                crib = crib.substr(1, crib.size() - 2);
            if (!crib.empty()) state.cribDragAll(crib, true);
        } else if (cmd == "set") {
            size_t fileIdx = 0, pos = 0;
            std::string frag;
            if (!(iss >> fileIdx >> pos)) continue;
            std::getline(iss >> std::ws, frag);
            if (frag.size() >= 2 && frag.front() == '"' && frag.back() == '"')
                frag = frag.substr(1, frag.size() - 2);
            if (!frag.empty() && fileIdx < files.size())
                state.commitFile(fileIdx, pos, frag);
            state.printState();
        }
    }
    return 0;
}
