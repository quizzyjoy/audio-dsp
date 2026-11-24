#include <napi/native_api.h>

#include <cstring>
#include <vector>

#include "audio_dsp.h"

namespace {

using audiodsp::SpectrumAnalyzer;

void FinalizeAnalyzer(napi_env /*env*/, void* data, void* /*hint*/) {
    delete static_cast<SpectrumAnalyzer*>(data);
}

bool GetInt16Array(napi_env env, napi_value value, const int16_t** data, size_t* length) {
    napi_typedarray_type type;
    napi_value arrayBuffer = nullptr;
    void* raw = nullptr;
    size_t byteOffset = 0;
    size_t elementLength = 0;

    if (napi_get_typedarray_info(env, value, &type, &elementLength, &raw, &arrayBuffer, &byteOffset) != napi_ok) {
        return false;
    }
    if (type != napi_int16_array || raw == nullptr) {
        return false;
    }
    *data = static_cast<const int16_t*>(raw);
    *length = elementLength;
    return true;
}

SpectrumAnalyzer* Unwrap(napi_env env, napi_value value) {
    void* data = nullptr;
    if (napi_get_value_external(env, value, &data) != napi_ok) {
        return nullptr;
    }
    return static_cast<SpectrumAnalyzer*>(data);
}

napi_value MakeFloat32Array(napi_env env, const float* values, size_t length) {
    napi_value arrayBuffer = nullptr;
    void* bufferData = nullptr;
    if (napi_create_arraybuffer(env, length * sizeof(float), &bufferData, &arrayBuffer) != napi_ok) {
        napi_value undefined = nullptr;
        napi_get_undefined(env, &undefined);
        return undefined;
    }
    if (length > 0) {
        std::memcpy(bufferData, values, length * sizeof(float));
    }

    napi_value result = nullptr;
    if (napi_create_typedarray(env, napi_float32_array, length, arrayBuffer, 0, &result) != napi_ok) {
        napi_value undefined = nullptr;
        napi_get_undefined(env, &undefined);
        return undefined;
    }
    return result;
}

napi_value MakeNumber(napi_env env, double value) {
    napi_value result = nullptr;
    napi_create_double(env, value, &result);
    return result;
}

void SetNumberProperty(napi_env env, napi_value object, const char* name, double value) {
    napi_set_named_property(env, object, name, MakeNumber(env, value));
}

napi_value NativeCreateAnalyzer(napi_env env, napi_callback_info info) {
    size_t argc = 6;
    napi_value args[6] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    int32_t fftSize = 1024;
    int32_t sampleRate = 48000;
    int32_t bands = 32;
    int32_t overlapPercent = 0;
    double bassHz = 250.0;
    double trebleHz = 2000.0;

    if (argc > 0) {
        napi_get_value_int32(env, args[0], &fftSize);
    }
    if (argc > 1) {
        napi_get_value_int32(env, args[1], &sampleRate);
    }
    if (argc > 2) {
        napi_get_value_int32(env, args[2], &bands);
    }
    if (argc > 3) {
        napi_get_value_int32(env, args[3], &overlapPercent);
    }
    if (argc > 4) {
        napi_get_value_double(env, args[4], &bassHz);
    }
    if (argc > 5) {
        napi_get_value_double(env, args[5], &trebleHz);
    }

    auto* analyzer = new SpectrumAnalyzer(fftSize, sampleRate, bands, overlapPercent, bassHz, trebleHz);
    napi_value external = nullptr;
    if (napi_create_external(env, analyzer, FinalizeAnalyzer, nullptr, &external) != napi_ok) {
        delete analyzer;
        napi_value nullValue = nullptr;
        napi_get_null(env, &nullValue);
        return nullValue;
    }
    return external;
}

napi_value NativePushFrame(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    napi_value nullValue = nullptr;
    napi_get_null(env, &nullValue);
    if (argc < 2) {
        return nullValue;
    }

    SpectrumAnalyzer* analyzer = Unwrap(env, args[0]);
    if (analyzer == nullptr) {
        return nullValue;
    }

    const int16_t* samples = nullptr;
    size_t length = 0;
    if (!GetInt16Array(env, args[1], &samples, &length)) {
        return nullValue;
    }

    std::vector<float> bands(128, 0.0f);
    float split[3] = {0.0f, 0.0f, 0.0f};
    const int produced = analyzer->push(samples, length, bands.data(), split);
    if (produced <= 0) {
        return nullValue;
    }
    if (argc > 2) {
        napi_typedarray_type type;
        napi_value arrayBuffer = nullptr;
        void* raw = nullptr;
        size_t byteOffset = 0;
        size_t splitLength = 0;
        if (napi_get_typedarray_info(env, args[2], &type, &splitLength, &raw, &arrayBuffer, &byteOffset) == napi_ok &&
            type == napi_float32_array && raw != nullptr && splitLength >= 3) {
            auto* out = static_cast<float*>(raw);
            out[0] = split[0];
            out[1] = split[1];
            out[2] = split[2];
        }
    }
    return MakeFloat32Array(env, bands.data(), static_cast<size_t>(produced));
}

napi_value NativeResetAnalyzer(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc >= 1) {
        SpectrumAnalyzer* analyzer = Unwrap(env, args[0]);
        if (analyzer != nullptr) {
            analyzer->reset();
        }
    }
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    return undefined;
}

