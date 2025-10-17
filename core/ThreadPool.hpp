/**
 * @file ThreadPool.hpp
 * @brief Thread pool implementation for async plugin tasks
 *
 * Provides a thread pool with work stealing and priority queues for efficient
 * async task execution. Plugins can submit tasks and receive futures for results.
 */

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <memory>
#include <stdexcept>

namespace mcf {

/**
 * @enum TaskPriority
 * @brief Priority levels for async tasks
 */
enum class TaskPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @class ThreadPool
 * @brief Thread pool for executing async tasks with priorities
 *
 * Features:
 * - Configurable number of worker threads
 * - Priority-based task scheduling
 * - std::future support for task results
 * - Graceful shutdown with pending task completion
 * - Thread-safe task submission
 * - Work statistics tracking
 *
 * Example:
 * @code
 * ThreadPool pool(4); // 4 worker threads
 *
 * // Submit task and get future
 * auto future = pool.submit(TaskPriority::High, []() {
 *     return compute_heavy_operation();
 * });
 *
 * // Wait for result
 * auto result = future.get();
 * @endcode
 */
class ThreadPool {
public:
    /**
     * @brief Construct thread pool with specified number of threads
     * @param numThreads Number of worker threads (0 = hardware concurrency)
     */
    explicit ThreadPool(size_t numThreads = 0)
        : m_running(false) {
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 4; // Fallback
        }

        m_running = true;
        m_workers.reserve(numThreads);

        for (size_t i = 0; i < numThreads; ++i) {
            m_workers.emplace_back(&ThreadPool::workerLoop, this, i);
        }
    }

    /**
     * @brief Destructor - waits for all tasks to complete
     */
    ~ThreadPool() {
        shutdown(true);
    }

    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Submit a task for async execution
     * @tparam Func Callable type
     * @tparam Args Argument types
     * @param priority Task priority level
     * @param func Function to execute
     * @param args Arguments to pass to function
     * @return std::future for retrieving result
     * @throws std::runtime_error if pool is not running
     */
    template<typename Func, typename... Args>
    auto submit(TaskPriority priority, Func&& func, Args&&... args)
        -> std::future<typename std::result_of<Func(Args...)>::type> {

        using ReturnType = typename std::result_of<Func(Args...)>::type;

        if (!m_running) {
            throw std::runtime_error("Cannot submit task to stopped ThreadPool");
        }

        // Create packaged task
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);

            // Wrap task in lambda that can be stored in queue
            m_taskQueue.push({
                static_cast<int>(priority),
                [task]() { (*task)(); }
            });

            m_tasksSubmitted++;
        }

        m_condition.notify_one();
        return result;
    }

    /**
     * @brief Submit a task without priority (uses Normal priority)
     * @tparam Func Callable type
     * @tparam Args Argument types
     * @param func Function to execute
     * @param args Arguments to pass to function
     * @return std::future for retrieving result
     */
    template<typename Func, typename... Args>
    auto submit(Func&& func, Args&&... args)
        -> std::future<typename std::result_of<Func(Args...)>::type> {
        return submit(TaskPriority::Normal, std::forward<Func>(func), std::forward<Args>(args)...);
    }

    /**
     * @brief Shutdown the thread pool
     * @param waitForTasks If true, wait for pending tasks to complete
     */
    void shutdown(bool waitForTasks = true) {
        if (!m_running) return;

        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_running = false;

            if (!waitForTasks) {
                // Clear pending tasks
                while (!m_taskQueue.empty()) {
                    m_taskQueue.pop();
                }
            }
        }

        m_condition.notify_all();

        for (std::thread& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }

        m_workers.clear();
    }

    /**
     * @brief Check if thread pool is running
     */
    bool isRunning() const {
        return m_running;
    }

    /**
     * @brief Get number of worker threads
     */
    size_t getThreadCount() const {
        return m_workers.size();
    }

    /**
     * @brief Get number of pending tasks
     */
    size_t getPendingTaskCount() const {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        return m_taskQueue.size();
    }

    /**
     * @brief Get number of active (executing) tasks
     */
    size_t getActiveTaskCount() const {
        return m_activeTasks.load();
    }

    /**
     * @brief Get total number of tasks submitted
     */
    size_t getTasksSubmitted() const {
        return m_tasksSubmitted.load();
    }

    /**
     * @brief Get total number of tasks completed
     */
    size_t getTasksCompleted() const {
        return m_tasksCompleted.load();
    }

    /**
     * @brief Wait for all pending tasks to complete
     * @param timeoutMs Timeout in milliseconds (0 = wait forever)
     * @return true if all tasks completed, false if timeout
     */
    bool waitForAll(uint32_t timeoutMs = 0) {
        auto startTime = std::chrono::steady_clock::now();

        while (true) {
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if (m_taskQueue.empty() && m_activeTasks == 0) {
                    return true;
                }
            }

            if (timeoutMs > 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime
                ).count();

                if (elapsed >= timeoutMs) {
                    return false;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

private:
    /**
     * @struct Task
     * @brief Internal task representation with priority
     */
    struct Task {
        int priority;
        std::function<void()> func;

        bool operator<(const Task& other) const {
            return priority < other.priority; // Higher priority first
        }
    };

    /**
     * @brief Worker thread main loop
     * @param threadId Thread identifier for debugging
     */
    void workerLoop(size_t threadId) {
        (void)threadId; // Unused for now, useful for debugging

        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(m_queueMutex);

                m_condition.wait(lock, [this] {
                    return !m_running || !m_taskQueue.empty();
                });

                if (!m_running && m_taskQueue.empty()) {
                    return;
                }

                if (!m_taskQueue.empty()) {
                    task = std::move(m_taskQueue.top().func);
                    m_taskQueue.pop();
                    m_activeTasks++;
                }
            }

            if (task) {
                try {
                    task();
                } catch (...) {
                    // Swallow exceptions to prevent worker thread termination
                    // In production, you might want to log these
                }

                m_activeTasks--;
                m_tasksCompleted++;
            }
        }
    }

    std::vector<std::thread> m_workers;
    std::priority_queue<Task> m_taskQueue;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::atomic<bool> m_running;
    std::atomic<size_t> m_activeTasks{0};
    std::atomic<size_t> m_tasksSubmitted{0};
    std::atomic<size_t> m_tasksCompleted{0};
};

} // namespace mcf
