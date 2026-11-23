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
  /** Window overlap: 0 or 50 (percent). 50 gives ~2x frame rate and smoother motion. Default 0. */
  overlap?: number;
  /** Upper bound of the bass group in Hz. Default 250. */
  bassHz?: number;
  /** Lower bound of the treble group in Hz. Default 2000. */
  trebleHz?: number;
}

/**
 * One analyzed frame.
 *
 * `bands` are log-spaced energies in 0..1; `bass` / `mid` / `treble` are the
 * averages of the bands that fall below `bassHz`, between the crossovers, and
 * above `trebleHz`.
 */
export class SpectrumFrame {
  readonly bands: Float32Array;
  readonly bass: number;
  readonly mid: number;
  readonly treble: number;

  constructor(bands: Float32Array, bass: number, mid: number, treble: number) {
    this.bands = bands;
    this.bass = bass;
    this.mid = mid;
    this.treble = treble;
  }
}
