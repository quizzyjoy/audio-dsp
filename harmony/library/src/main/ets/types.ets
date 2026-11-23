/**
 * Options for {@link SpectrumAnalyzer}.
 */
export interface SpectrumOptions {
  /** FFT size in samples: 256 / 512 / 1024 / 2048. Default 1024. */
  fftSize?: number;
  /** Sample rate of the PCM you feed in. Default 48000. */
  sampleRate?: number;
  /** Number of log-spaced output bands: 1–128. Default 32. */
  bands?: number;
}
