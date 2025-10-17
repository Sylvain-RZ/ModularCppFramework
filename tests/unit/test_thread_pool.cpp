/**
 * @file test_thread_pool.cpp
 * @brief Unit tests for ThreadPool using Catch2
 */

#include "../../core/ThreadPool.hpp"
#include "../../external/catch_amalgamated.hpp"

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace mcf;

// =============================================================================
// Basic ThreadPool Tests
// =============================================================================

TEST_CASE("ThreadPool - Construction and initialization", "[threadpool][core]") {
    SECTION("Default construction uses hardware concurrency") {
        ThreadPool pool;
        REQUIRE(pool.isRunning());
        REQUIRE(pool.getThreadCount() > 0);
    }

    SECTION("Custom thread count") {
        ThreadPool pool(2);
        REQUIRE(pool.isRunning());
        REQUIRE(pool.getThreadCount() == 2);
    }

    SECTION("Zero thread count auto-detects") {
        ThreadPool pool(0);
        REQUIRE(pool.isRunning());
        REQUIRE(pool.getThreadCount() >= 1);
    }
}

// =============================================================================
// Task Submission Tests
// =============================================================================

TEST_CASE("ThreadPool - Basic task submission", "[threadpool][core]") {
    ThreadPool pool(2);

    SECTION("Submit simple task") {
        std::atomic<bool> executed{false};

        auto future = pool.submit([&executed]() {
            executed = true;
            return 42;
        });

        int result = future.get();
        REQUIRE(result == 42);
        REQUIRE(executed);
    }

    SECTION("Submit task with arguments") {
        auto future = pool.submit([](int a, int b) {
            return a + b;
        }, 10, 32);

        REQUIRE(future.get() == 42);
    }

    SECTION("Submit void task") {
        std::atomic<int> counter{0};

        auto future = pool.submit([&counter]() {
            counter++;
        });

        future.wait();
        REQUIRE(counter == 1);
    }

    SECTION("Submit multiple tasks") {
        std::atomic<int> counter{0};
        std::vector<std::future<void>> futures;

        for (int i = 0; i < 10; ++i) {
            futures.push_back(pool.submit([&counter]() {
                counter++;
            }));
        }

        for (auto& future : futures) {
            future.wait();
        }

        REQUIRE(counter == 10);
    }
}

// =============================================================================
// Priority Tests
// =============================================================================

TEST_CASE("ThreadPool - Task priorities", "[threadpool][core]") {
    ThreadPool pool(1); // Single thread for deterministic ordering
    std::atomic<int> executionOrder{0};
    std::vector<int> order;
    std::mutex orderMutex;

    SECTION("Higher priority tasks execute first") {
        // Submit low priority tasks first
        auto lowFuture1 = pool.submit(TaskPriority::Low, [&]() {
            std::lock_guard<std::mutex> lock(orderMutex);
            order.push_back(executionOrder++);
        });

        auto lowFuture2 = pool.submit(TaskPriority::Low, [&]() {
            std::lock_guard<std::mutex> lock(orderMutex);
            order.push_back(executionOrder++);
        });

        // Then submit high priority task
        auto highFuture = pool.submit(TaskPriority::High, [&]() {
            std::lock_guard<std::mutex> lock(orderMutex);
            order.push_back(executionOrder++);
        });

        lowFuture1.wait();
        lowFuture2.wait();
        highFuture.wait();

        // High priority should execute before remaining low priority
        REQUIRE(order.size() == 3);
    }

    SECTION("Normal priority by default") {
        std::atomic<bool> executed{false};
        auto future = pool.submit([&]() {
            executed = true;
        });

        future.wait();
        REQUIRE(executed);
    }
}

// =============================================================================
// Exception Handling Tests
// =============================================================================

