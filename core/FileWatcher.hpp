#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace mcf {

/**
 * @brief File change event type
 */
enum class FileChangeType {
    Modified,
    Created,
    Deleted
};

/**
 * @brief File change callback
 */
using FileChangeCallback = std::function<void(const std::string& path, FileChangeType changeType)>;

/**
 * @brief Watches files for changes and triggers callbacks
 *
 * Thread-safe file system watcher that monitors files for modifications,
 * creation, and deletion. Used for hot reloading plugins.
 */
class FileWatcher {
private:
    struct WatchedFile {
        std::filesystem::file_time_type lastModified;
        bool exists;
        FileChangeCallback callback;
    };

    // Watched files map
    std::map<std::string, WatchedFile> m_watchedFiles;

    // Thread safety
    mutable std::mutex m_mutex;

    // Polling thread
    std::thread m_watchThread;
    std::atomic<bool> m_running{false};
    std::chrono::milliseconds m_pollInterval{1000}; // 1 second default

public:
    /**
     * @brief Constructor
     * @param pollInterval Interval between file checks in milliseconds
     */
    explicit FileWatcher(std::chrono::milliseconds pollInterval = std::chrono::milliseconds(1000))
        : m_pollInterval(pollInterval) {
    }

    ~FileWatcher() {
        stop();
    }

    // Non-copyable
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

    /**
     * @brief Start watching for file changes
     */
    void start() {
        if (m_running) {
            return;
        }

        m_running = true;
        m_watchThread = std::thread(&FileWatcher::watchLoop, this);
    }

    /**
     * @brief Stop watching for file changes
     */
    void stop() {
        if (!m_running) {
            return;
        }

        m_running = false;
        if (m_watchThread.joinable()) {
            m_watchThread.join();
        }
    }

    /**
     * @brief Add a file to watch
     * @param path Path to the file
     * @param callback Function to call when file changes
     * @return true if successfully added
     */
    bool addWatch(const std::string& path, FileChangeCallback callback) {
        std::lock_guard<std::mutex> lock(m_mutex);

        namespace fs = std::filesystem;

        WatchedFile watchedFile;
        watchedFile.callback = std::move(callback);

        try {
            if (fs::exists(path)) {
                watchedFile.lastModified = fs::last_write_time(path);
                watchedFile.exists = true;
            } else {
                watchedFile.exists = false;
            }
        } catch (const std::exception&) {
            // File doesn't exist or can't be accessed
            watchedFile.exists = false;
        }

        m_watchedFiles[path] = std::move(watchedFile);
        return true;
    }

    /**
     * @brief Remove a file from watch list
     * @param path Path to the file to stop watching
     */
    void removeWatch(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_watchedFiles.erase(path);
    }

    /**
     * @brief Remove all watches
     */
    void clearWatches() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_watchedFiles.clear();
    }

    /**
     * @brief Check if a file is being watched
     * @param path Path to the file to check
     * @return true if the file is being watched, false otherwise
     */
    bool isWatching(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_watchedFiles.find(path) != m_watchedFiles.end();
    }

    /**
     * @brief Get number of watched files
     * @return Number of files currently being watched
     */
    size_t getWatchCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_watchedFiles.size();
    }

    /**
     * @brief Set poll interval
     * @param interval Time between file checks in milliseconds
     */
    void setPollInterval(std::chrono::milliseconds interval) {
        m_pollInterval = interval;
    }

    /**
     * @brief Check if watcher is running
     * @return true if the watch thread is active, false otherwise
     */
    bool isRunning() const {
        return m_running;
    }

private:
    /**
     * @brief Main watch loop (runs in separate thread)
     */
    void watchLoop() {
        while (m_running) {
            checkFiles();
            std::this_thread::sleep_for(m_pollInterval);
        }
    }

    /**
     * @brief Check all watched files for changes
     */
    void checkFiles() {
        namespace fs = std::filesystem;

        // Copy watched files to avoid holding lock during callbacks
        std::map<std::string, WatchedFile> filesCopy;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            filesCopy = m_watchedFiles;
        }

        // Check each file for changes
        for (auto& [path, watchedFile] : filesCopy) {
            try {
                bool exists = fs::exists(path);

                if (exists && !watchedFile.exists) {
                    // File was created
                    watchedFile.exists = true;
                    watchedFile.lastModified = fs::last_write_time(path);

                    if (watchedFile.callback) {
                        watchedFile.callback(path, FileChangeType::Created);
                    }

                    // Update stored state
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        auto it = m_watchedFiles.find(path);
                        if (it != m_watchedFiles.end()) {
                            it->second.exists = true;
                            it->second.lastModified = watchedFile.lastModified;
                        }
                    }
                }
                else if (!exists && watchedFile.exists) {
                    // File was deleted
                    watchedFile.exists = false;

                    if (watchedFile.callback) {
                        watchedFile.callback(path, FileChangeType::Deleted);
                    }

                    // Update stored state
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        auto it = m_watchedFiles.find(path);
                        if (it != m_watchedFiles.end()) {
                            it->second.exists = false;
                        }
                    }
                }
                else if (exists && watchedFile.exists) {
                    // Check for modification
                    auto lastModified = fs::last_write_time(path);
                    if (lastModified != watchedFile.lastModified) {
                        watchedFile.lastModified = lastModified;

                        if (watchedFile.callback) {
                            watchedFile.callback(path, FileChangeType::Modified);
                        }

                        // Update stored state
                        {
                            std::lock_guard<std::mutex> lock(m_mutex);
                            auto it = m_watchedFiles.find(path);
                            if (it != m_watchedFiles.end()) {
                                it->second.lastModified = lastModified;
                            }
                        }
                    }
                }
            }
            catch (const std::exception&) {
                // Ignore errors (file might be temporarily inaccessible)
            }
        }
    }
};

} // namespace mcf
