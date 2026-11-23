/**
 * @quizzyjoy/audio-dsp — HarmonyOS audio DSP library.
 *
 * FFT spectrum, waveform envelope and level metering for PCM you already have
 * (AudioCapturer, files, or synthetic streams). No decoding, no playback.
 */
export { SpectrumAnalyzer } from './src/main/ets/analyzer';
export { computeWaveform, rms, peak } from './src/main/ets/waveform';
export { runSelfTest } from './src/main/ets/selftest';
export { SpectrumOptions } from './src/main/ets/types';
