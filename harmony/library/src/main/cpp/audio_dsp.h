#ifndef AUDIO_DSP_H
#define AUDIO_DSP_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "kiss_fftr.h"

namespace audiodsp {

/** Real-input FFT spectrum analyzer with log-spaced bands and bass/mid/treble split. */
class SpectrumAnalyzer {
public:
    /**
     * @param overlapPercent 0 (no overlap) or 50 (50% overlap between windows)
     * @param bassHz upper bound of the "bass" group
     * @param trebleHz lower bound of the "treble" group
     */
    SpectrumAnalyzer(int fftSize, int sampleRate, int bands, int overlapPercent, double bassHz, double trebleHz);
    ~SpectrumAnalyzer();

    SpectrumAnalyzer(const SpectrumAnalyzer&) = delete;
    SpectrumAnalyzer& operator=(const SpectrumAnalyzer&) = delete;

    /**
     * Feed mono S16LE samples.
     * @param outBands receives `bands()` floats (0..1) when a frame is produced
     * @param outSplit receives 3 floats: bass, mid, treble (0..1)
     * @return the band count when a frame was produced, else 0
     */
    int push(const int16_t* samples, size_t count, float* outBands, float* outSplit);

    void reset();

    int bands() const {
        return bands_;
    }

private:
    void process();

    int fftSize_ = 1024;
    int sampleRate_ = 48000;
    int bands_ = 32;
    int hop_ = 1024;
    double bassHz_ = 250.0;
    double trebleHz_ = 2000.0;

    kiss_fftr_cfg cfg_ = nullptr;
    std::vector<float> window_;
    std::vector<float> frame_;
    std::vector<float> work_;
    std::vector<kiss_fft_cpx> freq_;
    std::vector<float> smoothed_;
    size_t filled_ = 0;

    float bass_ = 0.0f;
    float mid_ = 0.0f;
    float treble_ = 0.0f;
};

/** min/max envelope; writes 2 * points values into out (min0, max0, min1, max1, ...). */
void computeWaveform(const int16_t* samples, size_t count, int points, float* out);

float computeRms(const int16_t* samples, size_t count);

float computePeak(const int16_t* samples, size_t count);

/** Runs the built-in sanity checks (sine 440 Hz, silence, waveform, levels). */
bool runSelfTest();

/**
 * ITU-R BS.1770-4 style loudness meter (K-weighting + 400 ms gating).
 * Mono input; 400 ms blocks with 75% overlap.
 */
class LoudnessMeter {
public:
    explicit LoudnessMeter(int sampleRate);
    ~LoudnessMeter();

    /**
     * @param out receives 3 floats: momentary (400 ms), short-term (3 s), integrated (gated)
     * @return true when a new 400 ms block was completed
     */
    bool push(const int16_t* samples, size_t count, float* out);

    void reset();

private:
    double computeBlockLufs() const;

    int sampleRate_ = 48000;
    double b0s_ = 1.0, b1s_ = 0.0, b2s_ = 0.0, a1s_ = 0.0, a2s_ = 0.0;
    double b0h_ = 1.0, b1h_ = 0.0, b2h_ = 0.0, a1h_ = 0.0, a2h_ = 0.0;

    double sx1_ = 0, sx2_ = 0, sy1_ = 0, sy2_ = 0;
    double hx1_ = 0, hx2_ = 0, hy1_ = 0, hy2_ = 0;

    std::vector<double> ring_;
    size_t ringPos_ = 0;
    size_t ringCount_ = 0;
    double ringSum_ = 0.0;
    size_t blockTarget_ = 0;
    size_t stepTarget_ = 0;
    size_t sinceBlock_ = 0;

    std::vector<double> blockLufs_;
    float momentary_ = -120.0f;
    float shortTerm_ = -120.0f;
    float integrated_ = -120.0f;
};

/** Onset + BPM estimator fed with per-frame energy. */
class TempoEstimator {
public:
    explicit TempoEstimator(double frameIntervalMs);

    /**
     * @param energy frame energy (0..1)
     * @param out receives 3 floats: bpm, confidence (0..1), onset (0 or 1)
     * @return true when out was updated
     */
    bool push(float energy, float* out);

    void reset();

private:
    double frameIntervalMs_ = 10.0;
    double nowMs_ = 0.0;
    double lastOnsetMs_ = -10000.0;
    float previousEnergy_ = 0.0f;
    std::vector<float> history_;
    std::vector<double> intervals_;
    float bpm_ = 0.0f;
    float confidence_ = 0.0f;
};

}  // namespace audiodsp

#endif  // AUDIO_DSP_H
