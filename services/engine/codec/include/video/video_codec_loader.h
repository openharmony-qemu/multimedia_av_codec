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

#ifndef VIDEO_CODEC_LOADER_H
#define VIDEO_CODEC_LOADER_H
#include "codecbase.h"
#include "codeclistbase.h"

namespace OHOS {
namespace MediaAVCodec {
class VideoCodecLoader {
public:
    VideoCodecLoader(const char *libPath, const char *createFuncName, const char *getCapsFuncName)
        : libPath_(libPath), createFuncName_(createFuncName), getCapsFuncName_(getCapsFuncName){};
    ~VideoCodecLoader() = default;

    std::shared_ptr<CodecBase> Create(const std::string &name);
    int32_t GetCaps(std::vector<CapabilityData> &caps);

    int32_t Init();
    void Close();

private:
    using CreateByNameFuncType = void (*)(const std::string &name, std::shared_ptr<CodecBase> &codec);
    using GetCapabilityFuncType = int32_t (*)(std::vector<CapabilityData> &caps);
    std::shared_ptr<void> codecHandle_ = nullptr;
    CreateByNameFuncType createFunc_ = nullptr;
    GetCapabilityFuncType getCapsFunc_ = nullptr;
    const char *libPath_ = nullptr;
    const char *createFuncName_ = nullptr;
    const char *getCapsFuncName_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif