/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include "demuxerinnerfunc_sample.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace std;
const char *FILE_PATH = "/data/test/fuzz_create.mp4";
DemuxerInnerFuncSample::~DemuxerInnerFuncSample()
{
    if (fd_ > 0) {
        close(fd_);
    }
    fd_ = -1;

    if (source_ != nullptr) {
        source_ = nullptr;
    }
    if (demuxer_ != nullptr) {
        demuxer_ = nullptr;
    }
}

bool DemuxerInnerFuncSample::Init(const uint8_t *data, size_t size)
{
    std::ofstream file(FILE_PATH, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(data), size);
        file.close();
    }
    int32_t fd = open(FILE_PATH, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    source_ = AVSourceFactory::CreateWithFD(fd, 0, size);
    if (source_ == nullptr) {
        return false;
    }
    demuxer_ = AVDemuxerFactory::CreateWithSource(source_);
    if (demuxer_ == nullptr) {
        return false;
    }
    int32_t ret = demuxer_->SelectTrackByID(0);
    if (ret != AV_ERR_OK) {
        return false;
    }
    return true;
}

void DemuxerInnerFuncSample::GetCurrentCacheSize(uint32_t trackIndex)
{
    uint32_t cacheSize = 0;
    demuxer_->GetCurrentCacheSize(trackIndex, cacheSize);
}
