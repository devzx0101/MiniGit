# Mini-Git: High-Performance Git Implementation in C++

<p align="center">
  <img src="https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=for-the-badge&logo=c%2B%2B" alt="C++17" />
  <img src="https://img.shields.io/badge/Build-CMake%20%7C%20Ninja-064F8C?style=for-the-badge&logo=cmake" alt="CMake" />
  <img src="https://img.shields.io/badge/Compression-zlib%20RFC%201950-blue?style=for-the-badge" alt="zlib" />
  <img src="https://img.shields.io/badge/Tests-14%2F14%20Passing%20(100%25)-success?style=for-the-badge" alt="Tests" />
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="MIT License" />
</p>

---

## 📌 Overview

**Mini-Git** is a lightweight, high-performance distributed version control system built from scratch in modern **C++17**. It models the exact architectural layers, algorithms, and on-disk binary structures of official Git:

- **Full Object Engine**: Content-addressable SHA-1 storage for **Blobs**, **Trees**, **Commits**, and **Annotated Tags**.
- **Binary Compatibility**: Standard RFC 1950 zlib streaming `DEFLATE`/`INFLATE` compression (`0x78` header family).
- **Packfile Compaction**: Garbage collection engine (`minigit gc`) bundling loose objects into binary `.pack` archives indexed with **256-bucket fan-out tables** for $O(\log N)$ binary search.
- **Graph & Sequence Algorithms**:
  - **Lowest Common Ancestor (LCA)** Directed Acyclic Graph (DAG) traversal for 3-way merges and history replays.
  - **Longest Common Subsequence (LCS)** dynamic programming for line-by-line unified diff calculation.
- **Advanced Workflows**: Stashing via dangling commits, interactive rebase (`pick`, `squash`, `reword`, `drop`), cherry-picking, safe clean, and local filesystem remotes (`clone`, `fetch`, `push`, `pull`).
- **Terminal CLI**: 27 fully registered commands with cross-platform ANSI color output.

---

## ⚡ Quick Start

### 1. Installation & Build

#### Prerequisites
- C++17 compatible compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.10+
- Ninja or Make
- `zlib` development library

#### Build from Source
```bash
# Clone repository
git clone https://github.com/devzx0101/MiniGit.git
cd MiniGit

# Configure & build executable
cmake -B build -G "Ninja"
cmake --build build
```

The compiled binaries will be generated inside the `build/` directory:
- `build/minigit.exe` (or `build/minigit` on Linux/macOS) — Command-Line Interface.
- `build/minigit_tests.exe` (or `build/minigit_tests` on Linux/macOS) — Test Runner.

### 2. Verify Installation
```bash
./build/minigit_tests.exe
```
Executes all **14 automated integration test suites (100% pass rate)**:
```text
========================================
  Mini-Git Automated Integration Tests  
========================================
...
========================================
  ALL 14 TEST SUITES PASSED! (100%)    
========================================
```

---

## 🚀 Basic Usage & Workflow

```bash
# 1. Initialize a repository
minigit init

# 2. Stage and commit files
echo "print('Hello Mini-Git')" > main.py
minigit status
minigit add main.py
minigit commit -m "feat: initial commit"

# 3. View commit graph
minigit log --oneline --graph

# 4. Branching and checkout
minigit branch feature
minigit checkout feature
echo "print('Feature ready')" >> main.py
minigit commit -m "feat: implement feature"
minigit log --oneline --graph

# 5. Merge feature branch into main
minigit checkout main
minigit merge feature

# 6. Tag releases
minigit tag -a v1.0.0 -m "Release version 1.0.0"
minigit cat-file -p v1.0.0

# 7. Optimize storage with garbage collection
minigit gc
```

---

## 💻 Command Reference (27 Commands)

| Category | Command | Description |
|---|---|---|
| **Repository & Remotes** | `minigit init` | Initialize an empty Mini-Git repository with `main` branch. |
| | `minigit clone <src> [<dest>]` | Clone a repository, object store, and refs into a new working tree. |
| | `minigit remote add <name> <dir>` | Register a local remote tracking endpoint. |
| | `minigit fetch [<remote>]` | Download objects and synchronize remote tracking references. |
| | `minigit push [<remote>] [<branch>]`| Push commits and update remote branch references. |
| | `minigit pull [<remote>] [<branch>]`| Fetch from remote and merge changes into active branch. |
| **Working Tree & Staging** | `minigit add <path>...` | Stage files or directories, honoring `.minigitignore` patterns. |
| | `minigit status` | 3-way status check (working directory, index, and HEAD). |
| | `minigit diff [--staged]` | Unified line diff between working copy & index, or index & HEAD. |
| | `minigit restore [--staged] <file>`| Discard working copy modifications or unstage files from index. |
| | `minigit reset [--hard] [<files>]` | Unstage index files or wipe working tree back to HEAD (`--hard`). |
| | `minigit stash [push\|pop\|list]` | Stash working directory changes onto dangling commit snapshots. |
| | `minigit clean [-f] [-d] [-n]` | Remove untracked files with force/dry-run safety validation. |
| | `minigit gc` | Pack loose objects into binary `.pack` archives and clean disk space. |
| **History & Branching** | `minigit commit -m "<msg>"` | Create root tree from staging index and advance active branch. |
| | `minigit log [--oneline] [--graph]`| Render commit history with branch decorations and ASCII DAG graph. |
| | `minigit reflog` | Display reference transition history of HEAD and local branches. |
| | `minigit branch [-d] [<name>]` | List branches (active marked `*`), create branch, or delete branch. |
| | `minigit checkout [-b] <target>` | Switch branches, create and switch (`-b`), or checkout commit hash. |
| | `minigit merge <branch>` | Perform 3-way LCA merge, fast-forward, or insert conflict markers. |
| | `minigit rebase [-i] <upstream>` | Replay commits onto upstream; edit history with interactive todo list. |
| | `minigit cherry-pick <commit>` | Apply patch delta from an existing commit directly onto HEAD. |
| | `minigit tag [-a] [-m] [-d] [<name>]`| Create lightweight/annotated tags, list tags, or delete tags. |
| **Inspection & Plumbing** | `minigit cat-file (-p\|-t\|-s) <obj>`| Pretty-print content (`-p`), display type (`-t`), or uncompressed size (`-s`). |
| | `minigit ls-files [--stage]` | List all tracked files and mode/SHA-1 entries in the staging area. |
| | `minigit hash-object [-w] <file>` | Compute SHA-1 hash of a file and optionally store it in object DB. |
| | `minigit help` | Display full colored CLI reference. |

