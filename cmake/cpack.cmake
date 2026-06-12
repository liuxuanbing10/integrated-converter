# CPack packaging configuration
# Included from root CMakeLists.txt after all targets are defined.
#
# Build a package with:
#   cmake --build build/release --target package
# or after configure:
#   cpack --config build/release/CPackConfig.cmake

set(CPACK_PACKAGE_NAME "IntegratedConverter")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Multimedia file converter — ffmpeg, Pandoc, ImageMagick frontend")
set(CPACK_PACKAGE_VENDOR "Integrated Converter Project")

# License and readme (optional — suppress warnings if missing)
if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
endif()

# Platform-specific archive / installer generators
if(WIN32)
    # ZIP works everywhere; NSIS is optional (requires NSIS installed)
    find_program(NSIS_MAKENSIS makensis)
    if(NSIS_MAKENSIS)
        set(CPACK_GENERATOR NSIS ZIP)
    else()
        set(CPACK_GENERATOR ZIP)
        message(STATUS "CPack: NSIS not found — using ZIP only")
    endif()
elseif(APPLE)
    set(CPACK_GENERATOR DragNDrop TGZ)
else()
    set(CPACK_GENERATOR TGZ DEB)
endif()

# ── Install rules ──────────────────────────────────────────────────
# The main executable
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE  DESTINATION .

    COMPONENT Runtime
)

# Test runner — only included in packages built from a test-enabled build
if(BUILD_TESTS)
    install(TARGETS test_runner
        RUNTIME DESTINATION bin
        COMPONENT Testing
    )
endif()

# Make sure CPack picks up our install rules
include(CPack)
