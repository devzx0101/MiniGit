# Mini-Git: A Functional Educational Git Implementation in C++

**Mini-Git** is a lightweight, educational Git implementation built from scratch in C++17. It models Git's core internal architecture, including content-addressable object storage (SHA-1 hashing), immutable blobs, hierarchical trees, commit directed acyclic graphs (DAG), symbolic references, index staging, 3-way branch merging with conflict resolution, true RFC 1950 zlib compression, history replay (cherry-pick, rebase), worktree stashing, and local repository remotes.

---

## Architecture & Subsystems

Mini-Git is architected into clean, decoupled subsystems:

```
src/
├── main.cpp                     # CLI dispatcher routing all 26 commands
├── terminal.hpp                 # Cross-platform ANSI color subsystem with Win32 VTP
├── filesystem/
│   ├── file_utils.hpp / .cpp    # Cross-platform filesystem operations, recursive iterators, timestamps
├── hashing/
│   ├── sha1.hpp / .cpp          # Standard cryptographic SHA-1 implementation
├── compression/
│   ├── zlib_utils.hpp / .cpp    # RFC 1950 zlib streaming DEFLATE / INFLATE compression
├── objects/
│   ├── object_store.hpp / .cpp  # Content-addressable object storage (.minigit/objects/xx/...)
│   ├── blob.hpp / .cpp          # File content blobs
│   ├── tree.hpp / .cpp          # Hierarchical directory trees with index serialization
│   └── commit.hpp / .cpp        # Commits (tree hash, parents, metadata, message)
├── repository/
│   ├── index.hpp / .cpp         # Staging area binary/text record persistence (.minigit/index)
│   ├── reference.hpp / .cpp     # HEAD, branch references, lightweight tags, and reflog
│   ├── ignore.hpp / .cpp        # .minigitignore / .gitignore wildcard and path filtering
│   ├── working_tree.hpp / .cpp  # 3-way status detection, file restoration, and safe checkout
│   ├── diff_engine.hpp / .cpp   # Line-by-line unified diff computation (LCS algorithm)
│   ├── merge_engine.hpp / .cpp  # Lowest Common Ancestor (LCA) traversal & 3-way line merge
│   └── remote_manager.hpp / .cpp# Local file-based remotes (clone, fetch, push, pull)
└── commands/
    ├── init.cpp                 # minigit init
    ├── add.cpp                  # minigit add <file>... / .
    ├── commit.cpp               # minigit commit -m <msg>
    ├── status.cpp               # minigit status (color-coded staged/unstaged)
    ├── log.cpp                  # minigit log [--oneline] [--graph]
    ├── branch.cpp               # minigit branch [-d] [<name>]
    ├── checkout.cpp             # minigit checkout [-b] <name>
    ├── diff.cpp                 # minigit diff [--staged]
    ├── reset.cpp                # minigit reset [--hard] [<file>...]
    ├── restore.cpp              # minigit restore [--staged] <file>...
    ├── stash.cpp                # minigit stash [push|pop|list|clear|drop]
    ├── clean.cpp                # minigit clean [-f] [-d] [-n]
    ├── cherry_pick.cpp          # minigit cherry-pick <commit>
    ├── rebase.cpp               # minigit rebase <upstream>
    ├── tag.cpp                  # minigit tag [-d] [<name>]
    ├── merge.cpp                # minigit merge <branch>
    ├── reflog.cpp               # minigit reflog
    ├── cat_file.cpp             # minigit cat-file (-p|-t|-s) <object>
    ├── ls_files.cpp             # minigit ls-files [--stage]
    ├── hash_object.cpp          # minigit hash-object [-w] <file>
    ├── remote.cpp               # minigit remote / clone / fetch / push / pull
    └── help.cpp                 # minigit help
```

---

## Object Model & Repository Structure

### The 4 Git Object Types
All objects are stored compressed as `zlib_deflate(<type> <size>\0<content>)` in `.minigit/objects/xx/xxxxxxxx...`:

