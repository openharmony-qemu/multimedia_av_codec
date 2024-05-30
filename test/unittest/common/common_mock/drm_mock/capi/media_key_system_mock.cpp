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

#include "media_key_system_mock.h"
#include "native_mediakeysession.h"
#include "native_mediakeysystem.h"

namespace OHOS {
namespace MediaAVCodec {
#define CLEARPLAY_NAME "com.clearplay.drm"
#define WISEPLAY_NAME "com.wiseplay.drm"
#define UKNOWN_NAME "unknown_name"

static const char *GetDrmUuid()
{
    if (OH_MediaKeySystem_IsSupported(CLEARPLAY_NAME)) {
        return CLEARPLAY_NAME;
    } else if (OH_MediaKeySystem_IsSupported(WISEPLAY_NAME)) {
        return WISEPLAY_NAME;
    }
    return UKNOWN_NAME;
}

MediaKeySystemCapiMock::MediaKeySystemCapiMock()
{}

MediaKeySystemCapiMock::~MediaKeySystemCapiMock()
{
    (void)OH_MediaKeySession_Destroy(mediaKeySession_);
    (void)OH_MediaKeySystem_Destroy(mediaKeySystem_);
}

int32_t MediaKeySystemCapiMock::CreateMediaKeySystem()
{
    return OH_MediaKeySystem_Create(GetDrmUuid(), &mediaKeySystem_);
}

int32_t MediaKeySystemCapiMock::CreateMediaKeySession()
{
    int32_t ret = AV_ERR_UNKNOWN;
    if (mediaKeySystem_ == nullptr) {
        return ret;
    }
    DRM_ContentProtectionLevel contentProtectionLevel = CONTENT_PROTECTION_LEVEL_SW_CRYPTO;
    ret = OH_MediaKeySystem_CreateMediaKeySession(mediaKeySystem_, &contentProtectionLevel, &mediaKeySession_);
    return ret;
}

MediaKeySystem *MediaKeySystemCapiMock::GetMediaKeySystem()
{
    return mediaKeySystem_;
}

MediaKeySession *MediaKeySystemCapiMock::GetMediaKeySession()
{
    return mediaKeySession_;
}

} // namespace MediaAVCodec
} // namespace OHOS