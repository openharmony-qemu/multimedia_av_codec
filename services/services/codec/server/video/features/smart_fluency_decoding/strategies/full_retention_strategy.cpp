/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "full_retention_strategy.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "FullStrategy"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

FullRetentionStrategy::FullRetentionStrategy()
{
    AVCODEC_LOGD("FullRetentionStrategy created");
}

bool FullRetentionStrategy::MakeRetentionDecision(const FrameRetentionContext& ctx)
{
    (void)ctx;
    AVCODEC_LOGD("FullRetentionStrategy decision: true (always retain)");
    return true;
}

RetentionStrategyType FullRetentionStrategy::GetMode() const
{
    return RetentionStrategyType::FULL;
}

double FullRetentionStrategy::GetRatio() const
{
    return 1.0;
}

SfdDecisionReason FullRetentionStrategy::GetLastDecisionReason() const
{
    return SfdDecisionReason::POST_RETAIN_FULL;
}

bool FullRetentionStrategy::IsPassthrough() const
{
    return true;
}

bool FullRetentionStrategy::RequiresNaluAnalysis() const
{
    return false;
}

bool FullRetentionStrategy::RequiresMvAnalysis() const
{
    return false;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
