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

#ifndef AVDEMUXER_DEMO_H
#define AVDEMUXER_DEMO_H

#include <memory>
#include <vector>
#include <map>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/dict.h"
#ifdef __cplusplus
}
#endif

#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "native_avdemuxer.h"

namespace OHOS {
namespace MediaAVCodec {
class AVDemuxerDemo {
public:
    AVDemuxerDemo();
    ~AVDemuxerDemo();
    int32_t CreateWithSource(OH_AVSource *avsource);
    int32_t Destroy();
    int32_t ReadAllSamples(OH_AVMemory *sample, int32_t tracks);
    int32_t SelectTrackByID(uint32_t trackIndex);
    int32_t UnselectTrackByID(uint32_t trackIndex);
    int32_t PrintInfo(int32_t tracks);
    bool isEOS(std::map<uint32_t, bool>& countFlag);
    int32_t ReadSample(uint32_t trackIndex, OH_AVMemory *sample, OH_AVCodecBufferAttr *bufferAttr);
    int32_t SeekToTime(int64_t millisecond, OH_AVSeekMode mode);
    // for drm test
    int32_t SetDrmAppCallback();
    void GetMediaKeySystemInfo();

    OH_AVCodecBufferAttr bufferInfo;
    std::map<int32_t, int64_t> frames_;
    std::map<int32_t, int64_t> key_frames_;
private:
    OH_AVSource* avsource_ = nullptr;
    OH_AVDemuxer* avdemxuer_ = nullptr;
};
}  // namespace MediaAVCodec
}  // namespace OHOS
#endif