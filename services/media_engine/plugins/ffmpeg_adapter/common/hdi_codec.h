/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef HDI_CODEC_H
#define HDI_CODEC_H
#include <condition_variable>
#include <mutex>
#include "buffer/avbuffer.h"
#include "v3_0/codec_ext_types.h"
#include "v3_0/icodec_component_manager.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Hdi {
using namespace OHOS::HDI::Codec::V3_0;

struct AudioCodecOmxParam {
    uint32_t size;
    union {
        struct {
            uint8_t nVersionMajor;
            uint8_t nVersionMinor;
            uint8_t nRevision;
            uint8_t nStep;
        } s;
        uint32_t nVersion;
    } version;
    uint32_t sampleRate;
    uint32_t sampleFormat;
    uint32_t channels;
    uint32_t bitRate;
    uint32_t reserved;
};

class HdiCodec : public std::enable_shared_from_this<HdiCodec> {
public:
    enum class PortIndex : uint32_t {
        INPUT_PORT = 0,
        OUTPUT_PORT = 1
    };

    HdiCodec();

    Status InitComponent(const std::string &name);

    bool IsSupportCodecType(const std::string &name);

    void InitParameter(AudioCodecOmxParam &param);

    Status GetParameter(uint32_t index, AudioCodecOmxParam &param);

    Status SetParameter(uint32_t index, const std::vector<int8_t> &paramVec);

    Status InitBuffers(uint32_t bufferSize);

    Status SendCommand(CodecCommandType cmd, uint32_t param);

    Status EmptyThisBuffer(const std::shared_ptr<AVBuffer> &buffer);

    Status FillThisBuffer(std::shared_ptr<AVBuffer> &buffer);

    Status OnEventHandler(CodecEventType event, const EventInfo &info);

    Status OnEmptyBufferDone(const OmxCodecBuffer &buffer);

    Status OnFillBufferDone(const OmxCodecBuffer &buffer);

    Status Reset();

    void Release();

    class HdiCallback : public ICodecCallback {
    public:
        explicit HdiCallback(std::shared_ptr<HdiCodec> hdiCodec);
        virtual ~HdiCallback() = default;
        int32_t EventHandler(CodecEventType event, const EventInfo &info) override;
        int32_t EmptyBufferDone(int64_t appData, const OmxCodecBuffer &buffer) override;
        int32_t FillBufferDone(int64_t appData, const OmxCodecBuffer &buffer) override;
    private:
        std::shared_ptr<HdiCodec> hdiCodec_ = nullptr;
    };

private:
    sptr<ICodecComponentManager> GetComponentManager();
    std::vector<CodecCompCapability> GetCapabilityList();
    Status InitBuffersByPort(PortIndex portIndex, uint32_t bufferSize);
    Status FreeBuffer(PortIndex portIndex, const std::shared_ptr<OmxCodecBuffer> &omxBuffer);

private:
    struct OmxBufferInfo {
        std::shared_ptr<OmxCodecBuffer> omxBuffer = nullptr;
        std::shared_ptr<AVBuffer> avBuffer = nullptr;

        ~OmxBufferInfo()
        {
            omxBuffer = nullptr;
            avBuffer = nullptr;
        }

        void Reset()
        {
            omxBuffer = nullptr;
            avBuffer = nullptr;
        }
    };

    uint32_t componentId_;
    std::string componentName_;
    sptr<ICodecCallback> compCb_;
    sptr<ICodecComponent> compNode_;
    sptr<ICodecComponentManager> compMgr_;
    std::shared_ptr<OmxCodecBuffer> outputOmxBuffer_;
    std::shared_ptr<OmxBufferInfo> omxInBufferInfo_;
    std::shared_ptr<OmxBufferInfo> omxOutBufferInfo_;
    CodecEventType event_;
    std::mutex mutex_;
    std::condition_variable condition_;
};
} // namespace Hdi
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif