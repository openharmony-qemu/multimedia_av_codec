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

#ifndef MEDIA_AVCODEC_LIST_H
#define MEDIA_AVCODEC_LIST_H

#include <cstdint>
#include <memory>
#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecList {
public:
    virtual ~AVCodecList() = default;

    /**
     * @brief Find the supported decoder name by format(must contains video or audio MIME).
     * @param format Indicates a media description which contains required decoder capability.
     * @return Returns decoder name, if not find, return empty string.
     * @since 10
     * @version 1.0
     */
    virtual std::string FindDecoder(const Format &format) = 0;

    /**
     * @brief Find the supported encoder name by format(must contains video or audio MIME).
     * @param format Indicates a media description which contains required encoder capability.
     * @return Returns encoder name, if not find, return empty string.
     * @since 10
     * @version 1.0
     */
    virtual std::string FindEncoder(const Format &format) = 0;

    /**
     * @brief Create a capability by codec name
     * @param codeName Codec name
     * @return Returns an array of supported video decoder capability, if not find, return invalid CapabilityData.
     * @since 10
     * @version 1.0
     */
    virtual CapabilityData *GetCapability(const std::string &mime, const bool isEncoder,
                                         const AVCodecCategory &category) = 0;
    virtual void *GetBuffer(const std::string &name, uint32_t sizeOfCap) = 0;
    virtual void *NewBuffer(size_t bufSize) = 0;
    virtual void DeleteBuffer(void *bufAddr) = 0;
};

class __attribute__((visibility("default"))) AVCodecListFactory {
public:
#ifdef UNSUPPORT_CODECLIST
    static std::shared_ptr<AVCodecList> CreateAVCodecList()
    {
        return nullptr;
    }
#else
    static std::shared_ptr<AVCodecList> CreateAVCodecList();
#endif
private:
    AVCodecListFactory() = default;
    ~AVCodecListFactory() = default;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_LIST_H
