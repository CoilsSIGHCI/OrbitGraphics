cmake_minimum_required(VERSION 3.10.0)
project(orbit VERSION 0.1.0 LANGUAGES C CXX)

add_executable(orbit
    main.c
    cocoa_platform.mm
)
set_target_properties(orbit PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)
set_source_files_properties(cocoa_platform.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")

#           DOWNLOAD ALL THE SUBMODULES
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()


# make sure they exist
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/Library/bx")
    message(FATAL_ERROR "The bx submodules was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/Library/bimg")
    message(FATAL_ERROR "The bimg submodules was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()

if(NOT EXISTS "${PROJECT_SOURCE_DIR}/Library/bgfx")
    message(FATAL_ERROR "The bgfx submodules was not downloaded! GIT_SUBMODULE was turned off or failed. Please update submodules and try again.")
endif()


if(APPLE)
    #include dirs
    target_include_directories(orbit
        PRIVATE
        Library/bgfx/include
        Library/bx/include
        Library/bimg/include
    )

    target_include_directories(orbit
        PRIVATE
        Library/bgfx/3rdparty/
        Library/bgfx/examples/common
    )

    target_include_directories(orbit
        PRIVATE
        /opt/homebrew/include
    )

    #LINK LIBS
    target_link_libraries(orbit
        ${CMAKE_DL_LIBS}
        pthread
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreFoundation"
        "-framework CoreVideo"
        "-framework Metal"
    )
    set_target_properties(orbit PROPERTIES LINKER_LANGUAGE CXX)
    target_link_libraries(orbit
        c++
        "-framework QuartzCore"
    )

    #BGFX LINKS LOGIC
    target_link_directories(orbit PRIVATE ./Library/bgfx/.build/osx-arm64/bin/)

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        if(NOT EXISTS "${PROJECT_SOURCE_DIR}/Library/bgfx/.build/osx-arm64/bin/libbgfxDebug.a")
            message(FATAL_ERROR "The bgfx submodule was not built as debug! Go to bgfx folder and run: make darwin-debug")
        endif()
        target_link_libraries(orbit
            bgfxDebug
            bimgDebug
            bxDebug
            example-glueDebug
            example-commonDebug
            bimg_decodeDebug
        )
    elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        if(NOT EXISTS "${PROJECT_SOURCE_DIR}/Library/bgfx/.build/osx-arm64/bin/libbgfxRelease.a")
            message(FATAL_ERROR "The bgfx submodule was not built as release! Go to bgfx folder and run: make linux-release64")
        endif()
        target_link_libraries(orbit
            example-glueRelease
            example-commonRelease
            bimg_decodeRelease
            bgfxRelease
            bimgRelease
            bxRelease
        )
    endif()
else()
    message(FATAL_ERROR "This is for UNIX-like only right now, what's on your Hard Drive?")
endif()

file(COPY ${CMAKE_SOURCE_DIR}/Library/bgfx/examples/runtime/ DESTINATION ${CMAKE_BINARY_DIR})
