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

#ifndef AVSOURCE_DEMO_H
#define AVSOURCE_DEMO_H

#include "native_avformat.h"
#include "native_avsource.h"

namespace OHOS {
namespace MediaAVCodec {
class AVSourceDemo {
public:
    AVSourceDemo();
    ~AVSourceDemo();
    OH_AVSource* GetAVSource();
    size_t GetFileSize(const std::string& fileName);
    int32_t CreateWithURI(char *uri);
    int32_t CreateWithFD(int32_t fd, int64_t offset, int64_t size);
    int32_t CreateWithDataSource(OH_AVDataSource *dataSource);
    int32_t Destroy();
    OH_AVFormat* GetSourceFormat();
    OH_AVFormat* GetTrackFormat(uint32_t trackIndex);
private:
    OH_AVSource* avsource_ = nullptr;
    OH_AVFormat* avformat_ = nullptr;
    OH_AVFormat* trackFormat_ = nullptr;
};
}  // namespace MediaAVCodec
}  // namespace OHOS

#endif
