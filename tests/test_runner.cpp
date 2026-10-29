#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <filesystem>
#include "filesystem/file_utils.hpp"
#include "hashing/sha1.hpp"
#include "objects/object_store.hpp"
#include "objects/blob.hpp"
#include "objects/tree.hpp"
#include "objects/commit.hpp"
#include "repository/index.hpp"
#include "repository/reference.hpp"
#include "repository/ignore.hpp"
#include "repository/working_tree.hpp"
#include "repository/diff_engine.hpp"
#include "repository/merge_engine.hpp"
#include "repository/remote_manager.hpp"
#include "compression/zlib_utils.hpp"
#include "commands/clean.hpp"
#include "commands/stash.hpp"
#include "commands/add.hpp"
#include "commands/commit.hpp"
#include "commands/branch.hpp"
#include "commands/checkout.hpp"
#include "commands/cherry_pick.hpp"
#include "commands/rebase.hpp"

namespace fs = std::filesystem;
using namespace Repository;
using namespace Objects;

#define TEST_ASSERT(cond, msg) \
    if (!(cond)) { \
        std::cerr << "[-] FAILED: " << msg << " (line " << __LINE__ << ")\n"; \
        return false; \
    } else { \
        std::cout << "    [+] Passed: " << msg << "\n"; \
    }

static const std::string TEST_DIR = "test_sandbox";

void cleanupTestDir() {
    std::error_code ec;
    fs::remove_all(TEST_DIR, ec);
    fs::create_directories(TEST_DIR, ec);
}

bool testInit() {
    std::cout << "\n=== Test 1: Init Repository ===\n";
    std::string repoDir = TEST_DIR + "/.minigit";
    FileUtils::createDirectories(repoDir + "/objects");
    FileUtils::createDirectories(repoDir + "/refs/heads");
    FileUtils::writeFile(repoDir + "/HEAD", "ref: refs/heads/main\n");

    TEST_ASSERT(FileUtils::directoryExists(repoDir + "/objects"), "Objects directory created");
    TEST_ASSERT(FileUtils::directoryExists(repoDir + "/refs/heads"), "Refs heads directory created");
    TEST_ASSERT(FileUtils::fileExists(repoDir + "/HEAD"), "HEAD file created");

    std::string target;
    bool isSym = false;
    Reference::getHead(target, isSym, repoDir);
    TEST_ASSERT(isSym && target == "refs/heads/main", "HEAD points symbolically to refs/heads/main");
    return true;
}

bool testBlobAndHashing() {
    std::cout << "\n=== Test 2: Blob Storage & SHA-1 ===\n";
    std::string repoDir = TEST_DIR + "/.minigit";
    std::string testContent = "Hello, MiniGit World!\n";
    Objects::Blob blob(testContent);
    std::string hash = blob.save(repoDir);

    TEST_ASSERT(!hash.empty() && hash.length() == 40, "Blob SHA-1 hash is 40 hex characters");
    TEST_ASSERT(FileUtils::fileExists(repoDir + "/objects/" + hash.substr(0, 2) + "/" + hash.substr(2)), "Blob stored in loose object fanout path");

    Objects::Blob loadedBlob;
    bool loaded = Objects::Blob::load(hash, loadedBlob, repoDir);
    TEST_ASSERT(loaded && loadedBlob.getContent() == testContent, "Blob retrieved from storage matches original content");
    return true;
}

bool testIndexAndTree() {
    std::cout << "\n=== Test 3: Index Staging & Tree Construction ===\n";
    std::string repoDir = TEST_DIR + "/.minigit";

    // Create staged files
    std::string f1Path = TEST_DIR + "/hello.txt";
    std::string f2Path = TEST_DIR + "/src/main.cpp";
    FileUtils::writeFile(f1Path, "print('hello')\n");
    FileUtils::writeFile(f2Path, "int main() { return 0; }\n");

    Objects::Blob b1, b2;
    Objects::Blob::fromFile(f1Path, b1);
    Objects::Blob::fromFile(f2Path, b2);
    std::string h1 = b1.save(repoDir);
    std::string h2 = b2.save(repoDir);

    Repository::Index index;
    index.addEntry("hello.txt", h1);
    index.addEntry("src/main.cpp", h2);
    index.save(repoDir);

    Repository::Index loadedIndex;
    loadedIndex.load(repoDir);
    TEST_ASSERT(loadedIndex.getEntries().size() == 2, "Index correctly preserves staged entries");

    std::string rootTreeHash = Objects::Tree::buildFromIndex(loadedIndex, repoDir);
    TEST_ASSERT(!rootTreeHash.empty() && rootTreeHash.length() == 40, "Root tree constructed and stored");

    std::map<std::string, Repository::IndexEntry> flattened;
    Objects::Tree::flatten(rootTreeHash, flattened, repoDir);
    TEST_ASSERT(flattened.size() == 2, "Tree flattened back to exactly 2 files");
    TEST_ASSERT(flattened.count("hello.txt") && flattened["hello.txt"].hash == h1, "hello.txt matched in flattened tree");
    TEST_ASSERT(flattened.count("src/main.cpp") && flattened["src/main.cpp"].hash == h2, "src/main.cpp matched in flattened tree");
    return true;
}

