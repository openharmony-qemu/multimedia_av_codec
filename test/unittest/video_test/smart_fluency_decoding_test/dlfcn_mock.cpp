/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <cstdint>
#include "smart_fluency_decoding_types.h"

#define NALU_MOCK_HANDLE ((void*)0x9999)
#define MV_MOCK_HANDLE   ((void*)0x8888)

extern "C" {
    void* __real_dlopen(const char* filename, int flags);
    void* __real_dlsym(void* handle, const char* symbol);
    int __real_dlclose(void* handle);

    void* FakeCreateNaluAnalyzer(int32_t codecType) { return NALU_MOCK_HANDLE; }
    void FakeDestroyNaluAnalyzer(void* analyzer) {}
    void FakeSetCallback(void* analyzer, void* cb, void* userData) {}
    bool FakeIsNonReferenceFrame(void* analyzer, const uint8_t* bufferAddr, uint32_t bufferSize) { 
        return true;
    }

    void* FakeCreateMvAnalyzer(int32_t codecType) { return MV_MOCK_HANDLE; }
    void FakeDestroyMvAnalyzer(void* parser) {}
    
    void FakeSetStreamAuxInfo(void* parser, const StreamAuxInfo* info) {
        (void)parser;
        (void)info;
    }
    
    void FakeSetVideoInfo(void* parser, uint32_t width, uint32_t height) {
        (void)parser;
        (void)width;
        (void)height;
    }
    
    MVStats FakeParseMvData(void* parser, const uint8_t* buffer) {
        (void)parser;
        (void)buffer;
        MVStats stats{};
        stats.validBlocks = 100;
        return stats;
    }

    void* __wrap_dlopen(const char* filename, int flags) {
        if (filename != nullptr) {
            std::string fname(filename);
            if (fname.find("libvideo_nalu_analyzer.z.so") != std::string::npos) {
                return NALU_MOCK_HANDLE;
            }
            if (fname.find("libvideo_mv_analyzer.z.so") != std::string::npos) {
                return MV_MOCK_HANDLE;
            }
        }
        return __real_dlopen(filename, flags);
    }

    void* __wrap_dlsym(void* handle, const char* symbol) {
        std::string sym(symbol);
        if (handle == NALU_MOCK_HANDLE) {
            if (sym == "CreateNaluAnalyzer") return (void*)FakeCreateNaluAnalyzer;
            if (sym == "DestroyNaluAnalyzer") return (void*)FakeDestroyNaluAnalyzer;
            if (sym == "SetCallback") return (void*)FakeSetCallback;
            if (sym == "IsNonReferenceFrame") return (void*)FakeIsNonReferenceFrame;
        } else if (handle == MV_MOCK_HANDLE) {
            if (sym == "CreateMvAnalyzer") return (void*)FakeCreateMvAnalyzer;
            if (sym == "DestroyMvAnalyzer") return (void*)FakeDestroyMvAnalyzer;
            if (sym == "SetStreamAuxInfo") return (void*)FakeSetStreamAuxInfo;
            if (sym == "SetVideoInfo") return (void*)FakeSetVideoInfo;
            if (sym == "ParseMvData") return (void*)FakeParseMvData;
        }
        return __real_dlsym(handle, symbol);
    }
    
    int __wrap_dlclose(void* handle) {
        if (handle == NALU_MOCK_HANDLE || handle == MV_MOCK_HANDLE) {
            return 0;
        }
        return __real_dlclose(handle);
    }
}
