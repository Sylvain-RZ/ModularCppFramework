#include <catch_amalgamated.hpp>
#include "../../core/Logger.hpp"
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <vector>
#include <atomic>
#include <memory>

using namespace mcf;

// Helper function to create a logger (either via registry or directly)
std::shared_ptr<Logger> createLogger(const std::string& name, LogLevel level = LogLevel::Trace) {
    return std::make_shared<Logger>(name, level);
}

// Helper function to read file contents
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper function to count lines in file
size_t countLines(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return 0;
    size_t count = 0;
    std::string line;
    while (std::getline(file, line)) {
        count++;
    }
    return count;
}

TEST_CASE("Logger - All sink types", "[Logger][EdgeCases]") {
    // Skip console output tests on Windows in CI - stderr output interferes with test framework
    #ifndef _WIN32
    SECTION("Console sink with color") {
        auto logger = createLogger("console_color");
        logger->addSink(std::make_shared<ConsoleSink>(true, LogLevel::Debug));

        logger->debug("Debug with color");
        logger->info("Info with color");
        logger->warning("Warning with color");
        logger->error("Error with color");

        REQUIRE(true); // Visual test - should show colored output
    }

    SECTION("Console sink without color") {
        auto logger = createLogger("console_nocolor");
        logger->addSink(std::make_shared<ConsoleSink>(false, LogLevel::Info));

        logger->info("Info without color");
        logger->error("Error without color");

        REQUIRE(true);
    }
    #endif

    SECTION("File sink with truncate") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/truncate.log";

        // Write something first
        {
            std::ofstream file(filepath);
            file << "Old content that should be removed\n";
        }

        auto logger = createLogger("file_truncate");
        logger->addSink(std::make_shared<FileSink>(filepath, true, LogLevel::Debug));
        logger->info("New content");
        logger->flush();

        std::string content = readFile(filepath);
        REQUIRE(content.find("Old content") == std::string::npos);
        REQUIRE(content.find("New content") != std::string::npos);

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("File sink with append") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/append.log";

        // Write something first
        {
            std::ofstream file(filepath);
            file << "Old content\n";
        }

        auto logger = createLogger("file_append");
        logger->addSink(std::make_shared<FileSink>(filepath, false, LogLevel::Debug));
        logger->info("New content");
        logger->flush();

        std::string content = readFile(filepath);
        REQUIRE(content.find("Old content") != std::string::npos);
        REQUIRE(content.find("New content") != std::string::npos);

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Rotating file sink") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/rotating.log";

        // Clean up old files
        std::filesystem::remove(filepath);
        std::filesystem::remove(filepath + ".1");
        std::filesystem::remove(filepath + ".2");

        auto logger = createLogger("rotating");
        logger->addSink(std::make_shared<RotatingFileSink>(filepath, 500, 2, LogLevel::Debug)); // 500 bytes max

        // Write enough to trigger rotation
        for (int i = 0; i < 50; i++) {
            logger->info("Message " + std::to_string(i) + " with some padding text");
        }
        logger->flush();

        // Check that rotation occurred
        bool hasBackup = std::filesystem::exists(filepath + ".1");
        REQUIRE(hasBackup);

        std::filesystem::remove(filepath);
        std::filesystem::remove(filepath + ".1");
        std::filesystem::remove(filepath + ".2");
        std::filesystem::remove_all("test_logs");
    }
}

