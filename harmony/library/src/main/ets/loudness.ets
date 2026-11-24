import { createLoudnessMeter, pushLoudness, resetLoudness, destroyLoudness } from 'libaudio_dsp.so';

const FLOOR: number = -120;

/** One loudness readout, all values in LUFS. */
export interface LoudnessReadout {
  /** 400 ms window. */
  momentary: number;
  /** 3 s window. */
  shortTerm: number;
  /** Gated integrated loudness since construction / last reset. */
  integrated: number;
}

/**
 * ITU-R BS.1770-4 style loudness meter (K-weighting, 400 ms blocks, 75% overlap, gated integration).
 *
 * ```ts
 * const meter = new LoudnessMeter(48000);
 * const lufs = meter.push(pcm);   // { momentary, shortTerm, integrated }
 * ```
 */
export class LoudnessMeter {
  private handle: object | null = null;

  constructor(sampleRate?: number) {
    let rate: number = 48000;
    if (sampleRate !== undefined) {
      rate = sampleRate;
    }
    this.handle = createLoudnessMeter(rate);
  }

  get ready(): boolean {
    return this.handle !== null;
  }

  /** Push mono S16LE samples. Values are the latest known ones (updated每 100 ms). */
  push(pcm: Int16Array): LoudnessReadout {
    const handle = this.handle;
    if (handle === null) {
      return { momentary: FLOOR, shortTerm: FLOOR, integrated: FLOOR };
    }
    const values = new Float32Array(3);
    pushLoudness(handle, pcm, values);
    return { momentary: values[0], shortTerm: values[1], integrated: values[2] };
  }

  reset(): void {
    const handle = this.handle;
    if (handle !== null) {
      resetLoudness(handle);
    }
  }

  release(): void {
    const handle = this.handle;
    if (handle !== null) {
      destroyLoudness(handle);
      this.handle = null;
    }
  }
}
