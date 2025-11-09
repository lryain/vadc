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

# ONNX Runtime 查找
if(EXISTS "${CMAKE_SOURCE_DIR}/lib/libonnxruntime.so")
    message(STATUS "✓ Using bundled ONNX Runtime from ./lib")
    set(ONNX_LIB "${CMAKE_SOURCE_DIR}/lib/libonnxruntime.so")
else()
    find_library(ONNX_LIB onnxruntime)
    if(NOT ONNX_LIB)
        message(FATAL_ERROR "ONNX Runtime library not found")
    endif()
    message(STATUS "✓ Using system ONNX Runtime")
endif()

# ============================================================================
# 库目标：libvadc (直接编译 vadc.c，无包装层，排除 main 函数)
# ============================================================================
add_library(vadc STATIC ${SOURCES} libvadc_api.c)
set_target_properties(vadc PROPERTIES OUTPUT_NAME "vadc")
target_link_libraries(vadc PRIVATE ${ONNX_LIB} m dl pthread)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l|armv7-a|aarch64|arm64")
    target_compile_definitions(vadc PRIVATE VADC_SLOW=1)
    message(STATUS "✓ ARM processor detected")
else()
    target_compile_definitions(vadc PRIVATE VADC_SLOW=0)
    message(STATUS "✓ x86 processor detected")
endif()

# 排除库中的 main 函数：将 main 重定向为虚函数，只在 CLI 中保留
target_compile_definitions(vadc PRIVATE ONNX_INFERENCE_ENABLED=1 TRACY_ENABLE=0 main=vadc_library_skip_main)

# 公开头文件
target_include_directories(vadc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# include frame-level API implementation in the static lib
target_sources(vadc PRIVATE libvadc_frame_api.c)

# ============================================================================
# 可执行文件：vadc (CLI 工具)
# ============================================================================
add_executable(vadc_cli ${SOURCES})
target_link_libraries(vadc_cli PRIVATE ${ONNX_LIB} m dl pthread)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "armv7l|armv7-a|aarch64|arm64")
    target_compile_definitions(vadc_cli PRIVATE VADC_SLOW=1)
else()
    target_compile_definitions(vadc_cli PRIVATE VADC_SLOW=0)
endif()

target_compile_definitions(vadc_cli PRIVATE ONNX_INFERENCE_ENABLED=1 TRACY_ENABLE=0)
set_target_properties(vadc_cli PROPERTIES OUTPUT_NAME "vadc")

install(TARGETS vadc DESTINATION lib)
install(TARGETS vadc_cli DESTINATION bin)
install(FILES vadc.h DESTINATION include)
install(FILES libvadc_api.h DESTINATION include)

# examples
add_executable(test_vadc examples/test_vadc.c)
target_link_libraries(test_vadc PRIVATE vadc ${ONNX_LIB} m dl pthread)
install(TARGETS test_vadc DESTINATION bin)
message(STATUS "✓ Build targets: libvadc.a (library), vadc (CLI tool)")
