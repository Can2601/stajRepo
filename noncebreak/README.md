# XOR License Cracker — Many-Time Pad Attack

A command-line cryptanalysis tool that recovers plaintext from XOR-encrypted license files that were all encrypted with the **same key** (a classic Many-Time Pad vulnerability).

---

## What Is This Tool?

This tool exploits a fundamental weakness: when you XOR-encrypt two different messages with the **exact same key**, the key cancels out.

```
A = PlaintextA XOR Key
B = PlaintextB XOR Key

A XOR B = PlaintextA XOR PlaintextB   ← Key is gone!
```

If parts of PlaintextA and PlaintextB are **identical** (template text like JSON keys), then `A XOR B == 0` at those positions. This reveals the structure of the data and ultimately lets us recover the keystream — and from the keystream, every message.

The license files in this project are JSON payloads of the form:

```json
{"createdAt":"<date>","expiryDate":"<date>","id":"<id>","password":"<password>"}
```

All license files are encrypted with the **same 24-byte nonce** (reused key). This is the vulnerability the tool exploits.

---

## The Cryptographic Vulnerability — Many-Time Pad

A stream cipher (like XSalsa20 used here) is perfectly secure if the nonce is **never reused**. The moment the same nonce encrypts two different messages, an attacker can XOR the two ciphertexts together and cancel the key entirely. With enough messages (files), the full keystream can be recovered byte by byte.

**This is why "nonce" literally means "number used once."** Reusing it is catastrophic.

---

## How the Attack Algorithm Works

The attack runs in four automatic phases:

### Phase 1 — Group by ID Length

License files are grouped by the length of their `id` field (taken from the filename, e.g., `FfKSm0.lic` has an ID of length 6).

**Why?** Files with the same ID length have the exact same JSON structure and payload size. When you XOR two files from the same group, the zero bytes (positions where both files have identical plaintext) form clean, contiguous **zero-runs**. These zero-runs mark all template/structural positions in the JSON.

Files in different-length groups cannot be paired for zero-run analysis because their payloads are different lengths — the `id` and everything after it is shifted.

---

### Phase 2 — Template Token Discovery via XOR Self-Validation

For each group with 2+ files, the tool:

1. Computes the pairwise XOR of all files in the group.
2. Builds a map of which byte positions are zero (template/shared) vs. non-zero (variable/different).
3. Has a list of **candidate tokens** — the known JSON key strings:
   - `{"createdAt":"`
   - `","expiryDate":"`
   - `","id":"`
   - `","password":"`

4. For each candidate token, scans the entire payload to find every position where the XOR is zero for the full token length. This gives a list of **mathematically valid positions** — positions where the token *could* live.

5. Runs a **combinatorial search** over all combinations of valid positions to find the unique assignment that is consistent: tokens must appear in order, and crucially, the gap between `createdAt` and `expiryDate` must equal the gap between `expiryDate` and `id` (because `createdAt` and `expiryDate` are both ISO 8601 date strings of the same length).

6. Once the layout is found, derives the keystream at all template positions by XORing the known token text with the ciphertext of file[0].

> **Note:** The stat cracker does NOT run here. Running it now would incorrectly lock the ID byte positions before the correct IDs can be committed in Phase 3.

---

### Phase 3 — Cross-Group ID + Password-Key Commit

Now that the keystream is known at all structural (template) positions, the tool can decode enough of every file to find the `","id":"` marker in the plaintext. It then knows the exact byte offset where each file's ID value begins.

For **every** file — including those from single-file groups that couldn't be XOR-paired — the tool commits:

```
<known_id_from_filename> + ","password":"
```

at the discovered ID offset. Since the filename IS the ID (by design of this license system), this is known plaintext. This bridges the keystream from the template region into the variable password region.

**Why a long-ID file recovers all other passwords:** When Phase 3 commits `longId + ","password":"` for a 30-character ID file, it derives the keystream at positions 76 through 119 (76 + 30 ID bytes + 14 key bytes). All shorter files have their passwords starting at positions 82–85 and ending well before 119 — so those passwords are fully decoded as a side effect of the long-ID commit, with no statistical guessing needed.

> **This is why files with longer IDs help crack passwords** — see the Input Strategy section below.

---

## Input Requirements

- **At least 2 `.lic` files** in the working directory.
- All files must have been encrypted with the **same nonce** (the tool verifies this automatically by comparing the first 24 bytes of every file).

### File Format

Each `.lic` file must be laid out as:

```
[24 bytes]  Nonce
[N bytes]   Encrypted payload  (the JSON ciphertext)
[16 bytes]  MAC (authentication tag, ignored by the cracker)
```

