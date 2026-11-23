import { computeWaveformNative, computeRmsNative, computePeakNative } from 'libaudio_dsp.so';

/**
 * Min/max envelope for drawing a waveform.
 *
 * @param pcm mono S16LE samples
 * @param points number of horizontal points (1–8192)
 * @returns `2 * points` floats: `[min0, max0, min1, max1, ...]`, each in -1..1
 */
export function computeWaveform(pcm: Int16Array, points: number): Float32Array {
  return computeWaveformNative(pcm, points);
}

/** Root-mean-square level in 0..1 (not dBFS). */
export function rms(pcm: Int16Array): number {
  return computeRmsNative(pcm);
}

/** Absolute peak level in 0..1. */
export function peak(pcm: Int16Array): number {
  return computePeakNative(pcm);
}
