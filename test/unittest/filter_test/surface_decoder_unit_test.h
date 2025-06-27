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
#ifndef HISTREAMER_SURFACE_DECODER_ADAPTER_UNIT_TEST_H
#define HISTREAMER_SURFACE_DECODER_ADAPTER_UNIT_TEST_H

#include "gtest/gtest.h"
#include "surface_decoder_adapter.h"
#include <cstring>
#include <shared_mutex>
#include "surface.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "common/status.h"
#include "common/log.h"
#include "osal/task/task.h"
#include "avcodec_common.h"
#include "osal/task/condition_variable.h"
#include "avcodec_video_decoder.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class SurfaceDecoderUnitTest : public testing::Test {
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
    std::shared_ptr<SurfaceDecoderAdapter> surfaceDecoderAdapter_{ nullptr };
};
class MyAVCodecVideoDecoder : public MediaAVCodec::AVCodecVideoDecoder {
public:
    ~MyAVCodecVideoDecoder() = default;
    int32_t Configure(const Format &format)
    {
        return ret;
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
    int32_t Reset()
    {
        return ret;
    }
    int32_t Release()
    {
        return 0;
    }
    int32_t SetOutputSurface(sptr<Surface> surface)
    {
        return 0;
    }
    int32_t QueueInputBuffer(uint32_t index, MediaAVCodec::AVCodecBufferInfo info, MediaAVCodec::AVCodecBufferFlag flag)
    {
        return ret;
    }
    int32_t QueueInputBuffer(uint32_t index)
    {
        return ret;
    }
    int32_t GetOutputFormat(Format &format)
    {
        return ret;
    }
    int32_t ReleaseOutputBuffer(uint32_t index, bool render)
    {
        return ret;
    }
    int32_t RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs)
    {
        return ret;
    }
    int32_t QueryInputBuffer(uint32_t &index, int64_t timeoutUs)
    {
        return 0;
    }
    int32_t QueryOutputBuffer(uint32_t &index, int64_t timeoutUs)
    {
        return 0;
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
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::AVCodecCallback> &callback)
    {
        return ret;
    }
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback)
    {
        return 0;
    }
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag)
    {
        (void)keySession;
        (void)svpFlag;
        return 0;
    }

    int32_t SetLowPowerPlayerMode(bool isLpp)
    {
        return ret;
    }

    int32_t GetChannelId(int32_t &channelId)
    {
        return ret;
    }

    int32_t ret = 1;
};
class MyDecoderAdapterCallback : public DecoderAdapterCallback {
public:
    ~MyDecoderAdapterCallback() = default;
    void OnError(MediaAVCodec::AVCodecErrorType type, int32_t errorCode)
    {
        return;
    }
    void OnOutputFormatChanged(const std::shared_ptr<Meta> &format)
    {
        return;
    }
    void OnBufferEos(int64_t pts, int64_t frameNum)
    {
        return;
    }
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_SURFACE_DECODER_ADAPTER_UNIT_TEST_H