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

#ifndef CODEABILITY_SINGLETON_H
#define CODEABILITY_SINGLETON_H

#include <mutex>
#include <optional>
#include <unordered_map>
#include "avcodec_codec_name.h"
#include "avcodec_info.h"
#include "codeclist_utils.h"

namespace OHOS {
namespace MediaAVCodec {
class __attribute__((visibility("default"))) CodecAbilitySingletonImpl : public NoCopyable {
public:
    CodecAbilitySingletonImpl();
    ~CodecAbilitySingletonImpl();
    static CodecAbilitySingletonImpl &GetInstance();
    void RegisterCapabilityArray(std::vector<CapabilityData> &capaArray, CodecType codecType);
    std::vector<CapabilityData> GetCapabilityArray();
    std::optional<CapabilityData> GetCapabilityByName(const std::string &name);
    std::unordered_map<std::string, CodecType> GetNameCodecTypeMap();
    std::unordered_map<std::string, std::vector<size_t>> GetMimeCapIdxMap();

private:
    std::vector<CapabilityData> capabilityDataArray_;
    std::unordered_map<std::string, std::vector<size_t>> mimeCapIdxMap_;
    std::unordered_map<std::string, CodecType> nameCodecTypeMap_;
    std::mutex mutex_;
};

#ifndef CodecAbilitySingleton
class __attribute__((visibility("default"))) CodecAbilitySingleton : public CodecAbilitySingletonImpl {
public:
    static CodecAbilitySingleton &GetInstance();
    void RegisterCapabilityArray(std::vector<CapabilityData> &capaArray, CodecType codecType);
    std::vector<CapabilityData> GetCapabilityArray();
    std::optional<CapabilityData> GetCapabilityByName(const std::string &name);
    std::unordered_map<std::string, CodecType> GetNameCodecTypeMap();
    std::unordered_map<std::string, std::vector<size_t>> GetMimeCapIdxMap();
};
#endif
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEABILITY_SINGLETON_H