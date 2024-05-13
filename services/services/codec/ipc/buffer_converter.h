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

#ifndef BUFFER_CONVERTER_H
#define BUFFER_CONVERTER_H
#include <functional>
#include <shared_mutex>
#include "avcodec_info.h"
#include "buffer/avbuffer.h"
#include "buffer/avsharedmemorybase.h"
#include "meta/format.h"

namespace OHOS {
namespace MediaAVCodec {
using AVBuffer = OHOS::Media::AVBuffer;
class CodecClient;

class BufferConverter {
public:
    explicit BufferConverter(bool isEncoder);
    ~BufferConverter() = default;
    static std::shared_ptr<BufferConverter> Create(AVCodecType type);
    int32_t ReadFromBuffer(std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory);
    int32_t WriteToBuffer(std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory);

    void NeedToResetFormatOnce();
    void GetFormat(Format &format);
    void SetFormat(const Format &format);
    void SetInputBufferFormat(std::shared_ptr<AVBuffer> &buffer);
    void SetOutputBufferFormat(std::shared_ptr<AVBuffer> &buffer);
    typedef struct AVCodecRect {
        int32_t wStride = 0;
        int32_t hStride = 0;
    } AVCodecRect;

private:
    void SetPixFormat(const VideoPixelFormat pixelFormat);
    void SetWidth(const int32_t width);
    void SetHeight(const int32_t height);
    void SetWidthStride(const int32_t wStride);
    void SetHeightStride(const int32_t hStride);

    bool SetBufferFormat(std::shared_ptr<AVBuffer> &buffer);
    void SetFormatInner(const int32_t &width);
    std::function<int32_t(uint8_t *, uint8_t *, AVCodecRect *, int32_t)> func_;
    int32_t pixcelSize_ = 1; // 1: byte
    AVCodecRect rect_;
    AVCodecRect hwRect_;
    AVCodecRect usrRect_;
    bool isEncoder_;
    bool isSharedMemory_;
    bool needResetFormat_;
    std::shared_mutex mutex_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // BUFFER_CONVERTER_H
