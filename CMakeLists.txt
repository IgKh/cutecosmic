cmake_minimum_required(VERSION 3.22)
project(cutecosmic LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# That's the whole point...
set(QT_NO_PRIVATE_MODULE_WARNING ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup(REQUIRES 6.8)

# In case something lower than 6.5.0 is used, the qt_standard_project_setup call
# won't detect it.
if(Qt6_VERSION VERSION_LESS 6.5.0)
    message(FATAL_ERROR "Qt6 version is too low! you have ${Qt6_VERSION}, at least 6.8.0 is required")
endif()

add_subdirectory(bindings)
add_subdirectory(platformtheme)
