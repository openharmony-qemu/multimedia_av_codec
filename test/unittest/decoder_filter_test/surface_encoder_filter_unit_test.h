/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SURFACE_ENCODER_FILTER_UNIT_TEST_H
#define SURFACE_ENCODER_FILTER_UNIT_TEST_H

#include <cstring>
#include "gtest/gtest.h"
#include "filter/filter.h"
#include "surface.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "common/status.h"
#include "avcodec_common.h"
#include "avcodec_video_encoder.h"

namespace OHOS {
namespace Media {
class SurfaceEncoderFilterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

class TestEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TestEventReceiver()
    {
        std::cout << "event receiver constructor" << std::endl;
    }

    void OnEvent(const Event &event)
    {
        std::cout << event.srcFilter << std::endl;
    }
};

class TestFilterCallback : public Pipeline::FilterCallback {
public:
    explicit TestFilterCallback()
    {
        std::cout << "filter back constructor" << std::endl;
    }

    Status OnCallback(const std::shared_ptr<Pipeline::Filter>& filter,
        Pipeline::FilterCallBackCommand cmd, Pipeline::StreamType outType)
    {
        return Status::OK;
    }
};

class TestFilterLinkCallback : public Pipeline::FilterLinkCallback {
public:
    explicit TestFilterLinkCallback()
    {
        std::cout << "filter back constructor" << std::endl;
    }
    void OnLinkedResult(const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnLinkedResult" << std::endl;
    }
    void OnUnlinkedResult(std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnUnlinkedResult" << std::endl;
    }
    void OnUpdatedResult(std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnUpdatedResult" << std::endl;
    }
};

class RecordableEventReceiver : public Pipeline::EventReceiver {
public:
    explicit RecordableEventReceiver() {}

    void OnEvent(const Event &event) override
    {
        lastEvent_ = event;
        eventCount_++;
    }

    Event lastEvent_{};
    int eventCount_ {0};
};

// Minimal AVCodecVideoEncoder stub that counts NotifyEos/SetParameter calls for guard verification.
class CountingVideoEncoder : public MediaAVCodec::AVCodecVideoEncoder {
public:
    ~CountingVideoEncoder() = default;
    int32_t Configure(const Format &)
    {
        return 0;
    }
    int32_t Prepare()
    {
        return 0;
    }
    int32_t Start()
    {
        return 0;
    }
    int32_t Stop()
    {
        return 0;
    }
    int32_t Flush()
    {
        return 0;
    }
    int32_t NotifyEos()
    {
        notifyEosCount_++;
        return 0;
    }
    int32_t Reset()
    {
        return 0;
    }
    int32_t Release()
    {
        return 0;
    }
    sptr<Surface> CreateInputSurface()
    {
        return nullptr;
    }
    int32_t QueueInputBuffer(uint32_t, MediaAVCodec::AVCodecBufferInfo, MediaAVCodec::AVCodecBufferFlag)
    {
        return 0;
    }
    int32_t QueueInputBuffer(uint32_t)
    {
        return 0;
    }
    int32_t QueueInputParameter(uint32_t)
    {
        return 0;
    }
    int32_t GetOutputFormat(Format &)
    {
        return 0;
    }
    int32_t GetCodecInfo(Format &)
    {
        return 0;
    }
    int32_t ReleaseOutputBuffer(uint32_t)
    {
        return 0;
    }
    int32_t SetParameter(const Format &)
    {
        setParameterCount_++;
        return 0;
    }
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::AVCodecCallback> &)
    {
        return 0;
    }
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &)
    {
        return 0;
    }
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecParameterCallback> &)
    {
        return 0;
    }
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecParameterWithAttrCallback> &)
    {
        return 0;
    }
    int32_t GetInputFormat(Format &)
    {
        return 0;
    }
    int32_t SetCustomBuffer(std::shared_ptr<AVBuffer>)
    {
        return 0;
    }

    int32_t notifyEosCount_ {0};
    int32_t setParameterCount_ {0};
};

