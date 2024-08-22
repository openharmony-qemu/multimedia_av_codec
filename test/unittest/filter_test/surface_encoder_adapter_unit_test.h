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
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // HISTREAMER_SURFACE_ENCODER_ADAPTER_UNIT_TEST_H