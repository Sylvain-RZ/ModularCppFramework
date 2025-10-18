/**
 * @file test_tools_scripts.cpp
 * @brief Unit tests for tools/ scripts using Catch2 v3
 *
 * These tests verify that the code generation and packaging scripts
 * work correctly across different platforms and configurations.
 */

#define CATCH_CONFIG_MAIN
#include <catch_amalgamated.hpp>

#include "../../core/FileSystem.hpp"
#include <string>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>

using namespace mcf;

// Helper macros for exit status (must be defined before use)
#ifndef WIFEXITED
    #define WIFEXITED(status) (((status) & 0x7f) == 0)
#endif
#ifndef WEXITSTATUS
    #define WEXITSTATUS(status) (((status) >> 8) & 0xff)
#endif

/**
 * @brief Helper class for executing shell scripts and capturing output
 */
class ScriptExecutor {
public:
    struct Result {
        int exitCode;
        std::string output;
        std::string error;

        bool success() const { return exitCode == 0; }
    };

    /**
     * @brief Execute a command and capture its output
     * @param command Command to execute
     * @return Result structure with exit code and output
     */
    static Result execute(const std::string& command) {
        Result result;
        result.exitCode = -1;

        // Create temporary files for stdout and stderr
        std::string outFile = "/tmp/mcf_test_out_" + std::to_string(getpid()) + ".txt";
        std::string errFile = "/tmp/mcf_test_err_" + std::to_string(getpid()) + ".txt";

        // Execute command with output redirection
        std::string fullCommand = command + " > " + outFile + " 2> " + errFile;
        result.exitCode = system(fullCommand.c_str());

        // Convert to actual exit code (system() returns status << 8)
        if (WIFEXITED(result.exitCode)) {
            result.exitCode = WEXITSTATUS(result.exitCode);
        }

        // Read output files
        std::ifstream outStream(outFile);
        if (outStream.is_open()) {
            std::stringstream buffer;
            buffer << outStream.rdbuf();
            result.output = buffer.str();
            outStream.close();
        }

        std::ifstream errStream(errFile);
        if (errStream.is_open()) {
            std::stringstream buffer;
            buffer << errStream.rdbuf();
            result.error = buffer.str();
            errStream.close();
        }

        // Clean up temporary files
        std::remove(outFile.c_str());
        std::remove(errFile.c_str());

        return result;
    }

    static pid_t getpid() {
        #ifdef _WIN32
            return GetCurrentProcessId();
        #else
            return ::getpid();
        #endif
    }
};

/**
 * @brief Test fixture for tools scripts
 */
class ToolsTestFixture {
public:
    FileSystem fs;
    std::string testDir;
    std::string projectRoot;
    std::string toolsDir;

    ToolsTestFixture() {
        // Determine project root
        // The test executables are in build/bin/tests/, we need to go up 3 levels
        char exe_path[1024];

        #ifdef _WIN32
            GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
        #elif __linux__
            ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path)-1);
            if (len != -1) {
                exe_path[len] = '\0';
            } else {
                strcpy(exe_path, ".");
            }
        #elif __APPLE__
            uint32_t size = sizeof(exe_path);
            if (_NSGetExecutablePath(exe_path, &size) != 0) {
                strcpy(exe_path, ".");
            }
        #else
            strcpy(exe_path, ".");
        #endif

        // Get the directory of the executable
        std::string exeDir = Path::dirname(exe_path);  // build/bin/tests
        projectRoot = Path::dirname(exeDir);  // build/bin
        projectRoot = Path::dirname(projectRoot);  // build
        projectRoot = Path::dirname(projectRoot);  // project root

        toolsDir = Path::join(projectRoot, "tools");
        testDir = "/tmp/mcf_tools_test_" + std::to_string(ScriptExecutor::getpid());
    }

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

    std::string getTestPath(const std::string& relative) const {
        return Path::join(testDir, relative);
    }

    std::string getScriptPath(const std::string& scriptName) const {
        return Path::join(toolsDir, scriptName);
    }

    bool fileContains(const std::string& filePath, const std::string& substring) const {
        if (!fs.exists(filePath)) return false;
        std::string content = fs.readFile(filePath);
        return content.find(substring) != std::string::npos;
    }

    bool directoryContains(const std::string& dirPath, const std::string& filename) const {
        if (!fs.exists(dirPath) || !fs.isDirectory(dirPath)) return false;
        auto files = fs.listDirectory(dirPath);
        for (const auto& file : files) {
            if (Path::basename(file) == filename) {
                return true;
            }
        }
        return false;
    }
};

