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

#ifndef SMART_FLUENCY_DECODING_TYPES_H
#define SMART_FLUENCY_DECODING_TYPES_H

#include <cstdint>

typedef struct {
    uint32_t directSpatialMvPredFlag;
    uint32_t direct8x8InferenceFlag;
    uint32_t frameMbsOnlyFlag;
    uint32_t ctuSize;
} StreamAuxInfo;

// MV 解析器产出的感知特征统计结果
typedef struct {
    uint32_t totalBlocks;
    uint32_t validBlocks;
    uint32_t skipBlocks;
    uint32_t zeroMotionBlocks;
    double avgRefDist;
    double perceptualMagnitude;
    double motionConsistency;
    double zeroMotionRatio;
} MVStats;

#if defined(__cplusplus) && !defined(NALU_ANALYZER_PLUGIN_COMPILE)
#include <functional>

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

// using AVBuffer = OHOS::Media::AVBuffer;
using OnStreamAuxInfoParsed = std::function<void(const StreamAuxInfo& info)>;

// 保留决策模式枚举
enum class RetentionStrategyType : int32_t {
    INVALID = -1,
    FULL = 0,             // 对应 FullRetentionStrategy (全量透传)
    ADAPTIVE = 1,         // 对应 AdaptiveRetentionStrategy (自适应)
    FIXED_RATIO = 2,      // 对应 FixedRatioRetentionStrategy (按比例固定的系统下发)
    AUTO_RATIO = 3        // 对应 AutoFallbackRetentionStrategy (系统保底或自动计算)
};

// 统一定义 SFD 内部支持的视频编码标准
enum class SFDCodecType : int32_t {
    INVALID = -1,
    AVC = 0,    // H.264
    HEVC = 1,   // H.265
    VVC = 2     // H.266
};

struct SFDConfig {
    int32_t width{0};
    int32_t height{0};
    SFDCodecType codecType{SFDCodecType::INVALID};
    RetentionStrategyType initMode{RetentionStrategyType::INVALID};
    std::optional<double> retentionRatio{std::nullopt};
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif /* defined(__cplusplus) && !defined(NALU_ANALYZER_PLUGIN_COMPILE) */
#endif // SMART_FLUENCY_DECODING_TYPES_H
