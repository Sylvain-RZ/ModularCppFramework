/**
 * @file test_filesystem.cpp
 * @brief Unit tests for FileSystem using Catch2 v3
 */

#define CATCH_CONFIG_MAIN
#include <catch_amalgamated.hpp>

#include "../../core/FileSystem.hpp"
#include <string>
#include <vector>
#include <thread>
#include <chrono>

using namespace mcf;

// Test fixture for file system tests
class FileSystemTestFixture {
public:
    FileSystem fs;
    std::string testDir = "./test_filesystem_temp";

    void SetUp() {
        // Clean up any existing test directory
        if (fs.exists(testDir)) {
            fs.removeAll(testDir);
        }
        // Create fresh test directory
        fs.createDirectory(testDir);
    }

    void TearDown() {
        // Clean up test directory
        if (fs.exists(testDir)) {
            fs.removeAll(testDir);
        }
    }

    std::string getTestPath(const std::string& relative) {
        return Path::join(testDir, relative);
    }
};

TEST_CASE("FileSystem - Basic existence checks", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    SECTION("Check directory exists") {
        REQUIRE(fixture.fs.exists(fixture.testDir));
        REQUIRE(fixture.fs.isDirectory(fixture.testDir));
    }

    SECTION("Check non-existent file") {
        REQUIRE_FALSE(fixture.fs.exists(fixture.getTestPath("nonexistent.txt")));
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - File writing and reading", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    std::string testFile = fixture.getTestPath("test.txt");
    std::string content = "Hello, FileSystem!";

    SECTION("Write and read string") {
        REQUIRE(fixture.fs.writeFile(testFile, content));
        REQUIRE(fixture.fs.exists(testFile));
        REQUIRE(fixture.fs.isFile(testFile));

        std::string readContent = fixture.fs.readFile(testFile);
        REQUIRE(readContent == content);
    }

    SECTION("Append to file") {
        REQUIRE(fixture.fs.writeFile(testFile, "Line 1\n"));
        REQUIRE(fixture.fs.writeFile(testFile, "Line 2\n", true));

        std::string readContent = fixture.fs.readFile(testFile);
        REQUIRE(readContent == "Line 1\nLine 2\n");
    }

    SECTION("File size") {
        REQUIRE(fixture.fs.writeFile(testFile, content));
        size_t size = fixture.fs.getFileSize(testFile);
        REQUIRE(size == content.length());
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - Binary file operations", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    std::string testFile = fixture.getTestPath("binary.dat");
    std::vector<uint8_t> data = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD};

    SECTION("Write and read binary") {
        REQUIRE(fixture.fs.writeBinary(testFile, data));

        auto readData = fixture.fs.readBinary(testFile);
        REQUIRE(readData == data);
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - Line-based operations", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    std::string testFile = fixture.getTestPath("lines.txt");
    std::vector<std::string> lines = {
        "First line",
        "Second line",
        "Third line"
    };

    SECTION("Write and read lines") {
        REQUIRE(fixture.fs.writeLines(testFile, lines));

        auto readLines = fixture.fs.readLines(testFile);
        REQUIRE(readLines.size() == lines.size());
        for (size_t i = 0; i < lines.size(); ++i) {
            REQUIRE(readLines[i] == lines[i]);
        }
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - Directory operations", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    SECTION("Create single directory") {
        std::string newDir = fixture.getTestPath("subdir");
        REQUIRE(fixture.fs.createDirectory(newDir));
        REQUIRE(fixture.fs.exists(newDir));
        REQUIRE(fixture.fs.isDirectory(newDir));
    }

    SECTION("Create nested directories") {
        std::string nestedDir = fixture.getTestPath("a/b/c");
        REQUIRE(fixture.fs.createDirectory(nestedDir, true));
        REQUIRE(fixture.fs.exists(nestedDir));
        REQUIRE(fixture.fs.isDirectory(nestedDir));
    }

    SECTION("Remove directory") {
        std::string dir = fixture.getTestPath("remove_me");
        fixture.fs.createDirectory(dir);
        REQUIRE(fixture.fs.exists(dir));

        REQUIRE(fixture.fs.removeDirectory(dir));
        REQUIRE_FALSE(fixture.fs.exists(dir));
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - List directory", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    // Create some test files
    fixture.fs.writeFile(fixture.getTestPath("file1.txt"), "content1");
    fixture.fs.writeFile(fixture.getTestPath("file2.txt"), "content2");
    fixture.fs.createDirectory(fixture.getTestPath("subdir"));

    SECTION("List directory contents") {
        auto entries = fixture.fs.listDirectory(fixture.testDir);
        REQUIRE(entries.size() >= 3);  // At least 3 items
    }

    SECTION("List with file info") {
        auto infos = fixture.fs.listDirectoryInfo(fixture.testDir);
        REQUIRE(infos.size() >= 3);

        // Check that we have both files and directories
        bool hasFile = false;
        bool hasDir = false;
        for (const auto& info : infos) {
            if (info.type == FileType::Regular) hasFile = true;
            if (info.type == FileType::Directory) hasDir = true;
        }
        REQUIRE(hasFile);
        REQUIRE(hasDir);
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - Recursive directory listing", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    // Create nested structure
    fixture.fs.createDirectory(fixture.getTestPath("dir1"), true);
    fixture.fs.createDirectory(fixture.getTestPath("dir1/dir2"), true);
    fixture.fs.writeFile(fixture.getTestPath("dir1/file1.txt"), "content");
    fixture.fs.writeFile(fixture.getTestPath("dir1/dir2/file2.txt"), "content");

    SECTION("Recursive listing") {
        auto entries = fixture.fs.listDirectory(fixture.testDir, true);

        // Should include all nested items
        bool foundFile1 = false;
        bool foundFile2 = false;
        bool foundDir2 = false;

        for (const auto& entry : entries) {
            std::string name = Path::basename(entry);
            if (name == "file1.txt") foundFile1 = true;
            if (name == "file2.txt") foundFile2 = true;
            if (name == "dir2") foundDir2 = true;
        }

        REQUIRE(foundFile1);
        REQUIRE(foundFile2);
        REQUIRE(foundDir2);
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - File operations", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    std::string sourceFile = fixture.getTestPath("source.txt");
    std::string destFile = fixture.getTestPath("dest.txt");
    std::string content = "Test content";

    SECTION("Copy file") {
        fixture.fs.writeFile(sourceFile, content);
        REQUIRE(fixture.fs.copyFile(sourceFile, destFile));
        REQUIRE(fixture.fs.exists(destFile));

        std::string copiedContent = fixture.fs.readFile(destFile);
        REQUIRE(copiedContent == content);
    }

    SECTION("Move file") {
        fixture.fs.writeFile(sourceFile, content);
        REQUIRE(fixture.fs.move(sourceFile, destFile));

        REQUIRE_FALSE(fixture.fs.exists(sourceFile));
        REQUIRE(fixture.fs.exists(destFile));

        std::string movedContent = fixture.fs.readFile(destFile);
        REQUIRE(movedContent == content);
    }

    SECTION("Remove file") {
        fixture.fs.writeFile(sourceFile, content);
        REQUIRE(fixture.fs.exists(sourceFile));

        REQUIRE(fixture.fs.removeFile(sourceFile));
        REQUIRE_FALSE(fixture.fs.exists(sourceFile));
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - Remove all recursively", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    // Create nested structure
    std::string dir = fixture.getTestPath("remove_all");
    fixture.fs.createDirectory(Path::join(dir, "sub1/sub2"), true);
    fixture.fs.writeFile(Path::join(dir, "file1.txt"), "content");
    fixture.fs.writeFile(Path::join(dir, "sub1/file2.txt"), "content");
    fixture.fs.writeFile(Path::join(dir, "sub1/sub2/file3.txt"), "content");

    SECTION("Remove entire tree") {
        REQUIRE(fixture.fs.exists(dir));
        REQUIRE(fixture.fs.removeAll(dir));
        REQUIRE_FALSE(fixture.fs.exists(dir));
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - Path utilities", "[filesystem][core]") {
    SECTION("Join paths") {
        std::string path = Path::join("dir", "file.txt");
        REQUIRE(path.find("file.txt") != std::string::npos);
    }

    SECTION("Join multiple paths") {
        std::string path = Path::join("a", "b", "c", "file.txt");
        REQUIRE(path.find("file.txt") != std::string::npos);
    }

    SECTION("Get dirname") {
        std::string dir = Path::dirname("/path/to/file.txt");
        REQUIRE(dir == "/path/to");

        dir = Path::dirname("file.txt");
        REQUIRE(dir == ".");
    }

    SECTION("Get basename") {
        std::string name = Path::basename("/path/to/file.txt");
        REQUIRE(name == "file.txt");
    }

    SECTION("Get extension") {
        std::string ext = Path::extension("file.txt");
        REQUIRE(ext == ".txt");

        ext = Path::extension("archive.tar.gz");
        REQUIRE(ext == ".gz");

        ext = Path::extension("noext");
        REQUIRE(ext == "");
    }

    SECTION("Get stem") {
        std::string stem = Path::stem("file.txt");
        REQUIRE(stem == "file");

        stem = Path::stem("archive.tar.gz");
        REQUIRE(stem == "archive.tar");
    }

    SECTION("Check absolute path") {
#ifdef _WIN32
        REQUIRE(Path::isAbsolute("C:\\path\\file.txt"));
        REQUIRE_FALSE(Path::isAbsolute("relative\\path"));
#else
        REQUIRE(Path::isAbsolute("/absolute/path"));
        REQUIRE_FALSE(Path::isAbsolute("relative/path"));
#endif
    }
}

TEST_CASE("FileSystem - File info", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    std::string testFile = fixture.getTestPath("info.txt");
    std::string content = "Test content for file info";
    fixture.fs.writeFile(testFile, content);

    SECTION("Get file info") {
        FileInfo info = fixture.fs.getFileInfo(testFile);

        REQUIRE(info.name == "info.txt");
        REQUIRE(info.type == FileType::Regular);
        REQUIRE(info.size == content.length());
        REQUIRE(info.path == testFile);
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - Current directory", "[filesystem][core]") {
    FileSystemTestFixture fixture;

    SECTION("Get current directory") {
        std::string cwd = fixture.fs.getCurrentDirectory();
        REQUIRE_FALSE(cwd.empty());
    }

    SECTION("Set and restore current directory") {
        std::string originalCwd = fixture.fs.getCurrentDirectory();

        fixture.SetUp();

        // Build absolute path to test directory
        std::string absoluteTestDir = Path::join(originalCwd, fixture.testDir);

        // Change to test directory
        REQUIRE(fixture.fs.setCurrentDirectory(absoluteTestDir));

        // Verify we're in the test directory
        std::string newCwd = fixture.fs.getCurrentDirectory();
        REQUIRE(newCwd.find("test_filesystem_temp") != std::string::npos);

        // Restore original directory
        REQUIRE(fixture.fs.setCurrentDirectory(originalCwd));

        fixture.TearDown();
    }
}

TEST_CASE("FileSystem - Find files with pattern", "[filesystem][core]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    // Create test files with different extensions
    fixture.fs.writeFile(fixture.getTestPath("file1.txt"), "content");
    fixture.fs.writeFile(fixture.getTestPath("file2.txt"), "content");
    fixture.fs.writeFile(fixture.getTestPath("file3.cpp"), "content");
    fixture.fs.writeFile(fixture.getTestPath("test.hpp"), "content");

    SECTION("Find all .txt files") {
        auto results = fixture.fs.find(fixture.testDir, "*.txt");
        REQUIRE(results.size() == 2);
    }

    SECTION("Find all files starting with 'file'") {
        auto results = fixture.fs.find(fixture.testDir, "file*");
        REQUIRE(results.size() == 3);
    }

    SECTION("Find specific file") {
        auto results = fixture.fs.find(fixture.testDir, "test.hpp");
        REQUIRE(results.size() == 1);
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - Thread safety", "[filesystem][core][threading]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    const int numThreads = 10;
    const int filesPerThread = 10;

    SECTION("Concurrent file writes") {
        std::vector<std::thread> threads;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&fixture, t, filesPerThread]() {
                for (int i = 0; i < filesPerThread; ++i) {
                    std::string filename = "thread_" + std::to_string(t) + "_file_" + std::to_string(i) + ".txt";
                    std::string path = fixture.getTestPath(filename);
                    std::string content = "Thread " + std::to_string(t) + " File " + std::to_string(i);

                    fixture.fs.writeFile(path, content);
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // Verify all files were created
        auto entries = fixture.fs.listDirectory(fixture.testDir);
        REQUIRE(entries.size() >= numThreads * filesPerThread);
    }

    SECTION("Concurrent directory operations") {
        std::vector<std::thread> threads;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&fixture, t]() {
                std::string dirname = "thread_dir_" + std::to_string(t);
                std::string path = fixture.getTestPath(dirname);
                fixture.fs.createDirectory(path);
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        // Verify all directories were created
        auto entries = fixture.fs.listDirectoryInfo(fixture.testDir);
        int dirCount = 0;
        for (const auto& info : entries) {
            if (info.type == FileType::Directory) {
                dirCount++;
            }
        }
        REQUIRE(dirCount >= numThreads);
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - Error handling", "[filesystem][core][error]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    SECTION("Read non-existent file throws") {
        REQUIRE_THROWS(fixture.fs.readFile(fixture.getTestPath("nonexistent.txt")));
    }

    SECTION("Read binary non-existent file throws") {
        REQUIRE_THROWS(fixture.fs.readBinary(fixture.getTestPath("nonexistent.dat")));
    }

    SECTION("Read lines non-existent file throws") {
        REQUIRE_THROWS(fixture.fs.readLines(fixture.getTestPath("nonexistent.txt")));
    }

    SECTION("Copy non-existent file fails") {
        REQUIRE_FALSE(fixture.fs.copyFile(
            fixture.getTestPath("nonexistent.txt"),
            fixture.getTestPath("dest.txt")
        ));
    }

    fixture.TearDown();
}

TEST_CASE("FileSystem - Benchmark operations", "[filesystem][.benchmark]") {
    FileSystemTestFixture fixture;
    fixture.SetUp();

    BENCHMARK("Write 1000 small files") {
        for (int i = 0; i < 1000; ++i) {
            std::string filename = fixture.getTestPath("bench_" + std::to_string(i) + ".txt");
            fixture.fs.writeFile(filename, "Benchmark content");
        }
        return 0;
    };

    BENCHMARK("Read 100 files") {
        // First create files
        for (int i = 0; i < 100; ++i) {
            std::string filename = fixture.getTestPath("read_bench_" + std::to_string(i) + ".txt");
            fixture.fs.writeFile(filename, "Read benchmark content");
        }

        // Now benchmark reading
        for (int i = 0; i < 100; ++i) {
            std::string filename = fixture.getTestPath("read_bench_" + std::to_string(i) + ".txt");
            auto content = fixture.fs.readFile(filename);
        }
        return 0;
    };

    fixture.TearDown();
}
