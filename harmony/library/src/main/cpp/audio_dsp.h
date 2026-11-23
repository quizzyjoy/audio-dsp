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

}  // namespace audiodsp

#endif  // AUDIO_DSP_H
