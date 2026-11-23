import { describe, it, expect } from '@ohos/hypium';
import {
  SpectrumAnalyzer,
  computeWaveform,
  rms,
  peak,
  rmsDb,
  topPeaks,
  runSelfTest
} from '@quizzyjoy/audio-dsp';

const SAMPLE_RATE: number = 48000;
const FFT_SIZE: number = 1024;

function sine(freq: number, length: number, amplitude: number): Int16Array {
  const pcm = new Int16Array(length);
  const step = (2 * Math.PI * freq) / SAMPLE_RATE;
  for (let i = 0; i < length; i++) {
    pcm[i] = Math.round(Math.sin(step * i) * amplitude);
  }
  return pcm;
}

function silence(length: number): Int16Array {
  return new Int16Array(length);
}

export default function audioDspTest() {
  describe('audio-dsp native self test', () => {
    it('selfTest_passes', 0, () => {
      expect(runSelfTest()).assertTrue();
    });
  });

  describe('SpectrumAnalyzer contract', () => {
    it('returns_null_until_window_is_full', 0, () => {
      const analyzer = new SpectrumAnalyzer({ fftSize: FFT_SIZE, sampleRate: SAMPLE_RATE, bands: 16 });
      const half = analyzer.push(sine(440, FFT_SIZE / 2, 12000));
      expect(half === null).assertTrue();
      const frame = analyzer.push(sine(440, FFT_SIZE / 2, 12000));
      expect(frame !== null).assertTrue();
      expect(frame!.length).assertEqual(16);
      analyzer.release();
    });

    it('bands_are_normalized', 0, () => {
      const analyzer = new SpectrumAnalyzer({ fftSize: FFT_SIZE, sampleRate: SAMPLE_RATE, bands: 16 });
      const tone = sine(440, FFT_SIZE, 20000);
      let frame: Float32Array | null = null;
      for (let i = 0; i < 4; i++) {
        frame = analyzer.push(tone);
      }
      expect(frame !== null).assertTrue();
      const values = frame!;
      for (let i = 0; i < values.length; i++) {
        expect(values[i] >= 0).assertTrue();
        expect(values[i] <= 1.001).assertTrue();
      }
      analyzer.release();
    });

    it('440hz_lands_in_a_low_mid_band', 0, () => {
      const analyzer = new SpectrumAnalyzer({ fftSize: FFT_SIZE, sampleRate: SAMPLE_RATE, bands: 16 });
      const tone = sine(440, FFT_SIZE, 20000);
      let frame: Float32Array | null = null;
      for (let i = 0; i < 4; i++) {
        frame = analyzer.push(tone);
      }
      expect(frame !== null).assertTrue();
      const values = frame!;
      let peakBand = 0;
      for (let i = 1; i < values.length; i++) {
        if (values[i] > values[peakBand]) {
          peakBand = i;
        }
      }
      expect(peakBand >= 4).assertTrue();
      expect(peakBand <= 7).assertTrue();
      analyzer.release();
    });

    it('silence_gives_near_zero_bands', 0, () => {
      const analyzer = new SpectrumAnalyzer({ fftSize: FFT_SIZE, sampleRate: SAMPLE_RATE, bands: 16 });
      const tone = sine(440, FFT_SIZE, 20000);
      for (let i = 0; i < 4; i++) {
        analyzer.push(tone);
      }
      analyzer.reset();
      const quiet = silence(FFT_SIZE);
      let frame: Float32Array | null = null;
      for (let i = 0; i < 4; i++) {
        frame = analyzer.push(quiet);
      }
      expect(frame !== null).assertTrue();
      const values = frame!;
      for (let i = 0; i < values.length; i++) {
        expect(values[i] < 0.05).assertTrue();
      }
      analyzer.release();
    });

    it('released_analyzer_stops_producing_frames', 0, () => {
      const analyzer = new SpectrumAnalyzer({ fftSize: FFT_SIZE, sampleRate: SAMPLE_RATE, bands: 8 });
      analyzer.release();
      const frame = analyzer.push(sine(440, FFT_SIZE, 12000));
      expect(frame === null).assertTrue();
      expect(analyzer.ready).assertFalse();
    });
  });

  describe('SpectrumFrame and band split (0.2)', () => {
    it('440hz_is_dominant_in_mid', 0, () => {
      const analyzer = new SpectrumAnalyzer({ fftSize: FFT_SIZE, sampleRate: SAMPLE_RATE, bands: 32 });
      const tone = sine(440, FFT_SIZE, 20000);
      let frame = analyzer.pushFrame(tone);
      for (let i = 0; i < 3; i++) {
        frame = analyzer.pushFrame(tone);
      }
      expect(frame !== null).assertTrue();
      const detail = frame!;
      expect(detail.bands.length).assertEqual(32);
      expect(detail.mid > detail.bass).assertTrue();
      expect(detail.mid > detail.treble).assertTrue();
      analyzer.release();
    });

    it('high_tone_is_dominant_in_treble', 0, () => {
      const analyzer = new SpectrumAnalyzer({ fftSize: FFT_SIZE, sampleRate: SAMPLE_RATE, bands: 32 });
      const tone = sine(6000, FFT_SIZE, 20000);
      let frame = analyzer.pushFrame(tone);
      for (let i = 0; i < 3; i++) {
        frame = analyzer.pushFrame(tone);
      }
      expect(frame !== null).assertTrue();
      const detail = frame!;
      expect(detail.treble > detail.mid).assertTrue();
      expect(detail.treble > detail.bass).assertTrue();
      analyzer.release();
    });

    it('50_percent_overlap_produces_frames_twice_as_often', 0, () => {
      const analyzer = new SpectrumAnalyzer({ fftSize: FFT_SIZE, sampleRate: SAMPLE_RATE, bands: 16, overlap: 50 });
      const half = sine(440, FFT_SIZE / 2, 16000);
      const first = analyzer.push(half);
      const second = analyzer.push(half);
      expect(first !== null).assertTrue();
      expect(second !== null).assertTrue();
      analyzer.release();
    });
  });

  describe('waveform and levels', () => {
    it('waveform_has_two_values_per_point', 0, () => {
      const wave = computeWaveform(sine(440, FFT_SIZE, 20000), 100);
      expect(wave.length).assertEqual(200);
      for (let i = 0; i < wave.length; i++) {
        expect(wave[i] >= -1.001).assertTrue();
        expect(wave[i] <= 1.001).assertTrue();
      }
    });

    it('rms_and_peak_match_a_sine', 0, () => {
      const tone = sine(440, FFT_SIZE, 20000);
      const level = rms(tone);
      // 20000 / 32768 / sqrt(2) ≈ 0.4316
      expect(Math.abs(level - 0.4316) < 0.02).assertTrue();
      const peakValue = peak(tone);
      // 20000 / 32768 ≈ 0.6104
      expect(Math.abs(peakValue - 0.6104) < 0.02).assertTrue();
    });

    it('db_helpers_are_consistent', 0, () => {
      const tone = sine(440, FFT_SIZE, 20000);
      // 20 * log10(0.4316) ≈ -7.30
      expect(Math.abs(rmsDb(tone) - (-7.30)) < 0.5).assertTrue();
      expect(rmsDb(silence(FFT_SIZE))).assertEqual(-120);
    });

    it('top_peaks_are_sorted_descending', 0, () => {
      const analyzer = new SpectrumAnalyzer({ fftSize: FFT_SIZE, sampleRate: SAMPLE_RATE, bands: 32 });
      const tone = sine(440, FFT_SIZE, 20000);
      let frame = analyzer.push(tone);
      for (let i = 0; i < 3; i++) {
        frame = analyzer.push(tone);
      }
      expect(frame !== null).assertTrue();
      const peaks = topPeaks(frame!, 3);
      expect(peaks.length).assertEqual(3);
      expect(frame![peaks[0]] >= frame![peaks[1]]).assertTrue();
      expect(frame![peaks[1]] >= frame![peaks[2]]).assertTrue();
      analyzer.release();
    });
  });
}