bool testCommitAndLog() {
    std::cout << "\n=== Test 4: Commit & History Navigation ===\n";
    std::string repoDir = TEST_DIR + "/.minigit";

    Repository::Index index;
    index.load(repoDir);
    std::string treeHash = Objects::Tree::buildFromIndex(index, repoDir);

    // Commit 1: Initial commit
    Objects::Commit c1;
    c1.setTreeHash(treeHash);
    c1.setMessage("Initial commit");
    std::string c1Hash = c1.save(repoDir);
    Repository::Reference::setBranchCommit("main", c1Hash, repoDir);
    Repository::Reference::appendReflog("HEAD", "", c1Hash, "commit: Initial commit", repoDir);

    std::string resolvedHead;
    Repository::Reference::resolveHead(resolvedHead, repoDir);
    TEST_ASSERT(resolvedHead == c1Hash, "HEAD resolves to initial commit hash");

    // Commit 2: Second commit
    Objects::Commit c2;
    c2.setTreeHash(treeHash);
    c2.addParentHash(c1Hash);
    c2.setMessage("Second commit");
    std::string c2Hash = c2.save(repoDir);
    Repository::Reference::setBranchCommit("main", c2Hash, repoDir);

    Objects::Commit loadedC2;
    Objects::Commit::load(c2Hash, loadedC2, repoDir);
    TEST_ASSERT(loadedC2.getFirstParentHash() == c1Hash, "Second commit parent points to first commit");
    TEST_ASSERT(loadedC2.getMessage() == "Second commit", "Commit message retained properly");
    return true;
}

bool testBranchAndCheckout() {
    std::cout << "\n=== Test 5: Branching & Checkout ===\n";
    std::string repoDir = TEST_DIR + "/.minigit";

    std::string headCommit;
    Repository::Reference::resolveHead(headCommit, repoDir);

    // Create branch 'feature'
    bool brCreated = Repository::Reference::setBranchCommit("feature", headCommit, repoDir);
    TEST_ASSERT(brCreated, "Branch 'feature' created pointing to HEAD");

    auto branches = Repository::Reference::listBranches(repoDir);
    TEST_ASSERT(branches.size() >= 2, "Branch list contains multiple branches");

    // Switch to feature branch
    Repository::Reference::setHeadSymbolic("refs/heads/feature", repoDir);
    TEST_ASSERT(Repository::Reference::getCurrentBranchName(repoDir) == "feature", "Current branch is now feature");

    // Switch back to main
    Repository::Reference::setHeadSymbolic("refs/heads/main", repoDir);
    TEST_ASSERT(Repository::Reference::getCurrentBranchName(repoDir) == "main", "Switched back to main branch");
    return true;
}

bool testDiffEngine() {
    std::cout << "\n=== Test 6: Line-by-Line Unified Diff ===\n";
    std::string oldText = "line 1\nline 2\nline 3\n";
    std::string newText = "line 1\nline 2 modified\nline 3\nline 4\n";

    std::string diff = Repository::DiffEngine::unifiedDiff("test.txt", oldText, newText);
    TEST_ASSERT(!diff.empty(), "Diff generated successfully");
    TEST_ASSERT(diff.find("-line 2") != std::string::npos, "Diff detects removed line");
    TEST_ASSERT(diff.find("+line 2 modified") != std::string::npos, "Diff detects modified line");
    TEST_ASSERT(diff.find("+line 4") != std::string::npos, "Diff detects added line");
    return true;
}

bool testIgnore() {
    std::cout << "\n=== Test 7: .minigitignore Patterns ===\n";
    Repository::Ignore ignore;
    ignore.addPattern("*.log");
    ignore.addPattern("build/");
    ignore.addPattern("secret.env");

    TEST_ASSERT(ignore.isIgnored("debug.log"), "*.log pattern correctly ignored");
    TEST_ASSERT(ignore.isIgnored("build/output.exe"), "build/ directory contents correctly ignored");
    TEST_ASSERT(ignore.isIgnored("secret.env"), "Exact filename match ignored");
    TEST_ASSERT(!ignore.isIgnored("main.cpp"), "Tracked source file NOT ignored");
    return true;
}

