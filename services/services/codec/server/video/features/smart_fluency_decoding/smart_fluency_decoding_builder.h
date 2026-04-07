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

#ifndef SMART_FLUENCY_DECODING_BUILDER_H
#define SMART_FLUENCY_DECODING_BUILDER_H

#include <memory>
#include <optional>
#include <string>
#include "meta/format.h"
#include "smart_fluency_decoding_types.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;

namespace SFD {

class SmartFluencyDecoding;

class SmartFluencyDecodingBuilder {
public:
    SmartFluencyDecodingBuilder();
    ~SmartFluencyDecodingBuilder();

    // 禁止拷贝以确保每个构建器会话的配置所有权唯一
    SmartFluencyDecodingBuilder(const SmartFluencyDecodingBuilder&) = delete;
    SmartFluencyDecodingBuilder& operator=(const SmartFluencyDecodingBuilder&) = delete;

    // 允许移动以实现参数的高效传递（例如从函数返回时）
    SmartFluencyDecodingBuilder(SmartFluencyDecodingBuilder&&) = default;
    SmartFluencyDecodingBuilder& operator=(SmartFluencyDecodingBuilder&&) = default;

    // 流式参数配置
    SmartFluencyDecodingBuilder& SetVideoWidth(int32_t width);
    SmartFluencyDecodingBuilder& SetVideoHeight(int32_t height);
    SmartFluencyDecodingBuilder& SetCodecMime(const std::string& mime);
    SmartFluencyDecodingBuilder& UpdatePlaybackSpeed(double speed);
    // 批量设置：从 Format 中解析并注入相关配置
    SmartFluencyDecodingBuilder& SetFromFormat(const Format& format);

    /**
     * @brief 执行联动参数校验并安全实例化 SFD
     * @note 如果参数组合非法，内部将进行安全降级；若基础必填参数缺失，则返回 nullptr
     */
    std::unique_ptr<SmartFluencyDecoding> Build();

    static bool IsCodecSupported(const std::string& codecMime, int32_t codecType);
    static SFDCodecType ParseCodecType(const std::string& mime);
    static void EnableMVOutput(Format& format);
    // 从原始首帧或 CODEC_DATA 中全量/部分截取固定小块（最多256字节）作为 CSD 安全副本
    static std::vector<uint8_t> ExtractCsdData(const std::shared_ptr<Media::AVBuffer>& buffer);
    // 播放中途开启 SFD 时，通过伪造 Buffer 重放 CSD 参数，以激活底层探针的解析能力
    static void ReplayCsdData(SmartFluencyDecoding* sfd, const std::vector<uint8_t>& csdData);

private:
    SFDConfig params_{};
    std::optional<double> playbackSpeed_{std::nullopt};

    bool ValidateParams();
    bool IsRetentionRateValid(double rate);
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif // SMART_FLUENCY_DECODING_BUILDER_H
