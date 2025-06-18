/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef VIDEO_DECODER_ADAPTER_UNIT_TEST_H
#define VIDEO_DECODER_ADAPTER_UNIT_TEST_H

#include "gtest/gtest.h"

#include <shared_mutex>
#include <vector>
#include "surface.h"
#include "avcodec_video_decoder.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "osal/task/condition_variable.h"
#include "meta/format.h"
#include "video_sink.h"

namespace OHOS {
namespace Media {
constexpr int64_t WARNING_TIME_MS = 60;

class VideoDecoderAdapterUnitTest : public testing::Test {
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

class TestMediaCodecCallback : public MediaAVCodec::MediaCodecCallback {
public:
    explicit TestMediaCodecCallback()
    {
        std::cout << "TestMediaCodecCallback constructor" << std::endl;
    }
    virtual void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
    {
        return;
    }

    virtual void OnOutputFormatChanged(const Format &format)
    {
        return;
    }

    virtual void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
    {
        return;
    }

    virtual void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
    {
        return;
    }
};

class TestAVCodecVideoDecoder : public MediaAVCodec::AVCodecVideoDecoder {
public:
    explicit TestAVCodecVideoDecoder()
    {
    }
    ~TestAVCodecVideoDecoder()
    {
    }
    void Init(int32_t status)
    {
        status_ = status;
    }

    virtual int32_t Configure(const Format &format)
    {
        return status_;
    }

    virtual int32_t Prepare()
    {
        return status_;
    }

    virtual int32_t Start()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(WARNING_TIME_MS));
        return status_;
    }

    virtual int32_t Stop()
    {
        return status_;
    }

    virtual int32_t Flush()
    {
        return status_;
    }

    virtual int32_t Reset()
    {
        return status_;
    }

    virtual int32_t Release()
    {
        return status_;
    }

    virtual int32_t SetOutputSurface(sptr<Surface> surface)
    {
        return status_;
    }

    virtual int32_t QueueInputBuffer(uint32_t index, MediaAVCodec::AVCodecBufferInfo info,
        MediaAVCodec::AVCodecBufferFlag flag)
    {
        return status_;
    }

    virtual int32_t QueueInputBuffer(uint32_t index)
    {
        return ret;
    }

    virtual int32_t GetOutputFormat(Format &format)
    {
        return status_;
    }

    virtual int32_t ReleaseOutputBuffer(uint32_t index, bool render)
    {
        return status_;
    }

    virtual int32_t RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs)
    {
        return status_;
    }

    virtual int32_t QueryInputBuffer(uint32_t &index, int64_t timeoutUs)
    {
        (void)index;
        (void)timeoutUs;
        return status_;
    }

    virtual int32_t QueryOutputBuffer(uint32_t &index, int64_t timeoutUs)
    {
        (void)index;
        (void)timeoutUs;
        return status_;
    }

    virtual std::shared_ptr<AVBuffer> GetInputBuffer(uint32_t index)
    {
        (void)index;
        return nullptr;
    }

    virtual std::shared_ptr<AVBuffer> GetOutputBuffer(uint32_t index)
    {
        (void)index;
        return nullptr;
    }

    virtual int32_t SetParameter(const Format &format)
    {
        return status_;
    }

    virtual int32_t SetCallback(const std::shared_ptr<MediaAVCodec::AVCodecCallback> &callback)
    {
        return status_;
    }

    virtual int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback)
    {
        return status_;
    }

    virtual int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag)
    {
        (void)keySession;
        (void)svpFlag;
        return status_;
    }

    virtual int32_t SetLowPowerPlayerMode(bool isLpp)
    {
        return status_;
    }

    virtual int32_t GetChannelId(int32_t &channelId)
    {
        return status_;
    }
private:
    int32_t status_ = 0;
    int32_t ret = 1;
};

class TestAVBufferQueueConsumer : public AVBufferQueueConsumer {
public:
    ~TestAVBufferQueueConsumer() override = default;
    TestAVBufferQueueConsumer(const TestAVBufferQueueConsumer&) = delete;
    TestAVBufferQueueConsumer operator=(const TestAVBufferQueueConsumer&) = delete;

    uint32_t GetQueueSize()
    {
        return 0;
    }
    Status SetQueueSize(uint32_t size)
    {
        return Status::OK;
    }
    bool IsBufferInQueue(const std::shared_ptr<AVBuffer>& buffer)
    {
        return true;
    }

    Status AcquireBuffer(std::shared_ptr<AVBuffer>& outBuffer)
    {
        if (outBuffer == nullptr) {
            return Status::ERROR_UNKNOWN;
        } else {
            return Status::OK;
        }
    }
    Status ReleaseBuffer(const std::shared_ptr<AVBuffer>& inBuffer)
    {
        return Status::ERROR_UNKNOWN;
    }

    Status AttachBuffer(std::shared_ptr<AVBuffer>& inBuffer, bool isFilled)
    {
        return Status::OK;
    }
    Status DetachBuffer(const std::shared_ptr<AVBuffer>& outBuffer)
    {
        return Status::OK;
    }

    Status SetBufferAvailableListener(sptr<IConsumerListener>& listener)
    {
        return Status::OK;
    }

    Status SetQueueSizeAndAttachBuffer(uint32_t size, std::shared_ptr<AVBuffer>& buffer, bool isFilled)
    {
        return Status::OK;
    }
protected:
    TestAVBufferQueueConsumer() = default;
};

class MyEventReceiver : public Pipeline::EventReceiver {
public:
    ~MyEventReceiver() = default;
    void OnEvent(const Event& event)
    {
        return;
    }
};

}
}

#endif