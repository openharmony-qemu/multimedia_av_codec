/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef VOD_STREAM_DEMUXER_H
#define VOD_STREAM_DEMUXER_H

#include <atomic>
#include <limits>
#include <string>
#include <shared_mutex>

#include "base_stream_demuxer.h"

#include "avcodec_common.h"
#include "buffer/avbuffer.h"
#include "common/media_source.h"
#include "demuxer/data_packer.h"
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

class VodStreamDemuxer : public BaseStreamDemuxer {
public:
    explicit VodStreamDemuxer();
    ~VodStreamDemuxer() override;

    std::string Init(std::string uri, uint64_t mediaDataSize) override;
    Status Reset() override;
    Status Pause() override;
    Status Resume() override;
    Status Start() override;
    Status Stop() override;
    Status Flush() override;
    Status CallbackReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) override;
private:
    Status PullData(uint64_t offset, size_t size, std::shared_ptr<Plugins::Buffer>& data);
    bool PullDataWithoutCache(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
    bool PullDataWithCache(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
    bool GetPeekRange(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
    bool GetPeekRangeSub(uint64_t offset, size_t size, std::shared_ptr<Buffer>& bufferPtr);
private:
    struct CacheData {
        std::shared_ptr<Buffer> data = nullptr;
        uint64_t offset = 0;
    };
    CacheData cacheData_;
    uint64_t position_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DEMUXER_H
