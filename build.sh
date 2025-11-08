#!/bin/bash
# VADC 编译脚本 - ARM Linux 版本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "=========================================="
echo "编译 VADC (Voice Activity Detection)"
echo "=========================================="
echo ""

# 检查 cmake
if ! command -v cmake &> /dev/null; then
    echo "❌ 错误: cmake 未安装"
    echo "请运行: sudo apt-get install cmake build-essential"
    exit 1
fi

# 检查 ONNX Runtime
if [ ! -f "${SCRIPT_DIR}/lib/libonnxruntime.so" ]; then
    echo "⚠️  警告: 未找到 ./lib/libonnxruntime.so"
    echo "尝试从系统查找..."
fi

# 创建构建目录
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置
echo "📋 配置 CMake..."
cmake -DCMAKE_BUILD_TYPE=Release .. 

# 编译
echo ""
echo "🔨 编译中..."
make clean
make -j$(nproc)

echo ""
echo "=========================================="
echo "✅ 编译完成！"
echo "=========================================="
echo ""
echo "可执行文件: $BUILD_DIR/vadc"
echo ""
echo "使用方法:"
echo "  $SCRIPT_DIR/run_vadc.sh audio.wav"
echo "  ffmpeg -i input.mp3 -f s16le -ac 1 -ar 16000 - | $SCRIPT_DIR/run_vadc.sh"
echo ""
