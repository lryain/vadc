#!/bin/bash
# VADC 测试和调试脚本
# 用于完整展示所有功能

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VADC_BIN="${SCRIPT_DIR}/build/vadc"

if [ ! -f "$VADC_BIN" ]; then
    echo "错误: VADC 可执行文件不存在: $VADC_BIN"
    exit 1
fi

export LD_LIBRARY_PATH="${SCRIPT_DIR}/lib:${LD_LIBRARY_PATH}"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}════════════════════════════════════════════${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}════════════════════════════════════════════${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_info() {
    echo -e "${YELLOW}ℹ $1${NC}"
}

# 命令
case "${1:-help}" in
    test_basic)
        print_header "测试 1: 基本实时检测 (10秒)"
        print_info "说话会显示概率值，保持沉默也会显示"
        echo ""
        print_info "按 Ctrl+C 停止录制"
        echo ""
        
        timeout 10 arecord -f S16_LE -c 1 -r 16000 -q - | "$VADC_BIN" --stdin 2>&1
        ;;
    
    test_verbose)
        print_header "测试 2: 启用详细日志"
        print_info "应该看到初始化信息和参数配置"
        echo ""
        
        timeout 5 arecord -f S16_LE -c 1 -r 16000 -q - | \
            "$VADC_BIN" --stdin --verbose 2>&1 | head -50
        ;;
    
    test_save_audio)
        print_header "测试 3: 保存音频文件"
        print_info "录制 5 秒音频并保存"
        echo ""
        
        OUTPUT="test_audio_$(date +%s).raw"
        timeout 5 arecord -f S16_LE -c 1 -r 16000 -q - | \
            "$VADC_BIN" --stdin --save_audio "$OUTPUT" 2>&1
        
        if [ -f "$OUTPUT" ]; then
            SIZE=$(stat -c%s "$OUTPUT" 2>/dev/null || stat -f%z "$OUTPUT" 2>/dev/null)
            print_success "音频已保存: $OUTPUT (大小: $SIZE 字节)"
            
            # 计算录制时长
            DURATION=$(echo "scale=2; $SIZE / 32000" | bc)
            print_info "录制时长: 约 ${DURATION}秒"
            
            # 尝试播放
            print_info "尝试播放录制的音频..."
            if command -v aplay &> /dev/null; then
                print_info "按 Ctrl+C 停止播放"
                aplay -f S16_LE -r 16000 -c 1 "$OUTPUT" 2>/dev/null || true
            else
                print_info "aplay 未安装，跳过播放"
            fi
        else
            print_error "音频文件未生成"
        fi
        ;;
    
    test_save_log)
        print_header "测试 4: 保存日志文件"
        print_info "启用详细日志并保存到文件"
        echo ""
        
        OUTPUT="test_log_$(date +%s).log"
        timeout 5 arecord -f S16_LE -c 1 -r 16000 -q - | \
            "$VADC_BIN" --stdin --verbose --save_log "$OUTPUT" 2>&1 | head -20
        
        if [ -f "$OUTPUT" ]; then
            print_success "日志已保存: $OUTPUT"
            echo ""
            print_info "日志内容:"
            cat "$OUTPUT" | head -20
        else
            print_error "日志文件未生成"
        fi
        ;;
    
    test_all)
        print_header "测试 5: 完整测试 (所有功能)"
        print_info "启用: 详细日志, 音频保存, 日志保存, 统计信息"
        echo ""
        
        OUTPUT_DIR="vadc_test_$(date +%s)"
        mkdir -p "$OUTPUT_DIR"
        
        print_info "输出目录: $OUTPUT_DIR"
        echo ""
        
        timeout 10 arecord -f S16_LE -c 1 -r 16000 -q - | \
            "$VADC_BIN" --stdin \
                --verbose \
                --save_audio "$OUTPUT_DIR/audio.raw" \
                --save_log "$OUTPUT_DIR/detection.log" \
                --stats 2>&1 | tee "$OUTPUT_DIR/console.log"
        
        echo ""
        print_header "测试完成"
        print_info "生成的文件:"
        ls -lh "$OUTPUT_DIR/"
        
        print_info "查看日志: cat $OUTPUT_DIR/detection.log"
        print_info "播放音频: aplay -f S16_LE -r 16000 -c 1 $OUTPUT_DIR/audio.raw"
        ;;
    
    test_threshold)
        print_header "测试 6: 调整阈值"
        print_info "使用更高的阈值 (0.7) - 只输出明确的说话"
        echo ""
        
        timeout 10 arecord -f S16_LE -c 1 -r 16000 -q - | \
            "$VADC_BIN" --stdin --threshold 0.7 2>&1
        ;;
    
    test_threshold_low)
        print_header "测试 7: 调整阈值 (低)"
        print_info "使用更低的阈值 (0.3) - 更灵敏"
        echo ""
        
        timeout 10 arecord -f S16_LE -c 1 -r 16000 -q - | \
            "$VADC_BIN" --stdin --threshold 0.3 2>&1
        ;;
    
    record)
        print_header "录制音频"
        
        if [ -z "$2" ]; then
            OUTPUT="recording_$(date +%s).wav"
        else
            OUTPUT="$2"
        fi
        
        print_info "录制到: $OUTPUT"
        print_info "按 Ctrl+C 停止录制"
        echo ""
        
        arecord -f S16_LE -c 1 -r 16000 -q "$OUTPUT" 2>/dev/null || true
        
        if [ -f "$OUTPUT" ]; then
            SIZE=$(stat -c%s "$OUTPUT" 2>/dev/null || stat -f%z "$OUTPUT" 2>/dev/null)
            print_success "录制完成: $OUTPUT (大小: $SIZE 字节)"
        fi
        ;;
    
    playback)
        print_header "回放音频"
        
        if [ -z "$2" ]; then
            print_error "请指定音频文件"
            echo "用法: $0 playback <audio_file>"
            exit 1
        fi
        
        if [ ! -f "$2" ]; then
            print_error "文件不存在: $2"
            exit 1
        fi
        
        print_info "文件: $2"
        print_info "格式: PCM 16-bit 16kHz mono"
        print_info "按 Ctrl+C 停止播放"
        echo ""
        
        aplay -f S16_LE -r 16000 -c 1 "$2" 2>/dev/null || true
        ;;
    
    diagnose)
        print_header "系统诊断"
        echo ""
        
        # 检查工具
        print_info "检查必要工具..."
        for tool in arecord ffmpeg sox; do
            if command -v $tool &> /dev/null; then
                print_success "$tool 已安装"
            else
                print_error "$tool 未安装"
            fi
        done
        
        echo ""
        
        # 检查麦克风
        print_info "检查 ALSA 设备..."
        arecord -l 2>/dev/null | head -10
        
        echo ""
        
        # 检查权限
        print_info "检查音频权限..."
        if groups | grep -q audio; then
            print_success "用户在 audio 组"
        else
            print_error "用户不在 audio 组"
            print_info "运行: sudo usermod -a -G audio $(whoami)"
        fi
        
        echo ""
        
        # 检查 VADC
        print_info "检查 VADC..."
        if [ -f "$VADC_BIN" ]; then
            print_success "VADC 可执行文件存在"
            print_info "文件大小: $(stat -c%s "$VADC_BIN" 2>/dev/null || stat -f%z "$VADC_BIN" 2>/dev/null) 字节"
        else
            print_error "VADC 可执行文件不存在"
        fi
        ;;
    
    *)
        cat << EOF

${BLUE}VADC 测试脚本${NC}

用法: $0 [命令]

${YELLOW}基本测试:${NC}
  test_basic          基本实时检测 (10秒)
  test_verbose        启用详细日志
  test_save_audio     保存音频到文件
  test_save_log       保存日志到文件
  test_all            完整功能测试
  
${YELLOW}高级测试:${NC}
  test_threshold      高阈值测试 (0.7)
  test_threshold_low  低阈值测试 (0.3)
  
${YELLOW}音频工具:${NC}
  record [文件]       录制音频 (默认: recording_<时间戳>.wav)
  playback <文件>     播放保存的音频
  
${YELLOW}系统:${NC}
  diagnose            诊断系统配置
  help                显示此帮助

${BLUE}快速开始:${NC}

1. 基本测试:
   $0 test_basic

2. 完整功能测试:
   $0 test_all

3. 诊断系统:
   $0 diagnose

${BLUE}示例:${NC}

# 检查系统是否正常
$0 diagnose

# 运行基本测试
$0 test_basic

# 录制 5 秒并分析
timeout 5 $0 record my_voice.wav
$0 playback my_voice.wav

# 运行完整测试（含所有日志和音频保存）
$0 test_all

EOF
        ;;
esac
