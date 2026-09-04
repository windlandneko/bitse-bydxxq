cmake_minimum_required(VERSION 3.16)
project(bydxxq LANGUAGES CXX)

# Qt 6 推荐使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(link_compile_commands ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        "${CMAKE_BINARY_DIR}/compile_commands.json"
        "${CMAKE_SOURCE_DIR}/compile_commands.json"
    COMMENT "Syncing compile_commands.json to root for clangd"
)

add_custom_target(format
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/format-cpp.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Formatting all C/C++ sources"
)

add_custom_target(format-check
    COMMAND "${CMAKE_SOURCE_DIR}/scripts/format-cpp.sh" --check
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Checking C/C++ source formatting"
)

# manage multi projects
# add_subdirectory()
