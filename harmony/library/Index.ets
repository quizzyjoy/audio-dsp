/**
 * @quizzyjoy/audio-dsp — HarmonyOS audio DSP library.
 *
 * FFT spectrum (with bass/mid/treble split), waveform envelope and level metering
 * for PCM you already have (AudioCapturer, files, or synthetic streams).
 * No decoding, no playback.
 */
export { SpectrumAnalyzer } from './src/main/ets/analyzer';
export { computeWaveform, rms, peak, rmsDb, peakDb, linearToDb, topPeaks } from './src/main/ets/waveform';
export { SpectrumFrame, SpectrumOptions } from './src/main/ets/types';
export { LoudnessMeter, LoudnessReadout } from './src/main/ets/loudness';
export { TempoEstimator, TempoReadout } from './src/main/ets/tempo';
export { runSelfTest } from './src/main/ets/selftest';
