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

#include "rosen_window_wrapper.h"
#include "surface/window.h"
#include "surface.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"

#include "av_codec_sample_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "RosenWindowWrapper"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
RosenWindowWrapper::RosenWindowWrapper()
{
    windowType_ = SampleWindowType::ROSEN;

    const char **perms = new const char *[1];
    perms[0] = "ohos.permission.SYSTEM_FLOAT_WINDOW";
    TokenInfoParams tokenInfo = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "video_codec_demo",
        .aplStr = "system_core",
    };
    SetSelfTokenID(GetAccessTokenId(&tokenInfo));
    Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;

    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_FLOAT);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FULLSCREEN);
    rosenWindow_ = Rosen::Window::Create("VideoCodecDemo", option);
    CHECK_AND_RETURN_LOG(rosenWindow_ != nullptr && rosenWindow_->GetSurfaceNode() != nullptr,
        "Create display window failed");
    rosenWindow_->SetTurnScreenOn(!rosenWindow_->IsTurnScreenOn());
    rosenWindow_->SetKeepScreenOn(true);
    rosenWindow_->Show();
    sptr<OHOS::Surface> surfaceProducer = rosenWindow_->GetSurfaceNode()->GetSurface();
    window_ = std::shared_ptr<OHNativeWindow>(
        CreateNativeWindowFromSurface(&surfaceProducer),
        [](OHNativeWindow *window) -> void {
            (void)window;
        }
    );
}

RosenWindowWrapper::~RosenWindowWrapper()
{
    if (rosenWindow_ != nullptr) {
        rosenWindow_->Destroy();
        rosenWindow_ = nullptr;
    }
}
} // Sample
} // MediaAVCodec
} // OHOS