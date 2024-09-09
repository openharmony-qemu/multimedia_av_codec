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

class CacheData {
public:
    CacheData() {}
    ~CacheData()
    {
        Reset();
    }
    void Reset()
    {
        data = nullptr;
        offset = 0;
    }
    bool CheckCacheExist(uint64_t len)
    {
        if (data != nullptr) {
            if (data->GetMemory() != nullptr) {
                return len >= offset && len < (offset + data->GetMemory()->GetSize());
            }
        }
        return false;
    }
    uint64_t GetOffset()
    {
        return offset;
    }
    std::shared_ptr<Buffer> GetData()
    {
        return data;
    }
    void SetData(const std::shared_ptr<Buffer>& buffer)
    {
        data = buffer;
    }
    void Init(const std::shared_ptr<Buffer>& buffer, uint64_t bufferOffset)
    {
        data = buffer;
        offset = bufferOffset;
    }
private:
    std::shared_ptr<Buffer> data = nullptr;
    uint64_t offset = 0;
};

class BaseStreamDemuxer {
public:
    explicit BaseStreamDemuxer();
    virtual ~BaseStreamDemuxer();

    virtual Status Init(const std::string& uri) = 0;
    virtual Status Pause() = 0;
    virtual Status Resume() = 0;
    virtual Status Start() = 0;
    virtual Status Stop() = 0;
    virtual Status Flush() = 0;
    virtual Status ResetCache(int32_t streamID) = 0;
    virtual Status ResetAllCache() = 0;

    void InitTypeFinder();
    void SetSource(const std::shared_ptr<Source>& source);

    virtual Status CallbackReadAt(int32_t streamID, int64_t offset, std::shared_ptr<Buffer>& buffer,
        size_t expectedLen) = 0;
    void SetInterruptState(bool isInterruptNeeded);
    void SetDemuxerState(int32_t streamId, DemuxerState state);
    void SetBundleName(const std::string& bundleName);
    void SetIsIgnoreParse(bool state);
    bool GetIsIgnoreParse();
    Plugins::Seekable GetSeekable();
    std::string SnifferMediaType(int32_t streamID);
    bool IsDash() const;
    void SetIsDash(bool flag);

    Status SetNewAudioStreamID(int32_t streamID);
    Status SetNewVideoStreamID(int32_t streamID);
    Status SetNewSubtitleStreamID(int32_t streamID);
    int32_t GetNewVideoStreamID();
    int32_t GetNewAudioStreamID();
    int32_t GetNewSubtitleStreamID();
    bool CanDoChangeStream();
    void SetChangeFlag(bool flag);
protected:
    std::shared_ptr<Source> source_;
    std::function<Status(int32_t, uint64_t, size_t)> checkRange_;
    std::function<Status(int32_t, uint64_t, size_t, std::shared_ptr<Buffer>&)> peekRange_;
    std::function<Status(int32_t, uint64_t, size_t, std::shared_ptr<Buffer>&)> getRange_;
    std::map<int32_t, DemuxerState> pluginStateMap_;
    std::atomic<bool> isIgnoreParse_{false};
    std::atomic<bool> isInterruptNeeded_{false};
    std::string bundleName_ {};
    std::string uri_ {};
public:
    uint64_t mediaDataSize_{0};
    Plugins::Seekable seekable_;
private:
    bool isDash_ = {false};
    std::atomic<int32_t> newVideoStreamID_ = -1;
    std::atomic<int32_t> newAudioStreamID_ = -1;
    std::atomic<int32_t> newSubtitleStreamID_ = -1;
    std::atomic<bool> changeStreamFlag_ = true;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DEMUXER_H
