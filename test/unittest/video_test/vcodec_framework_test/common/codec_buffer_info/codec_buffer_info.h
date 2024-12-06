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
#ifndef CODEC_BUFFER_INFO_H
#define CODEC_BUFFER_INFO_H
#include "codec_buffer_handle.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecBufferInfo {
public:
    CodecBufferInfo(uint32_t index, Any handle, OH_AVCodecBufferAttr attr = {0, 0, 0, 0});
    virtual ~CodecBufferInfo() = default;
    virtual CodecBufferHandle GetHandle() const;
    virtual CodecBufferType GetBufferType() const;
    virtual uint32_t GetIndex() const;
    virtual bool IsValid() const;

    virtual uint8_t *GetAddr();
    virtual OH_AVCodecBufferAttr GetAttr();
    virtual void SetAttr(OH_AVCodecBufferAttr attr);

    friend class CodecBufferQueue;

protected:
    virtual void Release();
    CodecBufferHandle handle_;
    uint32_t index_ = 0;
    OH_AVCodecBufferAttr attr_ = {0, 0, 0, 0};
    bool isValid_ = false;
    std::shared_mutex mutex_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif