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

#ifndef DRAGGING_PLAYER_H
#define DRAGGING_PLAYER_H

#include "demuxer_filter.h"
#include "decoder_surface_filter.h"
#include "common/log.h"
#include "common/status.h"
#include "meta/format.h"

namespace OHOS {
namespace Media {

// 模块主接口类
class __attribute__((visibility("default"))) DraggingPlayer {
public:
    virtual ~DraggingPlayer() = default;
    virtual Status Init(std::shared_ptr<Pipeline::DemuxerFilter> &demuxer,
                        std::shared_ptr<Pipeline::DecoderSurfaceFilter> &decoder) = 0;
    // 数据轮转接口
    virtual void UpdateSeekPos(int64_t seekMs) = 0;
    // 解码送帧控制
    virtual bool IsVideoStreamDiscardable(std::shared_ptr<AVBuffer> avBuffer) = 0;
    // 显示送帧控制
    virtual void ConsumeVideoFrame(std::shared_ptr<AVBuffer> AVBuffer, uint32_t bufferIndex) = 0;
    virtual void Release() = 0;
};

extern "C" __attribute__((visibility("default"))) DraggingPlayer *CreateDraggingPlayer();
extern "C" __attribute__((visibility("default"))) void DestroyDraggingPlayer(DraggingPlayer *draggingPlayer);

} // namespace Media
} // namespace OHOS
#endif // DRAGGING_PLAYER_H
