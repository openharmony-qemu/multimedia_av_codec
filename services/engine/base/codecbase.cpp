/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "avcodec_log.h"
#include "codecbase.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecBase"};
}

namespace OHOS {
namespace MediaAVCodec {
int32_t CodecBase::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    (void)callback;
    AVCODEC_LOGD("SetCallback of AVCodecCallback is not supported");
    return 0;
}
int32_t CodecBase::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    (void)callback;
    AVCODEC_LOGD("SetCallback of MediaCodecCallback is not supported");
    return 0;
}

int32_t CodecBase::QueueInputBuffer(uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag)
{
    (void)index;
    (void)info;
    (void)flag;
    AVCODEC_LOGD("QueueInputBuffer of AVSharedMemory is not supported");
    return 0;
}

int32_t CodecBase::QueueInputBuffer(uint32_t index)
{
    (void)index;
    AVCODEC_LOGD("QueueInputBuffer of AVBuffer is not supported");
    return 0;
}

int32_t CodecBase::NotifyEos()
{
    AVCODEC_LOGW("NotifyEos is not supported");
    return 0;
}

sptr<Surface> CodecBase::CreateInputSurface()
{
    AVCODEC_LOGW("CreateInputSurface is not supported");
    return nullptr;
}

int32_t CodecBase::SetInputSurface(sptr<Surface> surface)
{
    (void)surface;
    AVCODEC_LOGW("SetInputSurface is not supported");
    return 0;
}

int32_t CodecBase::SetOutputSurface(sptr<Surface> surface)
{
    (void)surface;
    AVCODEC_LOGW("SetOutputSurface is not supported");
    return 0;
}

int32_t CodecBase::RenderOutputBuffer(uint32_t index)
{
    (void)index;
    AVCODEC_LOGW("RenderOutputBuffer is not supported");
    return 0;
}

int32_t CodecBase::SignalRequestIDRFrame()
{
    AVCODEC_LOGW("SignalRequestIDRFrame is not supported");
    return 0;
}

int32_t CodecBase::GetInputFormat(Format& format)
{
    (void)format;
    AVCODEC_LOGW("GetInputFormat is not supported");
    return 0;
}
} // namespace MediaAVCodec
} // namespace OHOS
