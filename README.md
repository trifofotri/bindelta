# bindelta

A simple binary diffing tool that renders changed code as disassembly with capstone.

*bindelta* compares two binaries at the byte level and disassembles the before/after code with [Capstone](https://www.capstone-engine.org/). Executable sections are aligned using an [LCS](https://en.wikipedia.org/wiki/Longest_common_subsequence)-based diff.

> **Note:** only ELF binaries are supported right now. Raw binary & PE support is planned.

```diff
$ bindelta test/program1 test/program2
Comparing test/program1 (16000 bytes) and test/program2 (16000 bytes)...

[.text] 2 changed regions:
[.text] instruction diff:

  00001177  83 45 fc 01         add     dword ptr [rbp - 4], 1
- 0000117b  83 7d fc 04         cmp     dword ptr [rbp - 4], 4
+ 0000117b  83 7d fc 07         cmp     dword ptr [rbp - 4], 7
  0000117f  7e e4               jle     0x1165
[.text] instruction diff:

  0000117f  7e e4               jle     0x1165
- 00001181  83 7d f8 0a         cmp     dword ptr [rbp - 8], 0xa
+ 00001181  83 7d f8 14         cmp     dword ptr [rbp - 8], 0x14
  00001185  7e 1b               jle     0x11a2

[.strtab] 1 changed region:
  old: offset=0xaf len=1   new: offset=0xaf len=1
```

## Building

Requires CMake and C++20. Capstone is vendored as a git submodule.

```bash
git clone --recurse-submodules https://github.com/trifofotri/bindelta.git
cd bindelta
cmake -B build
cmake --build build
```

## Usage

```
bindelta <options> binary1 binary2

Options:
-h    --help                   displays this
-v    --verbose                show noisy metadata sections (build-id, comments, etc.)
-nc   --no-color               disable colored output
-nlcs --no-lcs                 disable instruction-level alignment, fall back to raw positional instruction diff
      --window <n>             instructions of context grabbed around each diff region (default 40)
      --context <n>            unchanged lines shown before/after a real change, rest collapsed (default 2)
      --merge-distance <n>     byte distance under which nearby diff regions get merged (default 128)
```