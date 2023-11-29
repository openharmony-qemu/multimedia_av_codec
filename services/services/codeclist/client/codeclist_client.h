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

#ifndef CODECLIST_CLIENT_H
#define CODECLIST_CLIENT_H

#include <mutex>
#include "i_codeclist_service.h"
#include "i_standard_codeclist_service.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecListClient : public ICodecListService {
public:
    static std::shared_ptr<CodecListClient> Create(const sptr<IStandardCodecListService> &ipcProxy);
    explicit CodecListClient(const sptr<IStandardCodecListService> &ipcProxy);
    ~CodecListClient();
    void MediaServerDied();
    // ICodecListService override
    std::string FindDecoder(const Media::Format &format) override;
    std::string FindEncoder(const Media::Format &format) override;
    int32_t GetCapability(CapabilityData &capabilityData, const std::string &mime, const bool isEncoder,
                          const AVCodecCategory &category) override;
    void AVCodecServerDied();

private:
    sptr<IStandardCodecListService> codecListProxy_ = nullptr;
    std::mutex mutex_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECLIST_CLIENT_H
