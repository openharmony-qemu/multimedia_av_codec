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
#include "common/status.h"
#include "plugin/demuxer_plugin.h"
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
    void CheckLoopForIndexFromPts(int32_t trackIndex);
    void CheckLoopForPtsFromIndex(int32_t trackIndex);
    void GetIndexByPtsForAudio(int32_t trackIndex);
    void GetIndexByPtsForVideo(int32_t trackIndex);
    void GetIndexFromPtsForVideo(int32_t trackIndex, uint64_t relativePresentationTimeUs, int64_t pair,
        int division, int value);
    void GetIndexFromPtsForAudio(int32_t trackIndex, uint64_t relativePresentationTimeUs, int64_t pair,
        int division, int value);
    int32_t CheckIndex(uint32_t index);
    bool CheckApeSourceData(const std::string &path, int32_t version);
    bool CheckDemuxer(int32_t &readMax);
    bool CheckCache(std::vector<std::vector<int32_t>> &cacheCheckSteps, int32_t times);
    bool CreateBuffer();
    bool ReadAudio(std::vector<std::vector<int32_t>> &cacheCheckSteps);
    bool ReadVideo(std::vector<std::vector<int32_t>> &cacheCheckSteps);
    int32_t ReadSample(int32_t videoFrame, int32_t audioFrame);
    void GetHdrType();
    int32_t SetEos(int32_t trackType);
    std::list<int64_t> videoIndexPtsList;
    std::list<int64_t> audioIndexPtsList;
    std::shared_ptr<AVSource> avsource_ = nullptr;
    Format source_format_;
    Format track_format_;
    int32_t fd = -1;
    int32_t trackCount;
    int64_t duration;
    int32_t videoTrackIdx;
    int64_t usleepTime = 100000;
    bool isVideoEosFlagForMeta = false;
    bool isMetaEosFlagForMeta = false;
    uint32_t videoIndexForMeta = 0;
    uint32_t metaIndexForMeta = 0;
    uint32_t videoIndexForRead = 0;
    uint32_t audioIndexForRead = 0;
    int32_t retForMeta = 0;
    bool isVideoEosFlagForSave = false;
    bool isAudioEosFlagForSave = false;
    int32_t retForSave = 0;
    int32_t retForIndex;
    int32_t retForPts;
    uint32_t indexForPts = 0;
    int64_t videoPtsOffset = 0;
    int64_t audioPtsOffset = 0;
    bool isPtsExist = false;
    bool isPtsCloseRight = false;
    bool isPtsCloseCenter = false;
    bool isPtsCloseLeft = false;
    uint32_t listIndex = 0;
    uint64_t previousValue = 0;
    uint32_t indexVideo = 0;
    uint32_t indexAudio = 0;
    uint32_t indexVid = 0;
    uint32_t indexAud = 0;
    uint32_t indexSub = 0;
    uint32_t indexData = 0;
    int32_t readPos = 0;
    int32_t unSelectTrack = -1;
    int32_t hdrType = -2;
    bool getHdrMetadata = true;
    bool audioIsEnd = true;
    bool videoIsEnd = true;
};
}
}
#endif
