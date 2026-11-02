cmake_minimum_required(VERSION 3.22)
project(cutecosmic VERSION 0.3.0 LANGUAGES CXX)

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

# Find out installation locations
find_package(Qt6 COMPONENTS CoreTools QUIET CONFIG)

if(NOT TARGET Qt6::qtpaths)
    message(SEND_ERROR "Could not find qtpaths tool, plugins won't be installed!")
else()
    get_target_property(qtpaths_executable Qt6::qtpaths LOCATION)

    execute_process(
        COMMAND ${qtpaths_executable} -qt-query QT_INSTALL_LIBS
        OUTPUT_VARIABLE QT_INSTALL_LIBS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "Installing libraries to ${QT_INSTALL_LIBS}")

    execute_process(
        COMMAND ${qtpaths_executable} -qt-query QT_INSTALL_PLUGINS
        OUTPUT_VARIABLE QT_INSTALL_PLUGINS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "Installing plugins to ${QT_INSTALL_PLUGINS}")

    set(CUTECOSMIC_PRIVATE_LIBDIR "${QT_INSTALL_LIBS}/cutecosmic")
endif()

add_subdirectory(bindings)
add_subdirectory(common)
add_subdirectory(platformtheme)
