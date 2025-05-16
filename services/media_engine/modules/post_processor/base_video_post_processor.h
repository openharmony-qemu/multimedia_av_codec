/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef BASE_VIDEO_POST_PROCESSOR_H
#define BASE_VIDEO_POST_PROCESSOR_H

#include "surface.h"
#include "sync_fence.h"
#include "buffer/avbuffer.h"
#include "meta/format.h"
#include "filter/filter.h"
#include "common/log.h"
#include "common/media_core.h"

namespace OHOS {
namespace Media {

class PostProcessorCallback {
public:
    virtual ~PostProcessorCallback() = default;
    virtual void OnError(int32_t errorCode) = 0;
    virtual void OnOutputFormatChanged(const Format &format) = 0;
    virtual void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) = 0;
    virtual void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) = 0;
};

enum VideoPostProcessorType {
    NONE,
    SUPER_RESOLUTION,
    CAMERA_INSERT_FRAME,
};

class BaseVideoPostProcessor {
public:
    BaseVideoPostProcessor() = default;
    virtual ~BaseVideoPostProcessor() = default;

    virtual Status Init() = 0;
    virtual Status Flush() = 0;
    virtual Status Stop() = 0;
    virtual Status Start() = 0;
    virtual Status Release() = 0;
    virtual Status Pause()
    {
        return Status::OK;
    }
    virtual Status NotifyEos(int64_t eosPts = 0)
    {
        return Status::OK;
    }

    virtual sptr<Surface> GetInputSurface() = 0;
    virtual Status SetOutputSurface(sptr<Surface> surface) = 0;

    virtual Status ReleaseOutputBuffer(uint32_t index, bool render) = 0;
    virtual Status RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs) = 0;

    virtual Status SetCallback(const std::shared_ptr<PostProcessorCallback> callback) = 0;
    virtual Status SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver> &receiver) = 0;

    virtual Status SetParameter(const Format &format) = 0;
    virtual Status SetPostProcessorOn(bool isPostProcessorOn) = 0;
    virtual Status SetVideoWindowSize(int32_t width, int32_t height) = 0;

    virtual Status StartSeekContinous()
    {
        return Status::OK;
    }
    virtual Status StopSeekContinous()
    {
        return Status::OK;
    }
    virtual Status SetFd(int32_t fd)
    {
        (void)fd;
        return Status::OK;
    }
    virtual void SetSeekTime(int64_t seekTimeUs, PlayerSeekMode mode)
    {
        (void)seekTimeUs;
        (void)mode;
    }
    virtual void ResetSeekInfo()
    {}
    virtual Status SetSpeed(float speed)
    {
        (void)speed;
        return Status::OK;
    }
};

} // namespace Media
} // namespace OHOS
#endif // BASE_VIDEO_POST_PROCESSOR_H
