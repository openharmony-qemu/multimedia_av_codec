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

#include <sys/stat.h>
#include <string>
#include "inner_source_demo.h"

using namespace std;
using namespace OHOS::MediaAVCodec;

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
InnerSourceDemo::InnerSourceDemo()
{
    printf("SourceDemo ()\n");
}

InnerSourceDemo::~InnerSourceDemo()
{
    printf("~SourceDemo ()\n");
}

int32_t InnerSourceDemo::CreateWithURI(const std::string& uri)
{
    this->avsource_ = AVSourceFactory::CreateWithURI(uri.c_str());
    if (!avsource_) {
        printf("Source is null\n");
        return -1;
    }
    return 0;
}

size_t InnerSourceDemo::GetFileSize(const std::string& fileName)
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

int32_t InnerSourceDemo::CreateWithFD(int32_t fd, int64_t offset, int64_t size)
{
    this->avsource_ = AVSourceFactory::CreateWithFD(fd, offset, size);
    if (!avsource_) {
        printf("Source is null\n");
        return -1;
    }
    return 0;
}

Format InnerSourceDemo::GetSourceFormat()
{
    int32_t ret = this->avsource_->GetSourceFormat(source_format_);
    if (ret != 0) {
        printf("GetSourceFormat is failed\n");
    }
    return source_format_;
}

Format InnerSourceDemo::GetTrackFormat(uint32_t trackIndex)
{
    int32_t ret = this->avsource_->GetTrackFormat(track_format_, trackIndex);
    if (ret != 0) {
        printf("GetTrackFormat is failed\n");
    }
    return track_format_;
}
}  // namespace MediaAVCodec
}  // namespace OHOS
