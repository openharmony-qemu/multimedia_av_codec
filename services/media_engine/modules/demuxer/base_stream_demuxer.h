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

#ifndef BASE_STREAM_DEMUXER_H
#define BASE_STREAM_DEMUXER_H

#include <atomic>
#include <limits>
#include <string>
#include <shared_mutex>

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
#include "source/source.h"

namespace OHOS {
namespace Media {

enum class DemuxerState {
    DEMUXER_STATE_NULL,
    DEMUXER_STATE_PARSE_HEADER,
    DEMUXER_STATE_PARSE_FIRST_FRAME,
    DEMUXER_STATE_PARSE_FRAME
};

class BaseStreamDemuxer {
public:
    explicit BaseStreamDemuxer();
    virtual ~BaseStreamDemuxer();

    virtual std::string Init(std::string uri, uint64_t mediaDataSize) = 0;
    virtual Status Reset() = 0;
    virtual Status Pause() = 0;
    virtual Status Resume() = 0;
    virtual Status Start() = 0;
    virtual Status Stop() = 0;
    virtual Status Flush() = 0;

    void InitTypeFinder();
    void SetSource(const std::shared_ptr<Source>& source);

    virtual Status CallbackReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) = 0;
    void SetDemuxerState(DemuxerState state);
    void SetBundleName(const std::string& bundleName);
    void SetIsIgnoreParse(bool state);
    bool GetIsIgnoreParse();
protected:
    std::shared_ptr<TypeFinder> typeFinder_;
    std::shared_ptr<Source> source_;
    std::function<bool(uint64_t, size_t)> checkRange_;
    std::function<bool(uint64_t, size_t, std::shared_ptr<Buffer>&)> peekRange_;
    std::function<bool(uint64_t, size_t, std::shared_ptr<Buffer>&)> getRange_;
    std::atomic<DemuxerState> pluginState_{DemuxerState::DEMUXER_STATE_NULL};
    std::atomic<bool> isIgnoreParse_{false};
    std::atomic<bool> isIgnoreRead_{false};
    std::string bundleName_ {};
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DEMUXER_H
