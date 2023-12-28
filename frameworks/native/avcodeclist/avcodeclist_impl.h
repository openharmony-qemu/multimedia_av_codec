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
#ifndef AVCODEC_LIST_IMPL_H
#define AVCODEC_LIST_IMPL_H
#include <mutex>
#include <set>
#include "avcodec_info.h"
#include "avcodec_list.h"
#include "nocopyable.h"
#include "i_codeclist_service.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecListImpl : public AVCodecList, public NoCopyable {
public:
    AVCodecListImpl();
    ~AVCodecListImpl();
    int32_t Init();
    // AVCodecList
    std::string FindDecoder(const Format &format) override;
    std::string FindEncoder(const Format &format) override;
    CapabilityData *GetCapability(const std::string &mime, const bool isEncoder,
                                  const AVCodecCategory &category) override;
    void *GetBuffer(const std::string &name, uint32_t sizeOfCap) override;
    void *NewBuffer(size_t bufSize) override;
    void DeleteBuffer(void *bufAddr) override;

private:
    std::shared_ptr<ICodecListService> codecListService_ = nullptr;
    std::unordered_map<std::string, std::vector<CapabilityData *>> mimeCapsMap_;
    std::unordered_map<std::string, void *> nameAddrMap_;
    std::set<uint8_t *> bufAddrSet_;
    std::mutex mutex_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_LIST_IMPL_H
