/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "demuxer_mock.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include "native_avcodec_base.h"

namespace OHOS {
namespace MediaAVCodec {
DemuxerMock::~DemuxerMock()
{
    if (demuxer_ != nullptr) {
        OH_AVDemuxer_Destroy(demuxer_);
        demuxer_ = nullptr;
    }
    if (source_ != nullptr) {
        OH_AVSource_Destroy(source_);
        source_ = nullptr;
        sourceFormat_ = nullptr;
    }
}

int32_t DemuxerMock::Start(std::string fileName)
{
    fd_ = open(fileName.c_str(), O_RDONLY);
    if (fd_ <= 0) {
        return -1;
    }
    struct stat fileStatus {};
    if (stat(fileName.c_str(), &fileStatus) != 0) {
        std::cout << "stat file failed" << std::endl;
        return -1;
    }
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    source_ = OH_AVSource_CreateWithFD(fd_, 0, fileSize);
    if (source_ == nullptr) {
        std::cout << "OH_AVSource_CreateWithFD failed!" << std::endl;
        return -1;
    }
    sourceFormat_ = OH_AVSource_GetSourceFormat(source_);
    if (sourceFormat_ == nullptr) {
        std::cout << "OH_AVSource_GetSourceFormat failed!" << std::endl;
        return -1;
    }
    demuxer_ = OH_AVDemuxer_CreateWithSource(source_);
    if (demuxer_ == nullptr) {
        std::cout << "OH_AVDemuxer_CreateWithSource failed!" << std::endl;
        return -1;
    }
    return UpdateTrackInfo();
}

int32_t DemuxerMock::UpdateTrackInfo()
{
    int32_t trackCount = 0;
    if (!OH_AVFormat_GetIntValue(sourceFormat_, OH_MD_KEY_TRACK_COUNT, &trackCount)) {
        std::cout << "get track count failed1" << std::endl;
        return -2;  // -2
    }
    int32_t trackType = 0;
    for (int32_t index = 0; index < trackCount; index++) {
        OH_AVFormat *trackFormat = OH_AVSource_GetTrackFormat(source_, index);
        if (trackFormat == nullptr) {
            std::cout << "get track:" << index << " format failed! continue" << std::endl;
            OH_AVFormat_Destroy(trackFormat);
            continue;
        }
        if (!OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType)) {
            std::cout << "get track:" << index << " type failed! continue" << std::endl;
            OH_AVFormat_Destroy(trackFormat);
            continue;
        }
        if (trackType == static_cast<int32_t>(OH_MediaType::MEDIA_TYPE_AUD)) {
            audioTrackId_ = index;
        } else if (trackType == static_cast<int32_t>(OH_MediaType::MEDIA_TYPE_VID)) {
            videoTrackId_ = index;
        }
        OH_AVFormat_Destroy(trackFormat);
    }
    return 0;
}

int32_t DemuxerMock::GetVideoFrame(OH_AVBuffer *buffer, OH_AVCodecBufferAttr *info)
{
    if (demuxer_ == nullptr || videoTrackId_ == 0xffffffff) {
        std::cout << "DemuxerMock demuxer start failed" << std::endl;
        return -1;
    }
    if (OH_AVDemuxer_SelectTrackByID(demuxer_, videoTrackId_) != 0) {
        std::cout << "demuxer select track failed! id:" << videoTrackId_ << std::endl;
        return -1;
    }
    int32_t ret = OH_AVDemuxer_ReadSampleBuffer(demuxer_, videoTrackId_, buffer);
    if (ret == 0) {
        ret = OH_AVBuffer_GetBufferAttr(buffer, info);
    }
    if (buffer == nullptr || info == nullptr) {
        std::cout << "get video track frame failed:" << ret << std::endl;
        return -1;
    }
    return ret;
}
} // namespace MediaAVCodec
} // namespace OHOS