/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "ptsandindexconversion_sample.h"
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

PtsAndIndexConversion::~PtsAndIndexConversion()
{
    if (fd_ > 0) {
        close(fd_);
    }
    fd_ = -1;

    index_ = 0;
    trackIndex_ = 0;
    relativePresentationTimeUs_ = 0;
}

bool PtsAndIndexConversion::Init(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    fd_ = open(filePath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd_ < 0) {
        return false;
    }
    int len = write(fd_, data, size);
    if (len <= 0) {
        close(fd_);
        fd_ = -1;
        return false;
    }
    std::string uri = "fd://" + std::to_string(fd_) + "?offset=0&size=" + std::to_string(size);
    TimeAndIndexConversions_ = std::make_shared<TimeAndIndexConversion>();
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(uri);
    Status ret = TimeAndIndexConversions_->SetDataSource(mediaSource);
    close(fd_);
    fd_ = -1;
    if (ret == Status::OK) {
        index_ = data[size - INDEX_MAX];
        trackIndex_ = data[size - TRACKINDEX_MAX];
        relativePresentationTimeUs_ = data[size - RELATIVEPRESENTATIONTIMEUS_MAX];
        return true;
    }
    TimeAndIndexConversions_ = nullptr;
    return false;
}

void PtsAndIndexConversion::RunNormalTimeAndIndexConversions()
{
    TimeAndIndexConversions_->GetIndexByRelativePresentationTimeUs(trackIndex_, relativePresentationTimeUs_, index_);
    TimeAndIndexConversions_->GetRelativePresentationTimeUsByIndex(trackIndex_, index_, relativePresentationTimeUs_);
    TimeAndIndexConversions_->GetFirstVideoTrackIndex(trackIndex_);
}