1. **Blob (`blob`)**: Stores raw file byte contents without path or permission metadata.
2. **Tree (`tree`)**: Stores directory nodes containing lists of `<mode> <type> <hash>\t<name>`. Hierarchically represents nested project folders.
3. **Commit (`commit`)**: Points to a root tree hash, zero or more parent commit hashes, author/committer identities, ISO timestamps, and a commit message.
4. **Tag (`tag`)**: Lightweight references in `.minigit/refs/tags/<name>` pointing to specific commit hashes.

### Directory Structure of `.minigit/`
```
.minigit/
├── HEAD                    # Contains "ref: refs/heads/main" or detached commit hash
├── config                  # Repository configuration settings
├── index                   # Staging area tracking mode, SHA-1, and relative file paths
├── objects/                # Object database indexed by first 2 hex chars of SHA-1
│   ├── 9e/
│   │   └── be0e134f79f4... # zlib-compressed object file
│   └── ...
├── refs/
│   ├── heads/              # Local branch pointers (e.g. main -> commit hash)
│   ├── tags/               # Lightweight tag pointers
│   ├── stash               # Top of the stash stack commit pointer
│   └── remotes/            # Remote tracking branches (e.g. origin/main)
├── remotes/                # Configured remote paths
└── logs/                   # Reference transition logs for safety & recovery
    ├── HEAD
    ├── refs/heads/
    └── refs/stash          # Stash log history (stash@{0}, stash@{1}, ...)
```

---

## Supported Commands (26 CLI Commands)

| Command | Description |
|---|---|
| `minigit init` | Initializes a new empty Mini-Git repository with default `main` branch. |
| `minigit clone <src-dir> [<dest>]`| Clones a repository, its object database, refs, and checks out working tree. |
| `minigit add <path>...` | Stages files or entire recursive directories, filtering `.minigitignore` patterns. |
| `minigit restore [--staged] <file>`| Discards working copy modifications or unstages files from index. |
| `minigit reset [--hard] [<files>]`| Unstages changes from index or restores entire working tree to HEAD (`--hard`). |
| `minigit status` | 3-way status: compares working directory, index, and HEAD commit with ANSI colors. |
| `minigit diff [--staged]` | Unified line diff between working directory & index, or index & HEAD with color output. |
| `minigit stash [push\|pop\|list\|clear\|drop]` | Stashes dirty changes into dangling commits; restores or prunes stash stack. |
| `minigit clean [-f] [-d] [-n]` | Removes untracked files/directories from working tree with force and dry-run safety. |
| `minigit log [--oneline] [--graph]`| Walks the commit DAG backwards, displaying ASCII topology graphs and decorations. |
| `minigit reflog` | Displays the reference transition history of HEAD and branches. |
| `minigit cat-file (-p\|-t\|-s) <obj>`| Pretty-prints object content (`-p`), displays type (`-t`), or size (`-s`). |
| `minigit ls-files [--stage]` | Lists all paths in the staging area along with object hashes. |
| `minigit hash-object [-w] <file>` | Computes SHA-1 hash of a file and optionally writes it to object DB. |
| `minigit branch [-d] [<name>]` | Lists all branches (marking active with `*`), creates new branch, or deletes branch. |
| `minigit checkout [-b] <target>` | Switches branch, creates new branch (`-b`), or checks out commit (detached HEAD). |
| `minigit commit -m "<msg>"` | Generates tree hierarchy from index, writes commit object, advances active branch. |
| `minigit merge <branch>` | Performs 3-way merge, fast-forwards, or detects conflicts with `<<<<<<<` markers. |
| `minigit rebase <upstream>` | Replays commits on top of another base tip using Lowest Common Ancestor (LCA). |
| `minigit cherry-pick <commit>` | Computes patch delta and applies changes from an existing commit onto HEAD. |
| `minigit tag [-d] [<tag-name>]` | Creates, lists, or deletes tags pointing to commits. |
| `minigit remote add <name> <dir>` | Registers a local repository remote endpoint. |
| `minigit fetch [<remote>]` | Downloads objects and updates remote tracking branches. |
| `minigit push [<remote>] [<branch>]`| Pushes missing local objects and updates remote branch pointers. |
| `minigit pull [<remote>] [<branch>]`| Fetches from remote and merges changes into active branch. |
| `minigit help` | Displays full colored CLI reference. |