class CapturingNextFilter : public Pipeline::Filter {
public:
    CapturingNextFilter()
        : Filter("CapturingNextFilter", Pipeline::FilterType::FILTERTYPE_MUXER),
          onLinkedReturn_(Status::OK) {}
    ~CapturingNextFilter() override = default;

    Status OnLinked(Pipeline::StreamType inType,
                    const std::shared_ptr<Meta> &meta,
                    const std::shared_ptr<Pipeline::FilterLinkCallback> &callback) override
    {
        capturedInType_ = inType;
        capturedMeta_ = meta;
        capturedCallback_ = callback;
        onLinkedCallCount_++;
        return onLinkedReturn_;
    }

    Pipeline::StreamType capturedInType_ {Pipeline::StreamType::STREAMTYPE_PACKED};
    std::shared_ptr<Meta> capturedMeta_;
    std::shared_ptr<Pipeline::FilterLinkCallback> capturedCallback_;
    int32_t onLinkedCallCount_ {0};
    Status onLinkedReturn_;
};

class MyAVBufferQueueProducer : public IRemoteStub<AVBufferQueueProducer> {
public:
    uint32_t GetQueueSize()
    {
        return 0;
    }
    Status SetQueueSize(uint32_t size)
    {
        return  Status::OK;
    }

    Status RequestBuffer(std::shared_ptr<AVBuffer>& outBuffer,
                                 const AVBufferConfig& config, int32_t timeoutMs)
    {
        if (outBuffer == nullptr) {
            return  Status::ERROR_NULL_POINTER;
        } else {
            return  Status::OK;
        }
    }
    Status RequestBufferWaitUs(std::shared_ptr<AVBuffer>& outBuffer,
                                 const AVBufferConfig& config, int64_t timeoutUs)
    {
        if (outBuffer == nullptr) {
            return  Status::ERROR_NULL_POINTER;
        } else {
            return  Status::OK;
        }
    }
    Status PushBuffer(const std::shared_ptr<AVBuffer>& inBuffer, bool available)
    {
        return  Status::OK;
    }
    Status ReturnBuffer(const std::shared_ptr<AVBuffer>& inBuffer, bool available)
    {
        return  Status::OK;
    }

    Status AttachBuffer(std::shared_ptr<AVBuffer>& inBuffer, bool isFilled)
    {
        return  Status::OK;
    }
    Status DetachBuffer(const std::shared_ptr<AVBuffer>& outBuffer)
    {
        return  Status::OK;
    }

    Status SetBufferFilledListener(sptr<IBrokerListener>& listener)
    {
        return  Status::OK;
    }
    Status RemoveBufferFilledListener(sptr<IBrokerListener>& listener)
    {
        return  Status::OK;
    }
    Status SetBufferAvailableListener(sptr<IProducerListener>& listener)
    {
        return  Status::OK;
    }
    Status Clear()
    {
        return  Status::OK;
    }
    Status ClearBufferIf(std::function<bool(const std::shared_ptr<AVBuffer> &)> pred)
    {
        return  Status::OK;
    }
    DECLARE_INTERFACE_DESCRIPTOR(u"Media.MyAVBufferQueueProducer");

protected:
    enum: uint32_t {
        PRODUCER_GET_QUEUE_SIZE = 0,
        PRODUCER_SET_QUEUE_SIZE = 1,
        PRODUCER_REQUEST_BUFFER = 2,
        PRODUCER_PUSH_BUFFER = 3,
        PRODUCER_RETURN_BUFFER = 4,
        PRODUCER_ATTACH_BUFFER = 5,
        PRODUCER_DETACH_BUFFER = 6,
        PRODUCER_SET_FILLED_LISTENER = 7,
        PRODUCER_REMOVE_FILLED_LISTENER = 8,
        PRODUCER_SET_AVAILABLE_LISTENER = 9
    };
};
}
}

#endif