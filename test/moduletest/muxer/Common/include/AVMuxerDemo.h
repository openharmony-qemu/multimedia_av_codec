/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

#ifndef AVMUXER_DEMO_COMMON_H
#define AVMUXER_DEMO_COMMON_H

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "avmuxer.h"
#include "nocopyable.h"
#include "native_avmuxer.h"
#include "avcodec_errors.h"

namespace OHOS {
namespace MediaAVCodec {
// only for demo
typedef struct AudioTrackParam {
    const char* mimeType;
    long long bitRate;
    int sampleFormat;
    int sampleRate;
    int channels;
    long long channelMask;
    int samplePerFrame;
} AudioTrackParam;

typedef struct VideoTrackParam {
    const char* mimeType;
    long long bitRate;
    int pixelFormat;
    int width;
    int height;
} VideoTrackParam;

class AVMuxerDemo : public NoCopyable {
public:
    AVMuxerDemo() = default;
    ~AVMuxerDemo() = default;

    int32_t GetFdByMode(OH_AVOutputFormat format);
    int32_t GetErrorFd();
    int32_t GetFdByName(OH_AVOutputFormat format, std::string fileName);
    int32_t InnerGetFdByMode(Plugins::OutputFormat format);
    int32_t InnerGetFdByName(Plugins::OutputFormat format, std::string fileName);
    // native api
    OH_AVMuxer* NativeCreate(int32_t fd, OH_AVOutputFormat format);
    OH_AVErrCode NativeSetRotation(OH_AVMuxer* muxer, int32_t rotation);
    OH_AVErrCode NativeAddTrack(OH_AVMuxer* muxer, int32_t* trackIndex, OH_AVFormat* trackFormat);
    OH_AVErrCode NativeStart(OH_AVMuxer* muxer);
    OH_AVErrCode NativeWriteSampleBuffer(OH_AVMuxer* muxer, uint32_t trackIndex,
        OH_AVMemory* sample, OH_AVCodecBufferAttr info);
    OH_AVErrCode NativeWriteSampleBuffer(OH_AVMuxer* muxer, uint32_t trackIndex,
        OH_AVBuffer* sample);
    OH_AVErrCode NativeStop(OH_AVMuxer* muxer);
    OH_AVErrCode NativeDestroy(OH_AVMuxer* muxer);

    // Inner api
    int32_t InnerCreate(int32_t fd, Plugins::OutputFormat format);
    int32_t InnerSetRotation(int32_t rotation);
    int32_t InnerAddTrack(int32_t& trackIndex, std::shared_ptr<Meta> trackDesc);
    int32_t InnerStart();
    int32_t InnerWriteSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample);
    int32_t InnerStop();
    int32_t InnerDestroy();
private:
    std::string filename = "";
    std::shared_ptr<AVMuxer> avmuxer_;
};
}
}
#endif // AVMUXER_DEMO_COMMON_H