find_package(Union REQUIRED)

file(REAL_PATH "${QT6_INSTALL_PREFIX}" QT_PREFIX)
file(REAL_PATH "${Union_DIR}" UNION_LOCATION)

cmake_path(IS_PREFIX QT_PREFIX "${UNION_LOCATION}" NORMALIZE union_matches_qt)
if(NOT union_matches_qt)
    message(WARNING "Union at ${Union_DIR} is outside Qt prefix ${QT6_INSTALL_PREFIX}; "
                    "make sure both were built against the same Qt installation!"
    )
endif()

set(SOURCES
    cutecosmicunionplatform.cpp
)

qt_add_plugin(cutecosmicunionplatform
    CLASS_NAME CuteCosmicUnionPlatform
    ${SOURCES}
)

target_link_libraries(cutecosmicunionplatform PRIVATE cutecosmiccommon Union::Union)
target_compile_features(cutecosmicunionplatform PRIVATE cxx_std_20)

set_target_properties(cutecosmicunionplatform PROPERTIES
    PREFIX ""
    OUTPUT_NAME "union-platform-cosmic"
)

if(QT_INSTALL_PLUGINS)
    set_target_properties(cutecosmicunionplatform PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
        INSTALL_RPATH ${CUTECOSMIC_PRIVATE_LIBDIR}
    )

    install(TARGETS cutecosmicunionplatform DESTINATION ${QT_INSTALL_PLUGINS}/union)
endif()
