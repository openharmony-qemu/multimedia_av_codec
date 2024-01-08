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

#ifndef INNER_DEMUXER_DEMO_H
#define INNER_DEMUXER_DEMO_H

#include <iostream>
#include "avdemuxer.h"

namespace OHOS {
namespace MediaAVCodec {
class InnerDemuxerDemo {
public:
    InnerDemuxerDemo();
    ~InnerDemuxerDemo();
    int32_t CreateWithSource(std::shared_ptr<AVSource> source);
    void Destroy();
    int32_t SelectTrackByID(uint32_t trackIndex);
    int32_t UnselectTrackByID(uint32_t trackIndex);
    int32_t PrintInfo(int32_t tracks);
    int32_t ReadAllSamples(std::shared_ptr<AVSharedMemory> mem, int32_t tracks);
    int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> mem,
                        AVCodecBufferInfo &bufInfo, uint32_t &bufferFlag);
    int32_t SeekToTime(int64_t millisecond, Media::SeekMode mode);
    bool isEOS(std::map<uint32_t, bool>& countFlag);
    AVCodecBufferInfo sampleInfo;
    std::map<int32_t, int64_t> frames_;
    std::map<int32_t, int64_t> key_frames_;
private:
    std::shared_ptr<AVSource> avsource_ = nullptr;
    std::shared_ptr<AVDemuxer> demuxer_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::AVSharedMemory> sharedMemory_ = nullptr;
    Format source_format_;
    Format track_format_;
};
}
}
#endif
