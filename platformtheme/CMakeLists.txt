find_package(Qt6 REQUIRED COMPONENTS Gui DBus)

if(Qt6_VERSION VERSION_GREATER_EQUAL 6.9.0)
    find_package(Qt6 COMPONENTS GuiPrivate)
endif()

set(SOURCES
    cutecosmiccolormanager.cpp
    cutecosmicfiledialog.cpp
    cutecosmictheme.cpp
    cutecosmicwatcher.cpp
    main.cpp
)

qt_add_plugin(cutecosmictheme
    CLASS_NAME CuteCosmicPlatformThemePlugin
    ${SOURCES}
)

target_compile_options(cutecosmictheme PRIVATE -Wall -Wextra -pedantic)
target_compile_definitions(cutecosmictheme PRIVATE QT_NO_CAST_FROM_ASCII QT_NO_KEYWORDS)

target_link_libraries(cutecosmictheme PRIVATE Qt::GuiPrivate Qt::DBus bindings)

# Find out where to install the plugin
find_package(Qt6 COMPONENTS CoreTools QUIET CONFIG)

if(NOT TARGET Qt6::qtpaths)
    message(SEND_ERROR "Could not find qtpaths tool, can't install the plugin!")
else()
    get_target_property(qtpaths_executable Qt6::qtpaths LOCATION)
    execute_process(
        COMMAND ${qtpaths_executable} -qt-query QT_INSTALL_PLUGINS
        OUTPUT_VARIABLE QT_INSTALL_PLUGINS
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    message(STATUS "Installing plugin to ${QT_INSTALL_PLUGINS}")
    install(TARGETS cutecosmictheme DESTINATION ${QT_INSTALL_PLUGINS}/platformthemes)
endif()
