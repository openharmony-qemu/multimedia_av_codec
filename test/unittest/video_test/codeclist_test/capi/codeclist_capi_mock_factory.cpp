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

#include "codeclist_capi_mock.h"

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<CodecListMock> CodecListMockFactory::GetCapability(const std::string &mime, bool isEncoder)
{
    auto codeclist = OH_AVCodec_GetCapability(mime.c_str(), isEncoder);
    if (codeclist != nullptr) {
        return std::make_shared<CodecListCapiMock>(codeclist);
    }
    return nullptr;
}

std::shared_ptr<CodecListMock> CodecListMockFactory::GetCapabilityByCategory(const std::string &mime, bool isEncoder,
                                                                             AVCodecCategory category)
{
    if (category == AVCodecCategory::AVCODEC_NONE) {
        return GetCapability(mime, isEncoder);
    }
    auto codeclist =
        OH_AVCodec_GetCapabilityByCategory(mime.c_str(), isEncoder, static_cast<OH_AVCodecCategory>(category));
    if (codeclist != nullptr) {
        return std::make_shared<CodecListCapiMock>(codeclist);
    }
    return nullptr;
}
} // namespace MediaAVCodec
} // namespace OHOS