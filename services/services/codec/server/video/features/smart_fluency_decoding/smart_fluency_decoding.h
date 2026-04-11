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

#ifndef SMART_FLUENCY_DECODING_H
#define SMART_FLUENCY_DECODING_H

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include "async_drop_dispatcher.h"
#include "drop_sync_coordinator.h"
#include "strategies/retention_strategy.h"
#include "meta/format.h"
#include "analyzers/mv_analyzer.h"
#include "analyzers/nalu_analyzer.h"
#include "smart_fluency_decoding_dumper.h"
#include "smart_fluency_decoding_types.h"

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {

class SmartFluencyDecoding {
public:
    SmartFluencyDecoding();
    ~SmartFluencyDecoding();

    int32_t Initialize(const SFDConfig& config);
    // 动态倍速调节（支持播放中途无缝切换）
    void UpdatePlaybackSpeed(double speed);
    // 新增：动态修改参数模式
    int32_t UpdateDynamicConfig(const Media::Format& format);
    // 输入buffer和index缓存
    void SetInputBuffer(uint32_t index, const std::shared_ptr<AVBuffer>& buffer);
    // 新增：向 SFD 反馈回调的解码相关统计数据
    void OnDecodingPerformanceUpdate(double speed, double decFps);
    using AsyncDropInputCallback = std::function<void(uint32_t index, std::shared_ptr<AVBuffer> buffer)>;
    using AsyncDropOutputCallback = std::function<void(uint32_t index)>;
    void SetCallbacks(AsyncDropInputCallback inCb, AsyncDropOutputCallback outCb);
    bool InitializeWithCsd(const std::shared_ptr<AVBuffer>& csdBuffer);
    bool MakePreDecodeDecision(uint32_t index);
    bool MakePostDecodeDecision(uint32_t index, int64_t pts, const sptr<SurfaceBuffer> surfaceBuffer);
    void Flush();
    void GetStatistics(uint32_t& totalFrames, uint32_t& droppedFrames) const;

private:
    int32_t initializeInternal();
    int64_t GetCurrentSysTimeMs();
    void parseMVData(const sptr<SurfaceBuffer> surfaceBuffer, MVStats& mvStats);
    void ParseDynamicFormat(const Media::Format& format, RetentionStrategyType& outMode, std::optional<double>& outRatio) const;

    void EnsureNaluAnalyzer();
    void EnsureMvAnalyzer();

    mutable std::mutex configMutex_;
    std::unique_ptr<DropSyncCoordinator> dropSyncCoordinator_{nullptr};
    std::unique_ptr<AsyncDropDispatcher> asyncDispatcher_{nullptr};
    std::unique_ptr<IRetentionStrategy> strategy_{nullptr};
    std::unique_ptr<IMvAnalyzer> mvAnalyzer_{nullptr};
    std::unique_ptr<INaluAnalyzer> naluAnalyzer_{nullptr};
    std::unique_ptr<SfdDumper> dumper_{nullptr};
    std::unordered_map<uint32_t, std::shared_ptr<AVBuffer>> inBufferMap_;
    std::shared_ptr<AVBuffer> csdBuffer_{nullptr};
    SFDConfig config_{};
    bool isInitialized_{false};
    std::atomic<uint32_t> totalInputFrames_{0};
    std::atomic<uint32_t> droppedInputFrames_{0};
    std::atomic<uint32_t> totalOutputFrames_{0};
    std::atomic<uint32_t> droppedOutputFrames_{0};
    double currentPlaybackSpeed_{1.0};
    AsyncDropInputCallback asyncDropInputCb_{nullptr};
    AsyncDropOutputCallback asyncDropOutputCb_{nullptr};
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif // SMART_FLUENCY_DECODING_H