#pragma once

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEPARATOR '\\'
#define PATH_SEPARATOR_STR "\\"
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define PATH_SEPARATOR '/'
#define PATH_SEPARATOR_STR "/"
#endif

namespace mcf {

/**
 * @brief File type enumeration
 */
enum class FileType {
    Regular,
    Directory,
    Symlink,
    Unknown
};

/**
 * @brief File information structure
 */
struct FileInfo {
    std::string name;  ///< File or directory name (without path)
    std::string path;  ///< Full path to the file or directory
    FileType type;     ///< Type of the file (Regular, Directory, Symlink, Unknown)
    size_t size;       ///< Size of the file in bytes (0 for directories)
    std::chrono::system_clock::time_point lastModified;  ///< Last modification timestamp
    bool isHidden;     ///< True if the file is hidden (starts with '.' on Unix, has hidden attribute on Windows)

    FileInfo()
        : type(FileType::Unknown)
        , size(0)
        , isHidden(false) {}
};

/**
 * @brief Path utility class for path manipulation
 */
class Path {
public:
    /**
     * @brief Join path components
     * @param path1 First path component
     * @param path2 Second path component
     * @return Combined path with platform-specific separator
     */
    static std::string join(const std::string& path1, const std::string& path2) {
        if (path1.empty()) return path2;
        if (path2.empty()) return path1;

        char lastChar = path1.back();
        if (lastChar == '/' || lastChar == '\\') {
            return path1 + path2;
        }
        return path1 + PATH_SEPARATOR_STR + path2;
    }

    /**
     * @brief Join multiple path components
     * @param first First path component
     * @param second Second path component
     * @param args Additional path components
     * @return Combined path with platform-specific separator
     */
    template<typename... Args>
    static std::string join(const std::string& first, const std::string& second, Args... args) {
        return join(join(first, second), args...);
    }

    /**
     * @brief Get the directory name from a path
     * @param path Path to extract directory from
     * @return Directory portion of the path, or "." if no directory component
     */
    static std::string dirname(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos == std::string::npos) {
            return ".";
        }
        if (pos == 0) {
            return PATH_SEPARATOR_STR;
        }
        return path.substr(0, pos);
    }

    /**
     * @brief Get the base name from a path
     * @param path Path to extract filename from
     * @return Filename portion of the path (including extension)
     */
    static std::string basename(const std::string& path) {
        size_t pos = path.find_last_of("/\\");
        if (pos == std::string::npos) {
            return path;
        }
        return path.substr(pos + 1);
    }

    /**
     * @brief Get file extension
     * @param path Path to extract extension from
     * @return File extension including the dot (e.g., ".txt"), or empty string if no extension
     */
    static std::string extension(const std::string& path) {
        std::string base = basename(path);
        size_t pos = base.find_last_of('.');
        if (pos == std::string::npos || pos == 0) {
            return "";
        }
        return base.substr(pos);
    }

    /**
     * @brief Get filename without extension
     * @param path Path to extract stem from
     * @return Filename without extension (e.g., "file" from "file.txt")
     */
    static std::string stem(const std::string& path) {
        std::string base = basename(path);
        size_t pos = base.find_last_of('.');
        if (pos == std::string::npos || pos == 0) {
            return base;
        }
        return base.substr(0, pos);
    }

    /**
     * @brief Normalize path separators
     * @param path Path to normalize
     * @return Path with platform-specific separators (/ on Unix, \\ on Windows)
     */
    static std::string normalize(const std::string& path) {
        std::string result = path;
        std::replace(result.begin(), result.end(),
                    PATH_SEPARATOR == '/' ? '\\' : '/',
                    PATH_SEPARATOR);
        return result;
    }

    /**
     * @brief Check if path is absolute
     * @param path Path to check
     * @return True if the path is absolute, false if relative
     */
    static bool isAbsolute(const std::string& path) {
        if (path.empty()) return false;

#ifdef _WIN32
        // Windows: C:\ or \\server\share
        if (path.length() >= 3 && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) {
            return true;
        }
        if (path.length() >= 2 && path[0] == '\\' && path[1] == '\\') {
            return true;
        }
        return false;
#else
        return path[0] == '/';
#endif
    }
};