---

## Build & Test Instructions

### Prerequisites
- C++17 compatible compiler (e.g. GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.10+
- ZLIB library (`libz` / MSYS2 `mingw-w64-ucrt-x86_64-zlib`)
- Ninja or Make

### Compiling with CMake
```bash
# Configure build directory
cmake -B build -G "Ninja"

# Build executable and test suite
cmake --build build
```

### Running the Automated Test Suite
```bash
# Execute integration test runner
./build/minigit_tests.exe
```

Mini-Git includes **13 comprehensive test suites (100% pass rate)**:
1. **Test 1**: Init Repository (`.minigit` structure, HEAD symbolic ref)
2. **Test 2**: Blob Storage & SHA-1 (40-char hashing, loose fanout storage)
3. **Test 3**: Index Staging & Tree Construction (hierarchical recursive trees)
4. **Test 4**: Commit & History Navigation (parent chaining, message retention)
5. **Test 5**: Branching & Checkout (pointer switching, branch lifecycle)
6. **Test 6**: Line-by-Line Unified Diff (LCS algorithm, insertions, deletions)
7. **Test 7**: `.minigitignore` Patterns (wildcards, directories, exact matches)
8. **Test 8**: Tag Objects (lightweight tags, tag listing, deletion)
9. **Test 9**: 3-Way Merge & Conflict Resolution (LCA discovery, `<<<<<<<` markers)
10. **Test 10**: Local Remote, Clone & Sync (object replication, tracking refs)
11. **Test 11**: Zlib Deflate/Inflate & Git Object Interoperability (RFC 1950 headers `0x78`)
12. **Test 12**: Worktree Stash & Clean Operations (dry-run, force clean, stash push/pop)
13. **Test 13**: History Replay: Cherry-Pick & Rebase (delta application, linear replay)

---

## Example Demonstration Workflow

```bash
# 1. Initialize repository
minigit init

# 2. Stage files and check status
echo "print('Hello MiniGit')" > app.py
minigit status
minigit add app.py
minigit status

# 3. Create initial commit
minigit commit -m "Initial commit"
minigit log --oneline --graph

# 4. Create and checkout a feature branch
minigit branch feature
minigit checkout feature

# 5. Modify file and commit on feature branch
echo "print('Hello MiniGit with Feature')" > app.py
minigit diff
minigit add app.py
minigit commit -m "Add feature enhancement"
minigit log --oneline --graph

# 6. Switch back to main and merge
minigit checkout main
minigit merge feature

# 7. Advanced history manipulation (stash, cherry-pick, rebase)
echo "WIP changes" >> app.py
minigit stash push
minigit status
minigit stash pop

# 8. Inspect Git objects and reflog
minigit tag v1.0
minigit tag
minigit reflog
minigit cat-file -p HEAD
```

---

## True Git Compatibility & Architectural Notes

1. **Object Compression**: Mini-Git implements **RFC 1950 zlib DEFLATE/INFLATE streaming compression**, matching the exact binary loose object format of official Git (beginning with the standard `0x78` header family) with transparent decompression.
2. **Commit Directed Acyclic Graph (DAG)**: Merges, rebases, and cherry-picks evaluate the commit graph using the **Lowest Common Ancestor (LCA)** algorithm to locate split points and compute 3-way line merges.
3. **Diff Engine**: Unified diff generation uses the dynamic programming **Longest Common Subsequence (LCS)** algorithm to detect minimal line edits.
4. **Local Remotes**: Faithfully replicates Git's object and ref synchronization model over the filesystem (`clone`, `fetch`, `push`, `pull`).