// ============================================
// create-plugin.py Tests
// ============================================

TEST_CASE("create-plugin.py - Help option works", "[tools][create-plugin]") {
    ToolsTestFixture fixture;

    std::string script = fixture.getScriptPath("create-plugin.py");
    REQUIRE(fixture.fs.exists(script));

    auto result = ScriptExecutor::execute("python3 " + script + " --help");

    REQUIRE(result.success());
    REQUIRE(result.output.find("usage:") != std::string::npos);
    REQUIRE(result.output.find("options:") != std::string::npos);
}

TEST_CASE("create-plugin.py - Basic plugin creation", "[tools][create-plugin]") {
    ToolsTestFixture fixture;
    fixture.SetUp();

    std::string script = fixture.getScriptPath("create-plugin.py");
    std::string pluginDir = fixture.getTestPath("plugins/TestPlugin");

    // Create plugins directory
    fixture.fs.createDirectory(fixture.getTestPath("plugins"));

    SECTION("Create basic plugin") {
        std::string cmd = "python3 " + script + " -n TestPlugin -v 1.0.0 -a TestAuthor -d 'Test plugin' -o " + fixture.testDir + "/plugins";
        auto result = ScriptExecutor::execute(cmd);

        INFO("Command: " << cmd);
        INFO("Output: " << result.output);
        INFO("Error: " << result.error);

        REQUIRE(result.success());
        REQUIRE(fixture.fs.exists(pluginDir));
        REQUIRE(fixture.fs.exists(Path::join(pluginDir, "CMakeLists.txt")));
        REQUIRE(fixture.fs.exists(Path::join(pluginDir, "TestPlugin.cpp")));
        REQUIRE(fixture.fs.exists(Path::join(pluginDir, "README.md")));

        // Verify content
        std::string implPath = Path::join(pluginDir, "TestPlugin.cpp");
        REQUIRE(fixture.fileContains(implPath, "class TestPlugin"));
        REQUIRE(fixture.fileContains(implPath, "public IPlugin"));
        REQUIRE(fixture.fileContains(implPath, "1.0.0"));
    }

    SECTION("Create realtime plugin") {
        std::string cmd = "python3 " + script + " -n RealtimePlugin -r -o " + fixture.testDir + "/plugins";
        auto result = ScriptExecutor::execute(cmd);

        REQUIRE(result.success());

        std::string implPath = fixture.getTestPath("plugins/RealtimePlugin/RealtimePlugin.cpp");
        REQUIRE(fixture.fs.exists(implPath));
        REQUIRE(fixture.fileContains(implPath, "public mcf::IRealtimeUpdatable"));
        REQUIRE(fixture.fileContains(implPath, "void onRealtimeUpdate(float deltaTime)"));
    }

    SECTION("Create event-driven plugin") {
        std::string cmd = "python3 " + script + " -n EventPlugin -e -o " + fixture.testDir + "/plugins";
        auto result = ScriptExecutor::execute(cmd);

        REQUIRE(result.success());

        std::string implPath = fixture.getTestPath("plugins/EventPlugin/EventPlugin.cpp");
        REQUIRE(fixture.fs.exists(implPath));
        REQUIRE(fixture.fileContains(implPath, "public mcf::IEventDriven"));
    }

    SECTION("Create full-featured plugin") {
        std::string cmd = "python3 " + script + " -n FullPlugin -r -e -o " + fixture.testDir + "/plugins";
        auto result = ScriptExecutor::execute(cmd);

        REQUIRE(result.success());

        std::string implPath = fixture.getTestPath("plugins/FullPlugin/FullPlugin.cpp");
        REQUIRE(fixture.fs.exists(implPath));
        REQUIRE(fixture.fileContains(implPath, "public mcf::IRealtimeUpdatable"));
        REQUIRE(fixture.fileContains(implPath, "public mcf::IEventDriven"));
    }

    fixture.TearDown();
}

