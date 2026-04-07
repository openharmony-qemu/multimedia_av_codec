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
#include "frame_dropper/i_frame_dropper.h"
#include "meta/format.h"
#include "mv_parser/mv_data.h"
#include "mv_parser/mv_parser_factory.h"
#include "mv_parser/mv_parser_interface.h"
#include "nalu_analyzer/nalu_analyzer.h"
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
    /**
     * @brief 使用 CSD (SPS/PPS) 头信息初始化底层的探测器。
     * 中途使能 SFD 或首帧带有 CSD 时调用。
     * @param csdBuffer 包含 CSD 数据的 AVBuffer
     * @return 成功解析并更新环境返回 true，否则返回 false
     */
    bool InitializeWithCsd(const std::shared_ptr<AVBuffer>& csdBuffer);
    /**
     * @brief 针对普通画面帧，做解码前决策。
     * @param index 输入 Buffer 的索引
     * @return true 代表保留（放行并送入解码器），false 代表丢弃
     */
    bool MakePreDecodeDecision(uint32_t index);
    // 解码后决策：负责调用策略层，做最终输出控制
    // 返回 true 表示该帧应当保留并送显，返回 false 表示丢弃并触发 AsyncDropOutputCallback
    bool MakePostDecodeDecision(uint32_t index, int64_t pts, const sptr<SurfaceBuffer> surfaceBuffer);
    // 播放器 Seek 或重置时调用，清空累计误差和历史 PTS
    void Flush();
    void GetStatistics(uint32_t& totalFrames, uint32_t& droppedFrames) const;
    
private:
    int32_t initializeInternal();
    int64_t GetCurrentSysTimeMs();
    void parseMVData(const sptr<SurfaceBuffer> surfaceBuffer, SFDMVStats& mvStats);
    void ParseDynamicFormat(const Media::Format& format, FrameDropMode& outMode, std::optional<double>& outRatio) const;

    void EnsureNaluAnalyzer();
    void EnsureMvParser();

    // 并发控制：保护配置的更新，避免与高频决策线程冲突
    mutable std::mutex configMutex_;

    // 核心统筹与决策组件
    std::unique_ptr<DropSyncCoordinator> dropSyncCoordinator_{nullptr};
    std::unique_ptr<AsyncDropDispatcher> asyncDispatcher_{nullptr};
    std::unique_ptr<IFrameDropper> strategy_{nullptr};
    
    // 解析器组件
    std::unique_ptr<IMVParser> mvParser_{nullptr};
    std::unique_ptr<INaluAnalyzer> naluAnalyzer_{nullptr};
    
    // Dumper组件
    std::unique_ptr<SfdDumper> dumper_{nullptr};

    std::unordered_map<uint32_t, std::shared_ptr<AVBuffer>> inBufferMap_;
    std::shared_ptr<AVBuffer> csdBuffer_{nullptr}; // 用于从 FULL 切换模式时重放头信息激活探针

    SFDConfig config_{};
    bool isInitialized_{false};

    // 统计信息
    std::atomic<uint32_t> totalInputFrames_{0};
    std::atomic<uint32_t> droppedInputFrames_{0};
    std::atomic<uint32_t> totalOutputFrames_{0};
    std::atomic<uint32_t> droppedOutputFrames_{0};

    // 动态倍速信息记录
    double currentPlaybackSpeed_{1.0};

    // 回调函数
    AsyncDropInputCallback asyncDropInputCb_{nullptr};
    AsyncDropOutputCallback asyncDropOutputCb_{nullptr};
};

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS

#endif // SMART_FLUENCY_DECODING_H