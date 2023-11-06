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

#ifndef MEDIA_CODEC_LISTENER_STUB_H
#define MEDIA_CODEC_LISTENER_STUB_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include "i_standard_media_codec_listener.h"
#include "avcodec_common.h"
#include "avbuffer.h"

namespace OHOS {
namespace MediaAVCodec {
class MediaCodecListenerStub : public IRemoteStub<IStandardMediaCodecListener> {
public:
    MediaCodecListenerStub();
    virtual ~MediaCodecListenerStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnStreamChanged(const Format &format) override;
    void SurfaceModeOnBufferFilled(std::shared_ptr<Media::AVBuffer> buffer) override;
    void SetCallback(const std::shared_ptr<VideoCodecCallback> &callback);
    bool FindBufferFromIndex(uint64_t index, std::shared_ptr<Media::AVBuffer> buffer) override;

private:
    std::weak_ptr<VideoCodecCallback> callback_;
    std::mutex syncMutex_;
    class MediaCodecBufferCache;
    std::unique_ptr<MediaCodecBufferCache> outputBufferCache_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_CODEC_LISTENER_STUB_H