napi_value NativeDestroyAnalyzer(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc >= 1) {
        SpectrumAnalyzer* analyzer = Unwrap(env, args[0]);
        delete analyzer;
    }
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    return undefined;
}

napi_value NativeComputeWaveform(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    const int16_t* samples = nullptr;
    size_t length = 0;
    if (argc < 2 || !GetInt16Array(env, args[0], &samples, &length)) {
        return MakeFloat32Array(env, nullptr, 0);
    }

    int32_t points = 0;
    napi_get_value_int32(env, args[1], &points);
    if (points <= 0) {
        return MakeFloat32Array(env, nullptr, 0);
    }
    if (points > 8192) {
        points = 8192;
    }

    std::vector<float> waveform(static_cast<size_t>(points) * 2, 0.0f);
    audiodsp::computeWaveform(samples, length, points, waveform.data());
    return MakeFloat32Array(env, waveform.data(), waveform.size());
}

napi_value NativeComputeRms(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    const int16_t* samples = nullptr;
    size_t length = 0;
    if (argc < 1 || !GetInt16Array(env, args[0], &samples, &length)) {
        return MakeNumber(env, 0.0);
    }
    return MakeNumber(env, audiodsp::computeRms(samples, length));
}

napi_value NativeComputePeak(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    const int16_t* samples = nullptr;
    size_t length = 0;
    if (argc < 1 || !GetInt16Array(env, args[0], &samples, &length)) {
        return MakeNumber(env, 0.0);
    }
    return MakeNumber(env, audiodsp::computePeak(samples, length));
}

napi_value NativeSelfTest(napi_env env, napi_callback_info info) {
    (void)info;
    napi_value result = nullptr;
    napi_get_boolean(env, audiodsp::runSelfTest(), &result);
    return result;
}

void FinalizeLoudness(napi_env /*env*/, void* data, void* /*hint*/) {
    delete static_cast<audiodsp::LoudnessMeter*>(data);
}

void FinalizeTempo(napi_env /*env*/, void* data, void* /*hint*/) {
    delete static_cast<audiodsp::TempoEstimator*>(data);
}

audiodsp::LoudnessMeter* UnwrapLoudness(napi_env env, napi_value value) {
    void* data = nullptr;
    if (napi_get_value_external(env, value, &data) != napi_ok) {
        return nullptr;
    }
    return static_cast<audiodsp::LoudnessMeter*>(data);
}

audiodsp::TempoEstimator* UnwrapTempo(napi_env env, napi_value value) {
    void* data = nullptr;
    if (napi_get_value_external(env, value, &data) != napi_ok) {
        return nullptr;
    }
    return static_cast<audiodsp::TempoEstimator*>(data);
}

napi_value NativeCreateLoudness(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    int32_t sampleRate = 48000;
    if (argc > 0) {
        napi_get_value_int32(env, args[0], &sampleRate);
    }

    auto* meter = new audiodsp::LoudnessMeter(sampleRate);
    napi_value external = nullptr;
    if (napi_create_external(env, meter, FinalizeLoudness, nullptr, &external) != napi_ok) {
        delete meter;
        napi_value nullValue = nullptr;
        napi_get_null(env, &nullValue);
        return nullValue;
    }
    return external;
}