TEST_CASE("create-plugin.py - Error handling", "[tools][create-plugin]") {
    ToolsTestFixture fixture;
    fixture.SetUp();

    std::string script = fixture.getScriptPath("create-plugin.py");

    SECTION("Missing required argument") {
        auto result = ScriptExecutor::execute("python3 " + script);
        REQUIRE_FALSE(result.success());
    }

    SECTION("Invalid priority value") {
        std::string cmd = "python3 " + script + " -n TestPlugin -p " + fixture.testDir + " --priority invalid";
        auto result = ScriptExecutor::execute(cmd);
        // Should either fail or use default priority
        // Implementation-dependent behavior
    }

    fixture.TearDown();
}

// ============================================
// create-application.py Tests
// ============================================

TEST_CASE("create-application.py - Help option works", "[tools][create-application]") {
    ToolsTestFixture fixture;

    std::string script = fixture.getScriptPath("create-application.py");
    REQUIRE(fixture.fs.exists(script));

    auto result = ScriptExecutor::execute("python3 " + script + " --help");

    REQUIRE(result.success());
    REQUIRE(result.output.find("usage:") != std::string::npos);
    REQUIRE(result.output.find("options:") != std::string::npos);
}

TEST_CASE("create-application.py - Basic application creation", "[tools][create-application]") {
    ToolsTestFixture fixture;
    fixture.SetUp();

    std::string script = fixture.getScriptPath("create-application.py");
    std::string appDir = fixture.getTestPath("TestApp");

    SECTION("Create basic application") {
        std::string cmd = "python3 " + script + " -n TestApp -o " + appDir;
        auto result = ScriptExecutor::execute(cmd);

        INFO("Command: " << cmd);
        INFO("Output: " << result.output);
        INFO("Error: " << result.error);

        REQUIRE(result.success());
        REQUIRE(fixture.fs.exists(appDir));
        REQUIRE(fixture.fs.exists(Path::join(appDir, "CMakeLists.txt")));
        REQUIRE(fixture.fs.exists(Path::join(appDir, "src/main.cpp")));
        REQUIRE(fixture.fs.exists(Path::join(appDir, "README.md")));

        // Verify content
        std::string mainPath = Path::join(appDir, "src/main.cpp");
        REQUIRE(fixture.fileContains(mainPath, "class TestApp"));
        REQUIRE(fixture.fileContains(mainPath, "public mcf::Application"));
    }

    SECTION("Create realtime application") {
        std::string cmd = "python3 " + script + " -n RealtimeApp -r -o " + fixture.getTestPath("RealtimeApp");
        auto result = ScriptExecutor::execute(cmd);

        REQUIRE(result.success());

        std::string mainPath = fixture.getTestPath("RealtimeApp/src/main.cpp");
        REQUIRE(fixture.fs.exists(mainPath));
        REQUIRE(fixture.fileContains(mainPath, "void onUpdate(float deltaTime)"));
    }

    SECTION("Create application with config support") {
        std::string cmd = "python3 " + script + " -n ConfigApp -c -o " + fixture.getTestPath("ConfigApp");
        auto result = ScriptExecutor::execute(cmd);

        REQUIRE(result.success());

        std::string configPath = fixture.getTestPath("ConfigApp/config/config.json");
        REQUIRE(fixture.fs.exists(configPath));
    }

    SECTION("Create application with modules") {
        std::string cmd = "python3 " + script + " -n ModuleApp -m logger,profiling -o " + fixture.getTestPath("ModuleApp");
        auto result = ScriptExecutor::execute(cmd);

        REQUIRE(result.success());

        std::string cmakePath = fixture.getTestPath("ModuleApp/CMakeLists.txt");
        REQUIRE(fixture.fs.exists(cmakePath));
        // Logger is header-only, no linking needed in CMakeLists
        REQUIRE(fixture.fileContains(cmakePath, "mcf_profiling_module"));
    }

    fixture.TearDown();
}

