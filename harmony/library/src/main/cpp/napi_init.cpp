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
    std::memcpy(bufferData, values, length * sizeof(float));

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

napi_value NativeCreateAnalyzer(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    int32_t fftSize = 1024;
    int32_t sampleRate = 48000;
    int32_t bands = 32;
    if (argc > 0) {
        napi_get_value_int32(env, args[0], &fftSize);
    }
    if (argc > 1) {
        napi_get_value_int32(env, args[1], &sampleRate);
    }
    if (argc > 2) {
        napi_get_value_int32(env, args[2], &bands);
    }

    auto* analyzer = new SpectrumAnalyzer(fftSize, sampleRate, bands);
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
    size_t argc = 2;
    napi_value args[2] = {nullptr};
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

    std::vector<float> bands(128);
    const int produced = analyzer->push(samples, length, bands.data());
    if (produced <= 0) {
        return nullValue;
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
