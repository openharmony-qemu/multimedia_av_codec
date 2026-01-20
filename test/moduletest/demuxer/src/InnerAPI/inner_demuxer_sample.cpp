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

#include <cinttypes>
#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <malloc.h>
#include <string>
#include <thread>
#include <sys/stat.h>
#include <fstream>
#include <ctime>
#include "native_avbuffer.h"
#include "inner_demuxer_sample.h"
#include "native_avcodec_base.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "native_avformat.h"
#include "av_common.h"
#include <random>
#include "avmuxer.h"
#include "media_description.h"

using namespace std;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
namespace OHOS {
namespace MediaAVCodec {
InnerDemuxerSample::InnerDemuxerSample()
{
}

InnerDemuxerSample::~InnerDemuxerSample()
{
    if (fd > 0) {
        close(fd);
        fd = -1;
    }
}

bool InnerDemuxerSample::CreateBuffer()
{
    uint32_t buffersize = 1024 * 1024;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    if (!avBuffer) {
        return false;
    }
    return true;
}
int32_t InnerDemuxerSample::InitWithFile(const std::string &path, bool local)
{
    if (local) {
        fd = open(path.c_str(), O_RDONLY);
        int64_t size = GetFileSize(path);
        this->avsource_ = AVSourceFactory::CreateWithFD(fd, 0, size);
    } else {
        this->avsource_ = AVSourceFactory::CreateWithURI(const_cast<char*>(path.data()));
    }
    if (!avsource_) {
        return -1;
    }
    this->demuxer_ = AVDemuxerFactory::CreateWithSource(avsource_);
    if (!demuxer_) {
        return -1;
    }
    int32_t ret = this->avsource_->GetSourceFormat(source_format_);
    if (ret != 0) {
        printf("GetSourceFormat is failed\n");
        return ret;
    }
    source_format_.GetIntValue(OH_MD_KEY_TRACK_COUNT, trackCount);
    source_format_.GetLongValue(OH_MD_KEY_DURATION, duration);
    printf("====>total tracks:%d duration:%" PRId64 "\n", trackCount, duration);
    for (int32_t i = 0; i < trackCount; i++) {
        int32_t trackType = -1;
        ret = this->avsource_->GetTrackFormat(track_format_, i);
        if (ret != 0) {
            return ret;
        }
        track_format_.GetIntValue(OH_MD_KEY_TRACK_TYPE, trackType);
        if (trackType == MEDIA_TYPE_VID) {
            GetHdrType();
            videoIsEnd = false;
            videoTrackIdx = i;
        } else {
            audioIsEnd = false;
        }
        if (unSelectTrack != i && (trackType == MEDIA_TYPE_VID || trackType == MEDIA_TYPE_AUD ||
            trackType == MEDIA_TYPE_SUBTITLE || trackType == MEDIA_TYPE_TIMED_METADATA ||
            trackType == MEDIA_TYPE_AUXILIARY)) {
            ret = this->demuxer_->SelectTrackByID(i);
            if (ret != 0) {
                printf("SelectTrackByID is failed\n");
                return ret;
            }
        }
    }
    return ret;
}

void InnerDemuxerSample::GetHdrType()
{
    if (!track_format_.GetIntValue(Media::Tag::VIDEO_HDR_TYPE, hdrType)) {
        cout << "get hdr type failed" << endl;
        getHdrMetadata = false;
    }
}
int32_t InnerDemuxerSample::ReadSampleAndSave()
{
    uint32_t buffersize = 1024 * 1024;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    while (!isAudioEosFlagForSave && !isVideoEosFlagForSave) {
        CheckLoopForSave();
    }
    videoIndexPtsList.sort();
    audioIndexPtsList.sort();
    return retForSave;
}

void InnerDemuxerSample::CheckLoopForSave()
{
    for (int32_t i = 0; i < trackCount; i++) {
        retForSave = this->demuxer_->ReadSampleBuffer(i, avBuffer);
        if (retForSave != 0) {
            cout << "ReadSampleBuffer fail ret:" << retForSave << endl;
            isVideoEosFlagForSave = true;
            isAudioEosFlagForSave = true;
            break;
        }
        if (avBuffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
            if (i == videoTrackIdx) {
                isVideoEosFlagForSave = true;
            } else {
                isAudioEosFlagForSave = true;
            }
            continue;
        }
        if (i == videoTrackIdx) {
            videoIndexPtsList.push_back(avBuffer->pts_);
            videoIndexForRead ++;
            if (videoIndexForRead == 1) {
                videoPtsOffset = avBuffer->pts_;
            }
        } else {
            audioIndexPtsList.push_back(avBuffer->pts_);
            audioIndexForRead ++;
            if (audioIndexForRead == 1) {
                audioPtsOffset = avBuffer->pts_;
            }
        }
    }
}

int32_t InnerDemuxerSample::CheckPtsFromIndex()
{
    int32_t num = 999;
    for (int32_t i = 0; i < trackCount; i++) {
        if (retForPts == num) {
            break;
        }
        indexForPts = 0;
        CheckLoopForPtsFromIndex(i);
    }
    cout << "CheckPtsFromIndex ret:" << retForPts << endl;
    return retForPts;
}

int32_t InnerDemuxerSample::CheckIndexFromPts()
{
    int32_t num = 999;
    for (int32_t i = 0; i < trackCount; i++) {
        if (retForIndex == num) {
            break;
        }
        listIndex = 0;
        previousValue = 0;
        CheckLoopForIndexFromPts(i);
    }
    cout << "CheckIndexFromPts ret:" << retForIndex << endl;
    return retForIndex;
}

void InnerDemuxerSample::CheckLoopForPtsFromIndex(int32_t trackIndex)
{
    uint64_t presentationTimeUs = 0;
    int32_t num = 999;
    if (trackIndex == videoTrackIdx) {
        for (const auto &pair : videoIndexPtsList) {
            retForPts = demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, indexForPts, presentationTimeUs);
            if (retForPts != 0) {
                cout << "GetRelativePresentationTimeUsByIndex fail ret:" << retForPts << endl;
                break;
            }
            if (!((pair - videoPtsOffset - 1) <= presentationTimeUs <= (pair - videoPtsOffset + 1))) {
                retForPts = num;
                break;
            }
            indexForPts ++;
        }
    } else {
        for (const auto &pair : audioIndexPtsList) {
            retForPts = demuxer_->GetRelativePresentationTimeUsByIndex(trackIndex, indexForPts, presentationTimeUs);
            if (retForPts != 0) {
                cout << "GetRelativePresentationTimeUsByIndex fail ret:" << retForPts << endl;
                break;
            }
            if (!((pair - audioPtsOffset - 1) <= presentationTimeUs <= (pair - audioPtsOffset + 1))) {
                retForPts = num;
                break;
            }
            indexForPts ++;
        }
    }
}
void InnerDemuxerSample::CheckLoopForIndexFromPts(int32_t trackIndex)
{
    if (trackIndex == videoTrackIdx) {
        GetIndexByPtsForVideo(trackIndex);
    } else {
        GetIndexByPtsForAudio(trackIndex);
    }
}

void InnerDemuxerSample::GetIndexByPtsForVideo(int32_t trackIndex)
{
    int division = 2;
    int value = 1;
    for (const auto &pair : videoIndexPtsList) {
        uint64_t tempValue = pair;
        uint64_t relativePresentationTimeUs = static_cast<uint64_t>(pair - videoPtsOffset);
        GetIndexFromPtsForVideo(trackIndex, relativePresentationTimeUs, pair, division, value);
        if (retForIndex != 0) {
            cout << "video GetIndexByRelativePresentationTimeUs fail ret:" << retForIndex << endl;
            break;
        }
        retForIndex = CheckIndex(indexVideo);
        if (retForIndex != 0) {
            break;
        }
        previousValue = tempValue;
        listIndex ++;
    }
}

void InnerDemuxerSample::GetIndexByPtsForAudio(int32_t trackIndex)
{
    int division = 2;
    int value = 1;
    for (const auto &pair : audioIndexPtsList) {
        uint64_t tempValue = pair;
        uint64_t relativePresentationTimeUs = static_cast<uint64_t>(pair - audioPtsOffset);
        GetIndexFromPtsForAudio(trackIndex, relativePresentationTimeUs, pair, division, value);
        if (retForIndex != 0) {
            cout << "audio GetIndexByRelativePresentationTimeUs fail ret:" << retForIndex << endl;
            break;
        }
        retForIndex = CheckIndex(indexAudio);
        if (retForIndex != 0) {
            break;
        }
        
        previousValue = tempValue;
        listIndex ++;
    }
}

void InnerDemuxerSample::GetIndexFromPtsForVideo(int32_t trackIndex,
    uint64_t relativePresentationTimeUs, int64_t pair, int division, int value)
{
    if (isPtsExist && listIndex != 0) {
        if (isPtsCloseLeft) {
            if (division != 0) {
                retForIndex = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex,
                    relativePresentationTimeUs - ((pair - previousValue) / division + value), indexVideo);
            }
        } else if (isPtsCloseCenter) {
            if (division != 0) {
                retForIndex = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex,
                    relativePresentationTimeUs - ((pair - previousValue) / division), indexVideo);
            }
        } else {
            if (division != 0) {
                retForIndex = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex,
                    relativePresentationTimeUs - ((pair - previousValue) / division - value), indexVideo);
            }
        }
    } else {
        retForIndex = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex,
            relativePresentationTimeUs, indexVideo);
    }
}

