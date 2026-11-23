#ifndef AUDIO_DSP_H
#define AUDIO_DSP_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "kiss_fftr.h"

namespace audiodsp {

/** Real-input FFT spectrum analyzer with log-spaced band aggregation. */
class SpectrumAnalyzer {
public:
    SpectrumAnalyzer(int fftSize, int sampleRate, int bands);
    ~SpectrumAnalyzer();

    SpectrumAnalyzer(const SpectrumAnalyzer&) = delete;
    SpectrumAnalyzer& operator=(const SpectrumAnalyzer&) = delete;

    /** Feed mono S16LE samples. Returns the band count when a frame was produced, else 0. */
    int push(const int16_t* samples, size_t count, float* outBands);

    void reset();

private:
    void process();

    int fftSize_ = 1024;
    int sampleRate_ = 48000;
    int bands_ = 32;

    kiss_fftr_cfg cfg_ = nullptr;
    std::vector<float> window_;
    std::vector<float> input_;
    std::vector<kiss_fft_cpx> freq_;
    std::vector<float> smoothed_;
    size_t filled_ = 0;
};

/** min/max envelope; writes 2 * points values into out (min0, max0, min1, max1, ...). */
void computeWaveform(const int16_t* samples, size_t count, int points, float* out);

float computeRms(const int16_t* samples, size_t count);

float computePeak(const int16_t* samples, size_t count);

/** Runs the built-in sanity checks (sine 440 Hz, silence, waveform, levels). */
bool runSelfTest();

}  // namespace audiodsp

#endif  // AUDIO_DSP_H
