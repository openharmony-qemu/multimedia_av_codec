/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "common/media_core.h"

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
     * @brief Reads data frames (synchronous version).
     *
     * This function uses a synchronous implementation mechanism. It is valid only after the RUNNING state.
     *
     * @note This synchronous interface must be used together with the synchronous version of GetNextSampleSize.
     *       Synchronous and asynchronous interfaces (with and without timeout) cannot be mixed in the same instance.
     *
     * @note Memory management: The data will be copied into the memory provided by @param sample->memory_.
     *       The caller must pre-allocate sufficient memory in sample->memory_ before calling this function.
     *
     * @param trackId Identifies the media track. ignore the invalid value is passed.
     * @param sample Buffer where store data frames. The sample->memory_ must be pre-allocated with sufficient capacity.
     * @return  Execution Status return
     *  @retval OK: Plugin ReadFrame succeeded.
     *  @retval ERROR_TIMED_OUT: Operation timeout.
     */
    virtual Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample) = 0;

    /**
     * @brief Reads data frames within @param timeout milliseconds (asynchronous version).
     *
     * This function uses an asynchronous implementation mechanism. It is valid only after the RUNNING state.
     *
     * @note This asynchronous interface must be used together with the asynchronous version of GetNextSampleSize.
     *       Synchronous and asynchronous interfaces (with and without timeout) cannot be mixed in the same instance.
     *
     * @note Memory management: The function will create new memory internally and replace sample->memory_ with it.
     *       The caller does not need to pre-allocate memory in sample->memory_. The original memory will be replaced.
     *
     * @param trackId Identifies the media track. ignore the invalid value is passed.
     * @param sample Buffer where store data frames. The sample->memory_ will be replaced with the new memory.
     * @param timeout If no result is available after @param timeout milliseconds, the function returns.
     * @return  Execution Status return
     *  @retval OK: Plugin ReadFrame succeeded.
     *  @retval ERROR_WAIT_TIMEOUT: Operation timeout.
     *  @retval END_OF_STREAM: Read end (EOS).
     *  @retval ERROR_UNKNOWN: Call av_read_frame failed.
     *  @retval ERROR_NULL_POINTER: Call av_packet_alloc failed.
     */
    virtual Status ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample, uint32_t timeout) = 0;

    /**
     * @brief Get next sample size (synchronous version).
     *
     * This function uses a synchronous implementation mechanism. It is valid only after the RUNNING state.
     *
     * @note This synchronous interface must be used together with the synchronous version of ReadSample.
     *       Synchronous and asynchronous interfaces (with and without timeout) cannot be mixed in the same instance.
     *
     * @param trackId Identifies the media track. ignore the invalid value is passed.
     * @param size Output parameter for the next sample size.
     * @return Execution Status
     */
    virtual Status GetNextSampleSize(uint32_t trackId, int32_t& size) = 0;

    /**
     * @brief Get next sample size within @param timeout milliseconds (asynchronous version).
     *
     * This function uses an asynchronous implementation mechanism. It is valid only after the RUNNING state.
     *
     * @note This asynchronous interface must be used together with the asynchronous version of ReadSample.
     *       Synchronous and asynchronous interfaces (with and without timeout) cannot be mixed in the same instance.
     *
     * @param trackId Identifies the media track. ignore the invalid value is passed.
     * @param size Output parameter for the next sample size.
     * @param timeout If no result is available after @param timeout milliseconds, the function returns.
     * @return Execution Status
     */
    virtual Status GetNextSampleSize(uint32_t trackId, int32_t& size, uint32_t timeout) = 0;

    /**
     * @brief Pause reading data from ffmpeg in another thread.
     *
     * @return Execution Status
     */
    virtual Status Pause() = 0;

    /**
     * @brief Get the latest PTS by trackId.
     *
     * @param trackId Identifies the media track. ignore the invalid value is passed.
     * @param lastPTS the latest PTS.
     * @return Execution Status
     */
    virtual Status GetLastPTSByTrackId(uint32_t trackId, int64_t &lastPTS) = 0;

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

    virtual void ResetEosStatus() = 0;

    virtual bool IsRefParserSupported() { return false; };
    virtual Status ParserRefUpdatePos(int64_t timeStampMs, bool isForward = true) = 0;
    virtual Status ParserRefInfo() = 0;
    virtual Status GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo) = 0;
    virtual Status GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo) = 0;
    virtual Status GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo) = 0;
    virtual Status GetIFramePos(std::vector<uint32_t> &IFramePos) = 0;
    virtual Status Dts2FrameId(int64_t dts, uint32_t &frameId) = 0;
    virtual Status SeekMs2FrameId(int64_t seekMs, uint32_t &frameId) = 0;
    virtual Status FrameId2SeekMs(uint32_t frameId, int64_t &seekMs) = 0;

    virtual Status GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo)
    {
        (void)drmInfo;
        return Status::OK;
    }

    virtual void SetInterruptState(bool isInterruptNeeded) {}

    virtual Status GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
        const uint64_t relativePresentationTimeUs, uint32_t &index) = 0;

    virtual Status GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
        const uint32_t index, uint64_t &relativePresentationTimeUs) = 0;

    virtual void SetCacheLimit(uint32_t limitSize) = 0;
    virtual Status GetCurrentCacheSize(uint32_t trackId, uint32_t& size)
    {
        size = 0;
        return Status::OK;
    };
    virtual bool GetProbeSize(int32_t &offset, int32_t &size) { return false; };
    virtual Status SetDataSourceWithProbSize(const std::shared_ptr<DataSource>& source,
        const int32_t probSize) = 0;

    /**
     * @brief Boosts the asynchronous read thread priority.
     *
     * Elevates thread priority if called with proper permissions and when thread is not RUNNING.
     * By default, the thread operates at normal priority.
     *
     * @return Execution status
     */
    virtual Status BoostReadThreadPriority() = 0;
    virtual Status SetAVReadPacketStopState(bool state) = 0;
    virtual Status SeekToStart() = 0;
    virtual Status SeekToKeyFrame(int32_t trackId, int64_t seekTime,
        SeekMode mode, int64_t& realSeekTime, uint32_t timeoutMs) { return Status::ERROR_UNKNOWN; };
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
