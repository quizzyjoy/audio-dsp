# Changelog

All notable changes are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) · Versioning: [SemVer](https://semver.org/).

## [0.3.0] - 2026-10-21

### Added

- `LoudnessMeter`：ITU-R BS.1770-4 风格响度（K 加权、400 ms 块、75% 重叠、门限积分），输出 momentary / shortTerm / integrated LUFS。
- `TempoEstimator`：基于帧能量的 onset 检测与 BPM 估计（中位间隔 + 一致性置信度）。
- demo：LUFS-I / LUFS-M / BPM 读数，新增「节拍模式」（500 ms 一次的点击包络）。
- Hypium 新增 4 个用例（-20 dBFS → -20 LUFS、静音、120 BPM 点击轨、reset）。

## [0.2.0] - 2026-10-21

### Added

- 频带拆分：`pushFrame()` 一次返回 `bands` + `bass` / `mid` / `treble`（交叉点可配，默认 250 Hz / 2 kHz）。
- `SpectrumFrame` 类型：`bands`、`bass`、`mid`、`treble`。
- 50% 重叠窗口：`overlap: 50`，帧率翻倍、频谱更平滑。
- 电平工具：`rmsDb()`、`peakDb()`、`linearToDb()`；峰值定位 `topPeaks()`。
- demo 增加 BASS/MID/TREBLE 电平条与「50% 重叠」开关。

## [0.1.1] - 2026-10-21

### Added

- `runSelfTest()`：原生自检（440 Hz 正弦 → 峰值频带、静音、波形范围、RMS/峰值容差），demo 首屏展示 PASS/FAIL。
- 仓库内新增 demo 工程（合成音驱动，无需麦克风权限）与 Hypium 设备测试（6 个断言组）。

## [0.1.0] - 2026-10-21

首个版本：音频分析层（不含解码与播放）。

### Added

- `SpectrumAnalyzer`：实时 FFT 频谱，支持 256/512/1024/2048 点，Hann 窗，60 Hz–16 kHz 对数频带（1–128，默认 32），归一化 0..1，轻度时间平滑。
- `computeWaveform(pcm, points)`：min/max 包络，供波形绘制。
- `rms(pcm)` / `peak(pcm)`：电平计算（0..1）。
- 原生实现：NAPI + kissfft（BSD-3），支持 `arm64-v8a` / `x86_64`。
- 不依赖解码器、不申请权限：输入为单声道 S16LE PCM。

### Known limitations

- 仅单声道 S16LE；立体声需自行下混。
- 无重叠（不做 50% overlap-add）。
- 不含节拍检测、变速变调、重采样（规划中）。