void InnerDemuxerSample::GetIndexFromPtsForAudio(int32_t trackIndex,
    uint64_t relativePresentationTimeUs, int64_t pair, int division, int value)
{
    if (isPtsExist && listIndex != 0) {
        if (isPtsCloseLeft) {
            if (division != 0) {
                retForIndex = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex,
                    relativePresentationTimeUs - ((pair - previousValue) / division + value), indexAudio);
            }
        } else if (isPtsCloseCenter) {
            if (division != 0) {
                retForIndex = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex,
                    relativePresentationTimeUs - ((pair - previousValue) / division), indexAudio);
            }
        } else {
            if (division != 0) {
                retForIndex = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex,
                    relativePresentationTimeUs - ((pair - previousValue) / division - value), indexAudio);
            }
        }
    } else {
        retForIndex = demuxer_->GetIndexByRelativePresentationTimeUs(trackIndex,
            relativePresentationTimeUs, indexAudio);
    }
}

int32_t InnerDemuxerSample::CheckIndex(uint32_t index)
{
    int32_t num = 999;
    if (isPtsExist) {
        if (isPtsCloseLeft && listIndex != 0) {
            if (index != (listIndex -1)) {
                retForIndex = num;
                cout << "pts != pair  index:" << index << " listIndex - 1 :"<< (listIndex - 1) << endl;
            }
        } else if (isPtsCloseCenter && listIndex != 0) {
            if (index != listIndex && index != (listIndex - 1)) {
                retForIndex = num;
                cout << "pts != pair index:" << index << " listIndex - 1 :"<< (listIndex - 1) << endl;
            }
        } else {
            if (index != listIndex) {
                retForIndex = num;
                cout << "pts != pair  index:" << index << " listIndex:"<< listIndex << endl;
            }
        }
    } else {
        if (index != listIndex) {
            retForIndex = num;
            cout << "pts != pair  index:" << index << " listIndex:"<< listIndex << endl;
        }
    }
    return retForIndex;
}
int32_t InnerDemuxerSample::CheckHasTimedMeta()
{
    int32_t hasMeta = 0;
    source_format_.GetIntValue(AVSourceFormat::SOURCE_HAS_TIMEDMETA, hasMeta);
    return hasMeta;
}

