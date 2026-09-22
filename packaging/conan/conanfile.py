import os

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy

required_conan_version = ">=2.0"


class PlusWebConan(ConanFile):
    name = "plusweb"
    version = "0.1.1"
    license = "MIT"
    homepage = "https://github.com/Amsozzer1/PlusWeb"
    url = "https://github.com/Amsozzer1/PlusWeb"
    description = (
        "Express-style HTTP framework for C++17 built on libuv and llhttp, "
        "with a segment-trie router and a middleware chain"
    )
    topics = ("http", "web-framework", "libuv", "llhttp", "rest-api")

    package_type = "static-library"
    settings = "os", "arch", "compiler", "build_type"
    options = {"fPIC": [True, False]}
    default_options = {"fPIC": True}

    def export_sources(self):
        copy(self, "*", src=os.path.join(self.recipe_folder, "..", ".."), dst=self.export_sources_folder)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def validate(self):
        check_min_cppstd(self, 17)
        if self.settings.os == "Windows":
            raise ConanInvalidConfiguration("PlusWeb has not been built or tested on Windows yet")

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        # nlohmann/json appears in the public headers, so consumers need it too.
        self.requires("nlohmann_json/3.12.0", transitive_headers=True)
        self.requires("libuv/1.51.0")
        self.requires("llhttp/9.2.1")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["PLUSWEB_BUILD_TESTS"] = False
        tc.cache_variables["PLUSWEB_BUILD_EXAMPLES"] = False
        tc.generate()
        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        CMake(self).install()

    def package_info(self):
        self.cpp_info.libs = ["PlusWeb"]
        self.cpp_info.set_property("cmake_file_name", "PlusWeb")
        self.cpp_info.set_property("cmake_target_name", "PlusWeb::PlusWeb")
        if self.settings.os in ("Linux", "FreeBSD"):
            self.cpp_info.system_libs.append("pthread")
