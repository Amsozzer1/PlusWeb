vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Amsozzer1/PlusWeb
    REF "v${VERSION}"
    SHA512 0
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DPLUSWEB_BUILD_TESTS=OFF
        -DPLUSWEB_BUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME PlusWeb CONFIG_PATH lib/cmake/PlusWeb)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
