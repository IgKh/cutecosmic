find_package(Qt6 REQUIRED COMPONENTS Gui QuickControls2 DBus)

if(Qt6_VERSION VERSION_GREATER_EQUAL 6.9.0)
    find_package(Qt6 COMPONENTS GuiPrivate)
endif()

set(SOURCES
    cutecosmicfiledialog.cpp
    cutecosmictheme.cpp
    cutecosmicwatcher.cpp
    main.cpp
)

qt_add_plugin(cutecosmictheme
    CLASS_NAME CuteCosmicPlatformThemePlugin
    ${SOURCES}
)

target_link_libraries(cutecosmictheme PRIVATE cutecosmiccommon Qt::QuickControls2 Qt::DBus)

if(QT_INSTALL_PLUGINS)
    set_target_properties(cutecosmictheme PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
        INSTALL_RPATH ${CUTECOSMIC_PRIVATE_LIBDIR}
    )

    install(TARGETS cutecosmictheme DESTINATION ${QT_INSTALL_PLUGINS}/platformthemes)
endif()
