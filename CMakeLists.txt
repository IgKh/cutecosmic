cmake_minimum_required(VERSION 3.22)
project(cutecosmic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# That's the whole point...
set(QT_NO_PRIVATE_MODULE_WARNING ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(bindings)
add_subdirectory(platformtheme)
