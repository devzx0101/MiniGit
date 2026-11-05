# Mini-Git: A Functional High-Performance Git Implementation in C++

<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=c%2B%2B" alt="C++17" />
  <img src="https://img.shields.io/badge/Build-CMake%20%7C%20Ninja-064F8C?style=for-the-badge&logo=cmake" alt="CMake" />
  <img src="https://img.shields.io/badge/Compression-zlib%20RFC%201950-blue?style=for-the-badge" alt="zlib" />
  <img src="https://img.shields.io/badge/Tests-14%2F14%20Passing%20(100%25)-success?style=for-the-badge" alt="Tests" />
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="MIT License" />
</p>

---

## 📌 Executive Summary

**Mini-Git** is a fully functional, production-engineered Git clone written from scratch in modern **C++17**. It faithfully models and reproduces the exact internal subsystems, on-disk binary layouts, and mathematical algorithms found in the official Linux Git kernel codebase:

- **100% Core Git Object Engine**: Content-addressable SHA-1 storage covering all 4 fundamental Git object types: **Blobs**, **Trees**, **Commits**, and **Annotated Tags**.
- **Official Git Binary Compatibility**: RFC 1950 zlib streaming `DEFLATE`/`INFLATE` compression matching Git's loose object header format (`0x78` magic byte family).
- **Packfile Compaction (`.pack` / `.idx`)**: `minigit gc` garbage collection bundling loose objects into binary packfiles with 256-bucket first-level fan-out index tables for $O(\log N)$ binary search lookups.
- **Graph & Sequence Algorithms**:
  - **LCA (Lowest Common Ancestor)** DAG traversal for 3-way merge resolution, conflict detection, and history replay.
  - **LCS (Longest Common Subsequence)** dynamic programming for line-by-line unified diff calculation.
- **Advanced Git Engineering**: Stash stacks via dangling commit graphs, interactive rebase (`pick`, `squash`, `reword`, `drop`), cherry-picking, safe cleaning, and local directory remote synchronization (`clone`, `fetch`, `push`, `pull`).
- **Complete Terminal CLI**: 27 fully registered commands with cross-platform ANSI terminal coloring and Win32 Virtual Terminal Processing (VTP).

---

## 🏗️ Architecture & Subsystem Layout

```
MiniGit/
├── CMakeLists.txt                 # Modern CMake build configuration & test target
├── README.md                      # Comprehensive technical documentation
├── src/
│   ├── main.cpp                   # Central CLI dispatcher routing all 27 commands
│   ├── terminal.hpp               # Cross-platform ANSI color subsystem with Win32 VTP
│   ├── filesystem/
│   │   ├── file_utils.hpp / .cpp  # Filesystem abstraction, recursive traversals, timestamps
│   ├── hashing/
│   │   ├── sha1.hpp / .cpp        # Standard cryptographic SHA-1 implementation
│   ├── compression/
│   │   ├── zlib_utils.hpp / .cpp  # RFC 1950 zlib streaming DEFLATE / INFLATE
│   ├── objects/
│   │   ├── object_store.hpp / .cpp# Content-addressable storage (.minigit/objects/xx/...)
│   │   ├── blob.hpp / .cpp        # Immutable byte content objects
│   │   ├── tree.hpp / .cpp        # Hierarchical directory tree serialization
│   │   ├── commit.hpp / .cpp      # Commit DAG nodes (tree, parents, author, message)
│   │   ├── tag.hpp / .cpp         # Git's 4th object type: Annotated tag objects
│   │   └── packfile.hpp / .cpp    # Binary .pack generation & .idx version 2 fanout index
│   ├── repository/
│   │   ├── index.hpp / .cpp       # Staging area persistence (.minigit/index)
│   │   ├── reference.hpp / .cpp   # HEAD, branches, tags, remote tracking refs, reflog
│   │   ├── ignore.hpp / .cpp      # .minigitignore / .gitignore wildcard & directory matching
│   │   ├── working_tree.hpp / .cpp# 3-way status detection, file restoration, checkout
│   │   ├── diff_engine.hpp / .cpp # Unified line diff engine (LCS dynamic programming)
│   │   ├── merge_engine.hpp / .cpp# Lowest Common Ancestor (LCA) traversal & 3-way merge
│   │   └── remote_manager.hpp / .cpp# Remote synchronization (clone, fetch, push, pull)
│   └── commands/                  # 27 Modular Command Handlers
│       ├── init.cpp               # minigit init
│       ├── add.cpp                # minigit add <file>... / .
│       ├── commit.cpp             # minigit commit -m <msg>
│       ├── status.cpp             # minigit status (color-coded staged/unstaged)
│       ├── log.cpp                # minigit log [--oneline] [--graph]
│       ├── branch.cpp             # minigit branch [-d] [<name>]
│       ├── checkout.cpp           # minigit checkout [-b] <name>
│       ├── diff.cpp               # minigit diff [--staged]
│       ├── reset.cpp              # minigit reset [--hard] [<file>...]
│       ├── restore.cpp            # minigit restore [--staged] <file>...
│       ├── stash.cpp              # minigit stash [push|pop|list|clear|drop]
│       ├── clean.cpp              # minigit clean [-f] [-d] [-n]
│       ├── cherry_pick.cpp        # minigit cherry-pick <commit>
│       ├── rebase.cpp             # minigit rebase [-i] <upstream>
│       ├── tag.cpp                # minigit tag [-a] [-m] [-d] [<name>]
│       ├── gc.cpp                 # minigit gc (packfile garbage collection)
│       ├── merge.cpp              # minigit merge <branch>
│       ├── reflog.cpp             # minigit reflog
│       ├── cat_file.cpp           # minigit cat-file (-p|-t|-s) <object>
│       ├── ls_files.cpp           # minigit ls-files [--stage]
│       ├── hash_object.cpp        # minigit hash-object [-w] <file>
│       ├── remote.cpp             # minigit remote / clone / fetch / push / pull
│       └── help.cpp               # minigit help
└── tests/
    └── test_runner.cpp            # 14 Automated Integration Test Suites (100% pass)
```

