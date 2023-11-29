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

#ifndef AVCODEC_PARCEL_H
#define AVCODEC_PARCEL_H

#include "meta/format.h"
#include "message_parcel.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecParcel {
public:
    AVCodecParcel() = delete;
    ~AVCodecParcel() = delete;
    static bool Marshalling(MessageParcel &parcel, const Media::Format &format);
    static bool Unmarshalling(MessageParcel &parcel, Media::Format &format);
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_PARCEL_H
