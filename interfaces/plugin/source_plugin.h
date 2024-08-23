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

#ifndef AVCODEC_SOURCE_PLUGIN_H
#define AVCODEC_SOURCE_PLUGIN_H

#include <map>
#include <string>

#include "common/media_source.h"
#include "plugin/plugin_base.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_caps.h"
#include "plugin/plugin_definition.h"
#include "meta/media_types.h"
#include "plugin/plugin_time.h"

namespace OHOS {
namespace Media {
namespace Plugins {

enum StreamType {
    MIXED = 0,
    VIDEO,
    AUDIO,
    SUBTITLE
};

enum VideoType {
    VIDEO_TYPE_SDR = 0,
    VIDEO_TYPE_HDR_VIVID = 1,
    VIDEO_TYPE_HDR_10
};

class StreamInfo {
public:
    int32_t streamId;
    StreamType type;
    uint32_t bitRate;

    int32_t videoHeight = 0;
    int32_t videoWidth = 0;
    std::string lang = "";
    VideoType videoType = VideoType::VIDEO_TYPE_SDR;
    std::string trackName = "";
};

/**
 * @brief Source Plugin Interface.
 *
 * The data source may be network push or active read.
 *
 * @since 1.0
 * @version 1.0
 */
class SourcePlugin : public PluginBase {
    /// constructor
public:
    explicit SourcePlugin(std::string name): PluginBase(std::move(name)) {}
    /**
     * @brief Set the data source to source plugin.
     *
     * The function is valid only in the CREATED state.
     *
     * @param source data source, uri or stream source
     * @return  Execution status return
     *  @retval OK: Plugin SetSource succeeded.
     *  @retval ERROR_WRONG_STATE: Call this function in non wrong state
     *  @retval ERROR_NOT_EXISTED: Uri is not existed.
     *  @retval ERROR_UNSUPPORTED_FORMAT: Uri is not supported.
     *  @retval ERROR_INVALID_PARAMETER: Uri is invalid.
     */
    virtual Status SetSource(std::shared_ptr<MediaSource> source) = 0;

    /**
     * @brief Read data from data source.
     *
     * The function is valid only after RUNNING state.
     *
     * @param buffer Buffer to store the data, it can be nullptr or empty to get the buffer from plugin.
     * @param expectedLen   Expected data size to be read
     * @return  Execution status return
     *  @retval OK: Plugin Read succeeded.
     *  @retval ERROR_NOT_ENOUGH_DATA: Data not enough
     *  @retval END_OF_STREAM: End of stream
     */
    virtual Status Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen) = 0;

    /**
     * @brief Read data from data source.
     *
     * The function is valid only after RUNNING state.
     *
     * @param streamId stream index.
     * @param buffer Buffer to store the data, it can be nullptr or empty to get the buffer from plugin.
     * @param expectedLen   Expected data size to be read
     * @return  Execution status return
     *  @retval OK: Plugin Read succeeded.
     *  @retval ERROR_NOT_ENOUGH_DATA: Data not enough
     *  @retval END_OF_STREAM: End of stream
     */
    virtual Status Read(int32_t streamId, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
    {
        return Status::OK;
    }

    /**
     * @brief Get data source size.
     *
     * The function is valid only after INITIALIZED state.
     *
     * @param size data source size.
     * @return  Execution status return.
     *  @retval OK: Plugin GetSize succeeded.
     */
    virtual Status GetSize(uint64_t& size) = 0;

    /**
     * @brief Indicates that the current source can be seek.
     *
     * The function is valid only after INITIALIZED state.
     *
     * @return  Execution status return
     *  @retval OK: Plugin GetSeekable succeeded.
     */
    virtual Seekable GetSeekable() = 0;

    /**
     * @brief Seeks for a specified position for the source.
     *
     * After being started, the source seeks for a specified position to read data frames.
     *
     * The function is valid only after RUNNING state.
     *
     * @param offset position to read data frames
     * @return  Execution status return
     *  @retval OK: Plugin SeekTo succeeded.
     *  @retval ERROR_INVALID_DATA: The offset is invalid.
     */
    virtual Status SeekTo(uint64_t offset) = 0;

    virtual Status Reset() = 0;

    virtual void SetDemuxerState(int32_t streamId) {}

    virtual void SetDownloadErrorState() {}

    virtual void SetBundleName(const std::string& bundleName) {}

    virtual Status GetDownloadInfo(DownloadInfo& downloadInfo)
    {
        return Status::OK;
    }

    virtual Status GetPlaybackInfo(PlaybackInfo& playbackInfo)
    {
        return Status::OK;
    }

    virtual Status GetBitRates(std::vector<uint32_t>& bitRates)
    {
        return Status::OK;
    }

    virtual Status SelectBitRate(uint32_t bitRate)
    {
        return Status::OK;
    }

    virtual bool IsSeekToTimeSupported()
    {
        return false;
    }

    virtual Status SeekToTime(int64_t seekTime, SeekMode mode)
    {
        return Status::OK;
    }

    virtual Status GetDuration(int64_t& duration)
    {
        duration = Plugins::HST_TIME_NONE;
        return Status::OK;
    }

    virtual bool IsNeedPreDownload()
    {
        return false;
    }

    virtual Status SetReadBlockingFlag(bool isReadBlockingAllowed)
    {
        return Status::OK;
    }

    virtual void SetInterruptState(bool isInterruptNeeded) {}

    virtual Status SetCurrentBitRate(int32_t bitRate, int32_t streamID)
    {
        return Status::OK;
    }

    virtual Status GetStreamInfo(std::vector<StreamInfo>& streams)
    {
        return Status::OK;
    }

    virtual Status SelectStream(int32_t streamID)
    {
        return Status::OK;
    }

    virtual Status Pause()
    {
        return Status::OK;
    }

    virtual Status Resume()
    {
        return Status::OK;
    }
};

/// Source plugin api major number.
#define SOURCE_API_VERSION_MAJOR (1)

/// Source plugin api minor number
#define SOURCE_API_VERSION_MINOR (0)

/// Source plugin version
#define SOURCE_API_VERSION MAKE_VERSION(SOURCE_API_VERSION_MAJOR, SOURCE_API_VERSION_MINOR)

/**
 * @brief Describes the source plugin information.
 *
 * @since 1.0
 * @version 1.0
 */
struct SourcePluginDef : public PluginDefBase {
    SourcePluginDef()
        : PluginDefBase()
    {
        apiVersion = SOURCE_API_VERSION; ///< Source plugin version.
        pluginType = PluginType::SOURCE; ///< Plugin type, MUST be SOURCE.
    }
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_SOURCE_PLUGIN_H
