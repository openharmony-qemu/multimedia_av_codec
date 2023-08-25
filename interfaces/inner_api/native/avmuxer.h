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

#ifndef MEDIA_AVCODEC_AVMUXER_H
#define MEDIA_AVCODEC_AVMUXER_H

#include "avsharedmemory.h"
#include "media_description.h"
#include "av_common.h"
#include "avcodec_common.h"

namespace OHOS {
namespace MediaAVCodec {
class AVMuxer {
public:
    virtual ~AVMuxer() = default;

    /**
     * @brief Set the rotation for output video playback.
     * Note: This interface can only be called before Start.
     * @param rotation The supported angles are 0, 90, 180, and 270 degrees.
     * @return Returns AVCS_ERR_OK if the execution is successful,
     * otherwise returns a specific error code, refer to {@link AVCodecServiceErrCode}
     * @since 10
     */
    virtual int32_t SetRotation(int32_t rotation) = 0;

    /**
     * @brief Add track format to the muxer.
     * Note: This interface can only be called before Start.
     * @param trackIndex Used to get the track index for this newly added track,
     * and it should be used in the WriteSample. The track index is greater than or equal to 0,
     * others is error index.
     * @param trackFormat OH_AVFormat handle pointer contain track format
     * @return Returns AVCS_ERR_OK if the execution is successful,
     * otherwise returns a specific error code, refer to {@link AVCodecServiceErrCode}
     * @since 10
     */
    virtual int32_t AddTrack(int32_t &trackIndex, const MediaDescription &trackDesc) = 0;

    /**
     * @brief Start the muxer.
     * Note: This interface is called after AddTrack and before WriteSample.
     * @return Returns AVCS_ERR_OK if the execution is successful,
     * otherwise returns a specific error code, refer to {@link AVCodecServiceErrCode}
     * @since 10
     */
    virtual int32_t Start() = 0;

    /**
     * @brief Write an encoded sample to the muxer.
     * Note: This interface can only be called after Start and before Stop. The application needs to
     * make sure that the samples are written to the right tacks. Also, it needs to make sure the samples
     * for each track are written in chronological order.
     * @param trackIndex The track index for this sample
     * @param sample The encoded or demuxer sample
     * @param info The buffer information related to this sample {@link AVCodecBufferInfo}
     * @param flag The buffer flag related to this sample {@link AVCodecBufferFlag}
     * @return Returns AVCS_ERR_OK if the execution is successful,
     * otherwise returns a specific error code, refer to {@link AVCodecServiceErrCode}
     * @since 10
     */
    virtual int32_t WriteSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
        AVCodecBufferInfo info, AVCodecBufferFlag flag) = 0;

    /**
     * @brief Stop the muxer.
     * Note: Once the muxer stops, it can not be restarted.
     * @return Returns AVCS_ERR_OK if the execution is successful,
     * otherwise returns a specific error code, refer to {@link AVCodecServiceErrCode}
     * @since 10
     */
    virtual int32_t Stop() = 0;
};

class __attribute__((visibility("default"))) AVMuxerFactory {
public:
    /**
     * @brief Create an AVMuxer instance by output file description and format.
     * @param fd Must be opened with read and write permission. Caller is responsible for closing fd.
     * @param format The output format is {@link OutputFormat} .
     * @return Returns a pointer to an AVMuxer instance.
     * @since 10
     */
    static std::shared_ptr<AVMuxer> CreateAVMuxer(int32_t fd, OutputFormat format);
private:
    AVMuxerFactory() = default;
    ~AVMuxerFactory() = default;
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif // MEDIA_AVCODEC_AVMUXER_H
