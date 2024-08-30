/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef STREAM_DEMUXER_H
#define STREAM_DEMUXER_H

#include <atomic>
#include <limits>
#include <string>
#include <shared_mutex>

#include "base_stream_demuxer.h"

#include "avcodec_common.h"
#include "buffer/avbuffer.h"
#include "common/media_source.h"
#include "demuxer/type_finder.h"
#include "filter/filter.h"
#include "meta/media_types.h"
#include "osal/task/task.h"
#include "plugin/plugin_base.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_time.h"
#include "plugin/demuxer_plugin.h"

namespace OHOS {
namespace Media {

class StreamDemuxer : public BaseStreamDemuxer {
public:
    explicit StreamDemuxer();
    ~StreamDemuxer() override;

    Status Init(const std::string& uri) override;
    Status Pause() override;
    Status Resume() override;
    Status Start() override;
    Status Stop() override;
    Status Flush() override;
    Status CallbackReadAt(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
        size_t expectedLen) override;
    Status ResetCache(int32_t streamID) override;
    Status ResetAllCache() override;
private:
    Status PullData(int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Plugins::Buffer>& data);
    Status PullDataWithoutCache(int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
    Status PullDataWithCache(int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
    Status GetPeekRange(int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
    Status ReadHeaderData(int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
    Status ReadFrameData(int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
    Status ReadRetry(int32_t streamID, uint64_t offset, size_t size, std::shared_ptr<Plugins::Buffer>& data);
    Status HandleReadHeader(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen);
    Status HandleReadPacket(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen);
    Status CheckChangeStreamID(int32_t streamID, std::shared_ptr<Buffer>& buffer);
    Status ProcInnerDash(int32_t streamID,  uint64_t offset, std::shared_ptr<Buffer>& bufferPtr);
private:
    std::map<int32_t, CacheData> cacheDataMap_;
    uint64_t position_;
};
} // namespace Media
} // namespace OHOS
#endif // STREAM_DEMUXER_H
