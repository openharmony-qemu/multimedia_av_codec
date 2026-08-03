/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef VIDEO_ENCODER_OBJECT_H
#define VIDEO_ENCODER_OBJECT_H

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include "native_avcodec_base.h"
#include "native_avcodec_videoencoder.h"
#include "native_avmagic.h"
#include "avcodec_video_encoder.h"
#include "buffer/avsharedmemory.h"
#include "native_mfmagic.h"

namespace OHOS {
namespace MediaAVCodec {

class NativeVideoEncoderCallback;

struct VideoEncoderObject : public OH_AVCodec {
    explicit VideoEncoderObject(const std::shared_ptr<AVCodecVideoEncoder>& encoder);
    ~VideoEncoderObject() = default;

    void ClearBufferList();
    void UpdateOutputBufferState(uint32_t index, uint8_t state);
    void ReleaseOutputBufferWrapper(uint32_t index);
    void ClearOutputBufferForUnbind();
    void MarkOutputBufferPending(uint32_t index);
    void StopCallback();
    void FormatToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVFormat>>& tempMap);
    void BufferToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>>& tempMap);
    void MemoryToTempFunc(std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>>& tempMap);
    OH_AVBuffer* GetTransData(const uint32_t& index, std::shared_ptr<AVBuffer>& buffer, bool isOutput);
    void GetTransData(const uint32_t& index, std::shared_ptr<AVBuffer>& buffer, bool isOutput,
        OHOS::sptr<OH_AVBuffer>& object);
    OH_AVMemory* GetTransData(const uint32_t& index, std::shared_ptr<AVSharedMemory>& memory, bool isOutput);
    OH_AVFormat* GetTransData(const uint32_t& index, std::shared_ptr<Media::Format>& format);

    const std::shared_ptr<AVCodecVideoEncoder> videoEncoder_;
    std::queue<OHOS::sptr<MFObjectMagic>> tempList_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVFormat>> inputFormatMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> outputMemoryMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVMemory>> inputMemoryMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> outputBufferMap_;
    std::unordered_map<uint32_t, OHOS::sptr<OH_AVBuffer>> inputBufferMap_;
    std::unordered_map<uint32_t, uint8_t> outputBufferStateMap_;
    std::shared_ptr<NativeVideoEncoderCallback> callback_ = nullptr;
    bool isSetMemoryCallback_ = false;
    bool isInputSurfaceMode_ = false;
    std::shared_mutex objListMutex_;
};

class NativeVideoEncoderCallback : public AVCodecCallback,
                                   public MediaCodecCallback,
                                   public MediaCodecParameterCallback {
public:
    NativeVideoEncoderCallback(struct OH_AVCodec* codec, struct OH_AVCodecAsyncCallback cb, void* userData);
    NativeVideoEncoderCallback(struct OH_AVCodec* codec, OH_VideoEncoder_OnNeedInputParameter onInputParameter,
                               void* userData);
    NativeVideoEncoderCallback(struct OH_AVCodec* codec, struct OH_AVCodecCallback cb, void* userData);
    virtual ~NativeVideoEncoderCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Media::Format& format) override;
    void OnOutputBufferUnbinded() override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                 std::shared_ptr<AVSharedMemory> buffer) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    void OnInputParameterAvailable(uint32_t index, std::shared_ptr<Media::Format> parameter) override;
    void StopCallback();
    void UpdateCallback(const struct OH_AVCodecAsyncCallback& cb, void* userData);
    void UpdateCallback(const OH_VideoEncoder_OnNeedInputParameter& onInputParameter, void* userData);
    void UpdateCallback(const struct OH_AVCodecCallback& cb, void* userData);

private:
    struct OH_AVCodec* codec_ = nullptr;
    struct OH_AVCodecAsyncCallback asyncCallback_ = {nullptr, nullptr, nullptr, nullptr};
    struct OH_AVCodecCallback callback_ = {nullptr, nullptr, nullptr, nullptr};
    OH_VideoEncoder_OnNeedInputParameter onInputParameter_ = nullptr;
    void* userData_ = nullptr;
    void* paramUserData_ = nullptr;
    std::shared_mutex mutex_;
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // VIDEO_ENCODER_OBJECT_H