The **filename stem** (without `.lic`) must equal the `id` field inside the encrypted payload. For example, `FfKSm0.lic` must contain `"id":"FfKSm0"` in its plaintext. This is the critical known-plaintext anchor.

---

## Input Strategy — What Do You Want to Crack?

### To crack the JSON structure / dates (createdAt, expiryDate)

You need **at least 2 files with the same ID length**. The XOR of these two files cancels the key and exposes all shared template text. The date fields will remain partially unknown because the varying suffix of the date is in a variable region.

**Example:** `FfKSm0.lic` + `kO27t9.lic` (both 6-char IDs) → exposes all JSON keys.

---

### To crack the passwords

Phase 4's statistical pass attempts this automatically, but it's a heuristic and may produce wrong characters.

**The key insight:** The longer a file's ID, the further its password is pushed in the payload. To lock in password bytes correctly, Phase 3 commits the ID + password-key prefix. The more of the password region is covered by the known-plaintext commit (via IDs from different length groups), the more password bytes get pinned before the stat cracker runs.

**In practice:** Having files with a range of ID lengths — especially longer IDs — greatly improves password recovery, because:
- The password offset shifts with ID length
- Cross-group coverage leaves fewer bytes for the fallible stat cracker
- If a file has a very long ID (e.g. 30 chars), its password starts at a very different offset, covering positions that other groups don't anchor

**Bottom line:** If you want to crack passwords, collect license files with as many **different ID lengths** as possible. A file with an ID of 30+ characters is especially valuable.

---

## Building the Tool

Requires a C++17 compiler.

```bash
g++ -o xor_solver.exe main.cpp -std=c++17
```

---

## Usage

Place `xor_solver.exe` in the same directory as your `.lic` files and run it:

```bash
./xor_solver.exe
```

The tool loads all `.lic` files from the current directory, verifies the nonce reuse, and opens an interactive prompt.

---

## Interactive Commands

| Command | Description |
|---|---|
| `auto` | Run the full 4-phase automatic attack. Always run this first. |
| `show` | Print the current plaintext recovery state for all files. |
| `crib <text>` | Manually slide a known word across all files and show where it produces valid (printable) output in all files simultaneously. Useful for finding unknown dates or passwords. |
| `set <idx> <pos> <text>` | Manually commit known plaintext. If you know file 2 has `"hello"` at byte position 96, use `set 2 96 "hello"`. |
| `help` | Show the command list. |
| `quit` / `q` | Exit. |

---

## Example Session

```
> auto
[Phase 1] Groups by ID length:
  idLen=6 (2 files): FfKSm0.lic kO27t9.lic
  idLen=7 (2 files): AcuH647.lic yR8n1cZ.lic
  ...

[Phase 2] Template token discovery via XOR self-validation:
  -> Group idLen=6:
  [i] Discovered structural layout by trying candidate words:
      [+] pos= 0 -> {"createdAt":"
      [+] pos=33 -> ","expiryDate":"
      [+] pos=68 -> ","id":"
      [+] pos=82 -> ","password":"
  ...

[Phase 3] Cross-group anchor: committing all IDs + password-key prefix...
  [+] '","id":"' found in file FfKSm0.lic at pos=68 -> ID values start at 76.
  [+] Committed id+password-key for FfKSm0.lic  (id="FfKSm0" + password-key)
  [+] Committed id+password-key for U0MOtExXABaseasgaxxvssfhdfgasr.lic  (id="U0MOtExXABaseasgaxxvssfhdfgasr" + password-key)
  ...

[Done] Recovery complete.
       Use 'crib <text>' or 'set' for any remaining unknown bytes.
```

After `auto`, use `crib` to narrow down any remaining `?` bytes:

```
> crib "2026-09"
  pos= 14 (Variable Hit)
    [FfKSm0.lic] has "2026-09"
    [kO27t9.lic] has "2026-09"
    ...

> set 0 14 "2026-09-01T"
```

---

## Inputs

The only two inputs the tool needs are:

1. **`.lic` files** in the current directory — the ciphertexts to attack.
2. **Candidate words** — the list of plaintext fragments to try against zero-runs. These are defined in the `words` vector inside `findAndCommitTemplateXOR` in the source. You can add, remove, or reorder them freely. The algorithm discovers their positions automatically by trying every word at every valid zero-run position and finding the only non-overlapping, consistent layout — it does not assume any particular order.

## Limitations

- Requires nonce reuse. If every file uses a unique nonce, the attack is not feasible.
- The combinatorial solver uses an equal-gap heuristic to disambiguate token positions. This is a *mathematical property of the data* (two fields of the same format have the same byte length), not a hardcoded schema assumption.
- If no long-ID file is available, some password bytes may remain unknown. Use `crib` + `set` interactively to resolve them.
