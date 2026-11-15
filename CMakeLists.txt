cmake_minimum_required(VERSION 3.16)

project(siv3d_app_template LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(SIV3D_BUILD_FUNCTIONAL_TEST "Build the functional test application" ON)
if(EMSCRIPTEN)
    set(DEFAULT_BUILD_UNIT_TEST OFF)
else()
    set(DEFAULT_BUILD_UNIT_TEST ON)
endif()
option(SIV3D_BUILD_UNIT_TEST "Build GoogleTest unit tests" ${DEFAULT_BUILD_UNIT_TEST})
set(ACTIVE_SOURCE "" CACHE FILEPATH "Source file selected in VS Code")

if(EMSCRIPTEN)
    set(SIV3D_WEB_TEMPLATE "" CACHE PATH "Path to the OpenSiv3D for Web VS Code template")
    set(WEB_SDK "${SIV3D_WEB_TEMPLATE}/OpenSiv3D")
    if(NOT EXISTS "${WEB_SDK}/lib/libSiv3D.a" OR NOT EXISTS "${SIV3D_WEB_TEMPLATE}/resources/engine")
        message(FATAL_ERROR "Set SIV3D_WEB_TEMPLATE to the OpenSiv3DForWeb-VSCode template directory.")
    endif()
    set(SIV3D_WEB_OPENCV_BUILD "" CACHE PATH "Path to OpenCV 4.5.1 built for WebAssembly")
    set(SIV3D_WEB_FREETYPE_BUILD "" CACHE PATH "Path to FreeType 2.10.4 built for WebAssembly")
    if(NOT EXISTS "${SIV3D_WEB_OPENCV_BUILD}/lib/libopencv_core.a")
        message(FATAL_ERROR "Set SIV3D_WEB_OPENCV_BUILD to the WebAssembly OpenCV build directory.")
    endif()
    if(NOT EXISTS "${SIV3D_WEB_FREETYPE_BUILD}/libfreetype.a")
        message(FATAL_ERROR "Set SIV3D_WEB_FREETYPE_BUILD to the WebAssembly FreeType build directory.")
    endif()
    add_library(Siv3D STATIC IMPORTED)
    set_target_properties(Siv3D PROPERTIES
        IMPORTED_LOCATION "${WEB_SDK}/lib/libSiv3D.a"
        INTERFACE_INCLUDE_DIRECTORIES "${WEB_SDK}/include/Siv3D;${WEB_SDK}/include/Siv3D/ThirdParty")
    target_compile_definitions(Siv3D INTERFACE _XM_NO_INTRINSICS_)
    target_link_libraries(Siv3D INTERFACE
        "${SIV3D_WEB_FREETYPE_BUILD}/libfreetype.a"
        "${WEB_SDK}/lib/Siv3D/giflib/libgif.a"
        "${WEB_SDK}/lib/Siv3D/harfbuzz/libharfbuzz.a"
        "${SIV3D_WEB_OPENCV_BUILD}/lib/libopencv_imgproc.a"
        "${SIV3D_WEB_OPENCV_BUILD}/lib/libopencv_core.a"
        "${WEB_SDK}/lib/Siv3D/turbojpeg/libturbojpeg.a"
        "${WEB_SDK}/lib/Siv3D/tiff/libtiff.a"
        "${WEB_SDK}/lib/Siv3D/opus/libopus.a"
        "${WEB_SDK}/lib/Siv3D/opus/libopusfile.a"
        "${WEB_SDK}/lib/Siv3D/png/libpng.a"
        "${WEB_SDK}/lib/Siv3D/SDL2/libSDL2.a"
        "${WEB_SDK}/lib/Siv3D/webp/libwebp.a"
        "${WEB_SDK}/lib/Siv3D/zlib/libz.a")
else()
    find_package(OpenCV REQUIRED COMPONENTS core aruco)
    set(SIV3D_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/OpenSiv3D/Linux external/OpenSiv3D-build EXCLUDE_FROM_ALL)
    target_compile_options(Siv3D PRIVATE -msse4.2)
endif()

add_subdirectory(src)

if(SIV3D_BUILD_UNIT_TEST AND EMSCRIPTEN)
    message(FATAL_ERROR "Unit tests require a native build.")
elseif(SIV3D_BUILD_UNIT_TEST)
    add_subdirectory(external/googletest EXCLUDE_FROM_ALL)
endif()

if(SIV3D_BUILD_FUNCTIONAL_TEST)
    enable_testing()
    add_subdirectory(functional_test)
endif()

if(SIV3D_BUILD_UNIT_TEST)
    enable_testing()
    add_subdirectory(test)
endif()

if(ACTIVE_SOURCE)
    if(NOT EXISTS "${ACTIVE_SOURCE}")
        message(FATAL_ERROR "ACTIVE_SOURCE does not exist: ${ACTIVE_SOURCE}")
    endif()

    add_executable(active_source
        "${ACTIVE_SOURCE}"
    )
    target_link_libraries(active_source PRIVATE Siv3D app_components)
    target_include_directories(active_source PRIVATE
        "${CMAKE_SOURCE_DIR}/src/hello"
        "${CMAKE_SOURCE_DIR}/external/OpenSiv3D/Siv3D/include"
        "${CMAKE_SOURCE_DIR}/external/OpenSiv3D/Siv3D/include/ThirdParty"
        "${CMAKE_SOURCE_DIR}/external/OpenSiv3D/Siv3D/src"
        "${CMAKE_SOURCE_DIR}/external/OpenSiv3D/Siv3D/src/ThirdParty"
    )
    target_compile_features(active_source PRIVATE cxx_std_20)

    set_target_properties(active_source PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    add_custom_command(TARGET active_source POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/external/OpenSiv3D/Linux/App/resources"
            "$<TARGET_FILE_DIR:active_source>/resources"
    )
endif()
