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

#include "demuxer_capi_mock.h"
#include "avsource_capi_mock.h"

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<DemuxerMock> AVDemuxerMockFactory::CreateDemuxer(std::shared_ptr<AVSourceMock> source)
{
    auto capiSource = std::static_pointer_cast<AVSourceCapiMock>(source);
    OH_AVSource *avSource = (capiSource != nullptr) ? capiSource->GetAVSource() : nullptr;
    OH_AVDemuxer *demuxer = OH_AVDemuxer_CreateWithSource(avSource);
    if (demuxer != nullptr) {
        return std::make_shared<DemuxerCapiMock>(demuxer);
    }
    return nullptr;
}
}
}