int32_t InnerDemuxerSample::CheckTimedMetaFormat(int32_t trackIndex, int32_t srcTrackIndex)
{
    int32_t ret = this->avsource_->GetTrackFormat(track_format_, trackIndex);
    if (ret != 0) {
        cout << "get track_format_ fail" << endl;
        return -1;
    }
    int32_t trackType = 0;
    std::string codecMime = "";
    std::string timedMetadataKey = "";
    int32_t srcTrackID = -1;
    std::string TIMED_METADATA_KEY = "com.openharmony.timed_metadata.test";
    track_format_.GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, trackType);
    if (trackType != MediaType::MEDIA_TYPE_TIMED_METADATA) {
        cout << "check MD_KEY_TRACK_TYPE fail" << endl;
        return -1;
    }
    track_format_.GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, codecMime);
    if (codecMime != "meta/timed-metadata") {
        cout << "check codecMime fail" << endl;
        return -1;
    }
    track_format_.GetStringValue(MediaDescriptionKey::MD_KEY_TIMED_METADATA_KEY, timedMetadataKey);
    if (timedMetadataKey != TIMED_METADATA_KEY) {
        cout << "check MD_KEY_TIMED_METADATA_KEY fail" << endl;
        return -1;
    }
    track_format_.GetIntValue(MediaDescriptionKey::MD_KEY_TIMED_METADATA_SRC_TRACK_ID, srcTrackID);
    if (srcTrackID != srcTrackIndex) {
        cout << "check MD_KEY_TIMED_METADATA_SRC_TRACK_ID fail" << endl;
        return -1;
    }
    return 0;
}

