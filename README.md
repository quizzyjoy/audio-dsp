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

## Demo 工程

`harmony/` 内含一个 demo（`entry` 模块），**用合成音驱动，不需要麦克风权限**：

- 32 条实时频谱（对数频带）
- 波形包络（96 点）
- RMS / 峰值 / 峰值频带 / 每秒帧数
- 频率滑块（110–2000 Hz）+ 暂停 / 重置
- 首屏显示原生自检结果（`selftest PASS/FAIL`）

运行方式（DevEco Studio）：

1. 打开 `harmony/`
2. File → Project Structure → Signing Configs → 勾选 **Automatically generate signature**
3. 选中 `entry` → Run

运行方式（命令行，无线真机）：

```bash
devecocli auth login            # 华为开发者账号（浏览器授权一次）
devecocli signature generate    # 生成调试证书/Profile，写入 harmony/build-profile.json5
devecocli run --device <serial> --module entry   # 构建 + 签名 + 安装 + 拉起
devecocli ui screenshot --device <serial> --path shot.png
```

签名材料落在 `~/.ohos/config/`，`harmony/build-profile.json5` 里的 `signingConfigs` 含本机路径与凭据，**不入库**（已用 `git update-index --skip-worktree harmony/build-profile.json5` 排除，换机重新 `signature generate` 即可）。

命令行构建（HAR）：见上文；命令行构建 HAP 需要本机签名配置。

## 测试

| 层 | 位置 | 说明 |
|---|---|---|
| 原生自检 | `library/src/main/cpp/audio_dsp.cpp` → `runSelfTest()` | 440 Hz 正弦 → 峰值频带、静音、波形范围、RMS/峰值容差 |
| 设备测试 | `harmony/entry/src/ohosTest` | Hypium：自检、push 契约、归一化、440 Hz 频带、静音、waveform/rms/peak |
| 发布一致性 | `scripts/check-release.mjs` | 版本同步、九要素 README、三件套、native 源文件（CI 可跑，无需 SDK） |

```bash
# 无需 SDK，本地/CI 均可
node scripts/check-release.mjs

# 设备测试（DevEco 配好签名后）
hvigorw --mode module -p module=entry@ohosTest -p product=default test
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
