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

#ifndef AVSOURCE_IMPL_H
#define AVSOURCE_IMPL_H

#include "avsource.h"
#include "nocopyable.h"
#include "meta/meta.h"

namespace OHOS {
namespace MediaAVCodec {
class AVSourceImpl : public AVSource, public NoCopyable {
public:
    AVSourceImpl();
    ~AVSourceImpl() override;

    int32_t GetSourceFormat(OHOS::Media::Format &format) override;
    int32_t GetTrackFormat(OHOS::Media::Format &format, uint32_t trackIndex) override;
    int32_t GetUserMeta(OHOS::Media::Format &format) override;
    int32_t InitWithURI(const std::string &uri);
    int32_t InitWithFD(int32_t fd, int64_t offset, int64_t size);
    int32_t InitWithDataSource(const std::shared_ptr<Media::IMediaDataSource> &dataSource);
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVSOURCE_IMPL_H