int32_t InnerDemuxerSample::CheckTimedMeta(int32_t metaTrack)
{
    uint32_t buffersize = 1024 * 1024;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    uint32_t twoHundredAndTen = 210;
    while (!isVideoEosFlagForMeta && !isMetaEosFlagForMeta && retForMeta == 0) {
        CheckLoop(metaTrack);
    }
    if (videoIndexForMeta != twoHundredAndTen || metaIndexForMeta != twoHundredAndTen) {
        retForMeta = -1;
    }
    return retForMeta;
}

void InnerDemuxerSample::CheckLoop(int32_t metaTrack)
{
    int32_t compaseSize = 0;
    int32_t metaSize = 0;
    for (int32_t i = 0; i < trackCount; i++) {
        retForMeta = this->demuxer_->ReadSampleBuffer(i, avBuffer);
        if (retForMeta != 0) {
            isVideoEosFlagForMeta = true;
            isMetaEosFlagForMeta = true;
            break;
        }
        if (avBuffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
            if (i == videoTrackIdx) {
                isVideoEosFlagForMeta = true;
            } else if (i == metaTrack) {
                isMetaEosFlagForMeta = true;
            }
            continue;
        }
        if (i == videoTrackIdx) {
            compaseSize = static_cast<int32_t>(avBuffer->memory_->GetSize());
            if (metaTrack == 0 && metaSize != compaseSize - 1) {
                    retForMeta = -1;
                    break;
            }
            videoIndexForMeta ++;
        } else if (i == metaTrack) {
            metaSize = static_cast<int32_t>(avBuffer->memory_->GetSize());
            if (metaTrack != 0 && metaSize != compaseSize - 1) {
                retForMeta = -1;
                break;
            }
            metaIndexForMeta ++;
        }
    }
}

