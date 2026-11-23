# @quizzyjoy/audio-dsp

鸿蒙上的实时音频分析库：**FFT 频谱、波形包络、RMS/峰值**。
只做分析，不做解码与播放——把 `AudioCapturer`（或文件、合成信号）拿到的 PCM 直接喂进来即可。

## 安装

```bash
ohpm install @quizzyjoy/audio-dsp
```

## 所需权限

本库**不需要任何权限**（只处理你传入的 PCM 数据）。
若你从麦克风采集，需要自己在应用侧申请 `ohos.permission.MICROPHONE` 并创建 AudioCapturer。

## 接口

### SpectrumAnalyzer

| 成员 | 说明 |
|---|---|
| `constructor(options?: SpectrumOptions)` | `fftSize`(256–2048，默认 1024)、`sampleRate`(默认 48000)、`bands`(1–128，默认 32) |
| `push(pcm: Int16Array): Float32Array \| null` | 喂入单声道 S16LE 样本；攒满一窗返回一帧频带能量（0..1），否则 `null` |
| `reset(): void` | 清空缓冲与平滑状态 |
| `release(): void` | 释放 native 资源 |
| `bands: number` | 每帧频带数 |

### 工具函数

| 函数 | 说明 |
|---|---|
| `computeWaveform(pcm, points): Float32Array` | 返回 `2 * points` 个值 `[min0,max0,...]`，范围 -1..1，用于画波形 |
| `rms(pcm): number` | 均方根电平（0..1） |
| `peak(pcm): number` | 绝对值峰值（0..1） |

## 示例

```ts
import { SpectrumAnalyzer, computeWaveform } from '@quizzyjoy/audio-dsp';

const analyzer = new SpectrumAnalyzer({ fftSize: 1024, sampleRate: 48000, bands: 32 });

// 任意来源的单声道 S16LE PCM
audioCapturer.on('readData', (buffer: ArrayBuffer) => {
  const pcm = new Int16Array(buffer);
  const frame = analyzer.push(pcm);
  if (frame !== null) {
    // frame: Float32Array(32)，每项 0..1
    drawBars(frame);
  }
});
```

合成测试音（不依赖麦克风）：

```ts
const pcm = new Int16Array(1024);
for (let i = 0; i < pcm.length; i++) {
  pcm[i] = Math.round(Math.sin((2 * Math.PI * 440 * i) / 48000) * 12000);
}
const frame = analyzer.push(pcm);
```

## 约束与限制

- **只接受单声道 S16LE**（`AudioCapturer` 默认 `SAMPLE_FORMAT_S16LE`）；立体声请先自行下混。
- **不做解码/播放/编码**：mp3/ogg 解码请配合 `@ospark/free-pcm` 或 ffmpeg 封装；播放用 `AVPlayer`/`AudioRenderer`。
- **一次 push 的样本数**不必等于 `fftSize`，内部会累积；攒满才出帧（无重叠）。
- **最低 API 12**，支持 `arm64-v8a` 与 `x86_64`（模拟器）。
- 频带为对数分布（60 Hz–16 kHz），已做归一化与轻度平滑，适合可视化；不适合做精密测量。

## 目录结构

```
library/
├── Index.ets                 # 对外入口
├── src/main/ets/             # ArkTS API（analyzer / waveform / types）
└── src/main/cpp/             # NAPI + kissfft
    ├── audio_dsp.cpp         # FFT、频带聚合、包络、RMS/峰值
    ├── napi_init.cpp         # NAPI 绑定
    └── third_party/kissfft/  # BSD-3（见 THIRD_PARTY_NOTICES.md）
```

## 贡献

Issue / PR 欢迎：<https://github.com/quizzyjoy/audio-dsp/issues>
新增能力请同时补：接口文档、示例、以及一段可断言的测试信号（正弦/方波）。

## 许可

Apache-2.0。第三方组件见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