bool testTags() {
    std::cout << "\n=== Test 8: Tag Objects ===\n";
    std::string repoDir = TEST_DIR + "/.minigit";
    std::string headCommit;
    Repository::Reference::resolveHead(headCommit, repoDir);

    Repository::Reference::setTag("v1.0", headCommit, repoDir);
    TEST_ASSERT(Repository::Reference::tagExists("v1.0", repoDir), "Tag v1.0 exists");

    std::string tagCommit;
    Repository::Reference::getTagCommit("v1.0", tagCommit, repoDir);
    TEST_ASSERT(tagCommit == headCommit, "Tag points to correct commit hash");

    auto tags = Repository::Reference::listTags(repoDir);
    TEST_ASSERT(!tags.empty() && tags[0] == "v1.0", "Tag list returns v1.0");

    Repository::Reference::deleteTag("v1.0", repoDir);
    TEST_ASSERT(!Repository::Reference::tagExists("v1.0", repoDir), "Tag successfully deleted");
    return true;
}

bool test3WayMergeAndConflicts() {
    std::cout << "\n=== Test 9: 3-Way Merge & Conflict Resolution ===\n";
    // 1. Test clean 3-way line merge
    std::string base = "apple\nbanana\ncherry\n";
    std::string ours = "apple\nbanana modified\ncherry\n";
    std::string theirs = "apple\nbanana\ncherry\n";
    std::string merged;
    bool clean = Repository::MergeEngine::merge3WayLines(base, ours, theirs, "HEAD", "feature", merged);
    TEST_ASSERT(clean && merged == ours, "Clean 3-way merge auto-selects non-conflicting changes");

    // 2. Test conflicting 3-way merge
    std::string theirsConflict = "apple\nbanana different\ncherry\n";
    clean = Repository::MergeEngine::merge3WayLines(base, ours, theirsConflict, "HEAD", "feature", merged);
    TEST_ASSERT(!clean, "Conflict detected when both sides modify the same lines differently");
    TEST_ASSERT(merged.find("<<<<<<< HEAD") != std::string::npos, "Conflict marker <<<<<<< inserted");
    TEST_ASSERT(merged.find("=======") != std::string::npos, "Conflict separator ======= inserted");
    TEST_ASSERT(merged.find(">>>>>>> feature") != std::string::npos, "Conflict marker >>>>>>> inserted");
    return true;
}

bool testLocalRemoteOperations() {
    std::cout << "\n=== Test 10: Local Remote, Clone & Sync ===\n";
    std::string srcRepo = TEST_DIR;
    std::string cloneDest = "test_clone_sandbox";

    std::error_code ec;
    fs::remove_all(cloneDest, ec);

    bool cloned = Repository::RemoteManager::cloneRepo(srcRepo, cloneDest);
    TEST_ASSERT(cloned, "Repository successfully cloned to target directory");
    TEST_ASSERT(FileUtils::directoryExists(cloneDest + "/.minigit"), "Cloned .minigit directory created");
    TEST_ASSERT(FileUtils::fileExists(cloneDest + "/.minigit/refs/remotes/origin/main"), "Remote tracking branch origin/main established");

    fs::remove_all(cloneDest, ec);
    return true;
}

bool testZlibCompression() {
    std::cout << "\n=== Test 11: Zlib Deflate/Inflate & Git Object Interoperability ===\n";
    std::string repoDir = TEST_DIR + "/.minigit";

    std::string testString = "MiniGit true Git compatibility with standard zlib compression!";
    std::string compressed;
    bool compSuccess = Compression::compressString(testString, compressed);
    TEST_ASSERT(compSuccess && !compressed.empty(), "String compressed successfully with zlib deflate");
    TEST_ASSERT(Compression::isZlibCompressed(compressed), "Compressed data conforms to zlib RFC 1950 header");

    std::string decompressed;
    bool decompSuccess = Compression::decompressString(compressed, decompressed);
    TEST_ASSERT(decompSuccess && decompressed == testString, "Decompressed data precisely matches original input");

    // Test loose object file compression on disk
    std::string fileContent = "Content-addressable storage under zlib compression";
    Blob blob(fileContent);
    std::string hash = blob.save(repoDir);
    TEST_ASSERT(!hash.empty(), "Compressed object stored into repository");

    std::string objPath = repoDir + "/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    std::string rawDiskBytes;
    FileUtils::readFile(objPath, rawDiskBytes);
    TEST_ASSERT(Compression::isZlibCompressed(rawDiskBytes), "Object on disk begins with zlib magic header (0x78)");

    Blob loadedBlob;
    bool loaded = Blob::load(hash, loadedBlob, repoDir);
    TEST_ASSERT(loaded && loadedBlob.getContent() == fileContent, "Object inflated transparently on read");
    return true;
}

