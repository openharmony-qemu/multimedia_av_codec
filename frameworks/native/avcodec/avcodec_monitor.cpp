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

#include "avcodec_monitor.h"
#include "avcodec_log.h"
#include "i_avcodec_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecMonitor"};
}

namespace OHOS {
namespace MediaAVCodec {
int32_t AVCodecMonitor::GetActiveSecureDecoderPids(std::vector<pid_t> &pidList)
{
    AVCODEC_LOGD("GetActiveSecureDecoderPids entry");
    return AVCodecServiceFactory::GetInstance().GetActiveSecureDecoderPids(pidList);
}
} // namespace MediaAVCodec
} // namespace OHOS