/**
 * @brief File system operations manager
 *
 * Provides cross-platform file system operations with:
 * - Thread-safe operations
 * - File reading and writing
 * - Directory navigation
 * - Path manipulation
 * - File information retrieval
 */
class FileSystem {
private:
    mutable std::mutex m_mutex;

    /**
     * @brief Internal helper for checking existence without lock
     */
    bool existsInternal(const std::string& path) const {
#ifdef _WIN32
        DWORD attrs = GetFileAttributesA(path.c_str());
        return (attrs != INVALID_FILE_ATTRIBUTES);
#else
        struct stat st;
        return (stat(path.c_str(), &st) == 0);
#endif
    }

    /**
     * @brief Internal helper for checking if directory without lock
     */
    bool isDirectoryInternal(const std::string& path) const {
#ifdef _WIN32
        DWORD attrs = GetFileAttributesA(path.c_str());
        return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
        struct stat st;
        return (stat(path.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
#endif
    }

    /**
     * @brief Internal helper for directory creation without lock (for recursion)
     */
    bool createDirectoryInternal(const std::string& path, bool createParents) {
        if (existsInternal(path)) {
            return isDirectoryInternal(path);
        }

        if (createParents) {
            std::string parent = Path::dirname(path);
            if (!parent.empty() && parent != "." && !existsInternal(parent)) {
                if (!createDirectoryInternal(parent, true)) {
                    return false;
                }
            }
        }

#ifdef _WIN32
        return CreateDirectoryA(path.c_str(), NULL) != 0;
#else
        return mkdir(path.c_str(), 0755) == 0;
#endif
    }

    /**
     * @brief Internal helper for file removal without lock
     */
    bool removeFileInternal(const std::string& path) {
#ifdef _WIN32
        return DeleteFileA(path.c_str()) != 0;
#else
        return unlink(path.c_str()) == 0;
#endif
    }

    /**
     * @brief Internal helper for directory removal without lock
     */
    bool removeDirectoryInternal(const std::string& path) {
#ifdef _WIN32
        return RemoveDirectoryA(path.c_str()) != 0;
#else
        return rmdir(path.c_str()) == 0;
#endif
    }

    /**
     * @brief Internal helper for recursive removal without lock
     */
    bool removeAllInternal(const std::string& path) {
        if (!existsInternal(path)) {
            return true;
        }

        if (isDirectoryInternal(path)) {
            auto entries = listDirectoryInternal(path, false);
            for (const auto& entry : entries) {
                if (!removeAllInternal(entry)) {
                    return false;
                }
            }
            return removeDirectoryInternal(path);
        } else {
            return removeFileInternal(path);
        }
    }

    /**
     * @brief Internal helper for directory listing without lock (for recursion)
     */
    std::vector<std::string> listDirectoryInternal(const std::string& path, bool recursive) const {
        std::vector<std::string> results;

#ifdef _WIN32
        std::string searchPath = path + "\\*";
        WIN32_FIND_DATAA findData;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

        if (hFind == INVALID_HANDLE_VALUE) {
            return results;
        }

        do {
            std::string name = findData.cFileName;
            if (name == "." || name == "..") continue;

            std::string fullPath = path + "\\" + name;
            results.push_back(fullPath);

            if (recursive && (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                auto subResults = listDirectoryInternal(fullPath, true);
                results.insert(results.end(), subResults.begin(), subResults.end());
            }
        } while (FindNextFileA(hFind, &findData));

        FindClose(hFind);
#else
        DIR* dir = opendir(path.c_str());
        if (!dir) {
            return results;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;

            std::string fullPath = path + "/" + name;
            results.push_back(fullPath);

            if (recursive && entry->d_type == DT_DIR) {
                auto subResults = listDirectoryInternal(fullPath, true);
                results.insert(results.end(), subResults.begin(), subResults.end());
            }
        }

        closedir(dir);
#endif

        return results;
    }

    /**
     * @brief Get file type from stat
     */
#ifdef _WIN32
    static FileType getFileType(DWORD attributes) {
        if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
            return FileType::Directory;
        }
        if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) {
            return FileType::Symlink;
        }
        return FileType::Regular;
    }
#else
    static FileType getFileType(mode_t mode) {
        if (S_ISREG(mode)) return FileType::Regular;
        if (S_ISDIR(mode)) return FileType::Directory;
        if (S_ISLNK(mode)) return FileType::Symlink;
        return FileType::Unknown;
    }
#endif

public:
    FileSystem() = default;
    ~FileSystem() = default;

    // Non-copyable
    FileSystem(const FileSystem&) = delete;
    FileSystem& operator=(const FileSystem&) = delete;

    /**
     * @brief Check if a file or directory exists
     * @param path Path to check
     * @return True if the path exists, false otherwise
     */
    bool exists(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
        DWORD attrs = GetFileAttributesA(path.c_str());
        return (attrs != INVALID_FILE_ATTRIBUTES);
#else
        struct stat st;
        return (stat(path.c_str(), &st) == 0);
#endif
    }

    /**
     * @brief Check if path is a directory
     * @param path Path to check
     * @return True if the path is a directory, false otherwise
     */
    bool isDirectory(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
        DWORD attrs = GetFileAttributesA(path.c_str());
        return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
        struct stat st;
        return (stat(path.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
#endif
    }

    /**
     * @brief Check if path is a regular file
     * @param path Path to check
     * @return True if the path is a regular file, false otherwise
     */
    bool isFile(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
        DWORD attrs = GetFileAttributesA(path.c_str());
        return (attrs != INVALID_FILE_ATTRIBUTES) && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
        struct stat st;
        return (stat(path.c_str(), &st) == 0) && S_ISREG(st.st_mode);
#endif
    }

    /**
     * @brief Get file size in bytes
     * @param path Path to the file
     * @return Size of the file in bytes, or 0 if file doesn't exist or is a directory
     */
    size_t getFileSize(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
        WIN32_FILE_ATTRIBUTE_DATA fileInfo;
        if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileInfo)) {
            LARGE_INTEGER size;
            size.HighPart = fileInfo.nFileSizeHigh;
            size.LowPart = fileInfo.nFileSizeLow;
            return static_cast<size_t>(size.QuadPart);
        }
        return 0;
#else
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            return static_cast<size_t>(st.st_size);
        }
        return 0;
#endif
    }

