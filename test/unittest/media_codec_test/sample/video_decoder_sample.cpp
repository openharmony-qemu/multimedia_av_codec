
/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef VIDEO_DECODER_SAMPLE_H
#define VIDEO_DECODER_SAMPLE_H

#include <memory>
#include <string>
#include <gtest/gtest.h>
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"
#include "buffer/avbuffer_queue_producer.h"
#include "common/status.h"
#include "meta/meta.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_event.h"
#include "unittest_log.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Media;

namespace OHOS {
namespace MediaAVCodec {
~VideoDecoderSample()
{
    return Status::OK;
}

void VideoDecoderSample::SetCodec(std::shared_ptr<MediaCodec> &codec)
{
    return Status::OK;
}

std::shared_ptr<MediaCodec> &VideoDecoderSample::GetCodec()
{
    return Status::OK;
}

Status VideoDecoderSample::Configure(const std::shared_ptr<Meta> &meta)
{
    return Status::OK;
}

Status VideoDecoderSample::SetCodecCallback(std::shared_ptr<CodecCallback> &codecCallback)
{
    return Status::OK;
}

Status VideoDecoderSample::SetOutputBufferQueue(std::shared_ptr<AVBufferQueueProducer> &bufferQueueProducer)
{
    return Status::OK;
}

Status VideoDecoderSample::SetOutputSurface(sptr<Surface> &surface)
{
    return Status::OK;
}

Status VideoDecoderSample::Prepare()
{
    return Status::OK;
}

std::shared_ptr<AVBufferQueueProducer> VideoDecoderSample::GetInputBufferQueue()
{
    return Status::OK;
}

sptr<Surface> VideoDecoderSample::GetInputSurface()
{
    return Status::OK;
}

Status VideoDecoderSample::Start()
{
    return Status::OK;
}

Status VideoDecoderSample::Stop()
{
    return Status::OK;
}

Status VideoDecoderSample::Flush()
{
    return Status::OK;
}

Status VideoDecoderSample::Reset()
{
    return Status::OK;
}

Status VideoDecoderSample::Release()
{
    return Status::OK;
}

Status VideoDecoderSample::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    return Status::OK;
}

std::shared_ptr<Meta> VideoDecoderSample::GetOutputFormat()
{
    return Status::OK;
}

Status VideoDecoderSample::NotifyEOS()
{
    return Status::OK;
}

} // namespace MediaAVCodec
} // namespace OHOS