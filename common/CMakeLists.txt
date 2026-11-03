find_package(Qt6 REQUIRED COMPONENTS Gui)

if(Qt6_VERSION VERSION_GREATER_EQUAL 6.9.0)
    find_package(Qt6 COMPONENTS GuiPrivate)
endif()

set(SOURCES
    cutecosmiccolormanager.cpp
    cutecosmiciconengine.cpp
)

add_library(cutecosmiccommon SHARED ${SOURCES})

target_compile_options(cutecosmiccommon PUBLIC -Wall -Wextra -pedantic)
target_compile_definitions(cutecosmiccommon PUBLIC QT_NO_CAST_FROM_ASCII QT_NO_KEYWORDS)

target_include_directories(cutecosmiccommon
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC $<TARGET_PROPERTY:bindings,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(cutecosmiccommon PRIVATE bindings PUBLIC Qt::GuiPrivate)

target_link_options(cutecosmiccommon PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/common.map")

set_target_properties(cutecosmiccommon PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

if(CUTECOSMIC_PRIVATE_LIBDIR)
    install(TARGETS cutecosmiccommon DESTINATION ${CUTECOSMIC_PRIVATE_LIBDIR})
endif()
