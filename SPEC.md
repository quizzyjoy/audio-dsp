# SPEC · audio-dsp v0.1

> 目标：给鸿蒙提供一层**音频分析 API**——输入 PCM，输出频谱/波形/电平；不做解码与播放。

## 1. 范围

| 做 | 不做 |
|---|---|
| FFT 频谱（256–2048 点） | 音频解码（mp3/ogg/aac…） |
| 对数频带聚合（1–128） | 播放 / 录音封装 |
| 波形 min/max 包络 | 编码 |
| RMS / 峰值 | 变速变调（v0.4） |
| 单声道 S16LE | 立体声下混（由调用方完成） |

## 2. 数据模型

- 输入：`Int16Array`（单声道 S16LE），长度任意；内部按 `fftSize` 累积。
- 输出：`Float32Array(bands)`，值域 0..1；未满一窗返回 `null`。
- 平滑：`0.6 * prev + 0.4 * current`（可视化友好，不做精密测量）。

## 3. 算法

1. 加 Hann 窗；
2. `kiss_fftr` 实数 FFT（`kiss_fft_scalar=float`）；
3. 幅度归一化：`mag / fftSize`；
4. 频带：60 Hz–16 kHz 对数分布，取每带平均幅度，除以本帧最大幅度，再开方（感知压缩）；
5. 更新平滑值。

波形：按 `points` 分段取 min/max，除以 32768。

## 4. 原生层

| 项 | 值 |
|---|---|
| 实现 | C++17 + NAPI，`libaudio_dsp.so` |
| 依赖 | kissfft（BSD-3，vendored） |
| ABI | arm64-v8a / x86_64 |
| 接口 | `createAnalyzer / pushFrame / resetAnalyzer / destroyAnalyzer / computeWaveformNative / computeRmsNative / computePeakNative` |

## 5. 质量门禁

- `ohpm-lint --strict`：0 error / 0 warn（1 条 info 为 hvigor 字节码 HAR 特性）。
- `ohpm prepublish`：通过。
- 后续加入：合成正弦断言（440 Hz → 峰值落在对应频带）、静音输入全 0。

## 6. 兼容性

- 最低 API 12；`compatibleSdkType: HarmonyOS`。
- 纯 native，无 ArkUI 依赖，可用于 Stage 模型任意 UIAbility。
