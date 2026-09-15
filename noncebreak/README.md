# Noncebreak — Interactive Many-Time Pad Attack

A command-line cryptanalysis tool that recovers plaintext from XOR-encrypted license files that were all encrypted with the **same nonce** (and same key, creating a classic Many-Time Pad vulnerability).

---

## What Is This Tool?

This tool exploits a fundamental cryptographic weakness: when you XOR-encrypt two different messages with the **exact same keystream** (caused by reusing a nonce), the keystream cancels out.

```
A = PlaintextA XOR Keystream
B = PlaintextB XOR Keystream

A XOR B = PlaintextA XOR PlaintextB   ← Keystream is gone!
```

If parts of PlaintextA and PlaintextB are **identical** (such as standard JSON keys), then `A XOR B == 0` at those positions. This creates "zero-runs" that reveal the structure of the data and ultimately let us recover the keystream — and from the keystream, every message.

The license files in this project are JSON payloads of the form:

```json
{"createdAt":"<date>","expiryDate":"<date>","id":"<id>","password":"<password>"}
```

All license files are encrypted with the **same 24-byte nonce** (reused key). This is the catastrophic vulnerability the tool exploits.

---

## How It Works (The Interactive Workflow)

Unlike tools that try to blindly guess and corrupt data, Noncebreak is an **interactive mathematical toolkit**. It handles the heavy XOR algebra and cross-file validation, but relies on your human intuition to resolve mathematical ambiguities.

### Recommended Workflow

1. **`auto` — Auto-Anchor IDs**
   Because the filename itself is the license `id`, the tool automatically calculates the exact byte offset for the IDs and anchors them. This is incredibly powerful because files with different ID lengths will expose different parts of the keystream, recovering surrounding passwords automatically!

2. **`wizard` — Anchor JSON Keys**
   The tool scans for "zero-runs" (uniform regions where ciphertext is identical across all files). It then prompts you to match these regions to known JSON keys (e.g., `{"createdAt":"`).
   
3. **`finddates` — Locate Dates**
   Mathematically scans the file for regions that could physically be `YYYY-MM-DD` strings. When it finds a valid region, it pauses and asks you to lock in the exact date for a specific file.

4. **`guess` & `crib` — Advanced Crib Dragging**
   Use these commands to laser-target any remaining unknown bytes (like passwords or times) using known plaintexts.

---

## Command Reference

When you run `noncebreak.exe`, you enter an interactive shell.

| Command | Description |
|---|---|
| `wizard` | Interactively matches zero-byte uniform regions to common JSON keys. |
| `auto` | Automatically anchors the license IDs (derived from the filenames). |
| `finddates` | Mathematically finds and interactively anchors `YYYY-MM-DD` date regions. |
| `guess <word>` | Tries to unambiguously place a shared keyword. If it fits in multiple mathematically valid spots, it provides structural context clues so you can visually choose the right one. |
| `crib <idx> <word>` | Finds the exact position of a word **unique to a single file**. It checks the XOR results against all other files to find the single unambiguous position. Excellent for targeting specific dates or passwords. |
| `set <idx> <pos> <text>` | Manually commit known plaintext to a file at a specific position. |
| `zeros` | Displays a visual map of uniform (`0`) vs varying (`.`) bytes across all files. |
| `show` | Print the current plaintext recovery state for all files. |
| `quit` / `q` | Exit. |

---

## Input Strategy — What Do You Want to Crack?

### To crack the JSON structure
You need **at least 2 files**. The XOR of these two files cancels the key and exposes all shared template text (the zero-runs). Use the `wizard` to lock these in.

### To crack the passwords
**The key insight:** The longer a file's ID, the further its password is pushed down the payload. 

When you anchor the ID and JSON keys of a file with a 30-character ID, it recovers keystream bytes far deeper into the file than a standard 6-character ID file. Because all files share the same keystream, recovering deep keystream bytes from the long-ID file will instantly and automatically decode the passwords of the shorter-ID files!

**Bottom line:** If you want to crack passwords, collect license files with as many **different ID lengths** as possible. A file with an exceptionally long ID is especially valuable.

---

## Building the Tool

Requires a standard C++ compiler.

```bash
g++ -O3 main.cpp -o noncebreak.exe
```

## Usage

Place `noncebreak.exe` in the same directory as your `.lic` files and run it:

```bash
./noncebreak.exe
```

---