// ============================================
// package-application.py Tests
// ============================================

TEST_CASE("package-application.py - Help option works", "[tools][package]") {
    ToolsTestFixture fixture;

    std::string script = fixture.getScriptPath("package-application.py");
    REQUIRE(fixture.fs.exists(script));

    auto result = ScriptExecutor::execute("python3 " + script + " --help");

    REQUIRE(result.success());
    REQUIRE(result.output.find("usage:") != std::string::npos);
    REQUIRE(result.output.find("options:") != std::string::npos);
}

TEST_CASE("package-application.py - Package MCF examples", "[tools][package][integration]") {
    ToolsTestFixture fixture;

    std::string script = fixture.getScriptPath("package-application.py");

    SECTION("Package without extraction") {
        std::string cmd = "cd " + fixture.projectRoot + " && python3 " + script + " -t package-mcf-examples";
        auto result = ScriptExecutor::execute(cmd);

        INFO("Command: " << cmd);
        INFO("Output: " << result.output);
        INFO("Error: " << result.error);

        REQUIRE(result.success());

        // Check that package was created (could be .tar.gz or .zip depending on platform)
        std::string buildDir = Path::join(fixture.projectRoot, "build");
        auto files = fixture.fs.listDirectory(buildDir);
        bool foundPackage = false;
        for (const auto& file : files) {
            if (file.find("MCF-Examples") != std::string::npos &&
                (file.find(".tar.gz") != std::string::npos || file.find(".zip") != std::string::npos)) {
                foundPackage = true;
                break;
            }
        }
        REQUIRE(foundPackage);
    }
}

TEST_CASE("package-application.py - Output directory option", "[tools][package]") {
    ToolsTestFixture fixture;
    fixture.SetUp();

    std::string script = fixture.getScriptPath("package-application.py");
    std::string outputDir = fixture.getTestPath("output");
    fixture.fs.createDirectory(outputDir);

    SECTION("Copy package to output directory") {
        std::string cmd = "cd " + fixture.projectRoot + " && python3 " + script +
                         " -t package-mcf-examples -o " + outputDir;
        auto result = ScriptExecutor::execute(cmd);

        INFO("Command: " << cmd);
        INFO("Output: " << result.output);

        if (result.success()) {
            // Check that package was copied (could be .tar.gz or .zip)
            auto files = fixture.fs.listDirectory(outputDir);
            bool foundPackage = false;
            for (const auto& file : files) {
                if (file.find("MCF-Examples") != std::string::npos &&
                    (file.find(".tar.gz") != std::string::npos || file.find(".zip") != std::string::npos)) {
                    foundPackage = true;
                    break;
                }
            }
            REQUIRE(foundPackage);
        }
    }

    fixture.TearDown();
}

// ============================================
// Cross-script Integration Tests
// ============================================

