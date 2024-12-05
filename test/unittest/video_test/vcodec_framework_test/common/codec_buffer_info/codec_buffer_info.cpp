/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "codec_buffer_info.h"

namespace OHOS {
namespace MediaAVCodec {
CodecBufferInfo::CodecBufferInfo(uint32_t index, Any handle, OH_AVCodecBufferAttr attr)
    : handle_(handle), index_(index), attr_(attr), isValid_(true)
{
}

CodecBufferHandle CodecBufferInfo::GetHandle() const
{
    return handle_;
}

uint32_t CodecBufferInfo::GetIndex() const
{
    return index_;
}

OH_AVCodecBufferAttr CodecBufferInfo::GetAttr() const
{
    return attr_;
}

void CodecBufferInfo::SetAttr(OH_AVCodecBufferAttr attr)
{
    attr_ = attr;
}

CodecBufferType CodecBufferInfo::GetBufferType() const
{
    if (Any::IsSameTypeWith<OH_AVBuffer *>(handle_.any_)) {
        return TEST_AVMEMORY_CAPI;
    } else if (Any::IsSameTypeWith<OH_AVMemory *>(handle_.any_)) {
        return TEST_AVBUFFER_CAPI;
    } else if (Any::IsSameTypeWith<OH_AVFormat *>(handle_.any_)) {
        return TEST_AVFORMAT_CAPI;
    } else if (Any::IsSameTypeWith<std::shared_ptr<AVBufferMock>>(handle_.any_)) {
        return TEST_AVMEMORY_MOCK;
    } else if (Any::IsSameTypeWith<std::shared_ptr<AVMemoryMock>>(handle_.any_)) {
        return TEST_AVBUFFER_MOCK;
    } else if (Any::IsSameTypeWith<std::shared_ptr<FormatMock>>(handle_.any_)) {
        return TEST_AVFORMAT_MOCK;
    } else if (Any::IsSameTypeWith<ParameterWithAttrMock>(handle_.any_)) {
        return TEST_PARAMETER_WITH_ATTR_MOCK;
    } else {
        return TEST_INVALID_BUFFER_TYPE;
    }
}

bool CodecBufferInfo::IsValid() const
{
    return isValid_;
}

void CodecBufferInfo::Release()
{
    isValid_ = false;
}
} // namespace MediaAVCodec
} // namespace OHOS