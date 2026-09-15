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
    std::cout << "\n[" << label << "]\n";
    std::cout << "       00 01 02 03 04 05 06 07 08 09\n";
    for (size_t i = 0; i < len; ++i) {
        if (i % 10 == 0) {
            if (i > 0) std::cout << "\n";
            std::cout << "  " << std::setw(3) << std::setfill('0') << i << " |";
        }
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


    // ---- ID Anchoring -----------------------------------------
    size_t findAndCommitIDs(const std::vector<size_t>& subset) {
        size_t tLen = files[subset[0]].idStr.size();
        for (size_t pos = 0; pos + tLen <= len; ++pos) {
            bool valid = true;
            for (size_t j = 0; j < tLen && valid; ++j) {
                size_t p = pos + j;
                for (size_t k = 1; k < subset.size() && valid; ++k) {
                    size_t fi = subset[k];
                    if (p >= files[fi].payload.size() || p >= files[subset[0]].payload.size()) { valid = false; break; }
                    
                    uint8_t c1 = files[subset[0]].payload[p];
                    uint8_t c2 = files[fi].payload[p];
                    uint8_t p1 = files[subset[0]].idStr[j];
                    uint8_t p2 = files[fi].idStr[j];
                    
                    if ((c1 ^ c2) != (p1 ^ p2)) {
                        valid = false;
                    }
                }
            }
            if (valid) {
                // Confirm no conflict with known KS
                for (size_t j = 0; j < tLen && valid; ++j) {
                    size_t p = pos + j;
                    uint8_t ks = files[subset[0]].payload[p] ^ files[subset[0]].idStr[j];
                    if (knownKs[p] != -1 && knownKs[p] != ks) valid = false;
                }
                
                if (valid) {
                    std::cout << "  [+] Discovered ID field at pos=" << pos << " for idLen=" << tLen << "\n";
                    for (size_t k = 0; k < subset.size(); ++k) {
                        commitFile(subset[k], pos, files[subset[k]].idStr);
                    }
                    return pos;
                }
            }
        }
        return std::string::npos;
    }
    std::string getContextClue(size_t guessPos, const std::string& word) const {
        int start = std::max(0, (int)guessPos - 15);
        int end = std::min((int)len, (int)(guessPos + word.size() + 15));
        std::string res = "";
        for (int p = start; p < end; ++p) {
            if (p >= (int)guessPos && p < (int)(guessPos + word.size())) {
                res += word[p - guessPos];
            } else if (knownKs[p] != -1) {
                char c = (char)(files[0].payload[p] ^ knownKs[p]);
                res += (c >= 0x20 && c <= 0x7E) ? c : '.';
            } else {
                res += '?';
            }
        }
        return res;
    }


    // ---- Auto Guessing ----------------------------------------
    void autoGuessWords(const std::vector<std::string>& words) {
        std::map<size_t, std::vector<size_t>> groups;
        for (size_t i = 0; i < files.size(); ++i)
            groups[files[i].idStr.size()].push_back(i);
            
        std::vector<size_t> refGroup;
        for (const auto& pair : groups) {
            if (pair.second.size() >= 2) {
                refGroup = pair.second;
                break;
            }
        }
        if (refGroup.empty()) {
            std::cout << "  [-] Need at least 2 files of the same ID length to auto-guess reliable templates.\n";
            return;
        }

        for (const auto& word : words) {
            std::vector<size_t> validPositions;
            for (size_t pos = 0; pos + word.size() <= len; ++pos) {
                bool possible = true;
                
                // 1. Must be identical in all files of the refGroup
                for (size_t j = 0; j < word.size() && possible; ++j) {
                    size_t p = pos + j;
                    uint8_t ks = files[refGroup[0]].payload[p] ^ (uint8_t)word[j];
                    if (knownKs[p] != -1 && knownKs[p] != ks) { possible = false; break; }
                    
                    for (size_t k = 1; k < refGroup.size(); ++k) {
                        size_t fi = refGroup[k];
                        if (p >= files[fi].payload.size() || 
                            (files[fi].payload[p] ^ ks) != (uint8_t)word[j]) { 
                            possible = false; break; 
                        }
                    }
                }
                
                // 2. Must yield printable ASCII in ALL files
                if (possible) {
                    for (size_t i = 0; i < files.size() && possible; ++i) {
                        for (size_t j = 0; j < word.size(); ++j) {
                            size_t p = pos + j;
                            if (p >= files[i].payload.size()) { possible = false; break; }
                            uint8_t ks = files[refGroup[0]].payload[p] ^ (uint8_t)word[j];
                            uint8_t pt = files[i].payload[p] ^ ks;
                            if (pt < 0x20 || pt > 0x7E) { possible = false; break; }
                        }
                    }
                }
                
                if (possible) validPositions.push_back(pos);
            }
            
            if (validPositions.size() == 1) {
                size_t pos = validPositions[0];
                std::cout << "  [+] Keyword '" << word << "' unambiguously fits as a shared template at pos=" << pos << ". Committing...\n";
                commitFile(refGroup[0], pos, word);
            } else if (validPositions.size() > 1) {
                std::cout << "  [?] Keyword '" << word << "' mathematically fits in " << validPositions.size() << " template positions.\n";
                for (size_t i = 0; i < validPositions.size(); ++i) {
                    size_t p = validPositions[i];
                    std::cout << "      [" << (i + 1) << "] pos=" << p << "\n";
                    std::cout << "          Context: " << getContextClue(p, word) << "\n";
                }
                std::cout << "      [0] Skip\n";
                std::cout << "      Which position is correct? ";
                std::string choiceStr;
                std::getline(std::cin, choiceStr);
                if (!choiceStr.empty()) {
                    int choice = atoi(choiceStr.c_str());
                    if (choice > 0 && choice <= (int)validPositions.size()) {
                        size_t pos = validPositions[choice - 1];
                        commitFile(refGroup[0], pos, word);
                        std::cout << "  [+] Committed '" << word << "' at pos=" << pos << "\n";
                    }
                }
            } else {
                // Not found as a shared template. Try as a unique crib across all files!
                std::vector<std::pair<size_t, size_t>> allValidCribs;
                for (size_t fi = 0; fi < files.size(); ++fi) {
                    for (size_t pos = 0; pos + word.size() <= len; ++pos) {
                        bool possible = true;
                        for (size_t j = 0; j < word.size() && possible; ++j) {
                            size_t p = pos + j;
                            if (p >= files[fi].payload.size()) { possible = false; break; }
                            uint8_t ks = files[fi].payload[p] ^ (uint8_t)word[j];
                            if (knownKs[p] != -1 && knownKs[p] != ks) { possible = false; break; }
                            
                            for (size_t i = 0; i < files.size(); ++i) {
                                if (p >= files[i].payload.size()) { possible = false; break; }
                                uint8_t pt = files[i].payload[p] ^ ks;
                                if (pt < 0x20 || pt > 0x7E) { possible = false; break; }
                            }
                        }
                        if (possible) allValidCribs.push_back({fi, pos});
                    }
                }
                
                if (allValidCribs.size() == 1) {
                    size_t fi = allValidCribs[0].first;
                    size_t pos = allValidCribs[0].second;
                    std::cout << "  [+] Word '" << word << "' uniquely belongs to File " << fi << " at pos=" << pos << "! Committing...\n";
                    commitFile(fi, pos, word);
                } else if (allValidCribs.size() > 1) {
                    std::cout << "  [?] Word '" << word << "' mathematically fits in " << allValidCribs.size() << " unique file/position combinations.\n";
                    for (size_t i = 0; i < allValidCribs.size(); ++i) {
                        size_t fi = allValidCribs[i].first;
                        size_t p = allValidCribs[i].second;
                        std::cout << "      [" << (i + 1) << "] File " << fi << " (" << files[fi].name << "), pos=" << p << "\n";
                        std::cout << "          Context: " << getContextClue(p, word) << "\n";
                    }
                    std::cout << "      [0] Skip\n";
                    std::cout << "      Which one is correct? ";
                    std::string choiceStr;
                    std::getline(std::cin, choiceStr);
                    if (!choiceStr.empty()) {
                        int choice = atoi(choiceStr.c_str());
                        if (choice > 0 && choice <= (int)allValidCribs.size()) {
                            size_t fi = allValidCribs[choice - 1].first;
                            size_t p = allValidCribs[choice - 1].second;
                            commitFile(fi, p, word);
                            std::cout << "  [+] Committed '" << word << "' for File " << fi << " at pos=" << p << "\n";
                        }
                    }
                } else {
                    std::cout << "  [-] Word '" << word << "' does not mathematically fit anywhere in any file.\n";
                }
            }
        }
    }

    // ---- Date Finding -----------------------------------------
    void findDates() {
        int found = 0;
        for (size_t pos = 0; pos + 10 <= len; ++pos) {
            bool validPos = true;
            std::vector<std::vector<char>> possibleChars(10);
            
            for (int j = 0; j < 10; ++j) {
                // Determine allowed characters based on YYYY-MM-DD
                std::string allowed;
                if (j == 4 || j == 7) allowed = "-";
                else if (j == 0) allowed = "12";     // Year starts with 1 or 2
                else if (j == 5) allowed = "01";     // Month starts with 0 or 1
                else if (j == 8) allowed = "0123";   // Day starts with 0, 1, 2, or 3
                else allowed = "0123456789";

                for (char guess : allowed) {
                    bool allValid = true;
                    for (size_t i = 1; i < files.size(); ++i) {
                        uint8_t diff = files[i].payload[pos+j] ^ files[0].payload[pos+j];
                        char resultForB = guess ^ diff;
                        
                        bool isValid = false;
                        for (char a : allowed) {
                            if (resultForB == a) { isValid = true; break; }
                        }
                        if (!isValid) { allValid = false; break; }
                    }
                    if (allValid) {
                        uint8_t k = files[0].payload[pos+j] ^ guess;
                        if (knownKs[pos+j] != -1 && knownKs[pos+j] != k) continue;
                        possibleChars[j].push_back(guess);
                    }
                }
                if (possibleChars[j].empty()) {
                    validPos = false;
                    break;
                }
            }

            if (validPos) {
                // Ensure region is not completely uniform (avoid false positives on uniform JSON keys)
                bool hasVariance = false;
                for (int j = 0; j < 10; ++j) {
                    for (size_t i = 1; i < files.size(); ++i) {
                        if (files[i].payload[pos+j] != files[0].payload[pos+j]) {
                            hasVariance = true; break;
                        }
                    }
                    if (hasVariance) break;
                }
                
                if (!hasVariance) continue;

                std::cout << "\n  [+] Mathematically valid YYYY-MM-DD region at pos=" << pos << "\n";
                for (int j = 0; j < 10; ++j) {
                    std::cout << "      Col " << j << " (File 0 can be): ";
                    for (char c : possibleChars[j]) {
                        std::cout << c << " ";
                    }
                    std::cout << "\n";
                    
                    if (possibleChars[j].size() == 1) {
                        // We intentionally DO NOT auto-commit here anymore.
                        // False positive regions (like uniform JSON keys that happen to mathematically
                        // align with valid digits/dashes due to a stray variance) would corrupt the state.
                        std::cout << "        -> (Mathematical single match for '" << possibleChars[j][0] << "')\n";
                    }
                }
                
                std::cout << "  [?] Multiple digits are mathematically possible due to XOR symmetry.\n";
                std::cout << "      Enter the exact date to commit (e.g. '5 2026-08-19' for File 5), or press Enter to skip: ";
                std::string dateInput;
                std::getline(std::cin, dateInput);
                if (!dateInput.empty()) {
                    std::istringstream iss(dateInput);
                    int fileIdx;
                    std::string dateStr;
                    if (iss >> fileIdx >> dateStr && fileIdx >= 0 && fileIdx < (int)files.size() && dateStr.length() == 10) {
                        commitFile(fileIdx, pos, dateStr);
                        std::cout << "  [+] Committed '" << dateStr << "' for File " << fileIdx << " at pos=" << pos << "\n";
                    } else {
                        std::cout << "  [-] Invalid input format. Skipped.\n";
                    }
                }
                
                found++;
            }
        }
        if (found == 0) {
            std::cout << "  [-] No date regions found.\n";
        }
    }

    void printZeroBytes() const {
        std::cout << "\n[Zero Bytes Map (0 = identical, . = variable)]\n";
        std::cout << "       00 01 02 03 04 05 06 07 08 09\n";
        int count = 0;
        for (size_t pos = 0; pos < len; ++pos) {
            if (pos % 10 == 0) {
                if (pos > 0) std::cout << "\n";
                std::cout << "  " << std::setw(3) << std::setfill('0') << pos << " |";
            }

            bool allIdentical = true;
            for (size_t i = 1; i < files.size(); ++i) {
                if (pos < files[0].payload.size() && pos < files[i].payload.size()) {
                    if (files[i].payload[pos] != files[0].payload[pos]) {
                        allIdentical = false;
                        break;
                    }
                } else {
                    allIdentical = false;
                    break;
                }
            }
            if (allIdentical) {
                std::cout << " 0 ";
                count++;
            } else {
                std::cout << " . ";
            }
        }
        std::cout << "\n  [Total: " << count << " zero bytes]\n";
    }

    // ---- Crib drag (all files, verbose for manual use) --------

    void wizardZeros() {
        std::vector<std::string> dict = {
            "{\"createdAt\":\"",
            "\",\"expiryDate\":\"",
            "\",\"password\":\"",
            "\"}",
            "{\"id\":\"",
            "\",\"name\":\"",
            "\",\"email\":\"",
            "\",\"username\":\"",
            "\",\"type\":\"",
            "\",\"version\":\"",
            "\",\"data\":\"",
            "\",\"timestamp\":\"",
            "\",\"token\":\"",
            "\",\"license\":\"",
            "\",\"key\":\"",
            "createdAt",
            "expiryDate",
            "password",
            "id",
            "name",
            "username",
            "email",
            "type",
            "version",
            "data",
            "timestamp",
            "token",
            "license",
            "key",
            "{\"",
            "\":\"",
            "\",\"",
            "\"}"
        };

        std::vector<std::pair<size_t, size_t>> regions;
        bool inRegion = false;
        size_t start = 0;
        for (size_t pos = 0; pos < len; ++pos) {
            bool allSame = true;
            for (size_t i = 1; i < files.size(); ++i) {
                if (files[i].payload[pos] != files[0].payload[pos]) {
                    allSame = false; break;
                }
            }
            if (allSame) {
                if (!inRegion) { inRegion = true; start = pos; }
            } else {
                if (inRegion) {
                    regions.push_back({start, pos - start});
                    inRegion = false;
                }
            }
        }
        if (inRegion) regions.push_back({start, len - start});

        for (auto& r : regions) {
            size_t rpos = r.first;
            size_t rlen = r.second;
            std::cout << "\n[!] Found uniform region at pos=" << rpos << " (length " << rlen << ")\n";
            std::vector<std::string> candidates;
            for (const auto& w : dict) {
                if (w.size() <= rlen) candidates.push_back(w);
            }
            if (candidates.empty()) {
                std::cout << "  [-] No default words fit this region.\n";
                continue;
            }

            std::cout << "  Candidates:\n";
            for (size_t i = 0; i < candidates.size(); ++i) {
                std::cout << "    [" << i + 1 << "] " << candidates[i] << " (len " << candidates[i].size() << ")\n";
            }
            std::cout << "    [0] Skip\n";
            std::cout << "  Choice: ";
            
            std::string choiceStr;
            std::getline(std::cin, choiceStr);
            if (choiceStr.empty()) continue;
            int choice = atoi(choiceStr.c_str());
            if (choice > 0 && choice <= (int)candidates.size()) {
                commitFile(0, rpos, candidates[choice - 1]);
                std::cout << "  [+] Committed '" << candidates[choice - 1] << "' at pos=" << rpos << "\n";
            }
        }
    }

    // ===========================================================
    //  MAIN AUTO ATTACK
    // ===========================================================
    void autoAttackGrouped() {
        std::map<size_t, std::vector<size_t>> groups;
        for (size_t i = 0; i < files.size(); ++i)
            groups[files[i].idStr.size()].push_back(i);

        std::cout << "\n[Phase 1] Auto-Anchoring IDs based on filename differences:\n";
        size_t idStart = std::string::npos;
        for (auto& pair : groups) {
            if (pair.second.size() < 2) continue;
            size_t pos = findAndCommitIDs(pair.second);
            if (pos != std::string::npos && idStart == std::string::npos) {
                idStart = pos;
            }
        }

        if (idStart != std::string::npos) {
            std::cout << "\n[Phase 2] Cross-group anchor: committing all remaining IDs at pos=" << idStart << "\n";
            for (size_t i = 0; i < files.size(); ++i) {
                commitFile(i, idStart, files[i].idStr);
                std::cout << "  [+] Committed id for " << files[i].name << " (id=\"" << files[i].idStr << "\")\n";
            }
        } else {
            std::cout << "  [-] Could not auto-anchor IDs. Need at least 2 files with the same ID length.\n";
        }

        std::cout << "\n[Done] Initial anchoring complete.\n"
                  << "       Use 'wizard' to find more templates.\n";
    }
};

// ---------------------------------------------------------------

void printHelp() {
    std::cout << R"(
Commands:
  auto                   -- Run auto-anchoring of IDs
  guess <w1> [w2...]     -- Try to unambiguously place keywords
  finddates              -- Find and interactively anchor YYYY-MM-DD date regions
  wizard                 -- Interactively match zero-byte regions to JSON keys
  set <idx> <pos> <text> -- Commit: file[idx] has <text> at byte <pos>
  zeros                  -- Show which bytes are identical across all files
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
        } else if (cmd == "zeros") {
            state.printZeroBytes();
        } else if (cmd == "show") {
            state.printState();
        } else if (cmd == "auto") {
            state.autoAttackGrouped();
            state.printState();
        } else if (cmd == "guess") {
            std::vector<std::string> words;
            std::string w;
            while (iss >> w) {
                if (w.size() >= 2 && w.front() == '"' && w.back() == '"')
                    w = w.substr(1, w.size() - 2);
                words.push_back(w);
            }
            if (!words.empty()) state.autoGuessWords(words);
            state.printState();
        } else if (cmd == "finddates") {
            state.findDates();
            state.printState();
        } else if (cmd == "wizard") {
            state.wizardZeros();
            state.printState();
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
