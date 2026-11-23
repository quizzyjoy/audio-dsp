#include "audio_dsp.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace audiodsp {

namespace {
constexpr float kMinMagnitude = 1e-6f;
constexpr double kMinBandHz = 60.0;
constexpr double kMaxBandHz = 16000.0;

double bandEdge(int fftSize, int sampleRate, int bands, int index) {
    const double maxHz = std::max(kMinBandHz * 1.5,
                                  std::min(kMaxBandHz, static_cast<double>(sampleRate) / 2.0));
    return kMinBandHz * std::pow(maxHz / kMinBandHz, static_cast<double>(index) / bands);
}
}  // namespace

SpectrumAnalyzer::SpectrumAnalyzer(int fftSize, int sampleRate, int bands, int overlapPercent, double bassHz,
                                   double trebleHz)
    : fftSize_(fftSize), sampleRate_(sampleRate), bands_(bands), bassHz_(bassHz), trebleHz_(trebleHz) {
    if (fftSize_ < 64) {
        fftSize_ = 64;
    }
    if (sampleRate_ <= 0) {
        sampleRate_ = 48000;
    }
    if (bands_ < 1) {
        bands_ = 1;
    }
    if (bands_ > 128) {
        bands_ = 128;
    }

    const int overlap = (overlapPercent >= 50) ? 50 : 0;
    hop_ = overlap == 50 ? fftSize_ / 2 : fftSize_;
    if (hop_ < 1) {
        hop_ = 1;
    }

    cfg_ = kiss_fftr_alloc(fftSize_, 0, nullptr, nullptr);
    window_.resize(static_cast<size_t>(fftSize_));
    frame_.assign(static_cast<size_t>(fftSize_), 0.0f);
    work_.resize(static_cast<size_t>(fftSize_));
    freq_.resize(static_cast<size_t>(fftSize_ / 2 + 1));
    smoothed_.assign(static_cast<size_t>(bands_), 0.0f);

    for (int i = 0; i < fftSize_; ++i) {
        window_[static_cast<size_t>(i)] =
            0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * static_cast<float>(i) /
                                    static_cast<float>(fftSize_ - 1)));
    }
}

SpectrumAnalyzer::~SpectrumAnalyzer() {
    if (cfg_ != nullptr) {
        kiss_fftr_free(cfg_);
        cfg_ = nullptr;
    }
}

int SpectrumAnalyzer::push(const int16_t* samples, size_t count, float* outBands, float* outSplit) {
    if (samples == nullptr || count == 0 || cfg_ == nullptr) {
        return 0;
    }

    size_t offset = 0;
    while (offset < count) {
        const size_t space = static_cast<size_t>(fftSize_) - filled_;
        const size_t take = std::min(space, count - offset);
        for (size_t i = 0; i < take; ++i) {
            frame_[filled_ + i] = static_cast<float>(samples[offset + i]) / 32768.0f;
        }
        filled_ += take;
        offset += take;

        if (filled_ == static_cast<size_t>(fftSize_)) {
            process();
            if (outBands != nullptr) {
                std::copy(smoothed_.begin(), smoothed_.end(), outBands);
            }
            if (outSplit != nullptr) {
                outSplit[0] = bass_;
                outSplit[1] = mid_;
                outSplit[2] = treble_;
            }

            // slide the rolling window by hop_ (overlap = 0 keeps nothing)
            const size_t hop = static_cast<size_t>(hop_);
            const size_t keep = static_cast<size_t>(fftSize_) - hop;
            if (keep > 0) {
                std::memmove(frame_.data(), frame_.data() + hop, keep * sizeof(float));
            }
            filled_ = keep;
            return bands_;
        }
    }
    return 0;
}

void SpectrumAnalyzer::reset() {
    filled_ = 0;
    std::fill(frame_.begin(), frame_.end(), 0.0f);
    std::fill(smoothed_.begin(), smoothed_.end(), 0.0f);
    bass_ = 0.0f;
    mid_ = 0.0f;
    treble_ = 0.0f;
}

void SpectrumAnalyzer::process() {
    for (int i = 0; i < fftSize_; ++i) {
        work_[static_cast<size_t>(i)] = frame_[static_cast<size_t>(i)] * window_[static_cast<size_t>(i)];
    }

    kiss_fftr(cfg_, work_.data(), freq_.data());

    const int half = fftSize_ / 2;
    std::vector<float> mags(static_cast<size_t>(half));
    float maxMagnitude = kMinMagnitude;

    for (int i = 0; i < half; ++i) {
        const kiss_fft_cpx& c = freq_[static_cast<size_t>(i)];
        const float magnitude = std::sqrt(c.r * c.r + c.i * c.i) / static_cast<float>(fftSize_);
        mags[static_cast<size_t>(i)] = magnitude;
        maxMagnitude = std::max(maxMagnitude, magnitude);
    }

    const bool silent = maxMagnitude <= kMinMagnitude;

    float bassSum = 0.0f;
    float midSum = 0.0f;
    float trebleSum = 0.0f;
    int bassCount = 0;
    int midCount = 0;
    int trebleCount = 0;

    for (int band = 0; band < bands_; ++band) {
        const double f0 = bandEdge(fftSize_, sampleRate_, bands_, band);
        const double f1 = bandEdge(fftSize_, sampleRate_, bands_, band + 1);

        int bin0 = static_cast<int>(std::floor(f0 * fftSize_ / sampleRate_));
        int bin1 = static_cast<int>(std::ceil(f1 * fftSize_ / sampleRate_));
        bin0 = std::clamp(bin0, 1, half - 1);
        bin1 = std::clamp(bin1, bin0, half - 1);

        float sum = 0.0f;
        int count = 0;
        for (int i = bin0; i <= bin1; ++i) {
            sum += mags[static_cast<size_t>(i)];
            ++count;
        }

        float value = 0.0f;
        if (!silent && count > 0) {
            value = std::sqrt(std::min(1.0f, (sum / static_cast<float>(count)) / maxMagnitude));
        }

        const float previous = smoothed_[static_cast<size_t>(band)];
        smoothed_[static_cast<size_t>(band)] = previous * 0.6f + value * 0.4f;

        const double center = std::sqrt(f0 * f1);
        if (center < bassHz_) {
            bassSum += value;
            ++bassCount;
        } else if (center < trebleHz_) {
            midSum += value;
            ++midCount;
        } else {
            trebleSum += value;
            ++trebleCount;
        }
    }

    const float bassValue = bassCount > 0 ? bassSum / static_cast<float>(bassCount) : 0.0f;
    const float midValue = midCount > 0 ? midSum / static_cast<float>(midCount) : 0.0f;
    const float trebleValue = trebleCount > 0 ? trebleSum / static_cast<float>(trebleCount) : 0.0f;

    bass_ = bass_ * 0.6f + bassValue * 0.4f;
    mid_ = mid_ * 0.6f + midValue * 0.4f;
    treble_ = treble_ * 0.6f + trebleValue * 0.4f;
}

