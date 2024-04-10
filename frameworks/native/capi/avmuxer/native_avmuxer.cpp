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

#include "native_avmuxer.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avmuxer.h"
#include "common/native_mfmagic.h"
#include "native_avmagic.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER_MUXER, "NativeAVMuxer"};
}

using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;

struct AVMuxerObject : public OH_AVMuxer {
    explicit AVMuxerObject(const std::shared_ptr<AVMuxer> &muxer)
        : OH_AVMuxer(AVMagic::AVCODEC_MAGIC_AVMUXER), muxer_(muxer) {}
    ~AVMuxerObject() = default;

    const std::shared_ptr<AVMuxer> muxer_;
};

struct OH_AVMuxer *OH_AVMuxer_Create(int32_t fd, OH_AVOutputFormat format)
{
    std::shared_ptr<AVMuxer> avmuxer = AVMuxerFactory::CreateAVMuxer(fd, static_cast<Plugins::OutputFormat>(format));
    CHECK_AND_RETURN_RET_LOG(avmuxer != nullptr, nullptr, "create muxer failed!");
    struct AVMuxerObject *object = new(std::nothrow) AVMuxerObject(avmuxer);
    return object;
}

OH_AVErrCode OH_AVMuxer_SetRotation(OH_AVMuxer *muxer, int32_t rotation)
{
    CHECK_AND_RETURN_RET_LOG(muxer != nullptr, AV_ERR_INVALID_VAL, "input muxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(muxer->magic_ == AVMagic::AVCODEC_MAGIC_AVMUXER, AV_ERR_INVALID_VAL, "magic error!");

    struct AVMuxerObject *object = reinterpret_cast<AVMuxerObject *>(muxer);
    CHECK_AND_RETURN_RET_LOG(object->muxer_ != nullptr, AV_ERR_INVALID_VAL, "muxer_ is nullptr!");

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::VIDEO_ROTATION>(static_cast<Plugins::VideoRotation>(rotation));
    int32_t ret = object->muxer_->SetParameter(param);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "muxer_ SetRotation failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVMuxer_AddTrack(OH_AVMuxer *muxer, int32_t *trackIndex, OH_AVFormat *trackFormat)
{
    CHECK_AND_RETURN_RET_LOG(muxer != nullptr, AV_ERR_INVALID_VAL, "input muxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(muxer->magic_ == AVMagic::AVCODEC_MAGIC_AVMUXER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(trackIndex != nullptr, AV_ERR_INVALID_VAL, "input track index is nullptr!");
    CHECK_AND_RETURN_RET_LOG(trackFormat != nullptr, AV_ERR_INVALID_VAL, "input track format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(trackFormat->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL,
        "track format magic error!");

    struct AVMuxerObject *object = reinterpret_cast<AVMuxerObject *>(muxer);
    CHECK_AND_RETURN_RET_LOG(object->muxer_ != nullptr, AV_ERR_INVALID_VAL, "muxer_ is nullptr!");

    int32_t ret = object->muxer_->AddTrack(*trackIndex, trackFormat->format_.GetMeta());
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "muxer_ AddTrack failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVMuxer_Start(OH_AVMuxer *muxer)
{
    CHECK_AND_RETURN_RET_LOG(muxer != nullptr, AV_ERR_INVALID_VAL, "input muxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(muxer->magic_ == AVMagic::AVCODEC_MAGIC_AVMUXER, AV_ERR_INVALID_VAL, "magic error!");

    struct AVMuxerObject *object = reinterpret_cast<AVMuxerObject *>(muxer);
    CHECK_AND_RETURN_RET_LOG(object->muxer_ != nullptr, AV_ERR_INVALID_VAL, "muxer_ is nullptr!");

    int32_t ret = object->muxer_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "muxer_ Start failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVMuxer_WriteSample(OH_AVMuxer *muxer, uint32_t trackIndex,
    OH_AVMemory *sample, OH_AVCodecBufferAttr info)
{
    CHECK_AND_RETURN_RET_LOG(muxer != nullptr, AV_ERR_INVALID_VAL, "input muxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(muxer->magic_ == AVMagic::AVCODEC_MAGIC_AVMUXER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(sample != nullptr, AV_ERR_INVALID_VAL, "input sample is nullptr!");
    CHECK_AND_RETURN_RET_LOG(sample->magic_ == MFMagic::MFMAGIC_SHARED_MEMORY, AV_ERR_INVALID_VAL,
        "sample magic error!");
    CHECK_AND_RETURN_RET_LOG(sample->memory_ != nullptr && info.offset >= 0 && info.size >= 0 &&
        sample->memory_->GetSize() >= (info.offset + info.size), AV_ERR_INVALID_VAL, "invalid memory");

    struct AVMuxerObject *object = reinterpret_cast<AVMuxerObject *>(muxer);
    CHECK_AND_RETURN_RET_LOG(object->muxer_ != nullptr, AV_ERR_INVALID_VAL, "muxer_ is nullptr!");

    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(sample->memory_->GetBase() + info.offset,
        sample->memory_->GetSize(), info.size);
    buffer->pts_ = info.pts;
    buffer->flag_ = info.flags;

    int32_t ret = object->muxer_->WriteSample(trackIndex, buffer);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "muxer_ WriteSample failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVMuxer_WriteSampleBuffer(OH_AVMuxer *muxer, uint32_t trackIndex,
    const OH_AVBuffer *sample)
{
    CHECK_AND_RETURN_RET_LOG(muxer != nullptr, AV_ERR_INVALID_VAL, "input muxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(muxer->magic_ == AVMagic::AVCODEC_MAGIC_AVMUXER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(sample != nullptr, AV_ERR_INVALID_VAL, "input sample is nullptr!");
    CHECK_AND_RETURN_RET_LOG(sample->magic_ == MFMagic::MFMAGIC_AVBUFFER, AV_ERR_INVALID_VAL,
        "sample magic error!");

    struct AVMuxerObject *object = reinterpret_cast<AVMuxerObject *>(muxer);
    CHECK_AND_RETURN_RET_LOG(object->muxer_ != nullptr, AV_ERR_INVALID_VAL, "muxer_ is nullptr!");

    int32_t ret = object->muxer_->WriteSample(trackIndex, sample->buffer_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "muxer_ WriteSampleBuffer failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVMuxer_Stop(OH_AVMuxer *muxer)
{
    CHECK_AND_RETURN_RET_LOG(muxer != nullptr, AV_ERR_INVALID_VAL, "input muxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(muxer->magic_ == AVMagic::AVCODEC_MAGIC_AVMUXER, AV_ERR_INVALID_VAL, "magic error!");

    struct AVMuxerObject *object = reinterpret_cast<AVMuxerObject *>(muxer);
    CHECK_AND_RETURN_RET_LOG(object->muxer_ != nullptr, AV_ERR_INVALID_VAL, "muxer_ is nullptr!");

    int32_t ret = object->muxer_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
        "muxer_ Stop failed!");

    return AV_ERR_OK;
}

OH_AVErrCode OH_AVMuxer_Destroy(OH_AVMuxer *muxer)
{
    CHECK_AND_RETURN_RET_LOG(muxer != nullptr, AV_ERR_INVALID_VAL, "input muxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(muxer->magic_ == AVMagic::AVCODEC_MAGIC_AVMUXER, AV_ERR_INVALID_VAL, "magic error!");

    delete muxer;

    return AV_ERR_OK;
}