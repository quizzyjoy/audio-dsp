export const createAnalyzer: (
  fftSize: number,
  sampleRate: number,
  bands: number,
  overlapPercent: number,
  bassHz: number,
  trebleHz: number
) => object | null;
export const pushFrame: (handle: object, pcm: Int16Array, split?: Float32Array) => Float32Array | null;
export const resetAnalyzer: (handle: object) => void;
export const destroyAnalyzer: (handle: object) => void;
export const computeWaveformNative: (pcm: Int16Array, points: number) => Float32Array;
export const computeRmsNative: (pcm: Int16Array) => number;
export const computePeakNative: (pcm: Int16Array) => number;
export const selfTest: () => boolean;
