add_executable(siv3d_app main.cpp)
target_link_libraries(siv3d_app PRIVATE Siv3D app_components)
target_include_directories(siv3d_app PRIVATE
    "${CMAKE_SOURCE_DIR}/src/aruco"
)
if(NOT EMSCRIPTEN)
    target_include_directories(siv3d_app PRIVATE
        "${CMAKE_SOURCE_DIR}/external/OpenSiv3D/Siv3D/include"
        "${CMAKE_SOURCE_DIR}/external/OpenSiv3D/Siv3D/include/ThirdParty"
        "${CMAKE_SOURCE_DIR}/external/OpenSiv3D/Siv3D/src"
        "${CMAKE_SOURCE_DIR}/external/OpenSiv3D/Siv3D/src/ThirdParty"
    )
endif()
target_compile_features(siv3d_app PRIVATE cxx_std_20)

if(EMSCRIPTEN)
    set(web_shell_template "${WEB_SDK}/Templates/Embeddable/web-player.html")
    set(web_shell "${CMAKE_CURRENT_BINARY_DIR}/web-player.html")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${web_shell_template}")
    file(READ "${web_shell_template}" web_shell_content)
    if(NOT web_shell_content MATCHES "<title>OpenSiv3D for Web</title>")
        message(FATAL_ERROR "The Web shell template's title was not found: ${web_shell_template}")
    endif()
    string(REPLACE "<title>OpenSiv3D for Web</title>" "<title>MarkerPlateMaker</title>"
        web_shell_content "${web_shell_content}")
    file(GENERATE OUTPUT "${web_shell}" CONTENT "${web_shell_content}")
    set_property(TARGET siv3d_app APPEND PROPERTY LINK_DEPENDS "${web_shell}")

    set(web_resources
        engine/font/min/siv3d-min.woff
        engine/font/mplus/mplus-1p-regular.ttf.zstdcmp
        engine/font/noto-cjk/NotoSansJP-Regular.otf.zstdcmp
        engine/shader/essl
        engine/texture/box-shadow
    )
    foreach(resource IN LISTS web_resources)
        set(resource_path "${SIV3D_WEB_TEMPLATE}/resources/${resource}")
        if(NOT EXISTS "${resource_path}")
            message(FATAL_ERROR "Required Web resource not found: ${resource_path}")
        endif()
        target_link_options(siv3d_app PRIVATE "--preload-file=${resource_path}@/resources/${resource}")
        if(IS_DIRECTORY "${resource_path}")
            file(GLOB_RECURSE resource_files CONFIGURE_DEPENDS "${resource_path}/*")
            set_property(TARGET siv3d_app APPEND PROPERTY LINK_DEPENDS ${resource_files})
        else()
            set_property(TARGET siv3d_app APPEND PROPERTY LINK_DEPENDS "${resource_path}")
        endif()
    endforeach()

    target_link_options(siv3d_app PRIVATE
        -fexceptions
        --emrun
        -sFULL_ES3=1
        -sMIN_WEBGL_VERSION=2
        -sMAX_WEBGL_VERSION=2
        -sUSE_GLFW=3
        -sUSE_OGG=1
        -sUSE_VORBIS=1
        -sALLOW_MEMORY_GROWTH=1
        -sERROR_ON_UNDEFINED_SYMBOLS=1
        -sASYNCIFY=1
        -sASYNCIFY_IGNORE_INDIRECT=1
        -sMODULARIZE=1
        -sEXPORT_NAME=Runtime
        "--shell-file=${web_shell}"
        "--pre-js=${WEB_SDK}/lib/Siv3D/Siv3D.pre.js"
        "--post-js=${WEB_SDK}/lib/Siv3D/Siv3D.post.js"
        "--post-js=${WEB_SDK}/Templates/Embeddable/web-player.js"
        "--js-library=${WEB_SDK}/lib/Siv3D/Siv3D.js"
        "-sASYNCIFY_IMPORTS=['siv3dRequestAnimationFrame','siv3dGetClipboardText','siv3dDecodeImageFromFile','siv3dSleepUntilWaked','invoke_v','invoke_vi']"
        "-sASYNCIFY_ADD=['main','Main()','dynCall_v','dynCall_vi','s3d::TryMain()','s3d::CSystem::init()','s3d::System::Update()','s3d::AACDecoder::decode(*) const','s3d::MP3Decoder::decode(*) const','s3d::CAudioDecoder::decode(*)','s3d::AudioDecoder::Decode(*)','s3d::Wave::Wave(*)','s3d::Audio::Audio(*)','s3d::GenericDecoder::decode(*) const','s3d::CImageDecoder::decode(*)','s3d::Image::Image(*)','s3d::Texture::Texture(*)','s3d::ImageDecoder::Decode(*)','s3d::ImageDecoder::GetImageInfo(*)','s3d::Model::Model(*)','s3d::CModel::create(*)','s3d::CRenderer2D_GLES3::init()','s3d::CRenderer2D_WebGPU::init()','s3d::Clipboard::GetText(*)','s3d::CClipboard::getText(*)','s3d::SimpleHTTP::Save(*)','s3d::SimpleHTTP::Load(*)','s3d::SimpleHTTP::Get(*)','s3d::SimpleHTTP::Post(*)','s3d::VideoReader::VideoReader(*)','s3d::VideoReader::open(*)','s3d::Platform::Web::FetchFile(*)']"
    )
    set_target_properties(siv3d_app PROPERTIES OUTPUT_NAME "index" SUFFIX ".html" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/web")
    add_custom_command(TARGET siv3d_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:siv3d_app>/licenses"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/LICENSES/opencv-aruco.txt"
            "${CMAKE_SOURCE_DIR}/LICENSES/opencv-apache-2.0.txt"
            "${CMAKE_SOURCE_DIR}/LICENSES/freetype-2.10.4.txt"
            "${CMAKE_SOURCE_DIR}/LICENSES/opensiv3d-0.6.6.txt"
            "${CMAKE_SOURCE_DIR}/LICENSES/opensiv3d-0.6.6-third-party.md"
            "$<TARGET_FILE_DIR:siv3d_app>/licenses")
    add_custom_command(TARGET siv3d_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SIV3D_WEB_TEMPLATE}/LICENSE" "$<TARGET_FILE_DIR:siv3d_app>/licenses/web-template.txt")
    add_custom_command(TARGET siv3d_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SIV3D_WEB_TEMPLATE}/resources/engine/font/min/LICENSE"
            "$<TARGET_FILE_DIR:siv3d_app>/licenses/siv3d-min-font.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SIV3D_WEB_TEMPLATE}/resources/engine/font/mplus/LICENSE_E"
            "$<TARGET_FILE_DIR:siv3d_app>/licenses/mplus-font.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SIV3D_WEB_TEMPLATE}/resources/engine/font/noto-cjk/LICENSE"
            "$<TARGET_FILE_DIR:siv3d_app>/licenses/noto-cjk-font.txt")
else()
    set_target_properties(siv3d_app PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_custom_command(TARGET siv3d_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/external/OpenSiv3D/Linux/App/resources"
            "$<TARGET_FILE_DIR:siv3d_app>/resources"
    )
    add_test(NAME siv3d_app COMMAND siv3d_app)
    include(${CMAKE_SOURCE_DIR}/cmake/add_default_build.cmake)
    add_subdirectory(hello)
endif()
