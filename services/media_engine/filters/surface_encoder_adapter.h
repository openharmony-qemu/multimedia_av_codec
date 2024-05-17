/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef FILTERS_SURFACE_ENCODER_ADAPTER_H
#define FILTERS_SURFACE_ENCODER_ADAPTER_H

#include <cstring>
#include <shared_mutex>
#include "surface.h"
#include "meta/meta.h"
#include "buffer/avbuffer.h"
#include "buffer/avallocator.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "common/status.h"
#include "common/log.h"
#include "osal/task/task.h"
#include "avcodec_common.h"
#include "osal/task/condition_variable.h"
#include "avcodec_video_encoder.h"

namespace OHOS {
namespace MediaAVCodec {
class ICodecService;
}

namespace Media {
class EncoderAdapterCallback {
public:
    virtual ~EncoderAdapterCallback() = default;
    virtual void OnError(MediaAVCodec::AVCodecErrorType type, int32_t errorCode) = 0;
    virtual void OnOutputFormatChanged(const std::shared_ptr<Meta> &format) = 0;
};

class SurfaceEncoderAdapter : public std::enable_shared_from_this<SurfaceEncoderAdapter>  {
public:
    explicit SurfaceEncoderAdapter();
    ~SurfaceEncoderAdapter();
public:
    Status Init(const std::string &mime, bool isEncoder);
    Status Configure(const std::shared_ptr<Meta> &meta);
    Status SetOutputBufferQueue(const sptr<AVBufferQueueProducer> &bufferQueueProducer);
    Status SetEncoderAdapterCallback(const std::shared_ptr<EncoderAdapterCallback> &encoderAdapterCallback);
    Status SetInputSurface(sptr<Surface> surface);
    sptr<Surface> GetInputSurface();
    Status Start();
    Status Stop();
    Status Pause();
    Status Resume();
    Status Flush();
    Status Reset();
    Status Release();
    Status NotifyEos();
    Status SetParameter(const std::shared_ptr<Meta> &parameter);
    std::shared_ptr<Meta> GetOutputFormat();
    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    void SetFaultEvent(const std::string &errMsg);
    void SetFaultEvent(const std::string &errMsg, int32_t ret);
    void SetCallingInfo(int32_t appUid, int32_t appPid, const std::string &bundleName, uint64_t instanceId);

    std::shared_ptr<EncoderAdapterCallback> encoderAdapterCallback_;

private:
    void ReleaseBuffer();
    void ConfigureGeneralFormat(MediaAVCodec::Format &format, const std::shared_ptr<Meta> &meta);
    void ConfigureAboutRGBA(MediaAVCodec::Format &format, const std::shared_ptr<Meta> &meta);
    void ConfigureAboutEnableTemporalScale(MediaAVCodec::Format &format, const std::shared_ptr<Meta> &meta);

    std::shared_ptr<MediaAVCodec::AVCodecVideoEncoder> codecServer_;
    sptr<AVBufferQueueProducer> outputBufferQueueProducer_;

    std::shared_ptr<Task> releaseBufferTask_{nullptr};
    std::mutex releaseBufferMutex_;
    std::condition_variable releaseBufferCondition_;
    std::vector<uint32_t> indexs_;
    std::atomic<bool> isThreadExit_ = true;

    std::mutex stopMutex_;
    std::condition_variable stopCondition_;
    int64_t stopTime_{-1};

    int64_t totalPauseTime_{0};

    int64_t startBufferTime_{-1};
    int64_t lastBufferTime_{-1};
    bool isStart_ = false;
    bool isResume_ = false;
    std::string codecMimeType_;
    std::string bundleName_;
    uint64_t instanceId_{0};
    int32_t appUid_ {0};
    int32_t appPid_ {0};
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FILTERS_SURFACE_ENCODER_ADAPTER_H
