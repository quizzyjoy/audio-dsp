import { computeWaveformNative, computeRmsNative, computePeakNative } from 'libaudio_dsp.so';

const FLOOR: number = 1e-6;

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

/** Root-mean-square level in 0..1 (linear). */
export function rms(pcm: Int16Array): number {
  return computeRmsNative(pcm);
}

/** Absolute peak level in 0..1 (linear). */
export function peak(pcm: Int16Array): number {
  return computePeakNative(pcm);
}

/** RMS in dBFS (≤ 0). Silence returns -120. */
export function rmsDb(pcm: Int16Array): number {
  return linearToDb(computeRmsNative(pcm));
}

/** Peak in dBFS (≤ 0). Silence returns -120. */
export function peakDb(pcm: Int16Array): number {
  return linearToDb(computePeakNative(pcm));
}

/** Convert a linear 0..1 level to dBFS. */
export function linearToDb(value: number): number {
  const safe = value < FLOOR ? FLOOR : value;
  const db = 20 * Math.log10(safe);
  return db < -120 ? -120 : db;
}

/**
 * Indices of the `count` strongest bands, strongest first.
 * Useful for labelling peaks in a spectrum.
 */
export function topPeaks(bands: Float32Array, count: number): number[] {
  const result: number[] = [];
  if (count <= 0) {
    return result;
  }

  const used: boolean[] = [];
  for (let i = 0; i < bands.length; i++) {
    used.push(false);
  }

  const limit = count > bands.length ? bands.length : count;
  while (result.length < limit) {
    let bestIndex = -1;
    let bestValue = -1;
    for (let i = 0; i < bands.length; i++) {
      if (!used[i] && bands[i] > bestValue) {
        bestValue = bands[i];
        bestIndex = i;
      }
    }
    if (bestIndex < 0) {
      break;
    }
    used[bestIndex] = true;
    result.push(bestIndex);
  }
  return result;
}
