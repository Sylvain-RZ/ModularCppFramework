/**
 * @file test_file_watcher_catch2.cpp
 * @brief Unit tests for FileWatcher using Catch2
 */

#include "../../core/FileWatcher.hpp"
#include "../../external/catch_amalgamated.hpp"

#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <filesystem>

using namespace mcf;

// Helper function to get temp directory path (cross-platform)
static std::string getTempFilePath(const std::string& filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

// =============================================================================
// Basic FileWatcher Operations Tests
// =============================================================================

TEST_CASE("FileWatcher - Add and remove watch", "[filewatcher][core]") {
    FileWatcher watcher;

    SECTION("Add watch for file") {
        bool result = watcher.addWatch(getTempFilePath("test_file.txt"), [](const std::string&, FileChangeType) {});
        REQUIRE(result);
        REQUIRE(watcher.isWatching(getTempFilePath("test_file.txt")));
    }

    SECTION("Remove watch for file") {
        watcher.addWatch(getTempFilePath("test_file.txt"), [](const std::string&, FileChangeType) {});
        watcher.removeWatch(getTempFilePath("test_file.txt"));
        REQUIRE_FALSE(watcher.isWatching(getTempFilePath("test_file.txt")));
    }

    SECTION("Remove non-existent watch is safe") {
        watcher.removeWatch(getTempFilePath("nonexistent.txt"));
        // Should not throw
    }

    SECTION("Add multiple watches") {
        watcher.addWatch(getTempFilePath("file1.txt"), [](const std::string&, FileChangeType) {});
        watcher.addWatch(getTempFilePath("file2.txt"), [](const std::string&, FileChangeType) {});
        watcher.addWatch(getTempFilePath("file3.txt"), [](const std::string&, FileChangeType) {});

        REQUIRE(watcher.getWatchCount() == 3);
        REQUIRE(watcher.isWatching(getTempFilePath("file1.txt")));
        REQUIRE(watcher.isWatching(getTempFilePath("file2.txt")));
        REQUIRE(watcher.isWatching(getTempFilePath("file3.txt")));
    }
}

// =============================================================================
// FileWatcher Lifecycle Tests
// =============================================================================

TEST_CASE("FileWatcher - Start and stop", "[filewatcher][core]") {
    FileWatcher watcher;

    SECTION("Initial state is not running") {
        REQUIRE_FALSE(watcher.isRunning());
    }

    SECTION("Start watcher") {
        watcher.start();
        REQUIRE(watcher.isRunning());
        watcher.stop();
    }

    SECTION("Stop watcher") {
        watcher.start();
        watcher.stop();
        REQUIRE_FALSE(watcher.isRunning());
    }

    SECTION("Multiple start/stop cycles") {
        watcher.start();
        REQUIRE(watcher.isRunning());
        watcher.stop();
        REQUIRE_FALSE(watcher.isRunning());

        watcher.start();
        REQUIRE(watcher.isRunning());
        watcher.stop();
        REQUIRE_FALSE(watcher.isRunning());
    }

    SECTION("Start when already running is safe") {
        watcher.start();
        watcher.start(); // Should be safe
        REQUIRE(watcher.isRunning());
        watcher.stop();
    }

    SECTION("Stop when not running is safe") {
        watcher.stop(); // Should be safe
        REQUIRE_FALSE(watcher.isRunning());
    }
}

// =============================================================================
// File Change Detection Tests
// =============================================================================

TEST_CASE("FileWatcher - File modification detection", "[filewatcher][hot-reload]") {
    FileWatcher watcher(std::chrono::milliseconds(100));

    std::string testFile = getTempFilePath("test_modify_catch2.txt");

    // Create initial file
    {
        std::ofstream file(testFile);
        file << "initial content";
    }

    std::atomic<bool> modified{false};
    std::atomic<int> changeCount{0};

    watcher.addWatch(testFile, [&](const std::string& path, FileChangeType type) {
        if (type == FileChangeType::Modified) {
            modified = true;
            changeCount++;
        }
    });

    watcher.start();

    // Wait for initial scan (at least 2x poll interval for Windows)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    SECTION("Detect file modification") {
        // Modify file
        {
            std::ofstream file(testFile, std::ios::app);
            file << "\nmodified content";
            file.flush(); // Ensure file is written to disk
        }

        // Small delay to ensure file system sync (especially on Windows)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Wait for change detection (at least 2x poll interval + buffer)
        std::this_thread::sleep_for(std::chrono::milliseconds(800));

        REQUIRE(modified);
        REQUIRE(changeCount > 0);
    }

    SECTION("Multiple modifications detected") {
        // Modify file multiple times
        for (int i = 0; i < 3; ++i) {
            {
                std::ofstream file(testFile, std::ios::app);
                file << "\nmodification " << i;
                file.flush(); // Ensure file is written
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(350));
        }

        // Wait for final change detection (at least 2x poll interval + buffer)
        std::this_thread::sleep_for(std::chrono::milliseconds(800));

        REQUIRE(modified);
        REQUIRE(changeCount >= 1);
    }

    watcher.stop();

    // Cleanup
    std::remove(testFile.c_str());
}

