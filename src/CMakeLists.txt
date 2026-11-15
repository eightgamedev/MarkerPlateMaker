add_library(app_components STATIC
    aruco/marker_plate.cpp
    hello/hello.cpp
)
target_include_directories(app_components PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/aruco"
    "${CMAKE_CURRENT_SOURCE_DIR}/hello"
)
if(NOT EMSCRIPTEN)
    target_include_directories(app_components SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(app_components PUBLIC ${OpenCV_LIBS})
else()
    target_compile_options(app_components PRIVATE -fexceptions)
endif()
