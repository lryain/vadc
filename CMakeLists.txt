cmake_minimum_required(VERSION 3.10)
project(vadc C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wno-unknown-pragmas -Wno-format-security -fPIC -O3)
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/tracy
)

set(SOURCES
    vadc.c
)

add_executable(vadc ${SOURCES})

if(EXISTS "${CMAKE_SOURCE_DIR}/lib/libonnxruntime.so")
    message(STATUS "Using bundled ONNX Runtime from ./lib")
    target_link_libraries(vadc PRIVATE "${CMAKE_SOURCE_DIR}/lib/libonnxruntime.so")
else()
    find_library(ONNXRUNTIME onnxruntime)
    if(ONNXRUNTIME)
        message(STATUS "Using system ONNX Runtime")
        target_link_libraries(vadc PRIVATE ${ONNXRUNTIME})
    else()
        message(FATAL_ERROR "ONNX Runtime library not found")
    endif()
endif()

target_link_libraries(vadc PRIVATE m dl pthread)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l|armv7-a|aarch64|arm64")
    target_compile_definitions(vadc PRIVATE VADC_SLOW=1)
    message(STATUS "ARM processor detected - disabling SIMD")
else()
    target_compile_definitions(vadc PRIVATE VADC_SLOW=0)
    message(STATUS "x86 processor - enabling SIMD")
endif()

target_compile_definitions(vadc PRIVATE ONNX_INFERENCE_ENABLED=1 TRACY_ENABLE=0)

install(TARGETS vadc DESTINATION bin)
message(STATUS "Build target: vadc")