    /**
     * @brief Get file information
     * @param path Path to the file or directory
     * @return FileInfo structure containing file metadata
     */
    FileInfo getFileInfo(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        FileInfo info;
        info.path = path;
        info.name = Path::basename(path);

#ifdef _WIN32
        WIN32_FILE_ATTRIBUTE_DATA fileData;
        if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &fileData)) {
            info.type = getFileType(fileData.dwFileAttributes);

            LARGE_INTEGER size;
            size.HighPart = fileData.nFileSizeHigh;
            size.LowPart = fileData.nFileSizeLow;
            info.size = static_cast<size_t>(size.QuadPart);

            info.isHidden = (fileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0;

            // Convert FILETIME to time_point
            ULARGE_INTEGER ull;
            ull.LowPart = fileData.ftLastWriteTime.dwLowDateTime;
            ull.HighPart = fileData.ftLastWriteTime.dwHighDateTime;
            auto duration = std::chrono::nanoseconds((ull.QuadPart - 116444736000000000ULL) * 100);
            info.lastModified = std::chrono::system_clock::time_point(
                std::chrono::duration_cast<std::chrono::system_clock::duration>(duration)
            );
        }
#else
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            info.type = getFileType(st.st_mode);
            info.size = static_cast<size_t>(st.st_size);
            info.isHidden = (info.name[0] == '.');
            info.lastModified = std::chrono::system_clock::from_time_t(st.st_mtime);
        }
