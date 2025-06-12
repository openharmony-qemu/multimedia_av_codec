/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef MOCK_AVCODEC_LIST_H
#define MOCK_AVCODEC_LIST_H

#include "gmock/gmock.h"
#include "avcodec_list.h"
#include "avcodec_info.h"

namespace OHOS {
namespace Media {
class MockAVCodecList : public MediaAVCodec::AVCodecList {
public:
    virtual ~MockAVCodecList() = default;

    MOCK_METHOD1(FindDecoder, std::string(const Format &));
    MOCK_METHOD1(FindEncoder, std::string(const Format &));
    MOCK_METHOD3(GetCapability, MediaAVCodec::CapabilityData *(
        const std::string &, const bool, const MediaAVCodec::AVCodecCategory &));
    MOCK_METHOD2(GetBuffer, void *(const std::string &, uint32_t));
    MOCK_METHOD1(NewBuffer, void *(size_t));
};
} // namespace Media
} // namespace OHOS
#endif // MOCK_AVCODEC_LIST_H