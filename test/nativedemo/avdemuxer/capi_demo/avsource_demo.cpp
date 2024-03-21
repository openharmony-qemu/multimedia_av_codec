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

#include <memory>
#include <vector>
#include <map>
#include <sys/stat.h>
#include <string>
#include "avsource_demo.h"

namespace OHOS {
namespace MediaAVCodec {
AVSourceDemo::AVSourceDemo()
{
    printf("AVSourceDemo construtor\n");
}

AVSourceDemo::~AVSourceDemo()
{
    printf("AVSourceDemo deconstrutor\n");
}

int32_t AVSourceDemo::CreateWithURI(char* uri)
{
    this->avsource_ = OH_AVSource_CreateWithURI(uri);
    if (!avsource_) {
        printf("OH_AVSource_CreateWithURI is failed\n");
        return -1;
    }
    return 0;
}

size_t AVSourceDemo::GetFileSize(const std::string& fileName)
{
    size_t fileSize = 0;
    if (!fileName.empty()) {
        struct stat fileStatus {};
        if (stat(fileName.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<size_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

int32_t AVSourceDemo::CreateWithFD(int32_t fd, int64_t offset, int64_t size)
{
    this->avsource_ = OH_AVSource_CreateWithFD(fd, offset, size);
    if (!avsource_) {
        printf("OH_AVSource_CreateWithFD is failed\n");
        return -1;
    }
    return 0;
}

int32_t AVSourceDemo::CreateWithDataSource(OH_AVDataSource *dataSource)
{
    this->avsource_ = OH_AVSource_CreateWithDataSource(dataSource);
    if (!avsource_) {
        printf("CreateWithDataSource is failed\n");
        return -1;
    }
    return 0;
}

OH_AVSource* AVSourceDemo::GetAVSource()
{
    return this->avsource_;
}

int32_t AVSourceDemo::Destroy()
{
    int32_t ret = static_cast<int32_t>(OH_AVSource_Destroy(this->avsource_));
    if (ret != 0) {
        printf("destroy failed\n");
        return ret;
    }
    return ret;
}

OH_AVFormat* AVSourceDemo::GetSourceFormat()
{
    this->avformat_ = OH_AVSource_GetSourceFormat(this->avsource_);
    if (!this->avformat_) {
        printf("OH_AVSource_GetSourceFormat is failed\n");
        return nullptr;
    }
    return this->avformat_;
}

OH_AVFormat* AVSourceDemo::GetTrackFormat(uint32_t trackIndex)
{
    this->trackFormat_ = OH_AVSource_GetTrackFormat(this->avsource_, trackIndex);
    if (!this->trackFormat_) {
        printf("OH_AVSourceTrack_GetTrackFormat is failed\n");
        return nullptr;
    }
    return this->trackFormat_;
}
}  // namespace MediaAVCodec
}  // namespace OHOS
