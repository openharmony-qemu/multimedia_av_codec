/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 *
 * Description: header of Type converter from framework to OMX
 */

#include "hcodec_api.h"
#include "hcodec_list.h"
#include "hcodec.h"

namespace OHOS::MediaAVCodec {
extern "C" {
int32_t GetHCodecCapabilityList(std::vector<CapabilityData> &caps)
{
    HCodecList list;
    return list.GetCapabilityList(caps);
}

void CreateHCodecByName(const std::string& name, std::shared_ptr<CodecBase>& codec)
{
    codec = HCodec::Create(name);
}
}
}