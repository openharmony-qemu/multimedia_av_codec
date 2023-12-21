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

#ifndef AV_CODEC_BUFFER_INFO_H
#define AV_CODEC_BUFFER_INFO_H

#include <atomic>
#include <memory>
#include <vector>

#include "audio_common_info.h"
#include "avcodec_common.h"
#include "buffer/avsharedmemorybase.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class AudioBufferInfo : public NoCopyable {
public:
    AudioBufferInfo(const uint32_t bufferSize, const std::string_view &name, const uint32_t metaSize = 0);

    ~AudioBufferInfo();

    std::shared_ptr<Media::AVSharedMemoryBase> GetBuffer() const noexcept;

    std::shared_ptr<Media::AVSharedMemoryBase> GetMetadata() const noexcept;

    bool IsHasMetaData() const noexcept;

    bool ResetBuffer();

    bool SetBufferOwned();

    bool IsAvailable() const noexcept;

    BufferStatus GetStatus() const noexcept;

    bool CheckIsEos() const noexcept;

    void SetEos(bool eos);

    void SetBufferAttr(const AVCodecBufferInfo &attr);

    AVCodecBufferInfo GetBufferAttr() const noexcept;

    AVCodecBufferFlag GetFlag() const noexcept;

    bool CheckIsFirstFrame() const noexcept;

    void SetFirstFrame() noexcept;

    bool CheckIsUsing() const noexcept;

    void SetUsing() noexcept;

    uint32_t GetBufferSize() const noexcept;

private:
    bool isHasMeta_;
    bool isEos_;
    bool isFirstFrame_;
    std::atomic<bool> isUsing;
    std::atomic<BufferStatus> status_;
    uint32_t bufferSize_;
    uint32_t metaSize_;
    std::string_view name_;
    std::shared_ptr<Media::AVSharedMemoryBase> buffer_;
    std::shared_ptr<Media::AVSharedMemoryBase> metadata_;
    AVCodecBufferInfo info_;
    AVCodecBufferFlag flag_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif