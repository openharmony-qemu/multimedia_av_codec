/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef HISTREAMER_SURFACE_ENCODER_ADAPTER_UNIT_TEST_H
#define HISTREAMER_SURFACE_ENCODER_ADAPTER_UNIT_TEST_H

#include <cstring>
#include <shared_mutex>
#include <deque>
#include <utility>
#include "gtest/gtest.h"
#include "surface_encoder_adapter.h"
#include "surface.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "common/status.h"
#include "osal/task/task.h"
#include "avcodec_common.h"
#include "osal/task/condition_variable.h"
#include "avcodec_video_encoder.h"
#include "surface_encoder_filter.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class SurfaceEncoderAdapterUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

protected:
    std::shared_ptr<SurfaceEncoderAdapter> surfaceEncoderAdapter_{ nullptr };
};

class MockEncoderAdapterKeyFramePtsCallback : public EncoderAdapterKeyFramePtsCallback {
public:
    void OnReportKeyFramePts(std::string KeyFramePts)
    {
        (void)KeyFramePts;
    }

    void OnReportFirstFramePts(int64_t firstFramePts)
    {
        (void)firstFramePts;
    }
};

class MyAVCodecVideoEncoder : public MediaAVCodec::AVCodecVideoEncoder {
public:
     ~MyAVCodecVideoEncoder() = default;
    int32_t Configure(const Format &format)
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
    int32_t QueueInputBuffer(uint32_t index, MediaAVCodec::AVCodecBufferInfo info,
                                     MediaAVCodec::AVCodecBufferFlag flag)
    {
        return 0;
    }
    int32_t QueueInputBuffer(uint32_t index)
    {
        return 0;
    }
    int32_t QueueInputParameter(uint32_t index)
    {
        return 0;
    }
    int32_t GetOutputFormat(Format &format)
    {
        return 0;
    }
    int32_t ReleaseOutputBuffer(uint32_t index)
    {
        return 0;
    }
    int32_t QueryInputParameterWithAttr(uint32_t &index, int64_t timeoutUs)
    {
        return 0;
    }
    int32_t QueryInputBuffer(uint32_t &index, int64_t timeoutUs)
    {
        return 0;
    }
    int32_t QueryOutputBuffer(uint32_t &index, int64_t timeoutUs)
    {
        return 0;
    }
    std::shared_ptr<Format> GetInputParameter(uint32_t index)
    {
        return nullptr;
    }
    std::shared_ptr<Format> GetInputAttribute(uint32_t index)
    {
        return nullptr;
    }
    std::shared_ptr<AVBuffer> GetInputBuffer(uint32_t index)
    {
        return nullptr;
    }
    std::shared_ptr<AVBuffer> GetOutputBuffer(uint32_t index)
    {
        return nullptr;
    }
    int32_t SetParameter(const Format &format)
    {
        return 0;
    }
    int32_t SetCallback(const std::shared_ptr< MediaAVCodec::AVCodecCallback> &callback)
    {
        return 0;
    }
    int32_t SetCallback(const std::shared_ptr< MediaAVCodec::MediaCodecCallback> &callback)
    {
        return 0;
    }
    int32_t SetCallback(const std::shared_ptr< MediaAVCodec::MediaCodecParameterCallback> &callback)
    {
        return 0;
    }
    int32_t SetCallback(const std::shared_ptr< MediaAVCodec::MediaCodecParameterWithAttrCallback> &callback)
    {
        return 0;
    }
    int32_t GetInputFormat(Format &format)
    {
        return 0;
    }
    int32_t SetCustomBuffer(std::shared_ptr<AVBuffer> buffer)
    {
        if (nullptr == buffer) {
            return 0;
        } else {
            return ret;
        }
    }
    int32_t ret = 1;
};
class MyEncoderAdapterCallback : public EncoderAdapterCallback {
public:
    ~MyEncoderAdapterCallback() = default;
    void OnError(MediaAVCodec::AVCodecErrorType type, int32_t errorCode) {}
    void OnOutputFormatChanged(const std::shared_ptr<Meta> &format) {}
};
class SurfaceEncoderAdapterUnitTestAP : public IRemoteStub<AVBufferQueueProducer> {
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
        return  Status::ERROR_UNKNOWN;
    }
    Status RequestBufferWaitUs(std::shared_ptr<AVBuffer>& outBuffer,
                                 const AVBufferConfig& config, int64_t timeoutUs)
    {
        return  Status::ERROR_UNKNOWN;
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

class MyEncoderAdapterKeyFramePtsCallback : public EncoderAdapterKeyFramePtsCallback {
public:
    ~MyEncoderAdapterKeyFramePtsCallback() = default;
    void OnReportKeyFramePts(std::string KeyFramePts)
    {
        return;
    }

    void OnReportFirstFramePts(int64_t firstFramePts)
    {
        return;
    }
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_SURFACE_ENCODER_ADAPTER_UNIT_TEST_H