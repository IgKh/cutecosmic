find_package(Corrosion QUIET)
if(NOT Corrosion_FOUND)
    include(FetchContent)

    FetchContent_Declare(
        Corrosion
        GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
        GIT_TAG v0.5.2
    )

    FetchContent_MakeAvailable(Corrosion)
endif()

corrosion_import_crate(
    MANIFEST_PATH Cargo.toml
)

corrosion_set_env_vars(bindings
    BINDINGS_HEADER_PATH=${CMAKE_CURRENT_BINARY_DIR}/bindings.h
)

target_include_directories(bindings INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
