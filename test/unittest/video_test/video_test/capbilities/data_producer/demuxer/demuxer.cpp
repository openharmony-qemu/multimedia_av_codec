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
int32_t Demuxer::Init(const std::shared_ptr<SampleInfo> &info)
{
    CHECK_AND_RETURN_RET_LOG(file_ == nullptr && source_ == nullptr && demuxer_ == nullptr,
        AVCODEC_SAMPLE_ERR_ERROR, "Demuxer has already initialized");
    sampleInfo_ = info;

    file_ = std::shared_ptr<FILE>(fopen(sampleInfo_->inputFilePath.data(), "r"), fclose);
    CHECK_AND_RETURN_RET_LOG(file_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Open input file failed");
    int32_t fileFd = fileno(file_.get());
    
    int64_t fileSize = GetFileSize(fileFd);
    source_ = std::shared_ptr<OH_AVSource>(OH_AVSource_CreateWithFD(fileFd, 0, fileSize), OH_AVSource_Destroy);
    CHECK_AND_RETURN_RET_LOG(source_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR,
        "Create source failed, fd: %{public}d, file size: %{public}" PRId64, fileFd, fileSize);
    demuxer_ = std::shared_ptr<OH_AVDemuxer>(OH_AVDemuxer_CreateWithSource(source_.get()), OH_AVDemuxer_Destroy);
    CHECK_AND_RETURN_RET_LOG(demuxer_ != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Create demuxer failed");
    auto sourceFormat = std::shared_ptr<OH_AVFormat>(OH_AVSource_GetSourceFormat(source_.get()), OH_AVFormat_Destroy);
    CHECK_AND_RETURN_RET_LOG(sourceFormat != nullptr, AVCODEC_SAMPLE_ERR_ERROR, "Get source format failed");

    int32_t ret = GetVideoTrackInfo(sourceFormat);
    CHECK_AND_RETURN_RET_LOG(ret == AVCODEC_SAMPLE_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Get video track info failed");

    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t Demuxer::FillBuffer(CodecBufferInfo &bufferInfo)
{
    int32_t ret = 0;
    if (static_cast<uint8_t>(sampleInfo_->codecRunMode) & 0b10) {  // ob10: AVBuffer mode mask
        ret = OH_AVDemuxer_ReadSampleBuffer(demuxer_.get(), videoTrackId_,
            reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer));
        (void)OH_AVBuffer_GetBufferAttr(reinterpret_cast<OH_AVBuffer *>(bufferInfo.buffer), &bufferInfo.attr);
    } else {
        ret = OH_AVDemuxer_ReadSample(demuxer_.get(), videoTrackId_,
            reinterpret_cast<OH_AVMemory *>(bufferInfo.buffer), &bufferInfo.attr);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Read sample failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int32_t Demuxer::Seek(int64_t position)
{
    int32_t ret = OH_AVDemuxer_SeekToTime(demuxer_.get(), position, sampleInfo_->dataProducerInfo.seekMode);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCODEC_SAMPLE_ERR_ERROR, "Seek failed");
    return AVCODEC_SAMPLE_ERR_OK;
}

int64_t Demuxer::GetFileSize(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(fd >= 0, 0, "File path is nullptr");

    struct stat fileStatus {};
    int32_t ret = fstat(fd, &fileStatus);
    CHECK_AND_RETURN_RET_LOG(ret == 0, 0, "stat file error: %{public}d", errno);
    return static_cast<int64_t>(fileStatus.st_size);
}

int32_t Demuxer::GetVideoTrackInfo(std::shared_ptr<OH_AVFormat> sourceFormat)
{
    int32_t trackCount = 0;
    OH_AVFormat_GetIntValue(sourceFormat.get(), OH_MD_KEY_TRACK_COUNT, &trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        int trackType = -1;
        
        auto trackFormat =
            std::shared_ptr<OH_AVFormat>(OH_AVSource_GetTrackFormat(source_.get(), index), OH_AVFormat_Destroy);
        OH_AVFormat_GetIntValue(trackFormat.get(), OH_MD_KEY_TRACK_TYPE, &trackType);
        if (trackType == MEDIA_TYPE_VID) {
            videoTrackId_ = index;
            OH_AVDemuxer_SelectTrackByID(demuxer_.get(), index);
            OH_AVFormat_GetIntValue(trackFormat.get(), OH_MD_KEY_WIDTH, &sampleInfo_->videoWidth);
            OH_AVFormat_GetIntValue(trackFormat.get(), OH_MD_KEY_HEIGHT, &sampleInfo_->videoHeight);
            if (sampleInfo_->frameRate == SAMPLE_DEFAULT_FRAMERATE) {
                OH_AVFormat_GetDoubleValue(trackFormat.get(), OH_MD_KEY_FRAME_RATE, &sampleInfo_->frameRate);
            }
            char *codecMime;
            OH_AVFormat_GetStringValue(trackFormat.get(), OH_MD_KEY_CODEC_MIME, const_cast<char const **>(&codecMime));
            sampleInfo_->codecMime = codecMime;
            OH_AVFormat_GetIntValue(trackFormat.get(), OH_MD_KEY_PROFILE, &sampleInfo_->profile);
        }
    }
    OH_AVFormat_GetLongValue(sourceFormat.get(), OH_MD_KEY_DURATION, &sampleInfo_->videoDuration);

    return AVCODEC_SAMPLE_ERR_OK;
}

bool Demuxer::IsEOS()
{
    CHECK_AND_RETURN_RET_LOG(file_ != nullptr, true, "File is not open");
    return feof(file_.get());
}
} // Sample
} // MediaAVCodec
} // OHOS