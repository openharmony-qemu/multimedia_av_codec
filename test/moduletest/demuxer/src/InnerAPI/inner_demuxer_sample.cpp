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
        fd = 0;
    }
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
        printf("Source is null\n");
        return -1;
    }
    this->demuxer_ = AVDemuxerFactory::CreateWithSource(avsource_);
    if (!demuxer_) {
        printf("AVDemuxerFactory::CreateWithSource is failed\n");
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
    int32_t trackType = 0;
    for (int32_t i = 0; i < trackCount; i++) {
        ret = this->avsource_->GetTrackFormat(track_format_, i);
        if (ret != 0) {
            printf("GetTrackFormat is failed\n");
            return ret;
        }
        track_format_.GetIntValue(OH_MD_KEY_TRACK_TYPE, trackType);
        if (trackType == MEDIA_TYPE_VID) {
            videoTrackIdx = i;
        }
        ret = this->demuxer_->SelectTrackByID(i);
        if (ret != 0) {
            printf("SelectTrackByID is failed\n");
            return ret;
        }
    }
    return ret;
}

int32_t InnerDemuxerSample::ReadSampleAndSave()
{
    bool isVideoEosFlag = false;
    bool isAudioEosFlag = false;
    uint32_t videoIndex = 0;
    uint32_t audioIndex = 0;
    uint32_t buffersize = 1024 * 1024;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    int32_t ret = 0;
    while (!isVideoEosFlag && !isAudioEosFlag) {
        for (int32_t i = 0; i < trackCount; i++) {
            ret = this->demuxer_->ReadSampleBuffer(i, avBuffer);
            if (ret != 0) {
                cout << "ReadSampleBuffer fail ret:" << ret << endl;
                isVideoEosFlag = true;
                isAudioEosFlag = true;
                break;
            }
            if (avBuffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
                if (i == videoTrackIdx) {
                    isVideoEosFlag = true;
                } else {
                    isAudioEosFlag = true;
                }
                continue;
            }
            if (i == videoTrackIdx) {
                videoIndexPtsMap.emplace(videoIndex, avBuffer->pts_);
                videoIndex ++;
            } else {
                audioIndexPtsMap.emplace(audioIndex, avBuffer->pts_);
                audioIndex ++;
            }
        }
    }
    return ret;
}

int32_t InnerDemuxerSample::CheckPtsFromIndex()
{
    int32_t ret;
    int64_t presentationTimeUs = 0;
    uint32_t index = 0;
    int32_t num = 999;
    for (int32_t i = 0; i < trackCount; i++) {
        if (ret == num) {
            break;
        }
        index = 0;
        if (i == videoTrackIdx) {
            for (const auto &pair : videoIndexPtsMap) {
                ret = demuxer_->GetPresentationTimeUsByFrameIndex(i, index, presentationTimeUs);
                cout << "video GetPresentationTimeUsByFrameIndex ret:" << ret << endl;
                if (ret != 0) {
                    cout << "GetPresentationTimeUsByFrameIndex fail ret:" << ret << endl;
                    break;
                }
                if (presentationTimeUs != pair.second) {
                    ret = num;
                    cout << "video pts != pair.second  pts:" << presentationTimeUs << " pair.second:"<< pair.second << endl;
                    break;
                }
                index ++;
            }
        } else {
            for (const auto &pair : audioIndexPtsMap) {
                ret = demuxer_->GetPresentationTimeUsByFrameIndex(i, index, presentationTimeUs);
                cout << "audio GetPresentationTimeUsByFrameIndex ret:" << ret << endl;
                if (ret != 0) {
                    cout << "GetPresentationTimeUsByFrameIndex fail ret:" << ret << endl;
                    break;
                }
                if (presentationTimeUs != pair.second) {
                    ret = 999;
                    cout << "audio pts != pair.second  pts:" << presentationTimeUs << " pair.second:"<< pair.second << endl;
                    break;
                }
                index ++;
            }
        }
    }
    cout << "CheckPtsFromIndex ret:" << ret << endl;
    return ret;
}

int32_t InnerDemuxerSample::CheckIndexFromPts()
{
    int32_t ret;
    uint32_t index = 0;
    int32_t num = 999;
    for (int32_t i = 0; i < trackCount; i++) {
        if (ret == num) {
            break;
        }
        if (i == videoTrackIdx) {
            for (const auto &pair : videoIndexPtsMap) {
                ret = demuxer_->GetFrameIndexByPresentationTimeUs(i, pair.second, index);
                if (ret != 0) {
                    cout << "video GetFrameIndexByPresentationTimeUs fail ret:" << ret << endl;
                    break;
                }
                if (index != pair.first) {
                    ret = num;
                    cout << "video pts != pair.second  index:" << index << " pair.first:"<< pair.first << endl;
                    break;
                }
            }
        } else {
            for (const auto &pair : audioIndexPtsMap) {
                ret = demuxer_->GetFrameIndexByPresentationTimeUs(i, pair.second, index);
                if (ret != 0) {
                    cout << "audio GetFrameIndexByPresentationTimeUs fail ret:" << ret << endl;
                    break;
                }
                if (index != pair.first) {
                    ret = num;
                    cout << "audio pts != pair.second  index:" << index << " pair.first:"<< pair.first << endl;
                    break;
                }
            }
        }
    }
    cout << "CheckIndexFromPts ret:" << ret << endl;
    return ret;
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
    bool isVideoEosFlag = false;
    bool isMetaEosFlag = false;
    uint32_t videoIndex = 0;
    uint32_t metaIndex = 0;
    uint32_t buffersize = 1024 * 1024;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuffer = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    int32_t ret = 0;
    int32_t compaseSize = 0;
    int32_t metaSize = 0;
    uint32_t twoHundredAndTen = 210;
    while (!isVideoEosFlag && !isMetaEosFlag && ret == 0) {
        for (int32_t i = 0; i < trackCount; i++) {
            ret = this->demuxer_->ReadSampleBuffer(i, avBuffer);
            if (ret != 0) {
                isVideoEosFlag = true;
                isMetaEosFlag = true;
                break;
            }
            if (avBuffer->flag_ == AVCODEC_BUFFER_FLAG_EOS) {
                if (i == videoTrackIdx) {
                    isVideoEosFlag = true;
                } else if (i == metaTrack) {
                    isMetaEosFlag = true;
                }
                continue;
            }
            if (i == videoTrackIdx) {
                compaseSize = static_cast<int32_t>(avBuffer->memory_->GetSize());
                if (metaTrack == 0 && metaSize != compaseSize) {
                        ret = -1;
                        break;
                }
                videoIndex ++;
            } else if (i == metaTrack) {
                metaSize = static_cast<int32_t>(avBuffer->memory_->GetSize());
                if (metaTrack != 0 && metaSize != compaseSize) {
                    ret = -1;
                    break;
                }
                metaIndex ++;
            }
        }
    }
    if (videoIndex != twoHundredAndTen || metaIndex != twoHundredAndTen) {
        ret = -1;
    }
    return ret;
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
}
}