void computeWaveform(const int16_t* samples, size_t count, int points, float* out) {
    if (samples == nullptr || out == nullptr || count == 0 || points <= 0) {
        return;
    }

    const size_t perPoint = std::max<size_t>(1, count / static_cast<size_t>(points));
    for (int p = 0; p < points; ++p) {
        const size_t start = static_cast<size_t>(p) * perPoint;
        size_t end = start + perPoint;
        if (end > count) {
            end = count;
        }
        if (start >= end) {
            out[p * 2] = 0.0f;
            out[p * 2 + 1] = 0.0f;
            continue;
        }

        int16_t minValue = samples[start];
        int16_t maxValue = samples[start];
        for (size_t i = start; i < end; ++i) {
            minValue = std::min(minValue, samples[i]);
            maxValue = std::max(maxValue, samples[i]);
        }
        out[p * 2] = static_cast<float>(minValue) / 32768.0f;
        out[p * 2 + 1] = static_cast<float>(maxValue) / 32768.0f;
    }
}

float computeRms(const int16_t* samples, size_t count) {
    if (samples == nullptr || count == 0) {
        return 0.0f;
    }
    double sum = 0.0;
    for (size_t i = 0; i < count; ++i) {
        const double v = static_cast<double>(samples[i]) / 32768.0;
        sum += v * v;
    }
    return static_cast<float>(std::sqrt(sum / static_cast<double>(count)));
}

float computePeak(const int16_t* samples, size_t count) {
    if (samples == nullptr || count == 0) {
        return 0.0f;
    }
    int16_t peakValue = 0;
    for (size_t i = 0; i < count; ++i) {
        const int16_t v = samples[i] < 0 ? static_cast<int16_t>(-samples[i]) : samples[i];
        peakValue = std::max(peakValue, v);
    }
    return static_cast<float>(peakValue) / 32768.0f;
}

bool runSelfTest() {
    constexpr int kFft = 1024;
    constexpr int kBands = 32;
    constexpr int kSampleRate = 48000;

    SpectrumAnalyzer analyzer(kFft, kSampleRate, kBands, 0, 250.0, 2000.0);
    std::vector<int16_t> sine(static_cast<size_t>(kFft));
    const double step = 2.0 * M_PI * 440.0 / static_cast<double>(kSampleRate);
    for (int i = 0; i < kFft; ++i) {
        sine[static_cast<size_t>(i)] = static_cast<int16_t>(std::lround(std::sin(step * i) * 20000.0));
    }

    std::vector<float> bands(static_cast<size_t>(kBands), 0.0f);
    float split[3] = {0.0f, 0.0f, 0.0f};
    int produced = 0;
    for (int k = 0; k < 4; ++k) {
        produced = analyzer.push(sine.data(), sine.size(), bands.data(), split);
    }
    if (produced != kBands) {
        return false;
    }

    int peakBand = 0;
    for (int i = 1; i < kBands; ++i) {
        if (bands[static_cast<size_t>(i)] > bands[static_cast<size_t>(peakBand)]) {
            peakBand = i;
        }
    }
    if (peakBand < 8 || peakBand > 15) {
        return false;
    }
    for (float v : bands) {
        if (v < 0.0f || v > 1.0001f) {
            return false;
        }
    }

    // 440 Hz belongs to the "mid" group for crossovers 250 / 2000 Hz
    if (!(split[1] > split[0] && split[1] > split[2])) {
        return false;
    }
    for (float v : split) {
        if (v < 0.0f || v > 1.0001f) {
            return false;
        }
    }

    analyzer.reset();
    std::vector<int16_t> silence(static_cast<size_t>(kFft), 0);
    for (int k = 0; k < 4; ++k) {
        analyzer.push(silence.data(), silence.size(), bands.data(), split);
    }
    for (float v : bands) {
        if (v > 0.05f) {
            return false;
        }
    }

    std::vector<float> wave(200, 0.0f);
    computeWaveform(sine.data(), sine.size(), 100, wave.data());
    for (float v : wave) {
        if (v < -1.01f || v > 1.01f) {
            return false;
        }
    }

    const float rmsValue = computeRms(sine.data(), sine.size());
    if (std::fabs(rmsValue - 0.4316f) > 0.03f) {
        return false;
    }
    const float peakValue = computePeak(sine.data(), sine.size());
    if (std::fabs(peakValue - 0.6104f) > 0.03f) {
        return false;
    }
    return true;
}

}  // namespace audiodsp
