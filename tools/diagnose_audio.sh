#!/bin/bash
# VADC 音频设备诊断脚本

echo "====== VADC 音频系统诊断 ======"
echo ""

echo "1. 检查必要工具..."
for tool in ffmpeg arecord pactl alsamixer; do
    if command -v $tool &> /dev/null; then
        echo "  ✓ $tool 已安装"
    else
        echo "  ✗ $tool 未安装"
    fi
done

echo ""
echo "2. 检查 ALSA 设备..."
if command -v arecord &> /dev/null; then
    echo "   ALSA 录音设备列表:"
    arecord -l 2>/dev/null || echo "   (无法列出设备)"
else
    echo "   ✗ arecord 未安装，跳过 ALSA 检查"
fi

echo ""
echo "3. 检查 PulseAudio..."
if command -v pactl &> /dev/null; then
    if pactl list short sources &>/dev/null; then
        echo "   ✓ PulseAudio 正在运行"
        echo "   输入设备:"
        pactl list short sources 2>/dev/null | head -5
    else
        echo "   ✗ PulseAudio 未运行或无法连接"
    fi
else
    echo "   ✗ pactl 未安装"
fi

echo ""
echo "4. 检查音频权限..."
groups | grep -q audio && echo "   ✓ 用户在 audio 组" || echo "   ✗ 用户不在 audio 组 (需要: sudo usermod -a -G audio $(whoami))"

echo ""
echo "5. 测试麦克风 (3秒)..."
if command -v ffmpeg &> /dev/null; then
    echo "   尝试录制 3 秒测试音频..."
    timeout 3 ffmpeg -f alsa -i default -acodec pcm_s16le -ar 16000 -ac 1 -f s16le test_audio.raw 2>&1 | grep -E "Stream|Duration" || echo "   (无法录制音频)"
    if [ -f test_audio.raw ]; then
        size=$(stat -f%z test_audio.raw 2>/dev/null || stat -c%s test_audio.raw 2>/dev/null)
        if [ "$size" -gt 0 ]; then
            echo "   ✓ 成功录制 $size 字节"
            rm -f test_audio.raw
        fi
    fi
else
    echo "   ✗ ffmpeg 未安装"
fi

echo ""
echo "====== 安装建议 ======"
if ! command -v ffmpeg &> /dev/null; then
    echo "安装 ffmpeg: sudo apt-get install ffmpeg"
fi
if ! command -v arecord &> /dev/null; then
    echo "安装 ALSA: sudo apt-get install alsa-utils"
fi
if ! groups | grep -q audio; then
    echo "添加到 audio 组: sudo usermod -a -G audio $(whoami)"
    echo "然后重新登录或运行: newgrp audio"
fi

echo ""
echo "====== 使用麦克风运行 VADC ======"
echo "执行: /path/to/run_vadc_microphone.sh"
echo "或: /path/to/run_vadc_microphone.sh ffmpeg"
