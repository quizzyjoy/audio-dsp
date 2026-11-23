import { createAnalyzer, pushFrame as nativePushFrame, resetAnalyzer, destroyAnalyzer } from 'libaudio_dsp.so';
import { SpectrumFrame, SpectrumOptions } from './types';

const DEFAULT_FFT_SIZE: number = 1024;
const DEFAULT_SAMPLE_RATE: number = 48000;
const DEFAULT_BANDS: number = 32;
const DEFAULT_OVERLAP: number = 0;
const DEFAULT_BASS_HZ: number = 250;
const DEFAULT_TREBLE_HZ: number = 2000;

/**
 * Real-time spectrum analyzer.
 *
 * Feed it mono S16LE PCM chunks (e.g. from AudioCapturer after down-mixing).
 * Every time a full window has been collected it returns one frame of
 * log-spaced band energies plus bass/mid/treble group levels.
 *
 * ```ts
 * const analyzer = new SpectrumAnalyzer({ bands: 32, overlap: 50 });
 * const frame = analyzer.pushFrame(pcm);   // SpectrumFrame | null
 * if (frame !== null) draw(frame.bands, frame.bass, frame.mid, frame.treble);
 * ```
 */
export class SpectrumAnalyzer {
  private handle: object | null = null;
  private readonly bandCount: number;

  constructor(options?: SpectrumOptions) {
    let fftSize: number = DEFAULT_FFT_SIZE;
    let sampleRate: number = DEFAULT_SAMPLE_RATE;
    let bands: number = DEFAULT_BANDS;
    let overlap: number = DEFAULT_OVERLAP;
    let bassHz: number = DEFAULT_BASS_HZ;
    let trebleHz: number = DEFAULT_TREBLE_HZ;

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
      if (options.overlap !== undefined) {
        overlap = options.overlap;
      }
      if (options.bassHz !== undefined) {
        bassHz = options.bassHz;
      }
      if (options.trebleHz !== undefined) {
        trebleHz = options.trebleHz;
      }
    }

    this.bandCount = bands;
    this.handle = createAnalyzer(fftSize, sampleRate, bands, overlap, bassHz, trebleHz);
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
   * Push mono S16LE samples and return the band energies only.
   * @returns Float32Array(bands) when a window completed, otherwise null.
   */
  push(pcm: Int16Array): Float32Array | null {
    const handle = this.handle;
    if (handle === null) {
      return null;
    }
    return nativePushFrame(handle, pcm);
  }

  /**
   * Push mono S16LE samples and return bands + bass/mid/treble in one pass.
   * @returns SpectrumFrame when a window completed, otherwise null.
   */
  pushFrame(pcm: Int16Array): SpectrumFrame | null {
    const handle = this.handle;
    if (handle === null) {
      return null;
    }
    const split = new Float32Array(3);
    const bands = nativePushFrame(handle, pcm, split);
    if (bands === null) {
      return null;
    }
    return new SpectrumFrame(bands, split[0], split[1], split[2]);
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
