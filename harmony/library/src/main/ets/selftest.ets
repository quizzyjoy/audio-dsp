import { selfTest } from 'libaudio_dsp.so';

/**
 * Run the built-in native sanity checks (440 Hz sine → expected band, silence,
 * waveform bounds, RMS/peak tolerance).
 *
 * Useful for on-device smoke tests and for demo pages that want to show a
 * "native module is healthy" badge.
 */
export function runSelfTest(): boolean {
  return selfTest();
}
