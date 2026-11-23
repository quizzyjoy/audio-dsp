# Changelog

## [0.1.0] - 2026-10-21

首个版本：音频分析层（不含解码与播放），已发布至 OHPM 中心仓。

### Added

- `SpectrumAnalyzer`：FFT 频谱（256/512/1024/2048 点，Hann 窗），60 Hz–16 kHz 对数频带（1–128），归一化 0..1，轻度平滑。
- `computeWaveform(pcm, points)`：min/max 包络。
- `rms(pcm)` / `peak(pcm)`：电平（0..1）。
- 原生实现：NAPI + kissfft（BSD-3），arm64-v8a / x86_64。

### Known limitations

- 仅单声道 S16LE；无重叠；无节拍/重采样/变速变调（见 SPEC 路线图）。