bool testCleanAndStash() {
    std::cout << "\n=== Test 12: Worktree Stash & Clean Operations ===\n";
    fs::path origPath = fs::current_path();
    fs::current_path(TEST_DIR);

    // 1. Test Clean
    FileUtils::writeFile("untracked_junk.tmp", "Temporary garbage\n");
    TEST_ASSERT(FileUtils::fileExists("untracked_junk.tmp"), "Untracked file created");

    char* dryArgs[] = {(char*)"minigit", (char*)"clean", (char*)"-n"};
    int dryRes = handleClean(3, dryArgs);
    TEST_ASSERT(dryRes == 0, "Clean dry-run executed successfully");
    TEST_ASSERT(FileUtils::fileExists("untracked_junk.tmp"), "Untracked file preserved after dry-run");

    char* forceArgs[] = {(char*)"minigit", (char*)"clean", (char*)"-f"};
    int forceRes = handleClean(3, forceArgs);
    TEST_ASSERT(forceRes == 0, "Clean with -f force flag executed successfully");
    TEST_ASSERT(!FileUtils::fileExists("untracked_junk.tmp"), "Untracked file cleaned from working tree");

    // 2. Test Stash
    FileUtils::writeFile("work_file.txt", "Initial base content\n");
    char* addArgs[] = {(char*)"minigit", (char*)"add", (char*)"work_file.txt"};
    handleAdd(3, addArgs);
    char* commitArgs[] = {(char*)"minigit", (char*)"commit", (char*)"-m", (char*)"Base commit for stash"};
    handleCommit(4, commitArgs);

    // Modify file
    FileUtils::writeFile("work_file.txt", "Uncommitted work in progress\n");

    char* stashPushArgs[] = {(char*)"minigit", (char*)"stash", (char*)"push"};
    int stashPushRes = handleStash(3, stashPushArgs);
    TEST_ASSERT(stashPushRes == 0, "Stash push captured dirty working tree");

    std::string contentReverted;
    FileUtils::readFile("work_file.txt", contentReverted);
    TEST_ASSERT(contentReverted.find("Initial base content") != std::string::npos, "Working tree reverted cleanly to HEAD");
    TEST_ASSERT(FileUtils::fileExists(".minigit/refs/stash"), "Stash commit ref written to .minigit/refs/stash");

    char* stashPopArgs[] = {(char*)"minigit", (char*)"stash", (char*)"pop"};
    int stashPopRes = handleStash(3, stashPopArgs);
    TEST_ASSERT(stashPopRes == 0, "Stash pop executed successfully");

    std::string contentRestored;
    FileUtils::readFile("work_file.txt", contentRestored);
    TEST_ASSERT(contentRestored.find("Uncommitted work in progress") != std::string::npos, "WIP modifications restored after stash pop");

    fs::current_path(origPath);
    return true;
}

