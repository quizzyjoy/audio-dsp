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
export const createLoudnessMeter: (sampleRate: number) => object | null;
export const pushLoudness: (handle: object, pcm: Int16Array, out: Float32Array) => boolean;
export const resetLoudness: (handle: object) => void;
export const destroyLoudness: (handle: object) => void;
export const createTempoEstimator: (frameIntervalMs: number) => object | null;
export const pushTempo: (handle: object, energy: number, out: Float32Array) => boolean;
export const resetTempo: (handle: object) => void;
export const destroyTempo: (handle: object) => void;
