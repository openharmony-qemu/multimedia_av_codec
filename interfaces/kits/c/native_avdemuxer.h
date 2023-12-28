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

#ifndef NATIVE_AVDEMUXER_H
#define NATIVE_AVDEMUXER_H

#include <stdint.h>
#include "native_avcodec_base.h"
#include "native_averrors.h"
#include "native_avmemory.h"
#include "native_avsource.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OH_AVDemuxer OH_AVDemuxer;
typedef struct OH_AVBuffer OH_AVBuffer;

/**
 * @brief Pssh info item. uuid points to the drm system unique identifier,
 * dataLen specifies the pssh length, and data points to the pssh buffer.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @since 10
 */
typedef struct OH_PsshEntry {
    unsigned char *uuid;
    size_t dataLen;
    void *data;
} OH_PsshEntry;

/**
 * @brief Drm info. psshCount indicates the number of entries,
 * and entries points to the pssh entry buffer.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @since 10
 */
typedef struct OH_DrmInfo {
    size_t psshCount;
    struct OH_PsshEntry *entries;
} OH_DrmInfo;

/**
 * @brief When new drm info is generated, the function will be called.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param demuxer OH_AVDemuxer instance.
 * @param drmInfo Drminfo.
 * @param userData specified data.
 * @since 10
 * @version 1.0
 */
typedef void (*OH_AVDemuxerOnDrmInfoChanged)(OH_AVDemuxer *demuxer, const OH_DrmInfo *drmInfo,
    void *userData);

/**
 * @brief A collection of all asynchronous callback function pointers in OH_AVDemuxer.
 * Register an instance of this structure to the OH_AVDemuxer instance, and process
 * the information reported through the callback to ensure the normal operation of OH_AVDemuxer.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param onDrmInfoChanged Monitor drm information, refer to {@link OH_AVDemuxerOnDrmInfoChanged}
 * @since 10
 * @version 1.0
 */
typedef struct OH_AVDemuxerCallback {
    OH_AVDemuxerOnDrmInfoChanged onDrmInfoChanged;
} OH_AVDemuxerCallback;

/**
 * @brief Creates an OH_AVDemuxer instance for getting samples from source.
 * Free the resources of the instance by calling OH_AVDemuxer_Destroy.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param source Pointer to an OH_AVSource instance.
 * @return Returns a pointer to an OH_AVDemuxer instance
 * @since 10
*/
OH_AVDemuxer *OH_AVDemuxer_CreateWithSource(OH_AVSource *source);

/**
 * @brief Destroy the OH_AVDemuxer instance and free the internal resources.
 * The same instance can only be destroyed once. The destroyed instance
 * should not be used before it is created again. It is recommended setting
 * the instance pointer to NULL right after the instance is destroyed successfully.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 10
*/
OH_AVErrCode OH_AVDemuxer_Destroy(OH_AVDemuxer *demuxer);

/**
 * @brief The specified track is selected and the demuxer will read samples from
 * this track. Multiple tracks are selected by calling this interface multiple times
 * with different track indexes. Only the selected tracks are valid when calling
 * OH_AVDemuxer_ReadSample to read samples. The interface returns AV_ERR_OK and the
 * track is selected only once if the same track is selected multiple times.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param trackIndex The index of the selected track.
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 10
*/
OH_AVErrCode OH_AVDemuxer_SelectTrackByID(OH_AVDemuxer *demuxer, uint32_t trackIndex);

/**
 * @brief The specified selected track is unselected. The unselected track's sample
 * can not be read from demuxer. Multiple selected tracks are unselected by calling
 * this interface multiple times with different track indexes. The interface returns
 * AV_ERR_OK and the track is unselected only once if the same track is unselected
 * multiple times.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param trackIndex The index of the unselected track.
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 10
*/
OH_AVErrCode OH_AVDemuxer_UnselectTrackByID(OH_AVDemuxer *demuxer, uint32_t trackIndex);

/**
 * @brief Get the current encoded sample and sample-related information from the specified
 * track. The track index must be selected before reading sample. The demuxer will advance
 * automatically after calling this interface.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param trackIndex The index of the track from which read an encoded sample.
 * @param sample The OH_AVMemory handle pointer to the buffer storing the sample data.
 * @param info The OH_AVCodecBufferAttr handle pointer to the buffer storing sample information.
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @deprecated since 11
 * @useinstead OH_AVDemuxer_ReadSampleBuffer
 * @since 10
*/
OH_AVErrCode OH_AVDemuxer_ReadSample(OH_AVDemuxer *demuxer, uint32_t trackIndex,
    OH_AVMemory *sample, OH_AVCodecBufferAttr *info);

/**
 * @brief Get the current encoded sample and sample-related information from the specified
 * track. The track index must be selected before reading sample. The demuxer will advance
 * automatically after calling this interface.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param trackIndex The index of the track from which read an encoded sample.
 * @param sample The OH_AVBuffer handle pointer to the buffer storing the sample data and corresponding attribute.
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 11
*/
OH_AVErrCode OH_AVDemuxer_ReadSampleBuffer(OH_AVDemuxer *demuxer, uint32_t trackIndex,
    OH_AVBuffer *sample);

/**
 * @brief All selected tracks seek near to the requested time according to the seek mode.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param millisecond The millisecond for seeking, the timestamp is the position of
 * the file relative to the start of the file.
 * @param mode The mode for seeking. See {@link OH_AVSeekMode}.
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 10
*/
OH_AVErrCode OH_AVDemuxer_SeekToTime(OH_AVDemuxer *demuxer, int64_t millisecond, OH_AVSeekMode mode);

/**
 * @brief Set callback.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param callback A collection of all callback functions, see {@link OH_AVDemuxerCallback}
 * @param userData User specific data
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 10
*/
OH_AVErrCode OH_AVDemuxer_SetCallback(OH_AVDemuxer *demuxer, OH_AVDemuxerCallback callback, void *userData);

#ifdef __cplusplus
}
#endif

#endif // NATIVE_AVDEMUXER_H