---

## 📦 Object Model & On-Disk Format

### The 4 Git Object Types
Every object is stored as `zlib_deflate("<type> <size>\0<content>")` inside `.minigit/objects/xx/xxxxxxxx...`:

| Type | On-Disk Payload Structure | Purpose |
|---|---|---|
| **`blob`** | `<raw byte contents>` | Stores file content without filename or metadata |
| **`tree`** | `<mode> <type> <hash>\t<name>\n...` | Represents directory hierarchies recursively |
| **`commit`**| `tree <hash>\nparent <hash>\nauthor ...\ncommitter ...\n\n<msg>` | Snapshot node in the commit DAG |
| **`tag`** | `object <hash>\ntype commit\ntag <name>\ntagger ...\n\n<msg>` | Dedicated object for annotated releases |

### Binary Packfiles (`.pack`) & Fan-Out Index (`.idx`)
When `minigit gc` runs:
1. **`.pack` File**: Prepend `PACK` magic header (4 bytes) + Version 2 (4 bytes) + Object Count (4 bytes). Each entry contains a 4-byte uncompressed size header followed by zlib-deflated payload and CRC32 verification. The pack concludes with a 20-byte SHA-1 trailer.
2. **`.idx` (Version 2) Index**: Starts with `\xFFtOc` header (4 bytes) + Version 2 (4 bytes) + **256-entry cumulative fanout table** (4 bytes per bucket) enabling $O(1)$ prefix partitioning followed by $O(\log N)$ binary search across sorted 20-byte SHA-1 hashes.

---

## 💻 Complete Command Reference (27 Commands)

| Category | Command | Description |
|---|---|---|
| **Setup & Remotes** | `minigit init` | Initialize a new empty Mini-Git repository with `main` branch. |
| | `minigit clone <src> [<dest>]` | Clone a repository, object store, and refs into a new working directory. |
| | `minigit remote add <name> <dir>` | Register a local remote tracking endpoint. |
| | `minigit fetch [<remote>]` | Download missing objects and update remote tracking refs (`refs/remotes/origin/main`). |
| | `minigit push [<remote>] [<branch>]`| Push local branch commits and synchronize remote refs. |
| | `minigit pull [<remote>] [<branch>]`| Fetch from remote and merge changes into active branch. |
| **Working Area** | `minigit add <path>...` | Stage files or directories, respecting `.minigitignore` patterns. |
| | `minigit status` | 3-way comparison of working directory, index, and HEAD with ANSI coloring. |
| | `minigit diff [--staged]` | Unified line diff between working copy & index, or index & HEAD. |
| | `minigit restore [--staged] <file>`| Discard working copy changes or unstage files from index. |
| | `minigit reset [--hard] [<files>]` | Unstage index files or wipe working tree back to HEAD (`--hard`). |
| | `minigit stash [push\|pop\|list]` | Safely stash uncommitted work onto dangling commits and restore them. |
| | `minigit clean [-f] [-d] [-n]` | Remove untracked files or directories with force/dry-run safety checks. |
| | `minigit gc` | Compact loose objects into binary `.pack` archives and prune disk space. |
| **Commit & History** | `minigit commit -m "<msg>"` | Create root tree from staging index, write commit DAG node, advance branch. |
| | `minigit log [--oneline] [--graph]`| Walk commit DAG backwards and render ASCII topology graphs. |
| | `minigit reflog` | Display reference transition history of HEAD and local branches. |
| | `minigit branch [-d] [<name>]` | List branches (active marked `*`), create branch, or delete branch. |
| | `minigit checkout [-b] <target>` | Switch branches, create and switch (`-b`), or checkout commit hash. |
| | `minigit merge <branch>` | Perform 3-way LCA merge, handle fast-forwards, or insert conflict markers. |
| | `minigit rebase [-i] <upstream>` | Linearly replay commits onto upstream; edit history with interactive todo list. |
| | `minigit cherry-pick <commit>` | Compute commit patch delta against its parent and apply cleanly onto HEAD. |
| | `minigit tag [-a] [-m] [-d] [<name>]`| Create lightweight tags, annotated tag objects, list, or delete tags. |
| **Plumbing** | `minigit cat-file (-p\|-t\|-s) <obj>`| Pretty-print content (`-p`), display type (`-t`), or uncompressed size (`-s`). |
| | `minigit ls-files [--stage]` | List all tracked files and mode/SHA-1 entries in the staging area. |
| | `minigit hash-object [-w] <file>` | Compute SHA-1 hash of a file and optionally write it to object database. |
| | `minigit help` | Display full colored CLI reference. |

