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

#include "avcodec_parcel.h"
#include "avcodec_log.h"
#include "meta/meta.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
bool AVCodecParcel::Marshalling(MessageParcel &parcel, const Format &format)
{
    Format *formatRef = const_cast<Format *>(&format);
    return formatRef->GetMeta()->ToParcel(parcel);
}

bool AVCodecParcel::Unmarshalling(MessageParcel &parcel, Format &format)
{
    auto meta = std::make_shared<Meta>();
    bool ret = meta->FromParcel(parcel) && format.SetMeta(std::move(meta));
    return ret;
}
} // namespace MediaAVCodec
} // namespace OHOS
