import { createAnalyzer, pushFrame, resetAnalyzer, destroyAnalyzer } from 'libaudio_dsp.so';
import { SpectrumOptions } from './types';

const DEFAULT_FFT_SIZE: number = 1024;
const DEFAULT_SAMPLE_RATE: number = 48000;
const DEFAULT_BANDS: number = 32;

/**
 * Real-time spectrum analyzer.
 *
 * Feed it mono S16LE PCM chunks (e.g. from AudioCapturer after down-mixing);
 * every time a full FFT window has been collected it returns one frame of
 * log-spaced band energies in 0..1.
 *
 * ```ts
 * const analyzer = new SpectrumAnalyzer({ bands: 32 });
 * const frame = analyzer.push(pcm);   // Float32Array(32) | null
 * ```
 */
export class SpectrumAnalyzer {
  private handle: object | null = null;
  private readonly bandCount: number;

  constructor(options?: SpectrumOptions) {
    let fftSize: number = DEFAULT_FFT_SIZE;
    let sampleRate: number = DEFAULT_SAMPLE_RATE;
    let bands: number = DEFAULT_BANDS;

    if (options !== undefined) {
      if (options.fftSize !== undefined) {
        fftSize = options.fftSize;
      }
      if (options.sampleRate !== undefined) {
        sampleRate = options.sampleRate;
      }
      if (options.bands !== undefined) {
        bands = options.bands;
      }
    }

    this.bandCount = bands;
    this.handle = createAnalyzer(fftSize, sampleRate, bands);
  }

  /** Number of bands produced by each frame. */
  get bands(): number {
    return this.bandCount;
  }

  /** Whether the native analyzer is alive. */
  get ready(): boolean {
    return this.handle !== null;
  }

  /**
   * Push mono S16LE samples.
   * @returns one frame of band energies (0..1) when a window completed, otherwise null.
   */
  push(pcm: Int16Array): Float32Array | null {
    const handle = this.handle;
    if (handle === null) {
      return null;
    }
    return pushFrame(handle, pcm);
  }

  /** Drop buffered samples and smoothing state. */
  reset(): void {
    const handle = this.handle;
    if (handle !== null) {
      resetAnalyzer(handle);
    }
  }

  /** Release native resources. The instance must not be used afterwards. */
  release(): void {
    const handle = this.handle;
    if (handle !== null) {
      destroyAnalyzer(handle);
      this.handle = null;
    }
  }
}