---

## 🛠️ Build & Installation Guide

### Prerequisites
- **C++17 Compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **Build System**: CMake 3.10+ and Ninja (or Make)
- **Dependencies**: `zlib` (`libz-dev` on Linux, `zlib` via Homebrew on macOS, or `mingw-w64-ucrt-x86_64-zlib` on Windows)

### 1. Clone the Repository
```bash
git clone https://github.com/devzx0101/MiniGit.git
cd MiniGit
```

### 2. Configure & Build
```bash
# Configure build directory
cmake -B build -G "Ninja"

# Compile Mini-Git executable and test runner
cmake --build build
```

The compiled binaries will be available in:
- `build/minigit.exe` (or `build/minigit` on Unix) — The CLI executable.
- `build/minigit_tests.exe` (or `build/minigit_tests` on Unix) — The automated test suite.

---

## 🧪 Automated Integration Test Suite

Mini-Git contains **14 comprehensive test suites (100% pass rate)** covering end-to-end repository lifecycles:

```bash
# Run test suite
./build/minigit_tests.exe
```

### Test Coverage Matrix:
```
========================================
  Mini-Git Automated Integration Tests  
========================================

=== Test 1:  Init Repository (.minigit fanout, HEAD symbolic pointer)
=== Test 2:  Blob Storage & SHA-1 (Content-addressable hashing)
=== Test 3:  Index Staging & Tree Construction (Hierarchical directory trees)
=== Test 4:  Commit & History Navigation (Parent DAG chaining, metadata)
=== Test 5:  Branching & Checkout (Symbolic HEAD switching, branch CRUD)
=== Test 6:  Line-by-Line Unified Diff (LCS dynamic programming algorithm)
=== Test 7:  .minigitignore Patterns (Wildcards, directories, exact matches)
=== Test 8:  Tag Objects (Lightweight tags, listing, deletion)
=== Test 9:  3-Way Merge & Conflict Resolution (LCA traversal, <<<<<<< markers)
=== Test 10: Local Remote, Clone & Sync (Replication, tracking refs)
=== Test 11: Zlib Deflate/Inflate & Interoperability (RFC 1950 headers 0x78)
=== Test 12: Worktree Stash & Clean Operations (Push/pop, dry-run, force clean)
=== Test 13: History Replay: Cherry-Pick & Rebase (Interactive squashing, replay)
=== Test 14: Annotated Tag Objects & Packfile Compaction (GC, 256-bucket fanout)

========================================
  ALL 14 TEST SUITES PASSED! (100%)    
========================================
```

---

## 🚀 Quick Start & Interactive Demo

```bash
# 1. Initialize a new Mini-Git repository
minigit init

# 2. Stage and commit files
echo "print('Hello Mini-Git')" > app.py
minigit status
minigit add app.py
minigit commit -m "feat: initial commit"

# 3. Branching and DAG navigation
minigit branch feature
minigit checkout feature
echo "print('Feature update')" >> app.py
minigit commit -m "feat: add feature enhancement"
minigit log --oneline --graph

# 4. 3-Way Merge back into main
minigit checkout main
minigit merge feature

# 5. Create an annotated release tag
minigit tag -a v1.0 -m "Production release version 1.0"
minigit cat-file -p v1.0

# 6. Compact repository with garbage collection
minigit gc
```

---

## 📄 License

This project is open-source and released under the **[MIT License](LICENSE)**. Built for educational and systems-programming reference.
