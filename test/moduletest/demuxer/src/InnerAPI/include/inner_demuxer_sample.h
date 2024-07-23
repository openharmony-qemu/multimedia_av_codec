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

#ifndef INNER_DEMUXER_SAMPLE_H
#define INNER_DEMUXER_SAMPLE_H

#include <map>
#include "avdemuxer.h"
#include "avsource.h"

namespace OHOS {
namespace MediaAVCodec {
using AVBuffer = OHOS::Media::AVBuffer;
using AVAllocator = OHOS::Media::AVAllocator;
using AVAllocatorFactory = OHOS::Media::AVAllocatorFactory;
using MemoryFlag = OHOS::Media::MemoryFlag;
using FormatDataType = OHOS::Media::FormatDataType;

class InnerDemuxerSample {
public:
    InnerDemuxerSample();
    ~InnerDemuxerSample();
    size_t GetFileSize(const std::string& filePath);
    std::shared_ptr<AVDemuxer> demuxer_ = nullptr;
    std::shared_ptr<OHOS::Media::AVBuffer> avBuffer;
private:
    int32_t InitWithFile(const std::string &path, bool local);
    int32_t ReadSampleAndSave();
    int32_t CheckPtsFromIndex();
    int32_t CheckIndexFromPts();
    int32_t CheckHasTimedMeta();
    void CheckLoop(int32_t metaTrack);
    int32_t CheckTimedMetaFormat(int32_t trackIndex, int32_t srcTrackIndex);
    int32_t CheckTimedMeta(int32_t metaTrack);
    void CheckLoopForSave();
    void CheckLoopForIndex(int32_t i);
    void CheckLoopForPts(int32_t i);
    std::map<uint32_t, int64_t> videoIndexPtsMap;
    std::map<uint32_t, int64_t> audioIndexPtsMap;
    std::shared_ptr<AVSource> avsource_ = nullptr;
    Format source_format_;
    Format track_format_;
    int32_t fd;
    int32_t trackCount;
    int64_t duration;
    int32_t videoTrackIdx;
    int64_t usleepTime = 100000;
    bool isVideoEosFlagForMeta = false;
    bool isMetaEosFlagForMeta = false;
    uint32_t videoIndexForMeta = 0;
    uint32_t metaIndexForMeta = 0;
    int32_t retForMeta = 0;
    bool isVideoEosFlagForSave = false;
    bool isAudioEosFlagForSave = false;
    int32_t retForSave = 0;
    int32_t retForIndex;
    int32_t retForPts;
    uint32_t indexForPts = 0;
};
}
}
#endif
