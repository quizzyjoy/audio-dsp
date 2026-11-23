# Changelog

## [0.2.0] - 2026-10-21

### Added

- 频带拆分（bass/mid/treble，交叉点可配）、`SpectrumFrame`。
- 50% 重叠窗口（帧率翻倍）。
- `rmsDb` / `peakDb` / `linearToDb` / `topPeaks`。
- demo：BASS/MID/TREBLE 电平条、重叠开关。
- Hypium 新增 6 个用例（拆分、重叠、dB、topPeaks）。

## [0.1.1] - 2026-10-21

### Added

- `runSelfTest()` 原生自检 API，demo 首屏展示结果。
- demo 工程：合成 440 Hz 音 → 实时频谱条、波形、RMS/峰值、峰值频带，无需麦克风权限。
- Hypium 设备测试：selfTest、push 契约、归一化、440 Hz 频带、静音、waveform/rms/peak。
- `scripts/check-release.mjs` + CI：版本一致性、九要素 README、三件套、native 源文件检查。

### Fixed

- demo 界面静止（数据在跑、画面不动）：频谱条与波形的 `ForEach` 键由纯索引改为「索引 + 值」，读数由 `@Builder` 值传参改为 `@Prop` 组件 `Readout`，状态变化现在会真正触发重绘。

## [0.1.0] - 2026-10-21

首个版本：音频分析层（不含解码与播放），已发布至 OHPM 中心仓。

### Added

- `SpectrumAnalyzer`：FFT 频谱（256/512/1024/2048 点，Hann 窗），60 Hz–16 kHz 对数频带（1–128），归一化 0..1，轻度平滑。
- `computeWaveform(pcm, points)`：min/max 包络。
- `rms(pcm)` / `peak(pcm)`：电平（0..1）。
- 原生实现：NAPI + kissfft（BSD-3），arm64-v8a / x86_64。

### Known limitations

- 仅单声道 S16LE；无重叠；无节拍/重采样/变速变调（见 SPEC 路线图）。
