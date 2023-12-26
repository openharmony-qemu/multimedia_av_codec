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

#include "avmuxer_mock.h"
#include "avmuxer_capi_mock.h"

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVMuxerMock> AVMuxerMockFactory::CreateMuxer(int32_t fd, const OH_AVOutputFormat &format)
{
    OH_AVMuxer *muxer = OH_AVMuxer_Create(fd, format);
    if (muxer != nullptr) {
        return std::make_shared<AVMuxerCapiMock>(muxer);
    }
    return nullptr;
}
}
}