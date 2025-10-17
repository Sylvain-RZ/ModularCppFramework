#include "../core/FileSystem.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace mcf;

/**
 * @brief Example demonstrating FileSystem module capabilities
 *
 * This example showcases:
 * - File and directory operations
 * - Reading and writing files
 * - Directory traversal
 * - Path manipulation
 * - File information retrieval
 */

void printSeparator(const std::string& title) {
    std::cout << "\n========================================\n";
    std::cout << "  " << title << "\n";
    std::cout << "========================================\n";
}

void demonstrateBasicFileOperations() {
    printSeparator("Basic File Operations");

    FileSystem fs;

    // Create a test directory
    std::string testDir = "./filesystem_example_data";
    if (fs.createDirectory(testDir)) {
        std::cout << "Created directory: " << testDir << std::endl;
    }

    // Write a text file
    std::string testFile = Path::join(testDir, "example.txt");
    std::string content = "Hello, ModularCppFramework!\nThis is a test file.";

    if (fs.writeFile(testFile, content)) {
        std::cout << "Wrote file: " << testFile << std::endl;
    }

    // Read the file back
    std::string readContent = fs.readFile(testFile);
    std::cout << "Read content:\n" << readContent << std::endl;

    // Get file size
    size_t fileSize = fs.getFileSize(testFile);
    std::cout << "File size: " << fileSize << " bytes" << std::endl;

    // Check file existence
    std::cout << "File exists: " << (fs.exists(testFile) ? "Yes" : "No") << std::endl;
    std::cout << "Is file: " << (fs.isFile(testFile) ? "Yes" : "No") << std::endl;
    std::cout << "Is directory: " << (fs.isDirectory(testFile) ? "Yes" : "No") << std::endl;
}

void demonstrateLineOperations() {
    printSeparator("Line-Based Operations");

    FileSystem fs;

    std::string testDir = "./filesystem_example_data";
    std::string linesFile = Path::join(testDir, "lines.txt");

    // Write lines
    std::vector<std::string> lines = {
        "Line 1: Introduction",
        "Line 2: Body",
        "Line 3: Conclusion"
    };

    if (fs.writeLines(linesFile, lines)) {
        std::cout << "Wrote " << lines.size() << " lines to " << linesFile << std::endl;
    }

    // Read lines
    auto readLines = fs.readLines(linesFile);
    std::cout << "\nRead " << readLines.size() << " lines:" << std::endl;
    for (size_t i = 0; i < readLines.size(); ++i) {
        std::cout << "  [" << i << "] " << readLines[i] << std::endl;
    }

    // Append more lines
    std::vector<std::string> additionalLines = {
        "Line 4: Appendix A",
        "Line 5: Appendix B"
    };

    if (fs.writeLines(linesFile, additionalLines, true)) {
        std::cout << "\nAppended " << additionalLines.size() << " lines" << std::endl;
    }

    // Read all lines again
    auto allLines = fs.readLines(linesFile);
    std::cout << "Total lines now: " << allLines.size() << std::endl;
}

void demonstrateBinaryOperations() {
    printSeparator("Binary File Operations");

    FileSystem fs;

    std::string testDir = "./filesystem_example_data";
    std::string binaryFile = Path::join(testDir, "data.bin");

    // Create binary data
    std::vector<uint8_t> data;
    for (int i = 0; i < 256; ++i) {
        data.push_back(static_cast<uint8_t>(i));
    }

    if (fs.writeBinary(binaryFile, data)) {
        std::cout << "Wrote " << data.size() << " bytes to " << binaryFile << std::endl;
    }

    // Read binary data
    auto readData = fs.readBinary(binaryFile);
    std::cout << "Read " << readData.size() << " bytes" << std::endl;

    // Verify data integrity
    bool dataMatches = (readData == data);
    std::cout << "Data integrity check: " << (dataMatches ? "PASSED" : "FAILED") << std::endl;
}

void demonstrateDirectoryOperations() {
    printSeparator("Directory Operations");

    FileSystem fs;

    std::string testDir = "./filesystem_example_data";

    // Create nested directories
    std::string nestedDir = Path::join(testDir, "subdir", "nested");
    if (fs.createDirectory(nestedDir, true)) {
        std::cout << "Created nested directory: " << nestedDir << std::endl;
    }

    // Create some files in different directories
    fs.writeFile(Path::join(testDir, "file1.txt"), "File 1");
    fs.writeFile(Path::join(testDir, "file2.txt"), "File 2");
    fs.writeFile(Path::join(testDir, "subdir", "file3.txt"), "File 3");
    fs.writeFile(Path::join(testDir, "subdir", "nested", "file4.txt"), "File 4");

    std::cout << "\nCreated test files in directory structure" << std::endl;

    // List directory (non-recursive)
    std::cout << "\nDirectory contents (non-recursive):" << std::endl;
    auto entries = fs.listDirectory(testDir, false);
    for (const auto& entry : entries) {
        std::cout << "  - " << Path::basename(entry) << std::endl;
    }

    // List directory (recursive)
    std::cout << "\nDirectory contents (recursive):" << std::endl;
    auto allEntries = fs.listDirectory(testDir, true);
    for (const auto& entry : allEntries) {
        std::string relativePath = entry.substr(testDir.length() + 1);
        std::cout << "  - " << relativePath << std::endl;
    }
}

