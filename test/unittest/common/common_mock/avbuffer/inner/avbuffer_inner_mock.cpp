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

#include "avbuffer_inner_mock.h"
#include "avformat_inner_mock.h"
#include "media_description.h"
#include "native_mferrors.h"
#include "securec.h"
#include "unittest_log.h"

using namespace OHOS::Media;
namespace OHOS {
namespace MediaAVCodec {
uint8_t *AVBufferInnerMock::GetAddr()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer_ != nullptr, nullptr, "buffer_ is nullptr!");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer_->memory_ != nullptr, nullptr, "buffer_->memory_ is nullptr!");
    return buffer_->memory_->GetAddr();
}

int32_t AVBufferInnerMock::GetCapacity()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer_ != nullptr, -1, "buffer_ is nullptr!");
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer_->memory_ != nullptr, -1, "buffer_->memory_ is nullptr!");
    return buffer_->memory_->GetCapacity();
}

OH_AVCodecBufferAttr AVBufferInnerMock::GetBufferAttr()
{
    OH_AVCodecBufferAttr attr;
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer_ != nullptr, attr, "buffer_ is nullptr!");
    attr.pts = buffer_->pts_;
    attr.offset = buffer_->memory_->GetOffset();
    attr.size = buffer_->memory_->GetSize();
    attr.flags = static_cast<uint32_t>(buffer_->flag_);
    return attr;
}
int32_t AVBufferInnerMock::SetBufferAttr(OH_AVCodecBufferAttr &attr)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer_ != nullptr, AV_ERR_UNKNOWN, "buffer_ is nullptr!");
    buffer_->pts_ = attr.pts;
    buffer_->memory_->SetOffset(attr.offset);
    buffer_->flag_ = static_cast<AVCodecBufferFlag>(attr.flags);
    return buffer_->memory_->SetSize(attr.size);
}

std::shared_ptr<FormatMock> AVBufferInnerMock::GetParameter()
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer_ != nullptr, nullptr, "buffer_ is nullptr!");
    // auto formatMock = std::make_shared<AVFormatInnerMock>(*(buffer_->meta_));
    // return formatMock;
    return nullptr;
}

int32_t AVBufferInnerMock::SetParameter(const std::shared_ptr<FormatMock> &format)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(buffer_ != nullptr, AV_ERR_UNKNOWN, "buffer_ is nullptr!");
    // *(buffer_->meta_) = std::static_pointer_cast<AVFormatInnerMock>(format)->GetFormat();
    return AV_ERR_OK;
}

int32_t AVBufferInnerMock::Destroy()
{
    buffer_ = nullptr;
    return AV_ERR_OK;
}

std::shared_ptr<AVBuffer> &AVBufferInnerMock::GetAVBuffer()
{
    return buffer_;
}
} // namespace MediaAVCodec
} // namespace OHOS