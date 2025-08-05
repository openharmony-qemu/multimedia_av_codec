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
#include <regex>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avmuxer.h"
#include "common/native_mfmagic.h"
#include "native_avmagic.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_MUXER, "NativeAVMuxer"};
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

static void CopyMetaData(const TagType &tag, std::shared_ptr<Meta> &fromMeta, std::shared_ptr<Meta> &toMeta)
{
    AnyValueType type = fromMeta->GetValueType(tag);
    if (type == AnyValueType::INVALID_TYPE || type == AnyValueType::STRING) {
        std::string value;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    } else if (type == AnyValueType::BOOL) {
        bool value = false;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    } else if (type == AnyValueType::INT8_T) {
        int8_t value = 0;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    } else if (type == AnyValueType::UINT8_T) {
        uint8_t value = 0;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    } else if (type == AnyValueType::INT32_T) {
        int32_t value = 0;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    } else if (type == AnyValueType::UINT32_T) {
        uint32_t value = 0;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    } else if (type == AnyValueType::INT64_T) {
        int64_t value = 0;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    } else if (type == AnyValueType::UINT64_T) {
        uint64_t value = 0;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    } else if (type == AnyValueType::FLOAT) {
        float value = 0.0f;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    } else if (type == AnyValueType::DOUBLE) {
        double value = 0.0;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    } else if (type == AnyValueType::VECTOR_UINT8) {
        std::vector<uint8_t> value;
        fromMeta->GetData(tag, value);
        toMeta->SetData(tag, value);
    }
}

static void SeparateMeta(std::shared_ptr<Meta> meta, std::shared_ptr<Meta> &definedMeta,
    std::shared_ptr<Meta> &userMeta)
{
    for (auto iter = meta->begin(); iter != meta->end(); ++iter) {
        TagType tag = iter->first;
        if (meta->IsDefinedKey(tag)) {
            CopyMetaData(tag, meta, definedMeta);
        } else {
            CopyMetaData(tag, meta, userMeta);
        }
    }
}

static OH_AVErrCode SetCreationTimeMeta(std::shared_ptr<Meta> definedMeta, std::shared_ptr<Meta> &param)
{
    if (definedMeta->Find(Tag::MEDIA_CREATION_TIME) == definedMeta->end()) {
        return AV_ERR_OK;
    }

    AVCODEC_LOGI("set format defined key %{public}s", Tag::MEDIA_CREATION_TIME);
    std::string value;
    definedMeta->Get<Tag::MEDIA_CREATION_TIME>(value);
    std::regex pattern(R"((\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})(.\d{1,6})?((\+|-\d{4})?)Z?)");
    std::smatch match;
    CHECK_AND_RETURN_RET_LOG(std::regex_match(value, match, pattern), AV_ERR_INVALID_VAL, "creation time is invalid");

    param->Set<Tag::MEDIA_CREATION_TIME>(value);
    return AV_ERR_OK;
}

static OH_AVErrCode SetCommentMeta(std::shared_ptr<Meta> definedMeta, std::shared_ptr<Meta> &param)
{
    if (definedMeta->Find(Tag::MEDIA_COMMENT) == definedMeta->end()) {
        return AV_ERR_OK;
    }

    AVCODEC_LOGI("set format defined key %{public}s", Tag::MEDIA_COMMENT);
    std::string comment;
    definedMeta->Get<Tag::MEDIA_COMMENT>(comment);
    std::regex pattern(R"(^.{1,256}$)");
    std::smatch match;
    CHECK_AND_RETURN_RET_LOG(std::regex_match(comment, match, pattern), AV_ERR_INVALID_VAL, "comment is invalid");

    param->Set<Tag::MEDIA_COMMENT>(comment);
    return AV_ERR_OK;
}

static OH_AVErrCode SetMoovFrontMeta(std::shared_ptr<Meta> definedMeta, std::shared_ptr<Meta> &param)
{
    if (definedMeta->Find(Tag::MEDIA_ENABLE_MOOV_FRONT) == definedMeta->end()) {
        return AV_ERR_OK;
    }

    AVCODEC_LOGI("set format defined key %{public}s", Tag::MEDIA_ENABLE_MOOV_FRONT);
    int32_t isFront;
    definedMeta->Get<Tag::MEDIA_ENABLE_MOOV_FRONT>(isFront);
    param->Set<Tag::MEDIA_ENABLE_MOOV_FRONT>(isFront);
    return AV_ERR_OK;
}

static OH_AVErrCode SetDefinedMetaParam(std::shared_ptr<Meta> definedMeta, AVMuxerObject* object)
{
    std::shared_ptr<Meta> param = std::make_shared<Meta>();

    OH_AVErrCode metaRet = SetCreationTimeMeta(definedMeta, param);
    CHECK_AND_RETURN_RET_LOG(metaRet == AV_ERR_OK, AV_ERR_INVALID_VAL, "input format is invalid");
    metaRet = SetCommentMeta(definedMeta, param);
    CHECK_AND_RETURN_RET_LOG(metaRet == AV_ERR_OK, AV_ERR_INVALID_VAL, "input format is invalid");
    metaRet = SetMoovFrontMeta(definedMeta, param);
    CHECK_AND_RETURN_RET_LOG(metaRet == AV_ERR_OK, AV_ERR_INVALID_VAL, "input format is invalid");

    if (param->Empty()) {
        AVCODEC_LOGW("input format does not have a valid key");
        return AV_ERR_OK;
    }

    int32_t ret = object->muxer_->SetParameter(param);
    return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
}

static OH_AVErrCode SetUserMetaParam(std::shared_ptr<Meta> userMeta, AVMuxerObject *object)
{
    int32_t ret = object->muxer_->SetUserMeta(userMeta);
    return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
}

OH_AVErrCode OH_AVMuxer_SetFormat(OH_AVMuxer *muxer, OH_AVFormat *format)
{
    CHECK_AND_RETURN_RET_LOG(muxer != nullptr, AV_ERR_INVALID_VAL, "input muxer is nullptr!");
    CHECK_AND_RETURN_RET_LOG(muxer->magic_ == AVMagic::AVCODEC_MAGIC_AVMUXER, AV_ERR_INVALID_VAL, "magic error!");
    CHECK_AND_RETURN_RET_LOG(format != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    CHECK_AND_RETURN_RET_LOG(format->magic_ == MFMagic::MFMAGIC_FORMAT, AV_ERR_INVALID_VAL,
        "format magic error!");

    struct AVMuxerObject *object = reinterpret_cast<AVMuxerObject *>(muxer);
    CHECK_AND_RETURN_RET_LOG(object->muxer_ != nullptr, AV_ERR_INVALID_VAL, "muxer_ is nullptr!");

    std::shared_ptr<Meta> meta = format->format_.GetMeta();
    std::shared_ptr<Meta> definedMeta = std::make_shared<Meta>();
    std::shared_ptr<Meta> userMeta = std::make_shared<Meta>();
    
    CHECK_AND_RETURN_RET_LOG(meta != nullptr, AV_ERR_INVALID_VAL, "input format is nullptr!");
    SeparateMeta(meta, definedMeta, userMeta);

    OH_AVErrCode ret = AV_ERR_OK;
    if (definedMeta != nullptr && !definedMeta->Empty()) {
        ret = SetDefinedMetaParam(definedMeta, object);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "muxer_ SetFormat SetDefinedMetaParam failed!");
    }

    if (userMeta != nullptr && !userMeta->Empty()) {
        ret = SetUserMetaParam(userMeta, object);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "muxer_ SetFormat SetUserMetaParam failed!");
    }
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
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AV_ERR_NO_MEMORY, "create buffer failed");
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