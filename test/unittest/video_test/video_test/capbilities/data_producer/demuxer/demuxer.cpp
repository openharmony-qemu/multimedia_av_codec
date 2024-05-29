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

#include "demuxer.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "av_codec_sample_log.h"
#include "av_codec_sample_error.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "Demuxer"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
int32_t Demuxer::Init(SampleInfo &info)
{
    file_ = fopen(info.inputFilePath.data(), "r");
    CHECK_AND_RETURN_RET_LOG(file_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Open input file failed");
    fileFd_ = fileno(file_);
    
    fileSize_ = GetFileSize(info.inputFilePath.data());
    source_ = OH_AVSource_CreateWithFD(fileFd_, 0, fileSize_);
    CHECK_AND_RETURN_RET_LOG(source_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR,
        "Create source failed, fd: %{public}d, file size: %{public}" PRId64, fileFd_, fileSize_);
    demuxer_ = OH_AVDemuxer_CreateWithSource(source_);
    CHECK_AND_RETURN_RET_LOG(demuxer_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create demuxer failed");
    auto sourceFormat = std::shared_ptr<OH_AVFormat>(OH_AVSource_GetSourceFormat(source_), OH_AVFormat_Destroy);
    CHECK_AND_RETURN_RET_LOG(sourceFormat != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Get source format failed");

    int32_t ret = GetVideoTrackInfo(sourceFormat, info);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Get video track info failed");

    sampleInfo_ = info;
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t Demuxer::ReadSample(CodecBufferInfo &bufferInfo)
{
    if ((frameCount_ >= sampleInfo_.maxFrames) || (feof(file_) && !Repeat())) {
        bufferInfo.attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        return AVCODEC_SAMPLE_ERR_OK;
    }

    int32_t ret = 0;
    if (static_cast<uint8_t>(sampleInfo_.codecRunMode) & 0b10) {  // ob10: AVBuffer mode mask
        ret = OH_AVDemuxer_ReadSampleBuffer(demuxer_, videoTrackId_,
            reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer));
        (void)OH_AVBuffer_GetBufferAttr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer), &bufferInfo.attr);
    } else {
        ret = OH_AVDemuxer_ReadSample(demuxer_, videoTrackId_,
            reinterpret_cast<OH_AVMemory *>(bufferInfo.buffer), &bufferInfo.attr);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Read sample failed");
    frameCount_++;
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t Demuxer::Seek(int64_t position)
{
    int32_t ret = OH_AVDemuxer_SeekToTime(demuxer_, position, sampleInfo_.dataProducerInfo.seekMode);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Seek failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t Demuxer::Release()
{
    fclose(file_);
    file_ = nullptr;
    if (demuxer_ != nullptr) {
        OH_AVDemuxer_Destroy(demuxer_);
        demuxer_ = nullptr;
    }
    if (source_ != nullptr) {
        OH_AVSource_Destroy(source_);
        source_ = nullptr;
    }
    return AVCODEC_SAMPLE_ERR_OK;
}

int64_t Demuxer::GetFileSize(char * const filePath)
{
    CHECK_AND_RETURN_RET_LOG(filePath != nullptr, 0, "File path is nullptr");

    struct stat fileStatus {};
    int32_t ret = stat(filePath, &fileStatus);
    CHECK_AND_RETURN_RET_LOG(ret == 0, 0, "stat file error: %{public}d", errno);
    fileSize_ = static_cast<int64_t>(fileStatus.st_size);
    return fileSize_;
}

int32_t Demuxer::GetVideoTrackInfo(std::shared_ptr<OH_AVFormat> sourceFormat, SampleInfo &info)
{
    int32_t trackCount = 0;
    OH_AVFormat_GetIntValue(sourceFormat.get(), OH_MD_KEY_TRACK_COUNT, &trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        int trackType = -1;
        
        auto trackFormat =
            std::shared_ptr<OH_AVFormat>(OH_AVSource_GetTrackFormat(source_, index), OH_AVFormat_Destroy);
        OH_AVFormat_GetIntValue(trackFormat.get(), OH_MD_KEY_TRACK_TYPE, &trackType);
        if (trackType == MEDIA_TYPE_VID) {
            OH_AVDemuxer_SelectTrackByID(demuxer_, index);
            OH_AVFormat_GetIntValue(trackFormat.get(), OH_MD_KEY_WIDTH, &info.videoWidth);
            OH_AVFormat_GetIntValue(trackFormat.get(), OH_MD_KEY_HEIGHT, &info.videoHeight);
            if (info.frameRate == SAMPLE_DEFAULT_FRAMERATE) {
                OH_AVFormat_GetDoubleValue(trackFormat.get(), OH_MD_KEY_FRAME_RATE, &info.frameRate);
            }
            int32_t isHDRVivid = 0;
            OH_AVFormat_GetIntValue(trackFormat.get(), "video_is_hdr_vivid", &isHDRVivid);
            if (isHDRVivid == 1) {
                info.isHDRVivid = true;
            }
            char *codecMime;
            OH_AVFormat_GetStringValue(trackFormat.get(), OH_MD_KEY_CODEC_MIME, const_cast<char const **>(&codecMime));
            info.codecMime = codecMime;
            OH_AVFormat_GetIntValue(trackFormat.get(), OH_MD_KEY_PROFILE, &info.hevcProfile);
            videoTrackId_ = index;
        }
    }
    OH_AVFormat_GetLongValue(sourceFormat.get(), OH_MD_KEY_DURATION, &info.videoDuration);

    return AVCODEC_SAMPLE_ERR_OK;
}
} // Sample
} // MediaAVCodec
} // OHOS