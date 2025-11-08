#!/bin/bash
# VADC 语音活动检测工具启动脚本
# 为 Raspberry Pi/ARM Linux 优化

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VADC_BIN="${SCRIPT_DIR}/build/vadc"
LIB_DIR="${SCRIPT_DIR}/lib"

if [ ! -f "$VADC_BIN" ]; then
    echo "错误: 未找到 vadc 可执行文件"
    echo "请先运行: cd $SCRIPT_DIR && ./build.sh"
    exit 1
fi

# 设置库路径
export LD_LIBRARY_PATH="${LIB_DIR}:${LD_LIBRARY_PATH}"

# 如果没有提供参数，显示帮助
if [ $# -eq 0 ]; then
    echo "VADC - Voice Activity Detection for Raspberry Pi"
    echo "用途: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  <audio_file>           处理音频文件 (支持 wav, mp3 等)"
    echo "  --stdin                从 stdin 读取 16kHz 单声道 PCM 数据"
    echo "  --stats                显示统计信息"
    echo ""
    echo "示例:"
    echo "  # 处理本地文件"
    echo "  $0 audio.wav"
    echo ""
    echo "  # 通过管道处理 MP3"
    echo "  ffmpeg -i audio.mp3 -f s16le -ac 1 -ar 16000 - | $0 --stdin"
    echo ""
    exit 0
fi

# 运行 vadc
exec "$VADC_BIN" "$@"
