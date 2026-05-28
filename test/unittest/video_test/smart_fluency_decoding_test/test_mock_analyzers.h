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

#ifndef TEST_MOCK_ANALYZERS_H
#define TEST_MOCK_ANALYZERS_H

#include <cstring>
#include <memory>
#include "analyzers/nalu_analyzer.h"
#include "analyzers/mv_analyzer.h"
#include "buffer/avbuffer.h"
#include "smart_fluency_decoding.h"

// Setters for dlfcn_mock.cpp global variables (SO loader tests verify C API delegation)
void SetTestVideoInfoGlobals(uint32_t width, uint32_t height);
void SetTestStreamAuxInfoGlobals(const StreamAuxInfo& info);
void ResetTestMvGlobals();

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

class MockNaluAnalyzer : public INaluAnalyzer {
public:
    explicit MockNaluAnalyzer(SFDCodecType codecType) : codecType_(codecType) {}
    void SetAuxInfoCallback(OnStreamAuxInfoParsed callback) override { callback_ = callback; }
    bool IsNonReferenceFrame(const std::shared_ptr<AVBuffer>& inputBuffer) override
    {
        if (!inputBuffer || !inputBuffer->memory_) {
            return false;
        }
        const uint8_t* addr = inputBuffer->memory_->GetAddr();
        uint32_t size = static_cast<uint32_t>(inputBuffer->memory_->GetSize());
        constexpr uint8_t START_CODE[] = {0x00, 0x00, 0x00, 0x01};
        constexpr size_t START_CODE_LEN = sizeof(START_CODE);
        constexpr size_t MIN_NALU_SIZE = START_CODE_LEN + 1;
        if (addr == nullptr || size < MIN_NALU_SIZE) {
            return false;
        }
        if (std::memcmp(addr, START_CODE, START_CODE_LEN) != 0) {
            return false;
        }
        uint8_t naluHeader = addr[START_CODE_LEN];
        uint8_t nalRefIdc = (naluHeader >> 5) & 0x03;
        return nalRefIdc == 0;
    }
    // Test hook: directly invoke the stored callback to simulate SO library trigger.
    void InvokeAuxInfoCallback(const StreamAuxInfo& info)
    {
        if (callback_) {
            callback_(info);
        }
    }
private:
    SFDCodecType codecType_;
    OnStreamAuxInfoParsed callback_{nullptr};
};

class MockMvAnalyzer : public IMvAnalyzer {
public:
    explicit MockMvAnalyzer(SFDCodecType codecType) : codecType_(codecType) {}
    void SetStreamAuxInfo(const StreamAuxInfo& info) override
    {
        auxInfo_ = info;
        SetTestStreamAuxInfoGlobals(info);
    }
    void SetVideoInfo(uint32_t width, uint32_t height) override
    {
        width_ = width; height_ = height;
        SetTestVideoInfoGlobals(width, height);
    }
    MVStats Analyze(const uint8_t* buffer, uint32_t length) override
    {
        (void)buffer;
        (void)length;
        MVStats stats{};
        stats.validBlocks = 100; // 100: mock valid blocks
        return stats;
    }
    SFDCodecType GetCodecType() const override { return codecType_; }
private:
    SFDCodecType codecType_;
    StreamAuxInfo auxInfo_{};
    uint32_t width_{0};
    uint32_t height_{0};
};

// Inject mock analyzers directly into SmartFluencyDecoding's private members.
// This bypasses EnsureNaluAnalyzer/EnsureMvAnalyzer (which would call dlopen).
// Relies on -Dprivate=public to access private members.
inline void InjectMockAnalyzers(SmartFluencyDecoding& sfd)
{
    sfd.naluAnalyzer_ = std::make_unique<MockNaluAnalyzer>(SFDCodecType::AVC);
    sfd.mvAnalyzer_ = std::make_unique<MockMvAnalyzer>(SFDCodecType::AVC);
}

// Try factory first; on platforms with --wrap or real .so this returns the real SoLoader
// whose methods call C API functions through function pointers (needed for coverage).
// Falls back to Mock on platforms without .so files.
inline std::unique_ptr<INaluAnalyzer> CreateNaluAnalyzerOrMock(SFDCodecType codecType)
{
    auto a = NaluAnalyzerFactory::CreateNaluAnalyzer(codecType);
    if (!a) {
        a = std::make_unique<MockNaluAnalyzer>(codecType);
    }
    return a;
}

inline std::unique_ptr<IMvAnalyzer> CreateMvAnalyzerOrMock(SFDCodecType codecType)
{
    auto a = MvAnalyzerFactory::CreateMvAnalyzer(codecType);
    if (!a) {
        a = std::make_unique<MockMvAnalyzer>(codecType);
    }
    return a;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
#endif // TEST_MOCK_ANALYZERS_H
