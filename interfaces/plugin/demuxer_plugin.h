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

#ifndef AVCODEC_DEMUXER_PLUGIN_H
#define AVCODEC_DEMUXER_PLUGIN_H

#include <memory>
#include <vector>
#include "meta/meta.h"
#include "plugin/plugin_base.h"
#include "plugin/plugin_caps.h"
#include "plugin/plugin_definition.h"
#include "plugin/plugin_info.h"

namespace OHOS {
namespace Media {
namespace Plugins {
/**
 * @brief Demuxer Plugin Interface.
 *
 * Used for audio and video media file parse.
 *
 * @since 1.0
 * @version 1.0
 */
struct DemuxerPlugin : public PluginBase {
    /// constructor
    explicit DemuxerPlugin(std::string name): PluginBase(std::move(name)) {}
    /**
     * @brief Set the data source to demuxer component.
     *
     * The function is valid only in the CREATED state.
     *
     * @param source Data source where data read from.
     * @return  Execution Status return
     *  @retval OK: Plugin SetDataSource succeeded.
     */
    virtual Status SetDataSource(const std::shared_ptr<DataSource>& source) = 0;

    /**
     * @brief Get the attributes of a media file.
     *
     * The attributes contain file and stream attributes.
     * The function is valid only after INITIALIZED state.
     *
     * @param mediaInfo Indicates the pointer to the source attributes
     * @return  Execution status return
     *  @retval OK: Plugin GetMediaInfo succeeded.
     */
    virtual Status GetMediaInfo(MediaInfo& mediaInfo) = 0;

    /**
     * @brief Get the user meta of a media file.
     *
     * The attributes contain file and stream attributes.
     * The function is valid only after INITIALIZED state.
     *
     * @param mediaInfo Indicates the pointer to the user meta attributes
     * @return  Execution status return
     *  @retval OK: Plugin GetUserMeta succeeded.
     */
    virtual Status GetUserMeta(std::shared_ptr<Meta> meta) = 0;

    /**
     * @brief Select a specified media track.
     *
     * The function is valid only after RUNNING state.
     *
     * @param trackId Identifies the media track. If an invalid value is passed, the default media track specified.
     * @return  Execution Status return
     *  @retval OK: Plugin SelectTrack succeeded.
     */
    virtual Status SelectTrack(uint32_t trackId) = 0;

    /**
     * @brief Unselect a specified media track from which the demuxer reads data frames.
     *
     * The function is valid only after RUNNING state.
     *
     * @param trackId Identifies the media track. ignore the invalid value is passed.
     * @return  Execution Status return
     *  @retval OK: Plugin UnselectTrack succeeded.
     */
    virtual Status UnselectTrack(uint32_t trackId) = 0;

    /**
     * @brief Reads data frames.
     *
     * The function is valid only after RUNNING state.
     *
     * @param trackId Identifies the media track. ignore the invalid value is passed.
     * @param sample Buffer where store data frames.
     * @return  Execution Status return
     *  @retval OK: Plugin ReadFrame succeeded.
     *  @retval ERROR_TIMED_OUT: Operation timeout.
     */
    virtual Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample) = 0;

    /**
     * @brief Get next sample size.
     *
     * The function is valid only after RUNNING state.
     *
     * @param trackId Identifies the media track. ignore the invalid value is passed.
     * @return Execution Status
     */
    virtual Status GetNextSampleSize(uint32_t trackId, int32_t& size) = 0;

    /**
     * @brief Seeks for a specified position for the demuxer.
     *
     * After being started, the demuxer seeks for a specified position to read data frames.
     *
     * The function is valid only after RUNNING state.
     *
     * @param trackId Identifies the stream in the media file.
     * @param seekTime Indicates the target position, based on {@link HST_TIME_BASE} .
     * @param mode Indicates the seek mode.
     * @param realSeekTime Indicates the accurate target position, based on {@link HST_TIME_BASE} .
     * @return  Execution Status return
     *  @retval OK: Plugin SeekTo succeeded.
     *  @retval ERROR_INVALID_DATA: The input data is invalid.
     */
    virtual Status SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode, int64_t& realSeekTime) = 0;

    virtual Status Reset() = 0;

    virtual Status Start() = 0;

    virtual Status Stop() = 0;

    virtual Status Flush() = 0;

    virtual Status GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo)
    {
        (void)drmInfo;
        return Status::OK;
    }
};

/// Demuxer plugin api major number.
#define DEMUXER_API_VERSION_MAJOR (1)

/// Demuxer plugin api minor number
#define DEMUXER_API_VERSION_MINOR (0)

/// Demuxer plugin version
#define DEMUXER_API_VERSION MAKE_VERSION(DEMUXER_API_VERSION_MAJOR, DEMUXER_API_VERSION_MINOR)

/**
 * @brief Describes the demuxer plugin information.
 *
 * @since 1.0
 * @version 1.0
 */
struct DemuxerPluginDef : public PluginDefBase {
    DemuxerPluginDef()
        : PluginDefBase()
    {
        apiVersion = DEMUXER_API_VERSION; ///< Demuxer plugin version.
        pluginType = PluginType::DEMUXER; ///< Plugin type, MUST be DEMUXER.
    }
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // AVCODEC_DEMUXER_PLUGIN_H