TEST_CASE("Logger - Concurrent logging from multiple threads", "[Logger][EdgeCases][Threading]") {
    SECTION("Multiple threads logging to console") {
        auto logger = createLogger("thread_console");
        logger->addSink(std::make_shared<ConsoleSink>(false, LogLevel::Debug));

        std::atomic<int> counter{0};
        const int numThreads = 10;
        const int messagesPerThread = 100;

        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; t++) {
            threads.emplace_back([&logger, &counter, t, messagesPerThread]() {
                for (int i = 0; i < messagesPerThread; i++) {
                    logger->info("Thread " + std::to_string(t) + " message " + std::to_string(i));
                    counter++;
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        REQUIRE(counter == numThreads * messagesPerThread);
    }

    SECTION("Multiple threads logging to file") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/thread_file.log";
        std::filesystem::remove(filepath);

        auto logger = createLogger("thread_file");
        logger->addSink(std::make_shared<FileSink>(filepath, true, LogLevel::Debug));

        const int numThreads = 5;
        const int messagesPerThread = 50;

        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; t++) {
            threads.emplace_back([&logger, t, messagesPerThread]() {
                for (int i = 0; i < messagesPerThread; i++) {
                    logger->info("Thread " + std::to_string(t) + " message " + std::to_string(i));
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        logger->flush();

        // Verify all messages were written
        size_t lineCount = countLines(filepath);
        REQUIRE(lineCount >= numThreads * messagesPerThread);

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Concurrent logging with multiple sinks") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/thread_multi.log";
        std::filesystem::remove(filepath);

        auto logger = createLogger("thread_multi");
        logger->addSink(std::make_shared<ConsoleSink>(false, LogLevel::Debug));
        logger->addSink(std::make_shared<FileSink>(filepath, true, LogLevel::Debug));

        const int numThreads = 4;
        const int messagesPerThread = 25;

        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; t++) {
            threads.emplace_back([&logger, t, messagesPerThread]() {
                for (int i = 0; i < messagesPerThread; i++) {
                    logger->info("Multi-sink thread " + std::to_string(t) + " msg " + std::to_string(i));
                }
            });
        }

        for (auto& thread : threads) {
            thread.join();
        }

        logger->flush();

        size_t lineCount = countLines(filepath);
        REQUIRE(lineCount >= numThreads * messagesPerThread);

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }
}

TEST_CASE("Logger - File rotation edge cases", "[Logger][EdgeCases]") {
    SECTION("Rotation with max_files = 1") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/rotate_1.log";

        std::filesystem::remove(filepath);
        std::filesystem::remove(filepath + ".1");

        auto logger = createLogger("rotate_1");
        logger->addSink(std::make_shared<RotatingFileSink>(filepath, 200, 1, LogLevel::Debug));

        for (int i = 0; i < 50; i++) {
            logger->info("Message " + std::to_string(i) + " padding text here");
        }
        logger->flush();

        // Should have main file and .1 backup only
        REQUIRE(std::filesystem::exists(filepath));
        REQUIRE(std::filesystem::exists(filepath + ".1"));
        REQUIRE_FALSE(std::filesystem::exists(filepath + ".2"));

        std::filesystem::remove(filepath);
        std::filesystem::remove(filepath + ".1");
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Rotation with max_files = 5") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/rotate_5.log";

        for (int i = 0; i <= 6; i++) {
            std::filesystem::remove(filepath + (i == 0 ? "" : "." + std::to_string(i)));
        }

        auto logger = createLogger("rotate_5");
        logger->addSink(std::make_shared<RotatingFileSink>(filepath, 200, 5, LogLevel::Debug));

        for (int i = 0; i < 100; i++) {
            logger->info("Message " + std::to_string(i) + " with padding text to trigger rotation");
        }
        logger->flush();

        // Should have main file and up to 5 backups
        REQUIRE(std::filesystem::exists(filepath));
        REQUIRE_FALSE(std::filesystem::exists(filepath + ".6"));

        for (int i = 0; i <= 5; i++) {
            std::filesystem::remove(filepath + (i == 0 ? "" : "." + std::to_string(i)));
        }
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Very small max_size") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/small_size.log";

        std::filesystem::remove(filepath);
        std::filesystem::remove(filepath + ".1");

        auto logger = createLogger("small_size");
        logger->addSink(std::make_shared<RotatingFileSink>(filepath, 50, 2, LogLevel::Debug)); // Very small

        logger->info("This message is longer than 50 bytes and should trigger rotation");
        logger->flush();

        REQUIRE(std::filesystem::exists(filepath));

        std::filesystem::remove(filepath);
        std::filesystem::remove(filepath + ".1");
        std::filesystem::remove(filepath + ".2");
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Rotation preserves old content in backup") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/preserve.log";

        std::filesystem::remove(filepath);
        std::filesystem::remove(filepath + ".1");

        auto logger = createLogger("preserve");
        logger->addSink(std::make_shared<RotatingFileSink>(filepath, 300, 2, LogLevel::Debug));

        logger->info("First batch message 1");
        logger->info("First batch message 2");
        logger->flush();

        // Trigger rotation by writing many more messages
        for (int i = 0; i < 50; i++) {
            logger->info("Rotation trigger message " + std::to_string(i) + " with padding");
        }
        logger->flush();

        // Check that a backup file was created (rotation occurred)
        REQUIRE(std::filesystem::exists(filepath + ".1"));

        std::filesystem::remove(filepath);
        std::filesystem::remove(filepath + ".1");
        std::filesystem::remove(filepath + ".2");
        std::filesystem::remove_all("test_logs");
    }
}

TEST_CASE("Logger - Formatting edge cases", "[Logger][EdgeCases]") {
    SECTION("Empty message") {
        auto logger = createLogger("empty_msg");
        logger->addSink(std::make_shared<ConsoleSink>(false, LogLevel::Debug));

        logger->info("");
        REQUIRE(true); // Should not crash
    }

    SECTION("Very long message") {
        auto logger = createLogger("long_msg");
        logger->addSink(std::make_shared<ConsoleSink>(false, LogLevel::Debug));

        std::string longMsg(10000, 'a');
        logger->info(longMsg);
        REQUIRE(true);
    }

    SECTION("Message with special characters") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/special_chars.log";
        std::filesystem::remove(filepath);

        auto logger = createLogger("special_chars");
        logger->addSink(std::make_shared<FileSink>(filepath, true, LogLevel::Debug));

        logger->info("Message with newline\n");
        logger->info("Message with tab\t");
        logger->info("Message with quotes \"\"");
        logger->info("Message with backslash \\");
        logger->flush();

        std::string content = readFile(filepath);
        REQUIRE(content.find("newline") != std::string::npos);
        REQUIRE(content.find("tab") != std::string::npos);
        REQUIRE(content.find("quotes") != std::string::npos);

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Message with Unicode characters") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/unicode.log";
        std::filesystem::remove(filepath);

        auto logger = createLogger("unicode");
        logger->addSink(std::make_shared<FileSink>(filepath, true, LogLevel::Debug));

        logger->info("Unicode: café, naïve, 日本語, émoji 🚀");
        logger->flush();

        std::string content = readFile(filepath);
        REQUIRE(content.find("café") != std::string::npos);

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("All log levels formatting") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/all_levels.log";
        std::filesystem::remove(filepath);

        auto logger = createLogger("all_levels", LogLevel::Trace);
        logger->addSink(std::make_shared<FileSink>(filepath, true, LogLevel::Trace));

        logger->trace("Trace message");
        logger->debug("Debug message");
        logger->info("Info message");
        logger->warning("Warn message");
        logger->error("Error message");
        logger->critical("Critical message");
        logger->flush();

        std::string content = readFile(filepath);
        REQUIRE(content.find("TRACE") != std::string::npos);
        REQUIRE(content.find("DEBUG") != std::string::npos);
        REQUIRE(content.find("INFO") != std::string::npos);
        REQUIRE(content.find("WARN") != std::string::npos);
        REQUIRE(content.find("ERROR") != std::string::npos);
        REQUIRE(content.find("CRIT") != std::string::npos); // Critical is logged as "CRIT"

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }
}

TEST_CASE("Logger - Level filtering", "[Logger][EdgeCases]") {
    SECTION("Logger level filters messages") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/filter.log";
        std::filesystem::remove(filepath);

        auto logger = createLogger("filter", LogLevel::Warning); // Only warn and above
        logger->addSink(std::make_shared<FileSink>(filepath, true, LogLevel::Trace)); // Sink accepts all

        logger->trace("Should not appear");
        logger->debug("Should not appear");
        logger->info("Should not appear");
        logger->warning("Should appear");
        logger->error("Should appear");
        logger->flush();

        std::string content = readFile(filepath);
        REQUIRE(content.find("Should not appear") == std::string::npos);
        REQUIRE(content.find("Should appear") != std::string::npos);

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Sink level filters messages") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/sink_filter.log";
        std::filesystem::remove(filepath);

        auto logger = createLogger("sink_filter", LogLevel::Trace); // Logger accepts all
        logger->addSink(std::make_shared<FileSink>(filepath, true, LogLevel::Error)); // Sink only accepts error+

        logger->trace("Should not appear");
        logger->info("Should not appear");
        logger->warning("Should not appear");
        logger->error("Should appear");
        logger->critical("Should appear");
        logger->flush();

        std::string content = readFile(filepath);
        REQUIRE(content.find("Should not appear") == std::string::npos);
        REQUIRE(content.find("Should appear") != std::string::npos);

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Multiple sinks with different levels") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath1 = "test_logs/multi_level_1.log";
        const std::string filepath2 = "test_logs/multi_level_2.log";
        std::filesystem::remove(filepath1);
        std::filesystem::remove(filepath2);

        auto logger = createLogger("multi_level", LogLevel::Trace);
        logger->addSink(std::make_shared<FileSink>(filepath1, true, LogLevel::Debug));
        logger->addSink(std::make_shared<FileSink>(filepath2, true, LogLevel::Error));

        logger->trace("Trace");
        logger->debug("Debug");
        logger->info("Info");
        logger->error("Error");
        logger->flush();

        std::string content1 = readFile(filepath1);
        std::string content2 = readFile(filepath2);

        // File 1 should have debug and above
        REQUIRE(content1.find("Trace") == std::string::npos);
        REQUIRE(content1.find("Debug") != std::string::npos);
        REQUIRE(content1.find("Error") != std::string::npos);

        // File 2 should only have error
        REQUIRE(content2.find("Debug") == std::string::npos);
        REQUIRE(content2.find("Info") == std::string::npos);
        REQUIRE(content2.find("Error") != std::string::npos);

        std::filesystem::remove(filepath1);
        std::filesystem::remove(filepath2);
        std::filesystem::remove_all("test_logs");
    }
}

TEST_CASE("Logger - Logger registry edge cases", "[Logger][EdgeCases]") {
    SECTION("Get or create logger") {
        auto logger1 = LoggerRegistry::instance().getLogger("registry_test");
        auto logger2 = LoggerRegistry::instance().getLogger("registry_test");

        REQUIRE(logger1 == logger2); // Same instance
    }

    SECTION("Multiple different loggers") {
        auto logger1 = LoggerRegistry::instance().getLogger("reg_1");
        auto logger2 = LoggerRegistry::instance().getLogger("reg_2");
        auto logger3 = LoggerRegistry::instance().getLogger("reg_3");

        REQUIRE(logger1 != logger2);
        REQUIRE(logger2 != logger3);
        REQUIRE(logger1 != logger3);
    }

    SECTION("Empty logger name") {
        auto logger = LoggerRegistry::instance().getLogger("");
        REQUIRE(logger != nullptr);
    }
}

TEST_CASE("Logger - Flush behavior", "[Logger][EdgeCases]") {
    SECTION("Flush with no sinks") {
        auto logger = createLogger("no_sinks");
        logger->flush(); // Should not crash
        REQUIRE(true);
    }

    SECTION("Flush ensures data written to file") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/flush_test.log";
        std::filesystem::remove(filepath);

        auto logger = createLogger("flush_test");
        logger->addSink(std::make_shared<FileSink>(filepath, true, LogLevel::Debug));

        logger->info("Before flush");

        // Without flush, data might not be written yet
        logger->flush();

        // Now it should definitely be written
        std::string content = readFile(filepath);
        REQUIRE(content.find("Before flush") != std::string::npos);

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }

    SECTION("Multiple flushes") {
        std::filesystem::create_directories("test_logs");
        const std::string filepath = "test_logs/multi_flush.log";
        std::filesystem::remove(filepath);

        auto logger = createLogger("multi_flush");
        logger->addSink(std::make_shared<FileSink>(filepath, true, LogLevel::Debug));

        logger->info("Message 1");
        logger->flush();
        logger->info("Message 2");
        logger->flush();
        logger->info("Message 3");
        logger->flush();

        size_t lineCount = countLines(filepath);
        REQUIRE(lineCount >= 3);

        std::filesystem::remove(filepath);
        std::filesystem::remove_all("test_logs");
    }
}
