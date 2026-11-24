import { createTempoEstimator, pushTempo, resetTempo, destroyTempo } from 'libaudio_dsp.so';

/** Result of one tempo frame. */
export interface TempoReadout {
  /** Estimated BPM; 0 until at least two onsets are seen. */
  bpm: number;
  /** 0..1, grows with more consistent intervals. */
  confidence: number;
  /** Whether this frame was detected as an onset. */
  onset: boolean;
}

/**
 * Onset / BPM estimator fed with per-frame energy (e.g. `rms(chunk)`).
 *
 * ```ts
 * const tempo = new TempoEstimator(10);      // your frame interval in ms
 * const beat = tempo.push(rms(pcm));         // { bpm, confidence, onset }
 * ```
 */
export class TempoEstimator {
  private handle: object | null = null;

  constructor(frameIntervalMs?: number) {
    let interval: number = 10;
    if (frameIntervalMs !== undefined) {
      interval = frameIntervalMs;
    }
    this.handle = createTempoEstimator(interval);
  }

  get ready(): boolean {
    return this.handle !== null;
  }

  push(energy: number): TempoReadout {
    const handle = this.handle;
    if (handle === null) {
      return { bpm: 0, confidence: 0, onset: false };
    }
    const values = new Float32Array(3);
    pushTempo(handle, energy, values);
    return { bpm: values[0], confidence: values[1], onset: values[2] > 0.5 };
  }

  reset(): void {
    const handle = this.handle;
    if (handle !== null) {
      resetTempo(handle);
    }
  }

  release(): void {
    const handle = this.handle;
    if (handle !== null) {
      destroyTempo(handle);
      this.handle = null;
    }
  }
}