## Example Execution

Here is a typical workflow using `noncebreak.exe` to break a set of license files and recover their passwords.

**1. Start the tool**
The tool automatically loads all `.lic` files in the directory and verifies they share the same nonce. 
```
> ./noncebreak.exe
Loaded 10 files. All nonces match.
```
*The Math:* Because the nonce is reused, all files share the exact same pseudo-random keystream. This allows us to use the fundamental property of the Many-Time Pad: `Ciphertext_A XOR Ciphertext_B = Plaintext_A XOR Plaintext_B`. The keystream mathematically cancels itself out.

**2. Map the zero-runs with `zeros`**
Use the `zeros` command to see where the files are identical (`0`) vs where they vary (`.`).
```
> zeros
  [Zero Bytes Map (0 = identical, . = variable)]
         00 01 02 03 04 05 06 07 08 09
    000 | 0  0  0  0  0  0  0  0  0  0
    010 | 0  0  0  0  .  .  .  .  0  .
```
*The Math:* Because all the license files are JSON objects, they share identical structural text (like `{"createdAt":"`). At these positions, `Plaintext_A == Plaintext_B`, meaning their XOR difference is exactly `0`. The `zeros` command visually maps these regions so you can see exactly where the shared structural keys are located.

**3. Auto-anchor IDs with `auto`**
Because the filename itself is the ID in this license system (e.g., `06UBavQf.lic`), we already know the exact plaintext for the `id` field! The `auto` command searches for this string in the ciphertext and locks it in.
```
> auto
[+] Committed id for 06UBavQf.lic at pos=76
[+] Committed id for AcuH647.lic at pos=76
```
*The Reason:* By XORing the known plaintext (the filename) with the ciphertext, we instantly recover the keystream for those specific byte positions. Because files have different ID lengths, they cover different segments of the keystream. A 30-character ID will recover keystream bytes deep into the file—bytes that overlap perfectly with the unknown passwords of the shorter-ID files, automatically decrypting them!

**4. Anchor JSON structure with `wizard`**
The wizard interactively walks you through the `0` regions identified in Step 2 and asks you to match them to known JSON keys.
```
> wizard
[!] Found uniform region at pos=0 (length 14)
    [1] {"createdAt":"
    [0] Skip
    Choose a template to commit: 1
```
*The Reason:* Even though the JSON keys are identical across all files, the math cannot automatically guess them without context (since any string XOR'd against itself is `0`). By manually anchoring these structural keys, you recover the keystream for the gaps between the variable data, isolating the unknown dates and passwords.

**5. Find dates mathematically with `finddates`**
This command scans the remaining unknown regions for mathematical patterns that perfectly fit the character constraints of a `YYYY-MM-DD` date (e.g., only digits and dashes in specific places).
```
> finddates
  [+] Mathematically valid YYYY-MM-DD region at pos=14
  [?] Multiple digits are mathematically possible due to XOR symmetry.
      Enter the exact date to commit (e.g. '5 2026-08-19' for File 5), or press Enter to skip: 0 2026-08-19
```
*The Math:* Because the year and month are usually identical across all files (e.g., all files are from `2026-08`), their XOR difference is `0`. Due to XOR symmetry, a difference of `0` means *any* valid digit works mathematically (e.g., `1^1=0`, `2^2=0`, `9^9=0`). The math literally cannot eliminate the false positives. By asking you to input the exact date for a specific file, the tool breaks this mathematical ambiguity, recovers the keystream from your input, and instantly decrypts the dates for all other files!

**6. Target specific unknown strings with `crib`**
If you know the exact password for a specific file (e.g., File 5), but don't know exactly where it sits in the file, you can laser-target it using the `crib` command.
```
> crib 5 "F1LvZeJHAyVLxTuO"
  [+] Crib 'F1LvZeJHAyVLxTuO' uniquely fits at pos=98. Committing...
```
*The Math:* `crib` guesses your string at every possible position in File 5, derives the hypothetical keystream, and checks if that keystream produces valid, readable ASCII characters in the 9 other files. Because it validates against all other files simultaneously, incorrect positions produce non-printable garbage and are eliminated. It locks onto the single mathematically perfect position instantly.

**7. Show the final decrypted state**
```
> show
[06UBavQf.lic]
{"createdAt":"2026-08-19...","id":"06UBavQf","password":"..."}
Recovered: 109/109 bytes (100%)
```