---

## 🏛️ Architecture & Software Design

Mini-Git follows clean, decoupled object-oriented system design principles:

| Design Concept | Component / Subsystem | Implementation Details |
|---|---|---|
| **Encapsulation** | `Objects::Commit`, `Objects::Tag`, `Repository::Index` | Internal states (tree hashes, author timestamps, index entries) are `private`. All data mutations are strictly controlled through public APIs, preventing repository corruption. |
| **Data Abstraction** | `Repository::DiffEngine`, `Repository::MergeEngine`, `Objects::ObjectStore` | Complex algorithmic implementations (dynamic programming 2D matrices for **LCS** line diffs, and DAG traversals for **LCA** merges) are encapsulated behind clean static interfaces (`unifiedDiff()`, `findLCA()`). |
| **Separation of Concerns** | `Objects::*`, `Repository::*`, `Compression::*`, `FileUtils::*` | Decouples immutable content-addressable storage (`Objects`) from mutable working trees, references, and branch pointers (`Repository`). |
| **RAII Resource Safety** | `zlib_utils.cpp`, `file_utils.cpp` | Clean lifecycle management for `z_stream` buffers, file streams, and memory allocations without manual pointer leaks. |
| **STL Data Structures** | Maps, Vectors, Sets | Employs `std::map<std::string, IndexEntry>` for sorted $O(\log N)$ file path staging, and `std::set<std::string>` for union computations during 3-way merges. |

---

## 🏗️ Repository Layout

```
MiniGit/
├── CMakeLists.txt                 # CMake configuration & test targets
├── README.md                      # Technical documentation & quick start
├── src/
│   ├── main.cpp                   # Central CLI dispatcher routing all 27 commands
│   ├── terminal.hpp               # Cross-platform ANSI color subsystem with Win32 VTP
│   ├── filesystem/
│   │   ├── file_utils.hpp / .cpp  # Cross-platform filesystem operations & timestamps
│   ├── hashing/
│   │   ├── sha1.hpp / .cpp        # SHA-1 cryptographic implementation
│   ├── compression/
│   │   ├── zlib_utils.hpp / .cpp  # RFC 1950 zlib streaming DEFLATE / INFLATE
│   ├── objects/
│   │   ├── object_store.hpp / .cpp# Content-addressable object store (.minigit/objects/xx/...)
│   │   ├── blob.hpp / .cpp        # Immutable raw byte content objects
│   │   ├── tree.hpp / .cpp        # Hierarchical directory tree serialization
│   │   ├── commit.hpp / .cpp      # Commit DAG nodes (tree, parents, author, message)
│   │   ├── tag.hpp / .cpp         # Annotated tag objects (target, tagger, timestamp, message)
│   │   └── packfile.hpp / .cpp    # Binary .pack generator & .idx version 2 fanout index
│   ├── repository/
│   │   ├── index.hpp / .cpp       # Staging area persistence (.minigit/index)
│   │   ├── reference.hpp / .cpp   # HEAD, branches, tags, remote tracking refs, reflog
│   │   ├── ignore.hpp / .cpp      # Pattern & path filtering (.minigitignore)
│   │   ├── working_tree.hpp / .cpp# 3-way status detection, file restoration, checkout
│   │   ├── diff_engine.hpp / .cpp # Unified line diff engine (LCS dynamic programming)
│   │   ├── merge_engine.hpp / .cpp# Lowest Common Ancestor (LCA) traversal & 3-way merge
│   │   └── remote_manager.hpp / .cpp# Local file-based remotes (clone, fetch, push, pull)
│   └── commands/                  # 27 Modular Command Handlers
│       ├── init.cpp, add.cpp, commit.cpp, status.cpp, log.cpp, branch.cpp, ...
└── tests/
    └── test_runner.cpp            # 14 Automated Integration Test Suites (100% pass)
```

---

## 📄 License

This project is licensed under the **[MIT License](LICENSE)**.
