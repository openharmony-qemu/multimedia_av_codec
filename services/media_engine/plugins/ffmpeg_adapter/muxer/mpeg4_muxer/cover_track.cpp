/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "cover_track.h"
#include "av_common.h"
#include "avcodec_common.h"
#include "avcodec_mime_type.h"

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "CoverTrack" };
}
#endif // !_WIN32

using namespace OHOS::MediaAVCodec;
namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
CoverTrack::CoverTrack(std::string mimeType, std::shared_ptr<BasicBox> moov)
    : BasicTrack(std::move(mimeType), moov)
{
}

CoverTrack::~CoverTrack()
{
}

Status CoverTrack::Init(const std::shared_ptr<Meta> &trackDesc)
{
    trackId_ = -1;
    constexpr int maxLength = 65535;  // unt16 max: 65535
    bool ret = trackDesc->GetData(Tag::VIDEO_WIDTH, width_); // width
    FALSE_RETURN_V_MSG_E((ret && width_ > 0 && width_ <= maxLength), Status::ERROR_MISMATCHED_TYPE,
        "get video width failed! width:%{public}d", width_);
    ret = trackDesc->GetData(Tag::VIDEO_HEIGHT, height_); // height
    FALSE_RETURN_V_MSG_E((ret && height_ > 0 && height_ <= maxLength), Status::ERROR_MISMATCHED_TYPE,
        "get video height failed! height:%{public}d", height_);

    covr_ = std::make_shared<BasicBox>(0, "covr");
    data_ = std::make_shared<DataBox>(0, "data");
    if (mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_IMAGE_JPG) == 0) {
        data_->dataType_ = 0x0D;
    } else if (mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_IMAGE_PNG) == 0) {
        data_->dataType_ = 0x0E;
    } else if (mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_IMAGE_BMP) == 0) {
        data_->dataType_ = 0x1B;
    }
    data_->default_ = 0;
    covr_->AddChild(data_);
    return Status::NO_ERROR;
}

Status CoverTrack::WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr,
        Status::ERROR_INVALID_OPERATION, "sample is null");
    FALSE_RETURN_V_MSG_E(data_ != nullptr, Status::ERROR_INVALID_OPERATION, "data box is empty");
    FALSE_RETURN_V_MSG_E(io != nullptr, Status::ERROR_INVALID_OPERATION, "io is null");
    data_->data_.reserve(sample->memory_->GetSize());
    data_->data_.assign(sample->memory_->GetAddr() + sample->memory_->GetOffset(),
        sample->memory_->GetAddr() + sample->memory_->GetOffset() + sample->memory_->GetSize());
    return Status::NO_ERROR;
}

Status CoverTrack::WriteTailer()
{
    FALSE_RETURN_V_MSG_E(moov_ != nullptr, Status::ERROR_INVALID_OPERATION, "moov box is empty");
    auto ilst = moov_->GetChild("udta.meta.ilst");
    FALSE_RETURN_V_MSG_E(ilst != nullptr, Status::ERROR_INVALID_OPERATION, "ilst box is empty");
    if (data_ != nullptr && data_->data_.size() > 0) {
        size_t count = ilst->GetChildCount();
        covr_->SetType(covr_->GetType() + std::to_string(count));
        ilst->AddChild(covr_);
    }
    return Status::NO_ERROR;
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS