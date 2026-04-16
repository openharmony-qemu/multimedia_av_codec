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

#ifndef MEDIA_AVCODEC_FRAME_DROP_STRATEGY_H
#define MEDIA_AVCODEC_FRAME_DROP_STRATEGY_H

#include <cstdint>
#include <memory>

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

enum class DropStrategyType { RATIO, TIMESTAMP };

class IDropStrategy {
public:
    virtual ~IDropStrategy() = default;
    virtual bool ShouldDropFrame(uint64_t pts) = 0;
    virtual void FlushTimeStamp() = 0;
    virtual DropStrategyType GetType() const = 0;
};

class RatioDropStrategy : public IDropStrategy {
public:
    RatioDropStrategy(double srcFrameRate, double targetFrameRate);
    bool ShouldDropFrame(uint64_t pts) override;
    void FlushTimeStamp() override;
    DropStrategyType GetType() const override;

private:
    double srcFrameRate_;
    double targetFrameRate_;
    uint64_t keptFrames_{0};
    uint64_t totalFrames_{0};
};

class TimestampDropStrategy : public IDropStrategy {
public:
    explicit TimestampDropStrategy(double targetFrameRate);
    bool ShouldDropFrame(uint64_t pts) override;
    void FlushTimeStamp() override;
    DropStrategyType GetType() const override;

private:
    uint64_t basePts_{0};
    bool hasBase_{false};
    uint64_t frameIntervalUs_{0};
};

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS

#endif // MEDIA_AVCODEC_FRAME_DROP_STRATEGY_H