void demonstrateFileInfo() {
    printSeparator("File Information");

    FileSystem fs;

    std::string testDir = "./filesystem_example_data";

    // Get file info for all entries
    auto infos = fs.listDirectoryInfo(testDir, false);

    std::cout << "\nDetailed file information:" << std::endl;
    std::cout << std::left;
    std::cout << "Type      Size      Name" << std::endl;
    std::cout << "--------- --------- --------------------" << std::endl;

    for (const auto& info : infos) {
        std::string typeStr;
        switch (info.type) {
            case FileType::Regular:   typeStr = "File"; break;
            case FileType::Directory: typeStr = "Dir "; break;
            case FileType::Symlink:   typeStr = "Link"; break;
            default:                  typeStr = "????"; break;
        }

        std::cout << typeStr << "      ";
        std::cout.width(9);
        std::cout << info.size << " ";
        std::cout << info.name << std::endl;
    }
}

void demonstratePathUtilities() {
    printSeparator("Path Utilities");

    std::string testPath = "/home/user/documents/report.pdf";

    std::cout << "Path: " << testPath << std::endl;
    std::cout << "  Directory:  " << Path::dirname(testPath) << std::endl;
    std::cout << "  Basename:   " << Path::basename(testPath) << std::endl;
    std::cout << "  Extension:  " << Path::extension(testPath) << std::endl;
    std::cout << "  Stem:       " << Path::stem(testPath) << std::endl;
    std::cout << "  Is absolute: " << (Path::isAbsolute(testPath) ? "Yes" : "No") << std::endl;

    std::cout << "\nPath joining examples:" << std::endl;
    std::cout << "  " << Path::join("dir", "file.txt") << std::endl;
    std::cout << "  " << Path::join("a", "b", "c", "d.txt") << std::endl;

    std::string relativePath = "relative/path/file.txt";
    std::cout << "\nRelative path: " << relativePath << std::endl;
    std::cout << "  Is absolute: " << (Path::isAbsolute(relativePath) ? "Yes" : "No") << std::endl;
}

void demonstrateFileCopyMove() {
    printSeparator("File Copy and Move");

    FileSystem fs;

    std::string testDir = "./filesystem_example_data";
    std::string sourceFile = Path::join(testDir, "source.txt");
    std::string copiedFile = Path::join(testDir, "copied.txt");
    std::string movedFile = Path::join(testDir, "moved.txt");

    // Create source file
    fs.writeFile(sourceFile, "This is the source file.");
    std::cout << "Created source file: " << sourceFile << std::endl;

    // Copy file
    if (fs.copyFile(sourceFile, copiedFile)) {
        std::cout << "Copied to: " << copiedFile << std::endl;
        std::cout << "  Content: " << fs.readFile(copiedFile) << std::endl;
    }

    // Move file
    if (fs.move(copiedFile, movedFile)) {
        std::cout << "Moved from " << Path::basename(copiedFile)
                  << " to " << Path::basename(movedFile) << std::endl;
        std::cout << "  Original exists: " << (fs.exists(copiedFile) ? "Yes" : "No") << std::endl;
        std::cout << "  New location exists: " << (fs.exists(movedFile) ? "Yes" : "No") << std::endl;
    }
}

void demonstrateFileSearch() {
    printSeparator("File Search with Patterns");

    FileSystem fs;

    std::string testDir = "./filesystem_example_data";

    // Create files with different extensions
    fs.writeFile(Path::join(testDir, "report.pdf"), "PDF content");
    fs.writeFile(Path::join(testDir, "data.json"), "JSON content");
    fs.writeFile(Path::join(testDir, "config.json"), "JSON config");
    fs.writeFile(Path::join(testDir, "script.py"), "Python script");

    std::cout << "Created test files with various extensions\n" << std::endl;

    // Find all .json files
    auto jsonFiles = fs.find(testDir, "*.json", false);
    std::cout << "JSON files (*.json):" << std::endl;
    for (const auto& file : jsonFiles) {
        std::cout << "  - " << Path::basename(file) << std::endl;
    }

    // Find files starting with 'report'
    auto reportFiles = fs.find(testDir, "report*", false);
    std::cout << "\nFiles starting with 'report':" << std::endl;
    for (const auto& file : reportFiles) {
        std::cout << "  - " << Path::basename(file) << std::endl;
    }

    // Find all .txt files recursively
    auto txtFiles = fs.find(testDir, "*.txt", true);
    std::cout << "\nAll .txt files (recursive):" << std::endl;
    for (const auto& file : txtFiles) {
        std::string relativePath = file.substr(testDir.length() + 1);
        std::cout << "  - " << relativePath << std::endl;
    }
}

void demonstrateCurrentDirectory() {
    printSeparator("Current Directory Operations");

    FileSystem fs;

    // Get current directory
    std::string cwd = fs.getCurrentDirectory();
    std::cout << "Current working directory:" << std::endl;
    std::cout << "  " << cwd << std::endl;

    // Note: We don't change directory in this example to avoid side effects
    std::cout << "\nNote: Directory changing not demonstrated to avoid side effects" << std::endl;
}

void cleanupExample() {
    printSeparator("Cleanup");

    FileSystem fs;

    std::string testDir = "./filesystem_example_data";

    if (fs.exists(testDir)) {
        std::cout << "Removing test directory and all contents..." << std::endl;
        if (fs.removeAll(testDir)) {
            std::cout << "Cleanup completed successfully!" << std::endl;
        } else {
            std::cout << "Warning: Cleanup may have failed" << std::endl;
        }
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  FileSystem Module Example\n";
    std::cout << "  ModularCppFramework\n";
    std::cout << "========================================\n";

    try {
        // Run all demonstrations
        demonstrateBasicFileOperations();
        demonstrateLineOperations();
        demonstrateBinaryOperations();
        demonstrateDirectoryOperations();
        demonstrateFileInfo();
        demonstratePathUtilities();
        demonstrateFileCopyMove();
        demonstrateFileSearch();
        demonstrateCurrentDirectory();

        // Cleanup
        cleanupExample();

        std::cout << "\n========================================\n";
        std::cout << "  All examples completed successfully!\n";
        std::cout << "========================================\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
}