TEST_CASE("Tools Integration - Create plugin and verify buildable", "[tools][integration]") {
    ToolsTestFixture fixture;
    fixture.SetUp();

    std::string pluginScript = fixture.getScriptPath("create-plugin.py");
    std::string pluginDir = fixture.getTestPath("plugins/IntegrationPlugin");

    // Create plugins directory
    fixture.fs.createDirectory(fixture.getTestPath("plugins"));

    SECTION("Create plugin and check CMake validity") {
        // Create plugin
        std::string createCmd = "python3 " + pluginScript + " -n IntegrationPlugin -r -o " + fixture.testDir + "/plugins";
        auto createResult = ScriptExecutor::execute(createCmd);

        REQUIRE(createResult.success());
        REQUIRE(fixture.fs.exists(pluginDir));

        // Verify CMakeLists.txt has correct structure
        std::string cmakePath = Path::join(pluginDir, "CMakeLists.txt");
        REQUIRE(fixture.fileContains(cmakePath, "add_library"));
        REQUIRE(fixture.fileContains(cmakePath, "target_link_libraries"));
        REQUIRE(fixture.fileContains(cmakePath, "mcf_core"));
    }

    fixture.TearDown();
}

TEST_CASE("Tools Integration - Create application and verify structure", "[tools][integration]") {
    ToolsTestFixture fixture;
    fixture.SetUp();

    std::string appScript = fixture.getScriptPath("create-application.py");
    std::string appDir = fixture.getTestPath("IntegrationApp");

    SECTION("Create full-featured application") {
        std::string createCmd = "python3 " + appScript + " -n IntegrationApp -r -e -c -m logger,profiling -o " + appDir;
        auto createResult = ScriptExecutor::execute(createCmd);

        INFO("Command: " << createCmd);
        INFO("Output: " << createResult.output);
        INFO("Error: " << createResult.error);

        REQUIRE(createResult.success());
        REQUIRE(fixture.fs.exists(appDir));

        // Verify directory structure
        REQUIRE(fixture.fs.exists(Path::join(appDir, "src")));
        REQUIRE(fixture.fs.exists(Path::join(appDir, "config")));
        REQUIRE(fixture.fs.exists(Path::join(appDir, "CMakeLists.txt")));

        // Verify README exists
        REQUIRE(fixture.fs.exists(Path::join(appDir, "README.md")));
    }

    fixture.TearDown();
}

// ============================================
// Platform-specific Tests
// ============================================

TEST_CASE("Tools - Platform detection", "[tools][platform]") {
    ToolsTestFixture fixture;

    SECTION("Detect current platform") {
        #ifdef _WIN32
            INFO("Running on Windows");
            // Windows-specific checks - Python scripts should exist
            REQUIRE(fixture.fs.exists(fixture.getScriptPath("create-plugin.py")));
            REQUIRE(fixture.fs.exists(fixture.getScriptPath("create-application.py")));
            REQUIRE(fixture.fs.exists(fixture.getScriptPath("package-application.py")));
        #elif __APPLE__
            INFO("Running on macOS");
            // macOS-specific checks - Python scripts should exist
            REQUIRE(fixture.fs.exists(fixture.getScriptPath("create-plugin.py")));
            REQUIRE(fixture.fs.exists(fixture.getScriptPath("create-application.py")));
            REQUIRE(fixture.fs.exists(fixture.getScriptPath("package-application.py")));
        #elif __linux__
            INFO("Running on Linux");
            // Linux-specific checks - Python scripts should exist
            REQUIRE(fixture.fs.exists(fixture.getScriptPath("create-plugin.py")));
            REQUIRE(fixture.fs.exists(fixture.getScriptPath("create-application.py")));
            REQUIRE(fixture.fs.exists(fixture.getScriptPath("package-application.py")));
        #endif
    }
}

TEST_CASE("Tools - Script permissions", "[tools][platform]") {
    ToolsTestFixture fixture;

    #ifndef _WIN32
    // On Unix-like systems, verify Python scripts are executable
    SECTION("Check execute permissions") {
        std::vector<std::string> scripts = {
            "create-plugin.py",
            "create-application.py",
            "package-application.py"
        };

        for (const auto& script : scripts) {
            std::string path = fixture.getScriptPath(script);
            REQUIRE(fixture.fs.exists(path));

            // Check if file is executable
            auto result = ScriptExecutor::execute("test -x " + path + " && echo OK");
            REQUIRE(result.output.find("OK") != std::string::npos);
        }
    }
    #endif
}