bool testCherryPickAndRebase() {
    std::cout << "\n=== Test 13: History Replay: Cherry-Pick & Rebase ===\n";
    fs::path origPath = fs::current_path();
    fs::current_path(TEST_DIR);

    // 1. Create a topic branch with a unique commit
    char* brTopicArgs[] = {(char*)"minigit", (char*)"branch", (char*)"topic"};
    handleBranch(3, brTopicArgs);
    char* coTopicArgs[] = {(char*)"minigit", (char*)"checkout", (char*)"topic"};
    handleCheckout(3, coTopicArgs);

    FileUtils::writeFile("topic.txt", "Topic independent change\n");
    char* addTopicArgs[] = {(char*)"minigit", (char*)"add", (char*)"topic.txt"};
    handleAdd(3, addTopicArgs);
    char* commitTopicArgs[] = {(char*)"minigit", (char*)"commit", (char*)"-m", (char*)"Topic branch commit"};
    handleCommit(4, commitTopicArgs);

    std::string topicHash;
    Repository::Reference::resolveHead(topicHash, ".minigit");
    TEST_ASSERT(!topicHash.empty(), "Topic commit generated");

    // 2. Advance main branch with a separate commit
    char* coMainArgs[] = {(char*)"minigit", (char*)"checkout", (char*)"main"};
    handleCheckout(3, coMainArgs);

    FileUtils::writeFile("main_update.txt", "Main branch update line\n");
    char* addMainArgs[] = {(char*)"minigit", (char*)"add", (char*)"main_update.txt"};
    handleAdd(3, addMainArgs);
    char* commitMainArgs[] = {(char*)"minigit", (char*)"commit", (char*)"-m", (char*)"Main branch update"};
    handleCommit(4, commitMainArgs);

    // 3. Test Cherry-Pick: Apply topic commit onto a separate hotfix branch
    char* brHotfixArgs[] = {(char*)"minigit", (char*)"branch", (char*)"hotfix"};
    handleBranch(3, brHotfixArgs);
    char* coHotfixArgs[] = {(char*)"minigit", (char*)"checkout", (char*)"hotfix"};
    handleCheckout(3, coHotfixArgs);

    char* cpArgs[] = {(char*)"minigit", (char*)"cherry-pick", (char*)topicHash.c_str()};
    int cpRes = handleCherryPick(3, cpArgs);
    TEST_ASSERT(cpRes == 0, "Cherry-pick completed successfully");
    TEST_ASSERT(FileUtils::fileExists("topic.txt"), "Cherry-picked file applied to hotfix working tree");

    std::string hotfixHead;
    Repository::Reference::resolveHead(hotfixHead, ".minigit");
    TEST_ASSERT(!hotfixHead.empty() && hotfixHead != topicHash, "Cherry-pick created a distinct commit hash");

    // 4. Test Rebase: Rebase topic branch on top of main
    handleCheckout(3, coTopicArgs);
    char* rebaseArgs[] = {(char*)"minigit", (char*)"rebase", (char*)"main"};
    int rebaseRes = handleRebase(3, rebaseArgs);
    TEST_ASSERT(rebaseRes == 0, "Rebase executed successfully");
    TEST_ASSERT(FileUtils::fileExists("main_update.txt"), "Upstream file present on rebased topic branch");
    TEST_ASSERT(FileUtils::fileExists("topic.txt"), "Topic file preserved after rebase");

    // 5. Test Interactive Rebase: Squashing commits
    FileUtils::writeFile("extra_part.txt", "Extra part\n");
    char* addExtra[] = {(char*)"minigit", (char*)"add", (char*)"extra_part.txt"};
    handleAdd(3, addExtra);
    char* commitExtra[] = {(char*)"minigit", (char*)"commit", (char*)"-m", (char*)"Extra part commit"};
    handleCommit(4, commitExtra);

    std::string extraCommitHash;
    Repository::Reference::resolveHead(extraCommitHash, ".minigit");

    // Create interactive todo list squashing extra commit into topic commit
    std::string todoContent = "pick " + extraCommitHash.substr(0, 7) + " Extra part commit\nsquash " + extraCommitHash.substr(0, 7) + " Extra part commit\n";
    FileUtils::writeFile(".minigit/rebase-todo", "squash " + extraCommitHash.substr(0, 7) + " Extra part commit\n");

    char* iRebaseArgs[] = {(char*)"minigit", (char*)"rebase", (char*)"-i", (char*)"main"};
    int iRebaseRes = handleRebase(4, iRebaseArgs);
    TEST_ASSERT(iRebaseRes == 0, "Interactive rebase executed successfully with squashing");
    TEST_ASSERT(FileUtils::fileExists("extra_part.txt"), "Squashed file preserved in working tree");

    fs::current_path(origPath);
    return true;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  Mini-Git Automated Integration Tests  \n";
    std::cout << "========================================\n";

    cleanupTestDir();

    bool allPassed = true;
    allPassed &= testInit();
    allPassed &= testBlobAndHashing();
    allPassed &= testIndexAndTree();
    allPassed &= testCommitAndLog();
    allPassed &= testBranchAndCheckout();
    allPassed &= testDiffEngine();
    allPassed &= testIgnore();
    allPassed &= testTags();
    allPassed &= test3WayMergeAndConflicts();
    allPassed &= testLocalRemoteOperations();
    allPassed &= testZlibCompression();
    allPassed &= testCleanAndStash();
    allPassed &= testCherryPickAndRebase();

    cleanupTestDir();

    std::cout << "\n========================================\n";
    if (allPassed) {
        std::cout << "  ALL 13 TEST SUITES PASSED! (100%)    \n";
        std::cout << "========================================\n";
        return 0;
    } else {
        std::cerr << "  SOME TESTS FAILED!                   \n";
        std::cout << "========================================\n";
        return 1;
    }
}