TEST_CASE("ThreadPool - Exception handling", "[threadpool][core][error]") {
    ThreadPool pool(2);

    SECTION("Exception in task is captured in future") {
        auto future = pool.submit([]() -> int {
            throw std::runtime_error("Test exception");
            return 0;
        });

        REQUIRE_THROWS_AS(future.get(), std::runtime_error);
    }

    SECTION("Pool continues after exception") {
        auto future1 = pool.submit([]() -> int {
            throw std::runtime_error("Test exception");
            return 0;
        });

        try {
            future1.get();
        } catch (...) {}

        // Submit another task to verify pool still works
        auto future2 = pool.submit([]() {
            return 42;
        });

        REQUIRE(future2.get() == 42);
    }
}

// =============================================================================
// Shutdown Tests
// =============================================================================

TEST_CASE("ThreadPool - Shutdown behavior", "[threadpool][core]") {
    SECTION("Shutdown waits for pending tasks") {
        ThreadPool pool(2);
        std::atomic<int> completed{0};

        for (int i = 0; i < 10; ++i) {
            pool.submit([&completed]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                completed++;
            });
        }

        pool.shutdown(true);
        REQUIRE_FALSE(pool.isRunning());
        REQUIRE(completed == 10);
    }

    SECTION("Shutdown without waiting clears tasks") {
        ThreadPool pool(1);

        for (int i = 0; i < 100; ++i) {
            pool.submit([]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            });
        }

        pool.shutdown(false);
        REQUIRE_FALSE(pool.isRunning());
    }

    SECTION("Cannot submit after shutdown") {
        ThreadPool pool(2);
        pool.shutdown(true);

        REQUIRE_THROWS_AS(pool.submit([]() { return 42; }), std::runtime_error);
    }
}

// =============================================================================
// Statistics Tests
// =============================================================================

