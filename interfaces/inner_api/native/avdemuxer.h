/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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


#ifndef MEDIA_AVCODEC_AVDEMUXER_H
#define MEDIA_AVCODEC_AVDEMUXER_H

#include <memory>
#include "avcodec_common.h"
#include "buffer/avbuffer.h"
#include "buffer/avsharedmemory.h"
#include "avsource.h"
#include "meta/media_types.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
class AVDemuxer {
public:
    ~AVDemuxer() = default;

    /**
     * @brief Select the sourceTrack by track index.
     * This function can only by called before {@link ReadSample}.
     * @param trackIndex The track index for being selected.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 4.0
     */
    virtual int32_t SelectTrackByID(uint32_t trackIndex) = 0;

    /**
     * @brief Unselect the sourceTrack by track index.
     * This function can only by called before {@link ReadSample}.
     * @param trackIndex The track index for being unselected.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 4.0
     */
    virtual int32_t UnselectTrackByID(uint32_t trackIndex) = 0;

    /**
     * @brief Retrieve the sample in selected tracks and store it in buffer, and store buffer's info to attr.
     * @param trackIndex Get the sampleBuffer from this track.
     * @param sample The AVSharedMemory handle pointer to get buffer data.
     * @param info The CodecBufferAttr handle pointer to get buffer info.
     * @param flag The buffer flags.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 4.0
     */
    virtual int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
        AVCodecBufferInfo &info, uint32_t &flag) = 0;

    /**
     * @brief Retrieve the sample in selected tracks and store it in buffer, and store buffer's info to attr.
     * @param trackIndex Get the sampleBuffer from this track.
     * @param sample The AVSharedMemory handle pointer to get buffer data.
     * @param info The CodecBufferAttr handle pointer to get buffer info.
     * @param flag The buffer flags.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 4.0
     */
    virtual int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
        AVCodecBufferInfo &info, AVCodecBufferFlag &flag) = 0;

     /**
     * @brief Retrieve the sample in selected tracks and store it in buffer, and store buffer's info to attr.
     * @param trackIndex Get the sampleBuffer from this track.
     * @param sample The AVBuffer handle pointer to get buffer data.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 4.1
     */
    virtual int32_t ReadSampleBuffer(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample) = 0;

    /**
     * @brief All selected tracks seek near to the requested time according to the seek mode.
     * @param millisecond The timestamp for seeking which is the position relative to the beginning of the file.
     * @param mode The mode for seeking. Value. For details, see {@link SeekMode}.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 4.0
     */
    virtual int32_t SeekToTime(int64_t millisecond, SeekMode mode) = 0;

    /**
     * @brief Registers a demuxer listener.
     *
     * @param callback Indicates the demuxer listener to register. For details, see {@link AVDemuxerCallback}.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 4.1
     * @version 4.1
     */
    virtual int32_t SetCallback(const std::shared_ptr<AVDemuxerCallback> &callback) = 0;

    virtual int32_t GetMediaKeySystemInfo(std::multimap<std::string, std::vector<uint8_t>> &infos) = 0;
};

class __attribute__((visibility("default"))) AVDemuxerFactory {
public:
#ifdef UNSUPPORT_DEMUXER
    static std::shared_ptr<AVDemuxer> CreateWithSource(std::shared_ptr<AVSource> source)
    {
        return nullptr;
    }
#else
    /**
     * @brief Instantiate the preferred demuxer of the given source instance.
     * @param sourceAddr The address for source instance.
     * @return Returns the preferred demuxer.
     * @since 4.0
     */
    static std::shared_ptr<AVDemuxer> CreateWithSource(std::shared_ptr<AVSource> source);
#endif
private:
    AVDemuxerFactory() = default;
    ~AVDemuxerFactory() = default;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_AVDEMUXER_H