napi_value NativePushLoudness(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    napi_value falseValue = nullptr;
    napi_get_boolean(env, false, &falseValue);
    if (argc < 3) {
        return falseValue;
    }

    audiodsp::LoudnessMeter* meter = UnwrapLoudness(env, args[0]);
    if (meter == nullptr) {
        return falseValue;
    }

    const int16_t* samples = nullptr;
    size_t length = 0;
    if (!GetInt16Array(env, args[1], &samples, &length)) {
        return falseValue;
    }

    float out[3] = {-120.0f, -120.0f, -120.0f};
    const bool produced = meter->push(samples, length, out);

    napi_typedarray_type type;
    napi_value arrayBuffer = nullptr;
    void* raw = nullptr;
    size_t byteOffset = 0;
    size_t outLength = 0;
    if (napi_get_typedarray_info(env, args[2], &type, &outLength, &raw, &arrayBuffer, &byteOffset) == napi_ok &&
        type == napi_float32_array && raw != nullptr && outLength >= 3) {
        auto* target = static_cast<float*>(raw);
        target[0] = out[0];
        target[1] = out[1];
        target[2] = out[2];
    }

    napi_value result = nullptr;
    napi_get_boolean(env, produced, &result);
    return result;
}

napi_value NativeResetLoudness(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc >= 1) {
        audiodsp::LoudnessMeter* meter = UnwrapLoudness(env, args[0]);
        if (meter != nullptr) {
            meter->reset();
        }
    }
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    return undefined;
}

napi_value NativeDestroyLoudness(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc >= 1) {
        delete UnwrapLoudness(env, args[0]);
    }
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    return undefined;
}

napi_value NativeCreateTempo(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    double frameIntervalMs = 10.0;
    if (argc > 0) {
        napi_get_value_double(env, args[0], &frameIntervalMs);
    }

    auto* tempo = new audiodsp::TempoEstimator(frameIntervalMs);
    napi_value external = nullptr;
    if (napi_create_external(env, tempo, FinalizeTempo, nullptr, &external) != napi_ok) {
        delete tempo;
        napi_value nullValue = nullptr;
        napi_get_null(env, &nullValue);
        return nullValue;
    }
    return external;
}

napi_value NativePushTempo(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    napi_value falseValue = nullptr;
    napi_get_boolean(env, false, &falseValue);
    if (argc < 3) {
        return falseValue;
    }

    audiodsp::TempoEstimator* tempo = UnwrapTempo(env, args[0]);
    if (tempo == nullptr) {
        return falseValue;
    }

    double energy = 0.0;
    napi_get_value_double(env, args[1], &energy);

    float out[3] = {0.0f, 0.0f, 0.0f};
    tempo->push(static_cast<float>(energy), out);

    napi_typedarray_type type;
    napi_value arrayBuffer = nullptr;
    void* raw = nullptr;
    size_t byteOffset = 0;
    size_t outLength = 0;
    if (napi_get_typedarray_info(env, args[2], &type, &outLength, &raw, &arrayBuffer, &byteOffset) == napi_ok &&
        type == napi_float32_array && raw != nullptr && outLength >= 3) {
        auto* target = static_cast<float*>(raw);
        target[0] = out[0];
        target[1] = out[1];
        target[2] = out[2];
    }

    napi_value result = nullptr;
    napi_get_boolean(env, true, &result);
    return result;
}

napi_value NativeResetTempo(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc >= 1) {
        audiodsp::TempoEstimator* tempo = UnwrapTempo(env, args[0]);
        if (tempo != nullptr) {
            tempo->reset();
        }
    }
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    return undefined;
}

napi_value NativeDestroyTempo(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (argc >= 1) {
        delete UnwrapTempo(env, args[0]);
    }
    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    return undefined;
}

}  // namespace

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"createAnalyzer", nullptr, NativeCreateAnalyzer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pushFrame", nullptr, NativePushFrame, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"resetAnalyzer", nullptr, NativeResetAnalyzer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"destroyAnalyzer", nullptr, NativeDestroyAnalyzer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"computeWaveformNative", nullptr, NativeComputeWaveform, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"computeRmsNative", nullptr, NativeComputeRms, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"computePeakNative", nullptr, NativeComputePeak, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"selfTest", nullptr, NativeSelfTest, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"createLoudnessMeter", nullptr, NativeCreateLoudness, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pushLoudness", nullptr, NativePushLoudness, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"resetLoudness", nullptr, NativeResetLoudness, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"destroyLoudness", nullptr, NativeDestroyLoudness, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"createTempoEstimator", nullptr, NativeCreateTempo, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"pushTempo", nullptr, NativePushTempo, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"resetTempo", nullptr, NativeResetTempo, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"destroyTempo", nullptr, NativeDestroyTempo, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module audioDspModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "audio_dsp",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterAudioDspModule(void) {
    napi_module_register(&audioDspModule);
}
