#include "../core/Application.hpp"
#include "../core/ConfigurationManager.hpp"
#include "../core/JsonParser.hpp"
#include "../core/JsonValue.hpp"

#include <iostream>

using namespace mcf;

/**
 * @brief Test application demonstrating configuration system
 */
class ConfigTestApp : public Application {
public:
    ConfigTestApp() : Application() {
        // Don't load external config file for testing
        m_config.autoLoadPlugins = false;  // Disable plugin loading for this test
        m_config.autoInitPlugins = false;
    }

    void runTests() {
        std::cout << "=== Configuration System Test ===" << std::endl;
        std::cout << std::endl;

        auto* config = getConfigurationManager();

        // Create test configuration programmatically
        JsonObject root;

        // Application config
        JsonObject appConfig;
        appConfig["name"] = JsonValue("TestApp");
        appConfig["version"] = JsonValue("1.0.0");
        appConfig["targetFPS"] = JsonValue(60);
        appConfig["vsync"] = JsonValue(true);
        root["application"] = JsonValue(appConfig);

        // Logging config
        JsonObject loggingConfig;
        loggingConfig["enabled"] = JsonValue(true);
        loggingConfig["level"] = JsonValue("info");
        loggingConfig["file"] = JsonValue("app.log");
        root["logging"] = JsonValue(loggingConfig);

        // Rendering config
        JsonObject renderingConfig;
        JsonObject windowConfig;
        windowConfig["width"] = JsonValue(1280);
        windowConfig["height"] = JsonValue(720);
        windowConfig["title"] = JsonValue("Test Application");
        windowConfig["fullscreen"] = JsonValue(false);
        JsonObject graphicsConfig;
        graphicsConfig["msaaSamples"] = JsonValue(4);
        renderingConfig["window"] = JsonValue(windowConfig);
        renderingConfig["graphics"] = JsonValue(graphicsConfig);
        root["rendering"] = JsonValue(renderingConfig);

        // Plugin config
        JsonObject pluginsConfig;
        JsonObject examplePluginConfig;
        examplePluginConfig["enabled"] = JsonValue(true);
        examplePluginConfig["priority"] = JsonValue(100);
        JsonObject pluginSettings;
        pluginSettings["updateInterval"] = JsonValue(0.016f);
        pluginSettings["debugMode"] = JsonValue(true);
        examplePluginConfig["settings"] = JsonValue(pluginSettings);
        pluginsConfig["ExamplePlugin"] = JsonValue(examplePluginConfig);
        root["plugins"] = JsonValue(pluginsConfig);

        // Load the test configuration
        config->set("", JsonValue(root));

        // Test 1: Read application config
        std::cout << "Application Configuration:" << std::endl;
        std::cout << "  Name: " << config->getString("application.name", "N/A") << std::endl;
        std::cout << "  Version: " << config->getString("application.version", "N/A") << std::endl;
        std::cout << "  Target FPS: " << config->getInt("application.targetFPS", 0) << std::endl;
        std::cout << "  VSync: " << (config->getBool("application.vsync", false) ? "enabled" : "disabled") << std::endl;
        std::cout << std::endl;

        // Test 2: Read logging config
        std::cout << "Logging Configuration:" << std::endl;
        std::cout << "  Enabled: " << (config->getBool("logging.enabled", false) ? "yes" : "no") << std::endl;
        std::cout << "  Level: " << config->getString("logging.level", "N/A") << std::endl;
        std::cout << "  File: " << config->getString("logging.file", "N/A") << std::endl;
        std::cout << std::endl;

        // Test 3: Read rendering config
        std::cout << "Rendering Configuration:" << std::endl;
        std::cout << "  Window Size: " << config->getInt("rendering.window.width", 0)
                  << "x" << config->getInt("rendering.window.height", 0) << std::endl;
        std::cout << "  Title: " << config->getString("rendering.window.title", "N/A") << std::endl;
        std::cout << "  Fullscreen: " << (config->getBool("rendering.window.fullscreen", false) ? "yes" : "no") << std::endl;
        std::cout << "  MSAA Samples: " << config->getInt("rendering.graphics.msaaSamples", 0) << std::endl;
        std::cout << std::endl;

        // Test 4: Read plugin config
        std::cout << "Plugin Configuration:" << std::endl;
        auto pluginConfig = config->get("plugins.ExamplePlugin");
        if (pluginConfig.isObject()) {
            std::cout << "  ExamplePlugin:" << std::endl;
            std::cout << "    Enabled: " << pluginConfig["enabled"].asBool(false) << std::endl;
            std::cout << "    Priority: " << pluginConfig["priority"].asInt(0) << std::endl;

            auto settings = pluginConfig["settings"];
            if (settings.isObject()) {
                std::cout << "    Settings:" << std::endl;
                std::cout << "      Update Interval: " << settings["updateInterval"].asFloat(0.0) << std::endl;
                std::cout << "      Debug Mode: " << (settings["debugMode"].asBool(false) ? "yes" : "no") << std::endl;
            }
        }
        std::cout << std::endl;

        // Test 5: Modify configuration
        std::cout << "Modifying configuration..." << std::endl;
        config->set("application.targetFPS", JsonValue(120));
        config->set("logging.level", JsonValue("debug"));
        config->set("custom.newValue", JsonValue(42));
        std::cout << "  New FPS: " << config->getInt("application.targetFPS", 0) << std::endl;
        std::cout << "  New Log Level: " << config->getString("logging.level", "N/A") << std::endl;
        std::cout << "  Custom Value: " << config->getInt("custom.newValue", 0) << std::endl;
        std::cout << std::endl;

        // Test 6: Watch for changes
        std::cout << "Setting up configuration watchers..." << std::endl;
        config->watch("application.targetFPS", [](const std::string& key, const JsonValue& value) {
            std::cout << "  [WATCHER] " << key << " changed to: " << value.asInt(0) << std::endl;
        });

        // Trigger watch callback
        config->set("application.targetFPS", JsonValue(144));
        std::cout << std::endl;

        // Test 7: Save configuration
        std::cout << "Saving configuration to test_output.json..." << std::endl;
        if (config->save("../test_output.json")) {
            std::cout << "  Configuration saved successfully!" << std::endl;
        } else {
            std::cout << "  Failed to save configuration!" << std::endl;
        }
        std::cout << std::endl;

        // Test 8: Array and object operations
        std::cout << "Testing arrays and objects..." << std::endl;
        JsonArray testArray;
        testArray.push_back(JsonValue(1));
        testArray.push_back(JsonValue(2));
        testArray.push_back(JsonValue(3));
        config->set("test.array", JsonValue(testArray));

        auto array = config->getArray("test.array");
        std::cout << "  Array size: " << array.size() << std::endl;
        std::cout << "  Array values: ";
        for (const auto& val : array) {
            std::cout << val.asInt(0) << " ";
        }
        std::cout << std::endl;
        std::cout << std::endl;

        // Test 9: Remove configuration
        std::cout << "Testing configuration removal..." << std::endl;
        std::cout << "  Has custom.newValue: " << (config->has("custom.newValue") ? "yes" : "no") << std::endl;
        config->remove("custom.newValue");
        std::cout << "  After removal: " << (config->has("custom.newValue") ? "yes" : "no") << std::endl;
        std::cout << std::endl;

        // Test 10: JSON parsing from string
        std::cout << "Testing JSON parsing from string..." << std::endl;
        std::string jsonStr = R"({
            "test": {
                "string": "hello",
                "number": 42,
                "bool": true,
                "array": [1, 2, 3],
                "nested": {
                    "value": "world"
                }
            }
        })";

        try {
            JsonValue parsed = JsonParser::parse(jsonStr);
            std::cout << "  Parsed successfully!" << std::endl;
            std::cout << "  test.string: " << parsed["test"]["string"].asString() << std::endl;
            std::cout << "  test.number: " << parsed["test"]["number"].asInt() << std::endl;
            std::cout << "  test.bool: " << parsed["test"]["bool"].asBool() << std::endl;
            std::cout << "  test.nested.value: " << parsed["test"]["nested"]["value"].asString() << std::endl;
        } catch (const std::exception& e) {
            std::cout << "  Parse error: " << e.what() << std::endl;
        }
        std::cout << std::endl;

        std::cout << "=== Configuration System Test Complete ===" << std::endl;
        std::cout << std::endl;
    }
};

int main() {
    try {
        ConfigTestApp app;

        // Initialize the application
        if (!app.initialize()) {
            std::cerr << "Failed to initialize application!" << std::endl;
            return 1;
        }

        // Run configuration tests
        app.runTests();

        app.shutdown();
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