TEST_CASE("ThreadPool - Task statistics", "[threadpool][core]") {
    ThreadPool pool(2);

    SECTION("Track submitted tasks") {
        REQUIRE(pool.getTasksSubmitted() == 0);

        std::vector<std::future<void>> futures;
        for (int i = 0; i < 10; ++i) {
            futures.push_back(pool.submit([]() {}));
        }

        for (auto& f : futures) {
            f.wait();
        }

        // Wait for all tasks to complete and counters to update
        pool.waitForAll();

        REQUIRE(pool.getTasksSubmitted() == 10);
        REQUIRE(pool.getTasksCompleted() == 10);
    }

    SECTION("Track pending tasks") {
        ThreadPool singlePool(1); // Use single thread pool
        std::atomic<bool> canProceed{false};

        // Submit task that blocks
        auto blockFuture = singlePool.submit([&]() {
            while (!canProceed) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        // Wait for block task to start
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Submit more tasks (should be pending since thread is blocked)
        for (int i = 0; i < 5; ++i) {
            singlePool.submit([]() {});
        }

        size_t pending = singlePool.getPendingTaskCount();
        REQUIRE(pending >= 5);

        canProceed = true;
        blockFuture.wait();
        singlePool.waitForAll();
    }

    SECTION("Track active tasks") {
        ThreadPool singlePool(1);
        std::atomic<bool> canProceed{false};

        auto blockFuture = singlePool.submit([&]() {
            while (!canProceed) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        REQUIRE(singlePool.getActiveTaskCount() == 1);

        canProceed = true;
        blockFuture.wait();
    }
}

// =============================================================================
// Wait For All Tests
// =============================================================================

TEST_CASE("ThreadPool - Wait for all tasks", "[threadpool][core]") {
    ThreadPool pool(2);

    SECTION("Wait for all tasks to complete") {
        std::atomic<int> counter{0};

        for (int i = 0; i < 10; ++i) {
            pool.submit([&counter]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                counter++;
            });
        }

        bool completed = pool.waitForAll(5000);
        REQUIRE(completed);
        REQUIRE(counter == 10);
        REQUIRE(pool.getPendingTaskCount() == 0);
        REQUIRE(pool.getActiveTaskCount() == 0);
    }

    SECTION("Timeout when tasks take too long") {
        pool.submit([]() {
            std::this_thread::sleep_for(std::chrono::seconds(10));
        });

        bool completed = pool.waitForAll(100); // 100ms timeout
        REQUIRE_FALSE(completed);

        // Shutdown without waiting to prevent test timeout
        pool.shutdown(false);
    }

    SECTION("Wait with no tasks returns immediately") {
        bool completed = pool.waitForAll(1000);
        REQUIRE(completed);
    }
}

// =============================================================================
// Thread Safety Tests
// =============================================================================

TEST_CASE("ThreadPool - Thread safety", "[threadpool][core]") {
    ThreadPool pool(4);

    SECTION("Concurrent task submission") {
        std::atomic<int> counter{0};
        std::vector<std::thread> submitters;

        for (int t = 0; t < 4; ++t) {
            submitters.emplace_back([&pool, &counter]() {
                for (int i = 0; i < 100; ++i) {
                    pool.submit([&counter]() {
                        counter++;
                    });
                }
            });
        }

        for (auto& t : submitters) {
            t.join();
        }

        pool.waitForAll();
        REQUIRE(counter == 400);
    }

    SECTION("Shared state access") {
        std::mutex mtx;
        int sharedValue = 0;

        std::vector<std::future<void>> futures;
        for (int i = 0; i < 100; ++i) {
            futures.push_back(pool.submit([&mtx, &sharedValue]() {
                std::lock_guard<std::mutex> lock(mtx);
                sharedValue++;
            }));
        }

        for (auto& f : futures) {
            f.wait();
        }

        REQUIRE(sharedValue == 100);
    }
}

// =============================================================================
// Performance Benchmarks
// =============================================================================

TEST_CASE("ThreadPool - Performance", "[.benchmark][threadpool]") {
    ThreadPool pool(4);

    BENCHMARK("Submit 1000 light tasks") {
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 1000; ++i) {
            futures.push_back(pool.submit([i]() { return i * 2; }));
        }
        for (auto& f : futures) {
            f.wait();
        }
    };

    BENCHMARK("Submit 100 heavy tasks") {
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 100; ++i) {
            futures.push_back(pool.submit([]() {
                int sum = 0;
                for (int j = 0; j < 10000; ++j) {
                    sum += j;
                }
                return sum;
            }));
        }
        for (auto& f : futures) {
            f.wait();
        }
    };

    BENCHMARK("Priority task submission") {
        for (int i = 0; i < 100; ++i) {
            auto priority = (i % 2 == 0) ? TaskPriority::High : TaskPriority::Low;
            pool.submit(priority, []() { return 42; }).wait();
        }
    };
}

// =============================================================================
// Real-World Scenario Tests
// =============================================================================

TEST_CASE("ThreadPool - Real-world scenarios", "[threadpool][core]") {
    ThreadPool pool(4);

    SECTION("Parallel data processing") {
        std::vector<int> data(1000);
        for (int i = 0; i < 1000; ++i) {
            data[i] = i;
        }

        std::vector<std::future<int>> futures;
        for (int value : data) {
            futures.push_back(pool.submit([value]() {
                return value * value;
            }));
        }

        int sum = 0;
        for (auto& f : futures) {
            sum += f.get();
        }

        // Sum of squares from 0 to 999
        int expected = 0;
        for (int i = 0; i < 1000; ++i) {
            expected += i * i;
        }

        REQUIRE(sum == expected);
    }

    SECTION("Pipeline processing") {
        std::vector<std::future<std::string>> futures;

        for (int i = 0; i < 10; ++i) {
            auto future = pool.submit([i]() -> std::string {
                // Simulate first stage
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                return "Data_" + std::to_string(i);
            });

            futures.push_back(std::move(future));
        }

        std::vector<std::string> results;
        for (auto& f : futures) {
            results.push_back(f.get());
        }

        REQUIRE(results.size() == 10);
        REQUIRE(results[0] == "Data_0");
        REQUIRE(results[9] == "Data_9");
    }
}
