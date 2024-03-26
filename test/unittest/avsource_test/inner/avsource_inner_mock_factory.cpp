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

#include "avsource_inner_mock.h"

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVSourceMock> AVSourceMockFactory::CreateSourceWithURI(char *uri)
{
    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithURI(uri);
    if (source != nullptr) {
        return std::make_shared<AVSourceInnerMock>(source);
    }
    return nullptr;
}

std::shared_ptr<AVSourceMock> AVSourceMockFactory::CreateSourceWithFD(int32_t fd, int64_t offset, int64_t size)
{
    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithFD(fd, offset, size);
    if (source != nullptr) {
        return std::make_shared<AVSourceInnerMock>(source);
    }
    return nullptr;
}

std::shared_ptr<AVSourceMock> AVSourceMockFactory::CreateWithDataSource(
    const std::shared_ptr<Media::IMediaDataSource> &dataSource)
{
    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithDataSource(dataSource);
    if (source != nullptr) {
        return std::make_shared<AVSourceInnerMock>(source);
    }
    return nullptr;
}

std::shared_ptr<AVSourceMock> AVSourceMockFactory::CreateWithDataSource(OH_AVDataSource *dataSource)
{
    return nullptr;
}
}
}