size_t InnerDemuxerSample::GetFileSize(const std::string& filePath)
{
    size_t fileSize = 0;
    if (!filePath.empty()) {
        struct stat fileStatus {};
        if (stat(filePath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<size_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

bool InnerDemuxerSample::CheckDemuxer(int32_t &readMax)
{
    for (int32_t i = 0; i < trackCount; i++) {
        int32_t ret = this->demuxer_->SelectTrackByID(i);
        if (ret != 0) {
            printf("SelectTrackByID is failed\n");
            return false;
        }
        ret = this->demuxer_->ReadSampleBuffer(i, avBuffer);
        if (ret != 0) {
            cout << "ReadSampleBuffer fail ret:" << ret << endl;
            isAudioEosFlagForSave = true;
            return false;
        }
        if (avBuffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
            printf("ReadSampleBuffer EOS\n");
            isAudioEosFlagForSave = true;
            continue;
        }
        if (avBuffer->memory_->GetSize() > readMax) {
            readMax = avBuffer->memory_->GetSize();
        }
    }
    return true;
}

bool InnerDemuxerSample::CheckApeSourceData(const std::string &path, int32_t version)
{
    fd = open(path.c_str(), O_RDONLY);
    int64_t size = GetFileSize(path);
    this->avsource_ = AVSourceFactory::CreateWithFD(fd, 0, size);
    if (!avsource_) {
        printf("Source is null\n");
        return false;
    }
    this->demuxer_ = AVDemuxerFactory::CreateWithSource(avsource_);
    if (!demuxer_) {
        printf("AVDemuxerFactory::CreateWithSource is failed\n");
        return false;
    }
    int32_t ret = this->avsource_->GetSourceFormat(source_format_);
    if (ret != 0) {
        return false;
    }
    source_format_.GetIntValue(OH_MD_KEY_TRACK_COUNT, trackCount);
    uint32_t buffersize = 2048 * 2048;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    int32_t trackType = 0;
    int32_t readSize = 0;
    int32_t frame = 0;
    int32_t sizeMax = 0;
    ret = this->avsource_->GetTrackFormat(track_format_, 0);
    if (ret != 0) {
        return false;
    }
    track_format_.GetIntValue(Media::Tag::AUDIO_SAMPLE_PER_FRAME, frame);
    track_format_.GetIntValue(Media::Tag::AUDIO_MAX_INPUT_SIZE, sizeMax);
    track_format_.GetIntValue(OH_MD_KEY_TRACK_TYPE, trackType);
    if (trackType != MEDIA_TYPE_AUD) {
        return false;
    }
    while (!isAudioEosFlagForSave) {
        if (!CheckDemuxer(readSize)) {
            return false;
        }
    }
    if (frame != version) {
        printf("frame not as expected\n");
        return false;
    }
    if (sizeMax != readSize) {
        printf("sizeMax = %d not as expected\n", sizeMax);
        return false;
    }
    return true;
}

bool InnerDemuxerSample::CheckCache(std::vector<std::vector<int32_t>> &cacheCheckSteps, int32_t times)
{
    uint32_t memoryUsage = 0;
    for (auto step : cacheCheckSteps) {
        demuxer_->GetCurrentCacheSize(step[0], memoryUsage);
        if (memoryUsage != step[times]) {
            return false;
        }
    }
    return true;
}
bool InnerDemuxerSample::ReadVideo(std::vector<std::vector<int32_t>> &cacheCheckSteps)
{
    int32_t readCount = 0;
    int32_t ret = 0;
    int32_t checkVector1 = 1;
    int32_t checkVector2 = 2;
    bool isEnd = true;
    while (isEnd) {
        if (readCount >= readPos) {
            isEnd = false;
            if (!CheckCache(cacheCheckSteps, checkVector1)) {
                return false;
            }
            ret = demuxer_->ReadSampleBuffer(indexAud, avBuffer);
            if (ret != 0) {
                cout << "ReadSampleBuffer fail ret:" << ret << endl;
                return false;
            }
            if (!CheckCache(cacheCheckSteps, checkVector2)) {
                return false;
            }
            break;
        } else {
            readCount++;
            ret = demuxer_->ReadSampleBuffer(indexVid, avBuffer);
            if (ret != 0) {
                cout << "ReadSampleBuffer fail ret:" << ret << endl;
                isEnd = false;
                return false;
            }
        }
    }
    return true;
}

bool InnerDemuxerSample::ReadAudio(std::vector<std::vector<int32_t>> &cacheCheckSteps)
{
    int32_t readCount = 0;
    int32_t ret = 0;
    int32_t checkVector1 = 1;
    int32_t checkVector2 = 2;
    bool isEnd = true;
    while (isEnd) {
        if (readCount >= readPos) {
            isEnd = false;
            if (!CheckCache(cacheCheckSteps, checkVector1)) {
                return false;
            }
            ret = demuxer_->ReadSampleBuffer(indexVid, avBuffer);
            if (ret != 0) {
                cout << "ReadSampleBuffer fail ret:" << ret << endl;
                return false;
            }
            if (!CheckCache(cacheCheckSteps, checkVector2)) {
                return false;
            }
            break;
        } else {
            readCount++;
            ret = demuxer_->ReadSampleBuffer(indexAud, avBuffer);
            if (ret != 0) {
                cout << "ReadSampleBuffer fail ret:" << ret << endl;
                isEnd = false;
                return false;
            }
        }
    }
    return true;
}

int32_t InnerDemuxerSample::ReadSample(int32_t videoFrame, int32_t audioFrame)
{
    int32_t ret = 0;
    uint32_t buffersize = 1024 * 1024;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t i = 0; i < trackCount; i++) {
            int32_t trackType = -1;
            ret = this->avsource_->GetTrackFormat(track_format_, i);
            if (ret != 0) {
                printf("GetTrackFormat is failed\n");
                return ret;
            }
            track_format_.GetIntValue(OH_MD_KEY_TRACK_TYPE, trackType);
            if ((audioIsEnd && (trackType == MEDIA_TYPE_AUD)) || (videoIsEnd && (trackType == MEDIA_TYPE_VID))) {
                continue;
            }
            if (!(trackType == MEDIA_TYPE_VID || trackType == MEDIA_TYPE_AUD ||
                trackType == MEDIA_TYPE_SUBTITLE || trackType == MEDIA_TYPE_TIMED_METADATA ||
                trackType == MEDIA_TYPE_AUXILIARY)) {
                continue;
            }
            ret = this->demuxer_->ReadSampleBuffer(i, avBuffer);
            if (ret != 0) {
                cout << "ReadSampleBuffer fail ret:" << retForSave << endl;
                break;
            }
            if (SetEos() != 0) {
                continue;
            }
            if (trackType == MEDIA_TYPE_VID) {
                videoIndexForRead ++;
            } else {
                audioIndexForRead ++;
            }
        }
    }
    if (videoIndexForRead != videoFrame || audioIndexForRead != audioFrame) {
        return -1;
    }
    return ret;
}

int32_t InnerDemuxerSample::SetEos()
{
    if (avBuffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
        if (trackType == MEDIA_TYPE_VID) {
            videoIsEnd = true;
        } else {
            audioIsEnd = true;
        }
        return -1;
    }
    return 0;  
}
}
}