TEST_CASE("FileWatcher - File creation detection", "[filewatcher][hot-reload]") {
    FileWatcher watcher(std::chrono::milliseconds(100));

    std::string testFile = getTempFilePath("test_create_catch2.txt");

    // Remove file if it exists
    std::remove(testFile.c_str());

    std::atomic<bool> created{false};

    watcher.addWatch(testFile, [&](const std::string& path, FileChangeType type) {
        if (type == FileChangeType::Created) {
            created = true;
        }
    });

    watcher.start();

    // Wait for initial scan (at least 2x poll interval for Windows)
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Create file
    {
        std::ofstream file(testFile);
        file << "new file content";
        file.flush(); // Ensure file is written to disk
    }

    // Small delay to ensure file system sync (especially on Windows)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Wait for change detection (at least 2x poll interval + buffer)
    std::this_thread::sleep_for(std::chrono::milliseconds(800));

    watcher.stop();

    REQUIRE(created);

    // Cleanup
    std::remove(testFile.c_str());
}

// =============================================================================
// Watch Management Tests
// =============================================================================

TEST_CASE("FileWatcher - Watch count", "[filewatcher][core]") {
    FileWatcher watcher;

    SECTION("Initial watch count is zero") {
        REQUIRE(watcher.getWatchCount() == 0);
    }

    SECTION("Watch count increases with additions") {
        watcher.addWatch(getTempFilePath("file1.txt"), [](const std::string&, FileChangeType) {});
        REQUIRE(watcher.getWatchCount() == 1);

        watcher.addWatch(getTempFilePath("file2.txt"), [](const std::string&, FileChangeType) {});
        REQUIRE(watcher.getWatchCount() == 2);

        watcher.addWatch(getTempFilePath("file3.txt"), [](const std::string&, FileChangeType) {});
        REQUIRE(watcher.getWatchCount() == 3);
    }

    SECTION("Watch count decreases with removals") {
        watcher.addWatch(getTempFilePath("file1.txt"), [](const std::string&, FileChangeType) {});
        watcher.addWatch(getTempFilePath("file2.txt"), [](const std::string&, FileChangeType) {});
        REQUIRE(watcher.getWatchCount() == 2);

        watcher.removeWatch(getTempFilePath("file1.txt"));
        REQUIRE(watcher.getWatchCount() == 1);
    }

    SECTION("Clear all watches") {
        watcher.addWatch(getTempFilePath("file1.txt"), [](const std::string&, FileChangeType) {});
        watcher.addWatch(getTempFilePath("file2.txt"), [](const std::string&, FileChangeType) {});
        watcher.addWatch(getTempFilePath("file3.txt"), [](const std::string&, FileChangeType) {});

        watcher.clearWatches();
        REQUIRE(watcher.getWatchCount() == 0);
    }
}

// =============================================================================
// Configuration Tests
// =============================================================================

TEST_CASE("FileWatcher - Poll interval configuration", "[filewatcher][core]") {
    SECTION("Set poll interval via constructor") {
        FileWatcher watcher(std::chrono::milliseconds(500));
        watcher.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        watcher.stop();
        // Should not crash
    }

    SECTION("Change poll interval dynamically") {
        FileWatcher watcher(std::chrono::milliseconds(500));
        watcher.setPollInterval(std::chrono::milliseconds(1000));

        watcher.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        watcher.stop();
        // Should not crash
    }

    SECTION("Very short poll interval") {
        FileWatcher watcher(std::chrono::milliseconds(10));
        watcher.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        watcher.stop();
        // Should handle fast polling
    }
}

// =============================================================================
// Thread Safety Tests
// =============================================================================

TEST_CASE("FileWatcher - Thread safety", "[filewatcher][core]") {
    FileWatcher watcher;

    SECTION("Add watches from multiple threads") {
        std::vector<std::thread> threads;
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&watcher, i]() {
                watcher.addWatch(getTempFilePath("file" + std::to_string(i) + ".txt"),
                                [](const std::string&, FileChangeType) {});
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        REQUIRE(watcher.getWatchCount() == 10);
    }

    SECTION("Start/stop from multiple threads") {
        watcher.addWatch(getTempFilePath("test.txt"), [](const std::string&, FileChangeType) {});

        std::vector<std::thread> threads;
        for (int i = 0; i < 5; ++i) {
            threads.emplace_back([&watcher]() {
                watcher.start();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                watcher.stop();
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        REQUIRE_FALSE(watcher.isRunning());
    }
}

// =============================================================================
// Performance Benchmarks
// =============================================================================

TEST_CASE("FileWatcher - Performance", "[.benchmark][filewatcher]") {
    FileWatcher watcher;

    BENCHMARK("Add 100 watches") {
        FileWatcher bench_watcher;
        for (int i = 0; i < 100; ++i) {
            bench_watcher.addWatch(getTempFilePath("file" + std::to_string(i) + ".txt"),
                                   [](const std::string&, FileChangeType) {});
        }
    };

    // Setup watches for benchmark
    for (int i = 0; i < 100; ++i) {
        watcher.addWatch(getTempFilePath("file" + std::to_string(i) + ".txt"),
                        [](const std::string&, FileChangeType) {});
    }

    BENCHMARK("Remove 100 watches") {
        for (int i = 0; i < 100; ++i) {
            watcher.removeWatch(getTempFilePath("file" + std::to_string(i) + ".txt"));
        }
    };

    BENCHMARK("Start and stop watcher") {
        FileWatcher bench_watcher;
        bench_watcher.addWatch(getTempFilePath("benchmark.txt"), [](const std::string&, FileChangeType) {});
        bench_watcher.start();
        bench_watcher.stop();
    };
}
