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

#ifndef RETENTION_STRATEGY_FACTORY_H
#define RETENTION_STRATEGY_FACTORY_H

#include <memory>
#include <optional>
#include "i_frame_retention_strategy.h"
#include "smart_fluency_decoding_types.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

class RetentionStrategyFactory {
public:
    static std::unique_ptr<IFrameRetentionStrategy> CreateStrategy(
        FrameRetentionMode mode,
        const std::optional<double>& retentionRate);
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif
