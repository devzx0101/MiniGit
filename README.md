# Mini-Git: A Functional Educational Git Implementation in C++

**Mini-Git** is a lightweight, educational Git implementation built from scratch in C++17. It models Git's core internal architecture, including content-addressable object storage (SHA-1 hashing), immutable blobs, hierarchical trees, commit directed acyclic graphs (DAG), symbolic references, index staging, 3-way branch merging with conflict resolution, and local repository remotes.

---

## Architecture & Subsystems

Mini-Git is architected into clean, decoupled subsystems:

```
src/
├── main.cpp                     # CLI dispatcher and argument routing
├── filesystem/
│   ├── file_utils.hpp / .cpp    # Cross-platform filesystem operations, recursive iterators, timestamps
├── hashing/
│   ├── sha1.hpp / .cpp          # Standard cryptographic SHA-1 implementation
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
    ├── status.cpp               # minigit status
    ├── log.cpp                  # minigit log [--oneline]
    ├── branch.cpp               # minigit branch [-d] [<name>]
    ├── checkout.cpp             # minigit checkout [-b] <name>
    ├── diff.cpp                 # minigit diff [--staged]
    ├── reset.cpp                # minigit reset [--hard] [<file>...]
    ├── restore.cpp              # minigit restore [--staged] <file>...
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
All objects are stored compressed or formatted as `<type> <size>\0<content>` in `.minigit/objects/xx/xxxxxxxx...`:

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
│   │   └── be0e134f79f4...
│   └── ...
├── refs/
│   ├── heads/              # Local branch pointers (e.g. main -> commit hash)
│   ├── tags/               # Lightweight tag pointers
│   └── remotes/            # Remote tracking branches (e.g. origin/main)
├── remotes/                # Configured remote paths
└── logs/                   # Reference transition logs for safety & recovery
    ├── HEAD
    └── refs/heads/
```

---

## Supported Commands

| Command | Description |
|---|---|
| `minigit init` | Initializes a new empty Mini-Git repository with default `main` branch. |
| `minigit add <path>...` | Stages files or entire recursive directories, filtering `.minigitignore` patterns. |
| `minigit commit -m "<msg>"` | Generates tree hierarchy from index, writes commit object, advances active branch. |
| `minigit status` | 3-way status: compares working directory, index, and HEAD commit. |
| `minigit log [--oneline]` | Walks the commit graph backwards and pretty-prints commit history. |
| `minigit branch [-d] [<name>]` | Lists all branches (marking active with `*`), creates new branch, or deletes branch. |
| `minigit checkout [-b] <target>` | Switches branch, creates new branch (`-b`), or checks out commit (detached HEAD). |
| `minigit diff [--staged]` | Unified line diff between working directory & index, or index & HEAD. |
| `minigit reset [--hard] [<files>]`| Unstages changes from index or restores entire working tree to HEAD (`--hard`). |
| `minigit restore [--staged] <file>`| Discards working copy modifications or unstages files from index. |
| `minigit tag [-d] [<tag-name>]` | Creates, lists, or deletes tags pointing to commits. |
| `minigit merge <branch>` | Performs 3-way merge, fast-forwards, or detects conflicts with `<<<<<<<` markers. |
| `minigit reflog` | Displays the reference transition history of HEAD and branches. |
| `minigit cat-file (-p\|-t\|-s) <obj>`| Pretty-prints object content (`-p`), displays type (`-t`), or size (`-s`). |
| `minigit ls-files [--stage]` | Lists all paths in the staging area along with object hashes. |
| `minigit hash-object [-w] <file>` | Computes SHA-1 hash of a file and optionally writes it to object DB. |
| `minigit remote add <name> <dir>` | Registers a local repository remote endpoint. |
| `minigit clone <src-dir> [<dest>]`| Clones a repository, its object database, refs, and checks out working tree. |
| `minigit fetch [<remote>]` | Downloads objects and updates remote tracking branches. |
| `minigit push [<remote>] [<branch>]`| Pushes missing local objects and updates remote branch pointers. |
| `minigit pull [<remote>] [<branch>]`| Fetches from remote and merges changes into active branch. |
| `minigit help` | Displays full CLI reference. |

---

## Build & Test Instructions

### Prerequisites
- C++17 compatible compiler (e.g. GCC 9+, Clang 10+, or MSVC 2019+)
- CMake 3.10+
- Ninja or Make

### Compiling with CMake
```bash
# Configure build directory
cmake -B build -G "Ninja"

# Build executable and test suite
cmake --build build
```

### Running the Test Suite
```bash
# Execute integration test runner
./build/minigit_tests.exe
```

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
minigit log --oneline

# 4. Create and checkout a feature branch
minigit branch feature
minigit checkout feature

# 5. Modify file and commit on feature branch
echo "print('Hello MiniGit with Feature')" > app.py
minigit diff
minigit add app.py
minigit commit -m "Add feature enhancement"
minigit log --oneline

# 6. Switch back to main and merge
minigit checkout main
minigit merge feature

# 7. Inspect Git objects and reflog
minigit tag v1.0
minigit tag
minigit reflog
minigit cat-file -p HEAD
```

---

## Differences & Simplifications Compared to Real Git

1. **Object Compression**: Real Git uses `zlib` deflate compression for loose objects and packfiles (`.pack` / `.idx`). MiniGit stores objects uncompressed in `.minigit/objects/xx/` to keep object inspection transparent and portable without external C library dependencies.
2. **Network Protocol**: Real Git implements the smart HTTP / SSH packfile wire protocol. MiniGit implements local file-based remotes (`clone`, `fetch`, `push`, `pull`), which faithfully replicate the object and ref synchronization model over the filesystem.
3. **Index Format**: Real Git uses a binary DIRC (Directory Cache) index with file timestamps, dev/inode stat caches. MiniGit uses a clear text-based index recording file modes, SHA-1 hashes, and relative paths.
4. **Merge Engine**: MiniGit implements a 3-way merge engine with Lowest Common Ancestor (LCA) commit graph discovery, line conflict marker insertion (`<<<<<<< HEAD`, `=======`, `>>>>>>>`), and fast-forward detection. Real Git includes additional merge strategies (recursive, octopus, rename detection).
