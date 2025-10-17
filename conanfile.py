from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy
import os


class ModularCppFrameworkConan(ConanFile):
    name = "modular-cpp-framework"
    version = "1.0.0"
    license = "MIT"
    author = "ModularCppFramework Contributors"
    url = "https://github.com/Sylvain-RZ/ModularCppFramework"
    description = "Modern C++17 framework with dynamic plugin system, hot-reload, and modular architecture"
    topics = ("c++", "framework", "plugins", "modular", "hot-reload", "dependency-injection")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "build_tests": [True, False],
        "build_examples": [True, False],
        "with_logger_module": [True, False],
        "with_networking_module": [True, False],
        "with_profiling_module": [True, False],
        "with_realtime_module": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "build_tests": False,
        "build_examples": False,
        "with_logger_module": True,
        "with_networking_module": True,
        "with_profiling_module": True,
        "with_realtime_module": True,
    }
    exports_sources = "core/*", "modules/*", "CMakeLists.txt", "LICENSE", "README.md", "docs/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        # Core is header-only, but modules can be shared or static
        if self.options.shared:
            self.options.rm_safe("fPIC")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = self.options.build_tests
        tc.variables["BUILD_EXAMPLES"] = self.options.build_examples
        tc.variables["BUILD_LOGGER_MODULE"] = self.options.with_logger_module
        tc.variables["BUILD_NETWORKING_MODULE"] = self.options.with_networking_module
        tc.variables["BUILD_PROFILING_MODULE"] = self.options.with_profiling_module
        tc.variables["BUILD_REALTIME_MODULE"] = self.options.with_realtime_module
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        copy(self, "*.hpp", src=os.path.join(self.source_folder, "core"), dst=os.path.join(self.package_folder, "include", "mcf", "core"))
        copy(self, "*.hpp", src=os.path.join(self.source_folder, "modules"), dst=os.path.join(self.package_folder, "include", "mcf", "modules"))

        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        # Core is header-only
        self.cpp_info.components["core"].includedirs = ["include"]
        self.cpp_info.components["core"].bindirs = []
        self.cpp_info.components["core"].libdirs = []

        # Platform-specific system libraries for core
        if self.settings.os == "Linux":
            self.cpp_info.components["core"].system_libs = ["dl", "pthread"]
        elif self.settings.os == "Windows":
            self.cpp_info.components["core"].system_libs = ["ws2_32"]

        # Modules
        if self.options.with_logger_module:
            self.cpp_info.components["logger"].libs = ["mcf_logger_module"]
            self.cpp_info.components["logger"].requires = ["core"]

        if self.options.with_networking_module:
            self.cpp_info.components["networking"].libs = ["mcf_networking_module"]
            self.cpp_info.components["networking"].requires = ["core"]

        if self.options.with_profiling_module:
            self.cpp_info.components["profiling"].libs = ["mcf_profiling_module"]
            self.cpp_info.components["profiling"].requires = ["core"]

        if self.options.with_realtime_module:
            self.cpp_info.components["realtime"].libs = ["mcf_realtime_module"]
            self.cpp_info.components["realtime"].requires = ["core"]

        # Set C++ standard
        self.cpp_info.set_property("cmake_target_name", "mcf::mcf")