#endif

        return info;
    }

    /**
     * @brief List directory contents
     * @param path Directory path
     * @param recursive If true, recursively list subdirectories
     * @return Vector of file paths
     */
    std::vector<std::string> listDirectory(const std::string& path, bool recursive = false) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return listDirectoryInternal(path, recursive);
    }

    /**
     * @brief List directory with file information
     * @param path Directory path to list
     * @param recursive If true, recursively list subdirectories
     * @return Vector of FileInfo structures for each entry
     */
    std::vector<FileInfo> listDirectoryInfo(const std::string& path, bool recursive = false) const {
        auto paths = listDirectory(path, recursive);
        std::vector<FileInfo> results;
        results.reserve(paths.size());

        for (const auto& p : paths) {
            results.push_back(getFileInfo(p));
        }

        return results;
    }

    /**
     * @brief Create a directory
     * @param path Directory path
     * @param createParents If true, create parent directories as needed
     * @return true if successful
     */
    bool createDirectory(const std::string& path, bool createParents = false) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return createDirectoryInternal(path, createParents);
    }

    /**
     * @brief Remove a file
     * @param path Path to the file to remove
     * @return True if successful, false otherwise
     */
    bool removeFile(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return removeFileInternal(path);
    }

    /**
     * @brief Remove a directory (must be empty)
     * @param path Path to the directory to remove
     * @return True if successful, false otherwise
     */
    bool removeDirectory(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return removeDirectoryInternal(path);
    }

    /**
     * @brief Remove a directory and all its contents
     * @param path Path to the directory to remove recursively
     * @return True if successful, false otherwise
     */
    bool removeAll(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);
        return removeAllInternal(path);
    }

    /**
     * @brief Copy a file
     * @param source Path to the source file
     * @param destination Path to the destination file
     * @return True if successful, false otherwise
     */
    bool copyFile(const std::string& source, const std::string& destination) {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::ifstream src(source, std::ios::binary);
        if (!src.is_open()) {
            return false;
        }

        std::ofstream dst(destination, std::ios::binary);
        if (!dst.is_open()) {
            return false;
        }

        dst << src.rdbuf();

        return src.good() && dst.good();
    }

    /**
     * @brief Move/rename a file or directory
     * @param source Path to the source file or directory
     * @param destination Path to the destination file or directory
     * @return True if successful, false otherwise
     */
    bool move(const std::string& source, const std::string& destination) {
        std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
        return MoveFileA(source.c_str(), destination.c_str()) != 0;
#else
        return rename(source.c_str(), destination.c_str()) == 0;
#endif
    }

    /**
     * @brief Read entire file as string
     * @param path Path to the file to read
     * @return File contents as a string
     * @throws std::runtime_error If file cannot be opened or read
     */
    std::string readFile(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        std::string content;
        file.seekg(0, std::ios::end);
        content.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        file.read(&content[0], content.size());

        if (!file.good()) {
            throw std::runtime_error("Failed to read file: " + path);
        }

        return content;
    }

    /**
     * @brief Read entire file as binary data
     * @param path Path to the file to read
     * @return File contents as a vector of bytes
     * @throws std::runtime_error If file cannot be opened or read
     */
    std::vector<uint8_t> readBinary(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> data(size);
        file.read(reinterpret_cast<char*>(data.data()), size);

        if (!file.good()) {
            throw std::runtime_error("Failed to read file: " + path);
        }

        return data;
    }

    /**
     * @brief Read file line by line
     * @param path Path to the file to read
     * @return Vector of strings, one per line
     * @throws std::runtime_error If file cannot be opened
     */
    std::vector<std::string> readLines(const std::string& path) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::ifstream file(path);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }

        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            lines.push_back(line);
        }

        return lines;
    }

    /**
     * @brief Write string to file
     * @param path File path
     * @param content Content to write
     * @param append If true, append to file; otherwise overwrite
     */
    bool writeFile(const std::string& path, const std::string& content, bool append = false) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto mode = std::ios::binary;
        if (append) {
            mode |= std::ios::app;
        }

        std::ofstream file(path, mode);
        if (!file.is_open()) {
            return false;
        }

        file.write(content.data(), content.size());
        return file.good();
    }

    /**
     * @brief Write binary data to file
     * @param path Path to the file to write
     * @param data Binary data to write
     * @param append If true, append to file; otherwise overwrite
     * @return True if successful, false otherwise
     */
    bool writeBinary(const std::string& path, const std::vector<uint8_t>& data, bool append = false) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto mode = std::ios::binary;
        if (append) {
            mode |= std::ios::app;
        }

        std::ofstream file(path, mode);
        if (!file.is_open()) {
            return false;
        }

        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        return file.good();
    }

    /**
     * @brief Write lines to file
     * @param path Path to the file to write
     * @param lines Vector of strings to write, each as a line
     * @param append If true, append to file; otherwise overwrite
     * @return True if successful, false otherwise
     */
    bool writeLines(const std::string& path, const std::vector<std::string>& lines, bool append = false) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto mode = std::ios::out;
        if (append) {
            mode |= std::ios::app;
        }

        std::ofstream file(path, mode);
        if (!file.is_open()) {
            return false;
        }

        for (const auto& line : lines) {
            file << line << '\n';
        }

        return file.good();
    }

    /**
     * @brief Get current working directory
     * @return Path to the current working directory, or empty string on failure
     */
    std::string getCurrentDirectory() const {
        std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
        char buffer[MAX_PATH];
        if (GetCurrentDirectoryA(MAX_PATH, buffer)) {
            return std::string(buffer);
        }
        return "";
#else
        char buffer[PATH_MAX];
        if (getcwd(buffer, sizeof(buffer))) {
            return std::string(buffer);
        }
        return "";
#endif
    }

    /**
     * @brief Set current working directory
     * @param path Path to set as the current working directory
     * @return True if successful, false otherwise
     */
    bool setCurrentDirectory(const std::string& path) {
        std::lock_guard<std::mutex> lock(m_mutex);

#ifdef _WIN32
        return SetCurrentDirectoryA(path.c_str()) != 0;
#else
        return chdir(path.c_str()) == 0;
#endif
    }

    /**
     * @brief Find files matching a pattern (simple wildcard matching)
     * @param path Directory to search
     * @param pattern Pattern to match (supports * and ? wildcards)
     * @param recursive If true, search recursively in subdirectories
     * @return Vector of file paths matching the pattern
     */
    std::vector<std::string> find(const std::string& path, const std::string& pattern, bool recursive = false) const {
        auto files = listDirectory(path, recursive);
        std::vector<std::string> results;

        for (const auto& file : files) {
            std::string filename = Path::basename(file);
            if (matchPattern(filename, pattern)) {
                results.push_back(file);
            }
        }

        return results;
    }

private:
    /**
     * @brief Simple wildcard pattern matching (supports *)
     */
    bool matchPattern(const std::string& str, const std::string& pattern) const {
        size_t strPos = 0;
        size_t patPos = 0;
        size_t starPos = std::string::npos;
        size_t matchPos = 0;

        while (strPos < str.length()) {
            if (patPos < pattern.length() &&
                (pattern[patPos] == str[strPos] || pattern[patPos] == '?')) {
                strPos++;
                patPos++;
            } else if (patPos < pattern.length() && pattern[patPos] == '*') {
                starPos = patPos;
                matchPos = strPos;
                patPos++;
            } else if (starPos != std::string::npos) {
                patPos = starPos + 1;
                matchPos++;
                strPos = matchPos;
            } else {
                return false;
            }
        }

        while (patPos < pattern.length() && pattern[patPos] == '*') {
            patPos++;
        }

        return patPos == pattern.length();
    }
};

} // namespace mcf
