/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef MEDIA_KEY_SYSTEM_CAPI_MOCK_H
#define MEDIA_KEY_SYSTEM_CAPI_MOCK_H

#include "common_mock.h"

typedef struct MediaKeySystem MediaKeySystem;
typedef struct MediaKeySession MediaKeySession;

namespace OHOS {
namespace MediaAVCodec {
class MediaKeySystemCapiMock : public NoCopyable {
public:
    MediaKeySystemCapiMock();
    ~MediaKeySystemCapiMock();
    int32_t CreateMediaKeySystem();
    int32_t CreateMediaKeySession();
    MediaKeySystem *GetMediaKeySystem();
    MediaKeySession *GetMediaKeySession();

private:
    MediaKeySystem *mediaKeySystem_ = nullptr;
    MediaKeySession *mediaKeySession_ = nullptr;
};
}  // namespace MediaAVCodec
}  // namespace OHOS
#endif // MEDIA_KEY_SYSTEM_CAPI_MOCK_H