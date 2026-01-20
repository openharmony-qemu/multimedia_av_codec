/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Interpose av_read_frame for unit tests: override symbol at link/load time
 * without touching production demuxer code. Uses AvReadFrameMockController to
 * control output order; falls back to real FFmpeg when not enabled.
 */
#include <dlfcn.h>
#include <iostream>
#include "ffmpeg_avreadframe_mock_helper.h"

using namespace OHOS::Media::Plugins::Ffmpeg;

namespace {
AvReadFrameMockController& Controller()
{
    static AvReadFrameMockController ctrl;
    return ctrl;
}

int RealAvReadFrame(AVFormatContext* ctx, AVPacket* pkt)
{
    static auto fn = reinterpret_cast<int (*)(AVFormatContext*, AVPacket*)>(
        dlsym(RTLD_NEXT, "av_read_frame"));
    return fn ? fn(ctx, pkt) : AVERROR(EINVAL);
}
} // namespace

// ----------- C API exposed to tests -----------
extern "C" {
void AvReadFrameMockSetOrder(const int* streams, int count)
{
    std::vector<int> order;
    if (streams != nullptr && count > 0) {
        order.assign(streams, streams + count);
    }
    Controller().SetOrder(order);
}

void AvReadFrameMockEnable(int enable)
{
    Controller().Enable(enable != 0);
}

void AvReadFrameMockLogEnable(int enable)
{
    Controller().SetLogEnabled(enable != 0);
}

void AvReadFrameMockReset()
{
    Controller().Reset();
}

uint64_t AvReadFrameMockGetReadCount()
{
    return Controller().GetReadCount();
}
} // extern "C"

// ----------- Interposed av_read_frame -----------
extern "C" int av_read_frame(AVFormatContext* ctx, AVPacket* pkt)
{
    // Controller internally calls back to real av_read_frame when not enabled
    // or when order queue is empty.
    Controller().SetRealReadFrame(RealAvReadFrame);
    int ret = Controller().MockReadFrame(ctx, pkt);
    if (Controller().IsLogEnabled()) {
        if (ret >= 0 && pkt != nullptr) {
            std::cout << "[INTERPOSE] av_read_frame ret=" << ret << ", stream_index=" << pkt->stream_index <<
                      ", pts=" << pkt->pts << ", dts=" << pkt->dts << std::endl;
        } else {
            std::cout << "[INTERPOSE] av_read_frame ret=" << ret << std::endl;
        }
    }
    return ret;
}

