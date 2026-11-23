# audio-dsp

**鸿蒙上的音频分析基础件**：把 PCM 变成可用的分析结果——FFT 频谱、波形包络、RMS/峰值。

> 包名：`@quizzyjoy/audio-dsp`　组织：quizzyjoy　许可：Apache-2.0
> 定位：**只做分析**，不做解码、不做播放。解码交给 `@ospark/free-pcm` / ffmpeg，播放交给 `AVPlayer` / `AudioRenderer`。

## 为什么需要它

OHPM 上音频相关的包不少（播放器、opus/ogg 编解码、soundtouch 变速变调、ffmpeg 封装），但**没有任何一个提供"每帧频谱数据"**：

| 能力 | OHPM 现状 |
|---|---|
| 解码 / 播放 | 有（多个） |
| 变速变调 | 2 个 soundtouch 移植（下载 20 / 105，且 LGPL） |
| 均衡 | 1 个（free-pcm） |
| **FFT / 频谱 / 波形 / 节拍** | **0** |

## 安装与使用

```bash
ohpm install @quizzyjoy/audio-dsp
```

```ts
import { SpectrumAnalyzer, computeWaveform } from '@quizzyjoy/audio-dsp';

const analyzer = new SpectrumAnalyzer({ fftSize: 1024, sampleRate: 48000, bands: 32 });
const frame = analyzer.push(pcm);   // Float32Array(32) | null，值域 0..1
const wave = computeWaveform(pcm, 200);
```

完整接口说明见 [harmony/library/README.md](harmony/library/README.md)。

## 仓库结构

```
audio-dsp/
└── harmony/                 # HarmonyOS 工程（DevEco / hvigor）
    └── library/             # HAR 模块（= OHPM 包）
        ├── Index.ets        # 对外入口
        ├── src/main/ets/    # ArkTS API
        └── src/main/cpp/    # NAPI + kissfft
```

## 构建

```bash
cd harmony
node "C:/Program Files/Huawei/DevEco Studio 26/DevEco Studio/tools/hvigor/bin/hvigorw.js" \
  assembleHar -p product=default -p buildMode=release --no-daemon
# 产物：harmony/library/build/default/outputs/default/library.har
```

发布前自检：

```bash
npx @quizzyjoy/ohpm-lint harmony/library/build/default/outputs/default/library.har
ohpm prepublish harmony/library/build/default/outputs/default/library.har
ohpm publish  harmony/library/build/default/outputs/default/library.har
```

## 路线图

| 版本 | 内容 |
|---|---|
| 0.1（已发布） | FFT 频谱、波形包络、RMS/峰值 |
| 0.2 | 频带拆分（bass/mid/treble）、响应谱峰值、50% 重叠 |
| 0.3 | onset / BPM、LUFS 响度 |
| 0.4 | 重采样、变速不变调（自研 WSOLA，避开 LGPL） |

## 贡献

Issue / PR：<https://github.com/quizzyjoy/audio-dsp/issues>

## 许可

Apache-2.0；第三方组件见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
