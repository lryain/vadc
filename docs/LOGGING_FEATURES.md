# VADC 详细日志和音频保存功能

本更新为 VADC 添加了详细的日志记录和音频流保存功能，方便调试和分析。

## 新增功能

### 1. 详细日志输出 (`--verbose`)

启用详细的日志输出，显示所有检测到的语音事件和系统信息。

```bash
cd /path/to/test_vadc
arecord -f S16_LE -c 1 -r 16000 -q - | ./build/test_vadc --stdin --verbose
```

**输出示例:**
```
════════════════════════════════════════════
VADC - 语音活动检测系统
════════════════════════════════════════════
参数配置:
  说话概率阈值: 0.50
  最小沉默时长: 200ms
  最小说话时长: 250ms
  语音边界填充: 30ms
════════════════════════════════════════════
🎤 事件 #1 | 说话: 0.10-0.50秒 (时长: 0.40秒) | 概率: 85.20%
🎤 事件 #2 | 说话: 1.20-1.80秒 (时长: 0.60秒) | 概率: 92.10%
════════════════════════════════════════════
检测完成
  总处理时长: 3.24秒
  检测到语音事件: 2
════════════════════════════════════════════
```

### 2. 音频流保存 (`--save_audio`)

将获取的原始音频 (16-bit PCM, 16kHz) 保存到文件，供后续播放或分析。

```bash
arecord -f S16_LE -c 1 -r 16000 -q - | ./build/test_vadc --stdin --save_audio my_audio.raw
```

**播放保存的音频:**
```bash
# 使用 aplay
aplay -f S16_LE -r 16000 -c 1 my_audio.raw

# 或转换为 WAV
sox -t raw -r 16000 -b 16 -c 1 -e signed-integer my_audio.raw audio.wav
aplay audio.wav
```

### 3. 日志保存 (`--save_log`)

将检测日志保存到文件，便于事后分析和调试。

```bash
arecord -f S16_LE -c 1 -r 16000 -q - | ./build/test_vadc --stdin --save_log detection.log
```

### 4. 组合使用

一次性启用所有功能：

```bash
arecord -f S16_LE -c 1 -r 16000 -q - | \
  ./build/test_vadc --stdin \
    --verbose \
    --save_audio speech.raw \
    --save_log speech.log \
    --stats
```

## 便捷脚本

使用 `run_vadc_with_logging.sh` 快速启动：

### 实时检测（含详细日志）
```bash
./run_vadc_with_logging.sh run
```

### 调试模式（自动保存音频和日志）
```bash
./run_vadc_with_logging.sh debug my_test
```

这会生成：
- `my_test_audio.raw` - 原始音频
- `my_test_log.txt` - 检测日志

然后可以：
```bash
# 播放音频
aplay -f S16_LE -r 16000 -c 1 my_test_audio.raw

# 查看日志
cat my_test_log.txt
```

### 录制和回放
```bash
# 录制 5 秒
timeout 5 ./run_vadc_with_logging.sh record my_voice.wav

# 处理录制的文件
./run_vadc_with_logging.sh playback my_voice.wav --verbose
```

##全部参数说明

| 参数 | 说明 | 示例 |
|------|------|------|
| `--verbose` | 启用详细日志输出 | `--verbose` |
| `--stats` | 显示统计信息 | `--stats` |
| `--save_audio <文件>` | 保存原始音频 | `--save_audio audio.raw` |
| `--save_log <文件>` | 保存检测日志 | `--save_log log.txt` |
| `--threshold <值>` | 说话概率阈值 (0.0-1.0) | `--threshold 0.6` |
| `--min_speech <ms>` | 最小说话时长 | `--min_speech 300` |
| `--min_silence <ms>` | 最小沉默时长 | `--min_silence 250` |
| `--stdin` | 从标准输入读取 | `--stdin` |

## 编译

新功能已集成，直接编译即可：

```bash
cd /path/to/test_vadc
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

## 原始音频格式

保存的音频格式：
- **格式**: PCM (Pulse Code Modulation)
- **位深**: 16-bit
- **采样率**: 16000 Hz (16kHz)
- **通道**: 单声道 (Mono)
- **字节序**: 小端 (Little Endian)

转换为其他格式：

```bash
# 转 WAV
sox -t raw -r 16000 -b 16 -c 1 -e signed-integer audio.raw audio.wav

# 转 MP3
sox -t raw -r 16000 -b 16 -c 1 -e signed-integer audio.raw audio.mp3

# 转 OGG
sox -t raw -r 16000 -b 16 -c 1 -e signed-integer audio.raw audio.ogg

# 转 FLAC
sox -t raw -r 16000 -b 16 -c 1 -e signed-integer audio.raw audio.flac
```

## 常见问题

**Q: 为什么没有生成日志文件?**
- 确保指定了 `--save_log` 参数
- 检查目录是否有写权限
- 尝试指定绝对路径

**Q: 音频文件很大吗?**
- 16-bit 16kHz mono = 32,000 字节/秒
- 10 分钟的音频 ≈ 19.2 MB

**Q: 可以处理后的实时播放吗?**
- 可以用管道：`./build/test_vadc --stdin | aplay`
- 但需要保证处理速度快于实时

## 性能说明

- 日志输出会略微增加处理时间 (~5-10%)
- 音频保存需要额外 I/O，可能影响实时性
- 建议调试时使用，生产环境可关闭这些功能
