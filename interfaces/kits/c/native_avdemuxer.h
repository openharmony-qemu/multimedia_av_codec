/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @addtogroup AVDemuxer
 * @{
 *
 * @brief The AVDemuxer module provides an interface for extracting samples from media file streams.
 *
 * @since 10
 */
/**
 * @file native_avdemuxer.h
 *
 * @brief Declare the interface for parsing audio and video media data.
 *
 * @kit AVCodecKit
 * @library libnative_media_avdemuxer.so
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @since 10
 */

#ifndef NATIVE_AVDEMUXER_H
#define NATIVE_AVDEMUXER_H

#include <stdint.h>
#include "native_avcodec_base.h"
#include "native_avsource.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The struct describes a native object for the OH_AVDemuxer interface.
 *
 * @since 10
 */
typedef struct OH_AVDemuxer OH_AVDemuxer;
/**
 * @brief The struct describes a native object for the DRM_MediaKeySystemInfo interface.
 *
 * @since 11
 */
typedef struct DRM_MediaKeySystemInfo DRM_MediaKeySystemInfo;

/**
 * @brief Defines a pointer to the callback function for DRM_MediaKeySystemInfo. No demuxer instance is returned. This
 * callback function applies to the scenario where a single demuxer instance is used.
 *
 * You need to call {@link OH_AVDemuxer_SetMediaKeySystemInfoCallback} to set the callback function as a callback.
 *
 * @deprecated since 14
 * @useinstead Demuxer_MediaKeySystemInfoCallback
 * @since 11
 */
typedef void (*DRM_MediaKeySystemInfoCallback)(DRM_MediaKeySystemInfo *mediaKeySystemInfo);

/**
 * @brief Defines a pointer to the callback function for {@link DRM_MediaKeySystemInfo}.
 * A demuxer instance is returned.
 * This callback function applies to the scenario where multiple demuxer instances are used.
 *
 * You need to call {@link OH_AVDemuxer_SetDemuxerMediaKeySystemInfoCallback} to set the callback function as a
 * callback. This callback function is recommended.
 *
 * @param demuxer Player OH_AVDemuxer.
 * @param mediaKeySystemInfo DRM information.
 * @since 12
 */
typedef void (*Demuxer_MediaKeySystemInfoCallback)(OH_AVDemuxer *demuxer, DRM_MediaKeySystemInfo *mediaKeySystemInfo);

/**
 * @brief Creates an OH_AVDemuxer instance based on a source instance.
 *
 * For details about how to create, destroy, and use a source instance, see {@link OH_AVSource}.
 *
 * @param source Pointer to an OH_AVSource instance.
 * @return Pointer to an OH_AVDemuxer instance.
 *     <br>If the operation fails, NULL is returned.
 *     <br>The possible causes of an operation failure are as follows:
 *     <br>1. The value of **source** is invalid (either nullptr or a pointer to a non-OH_AVSource instance).
 *     <br>2. The value of **source** does not point to an OH_AVSource instance.
 * @since 10
 */
OH_AVDemuxer *OH_AVDemuxer_CreateWithSource(OH_AVSource *source);

/**
 * @brief Destroys an OH_AVDemuxer instance and clears internal resources. An instance can be destroyed only once.
 *
 * The destroyed instance cannot be used until it is re-created. You are advised to set the pointer to NULL after the
 * instance is destroyed.
 *
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @return {@link AV_ERR_OK}: The operation is successful.
 *     <br>{@link AV_ERR_INVALID_VAL}: The value of **demuxer** is nullptr or does not point to a demuxer instance.
 * @since 10
 */
OH_AVErrCode OH_AVDemuxer_Destroy(OH_AVDemuxer *demuxer);

/**
 * @brief Selects a track from which the demuxer reads sample data.
 *
 * You can select multiple tracks by calling this API multiple times, with a different track index passed in each time.
 *
 * When {@link OH_AVDemuxer_ReadSample} is called, only the data in the selected track is read. If the same track is
 * selected multiple times, {@link AV_ERR_OK} is returned and the API call takes effect only once.
 *
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param trackIndex Index of the track.
 * @return {@link AV_ERR_OK}: The operation is successful.
 *     <br>{@link AV_ERR_INVALID_VAL}:
 *     <br>1. The value of **demuxer** is nullptr or does not point to a demuxer instance.
 *     <br>2. The track index is out of range.
 *     <br>3. Track reading is not supported.
 *     <br>{@link AV_ERR_OPERATE_NOT_PERMIT}: The demuxer is not correctly initialized.
 * @since 10
 */
OH_AVErrCode OH_AVDemuxer_SelectTrackByID(OH_AVDemuxer *demuxer, uint32_t trackIndex);

/**
 * @brief Deselects a track. The demuxer no longer reads sample data from a track after it is deselected.
 *
 * You can deselect multiple tracks by calling this API multiple times, with a different track index passed in each
 * time.
 * If the same track is deselected multiple times,
 * {@link AV_ERR_OK} is returned and the API call takes effect only once.
 *
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param trackIndex Index of the track.
 * @return {@link AV_ERR_OK}: The operation is successful.
 *     <br>{@link AV_ERR_INVALID_VAL}: The value of **demuxer** is nullptr or does not point to a demuxer instance.
 *     <br>{@link AV_ERR_OPERATE_NOT_PERMIT}: The demuxer is not correctly initialized.
 * @since 10
 */
OH_AVErrCode OH_AVDemuxer_UnselectTrackByID(OH_AVDemuxer *demuxer, uint32_t trackIndex);

/**
 * @brief Reads the sample and related information from the specified track.
 *
 * You must select a track before reading the sample. After this API is called, the demuxer automatically proceeds to
 * the next frame.
 *
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param trackIndex Index of the track from which the compressed frame is to be read.
 * @param sample Pointer to the OH_AVMemory instance for storing the compressed frame data.
 * @param info Pointer to the OH_AVCodecBufferAttr instance for storing the compressed frame information.
 * @return {@link AV_ERR_OK}: The operation is successful.
 *     <br>{@link AV_ERR_INVALID_VAL}:
 *     <br>1. The value of **demuxer** is nullptr or does not point to a demuxer instance.
 *     <br>2. The track index is out of range.
 *     <br>3. Track reading is not supported.
 *     <br>4. The value of **sample** is null.
 *     <br>5. The value of **info** is null.
 *     <br>{@link AV_ERR_OPERATE_NOT_PERMIT}:
 *     <br>1. The track with the specified index is not selected.
 *     <br>2. The demuxer is not correctly initialized.
 *     <br>{@link AV_ERR_NO_MEMORY}: The sample capacity is insufficient to store all frame data.
 *     <br>{@link AV_ERR_UNKNOWN}: Failed to read or parse the frame from the file.
 * @deprecated since 11
 * @useinstead OH_AVDemuxer_ReadSampleBuffer
 * @since 10
 */
OH_AVErrCode OH_AVDemuxer_ReadSample(OH_AVDemuxer *demuxer, uint32_t trackIndex,
    OH_AVMemory *sample, OH_AVCodecBufferAttr *info);

/**
 * @brief Reads the sample and related information from the specified track.
 *
 * You can use {@link OH_AVDemuxer_SelectTrackByID} to select a track before reading the sample. After this API is
 * called, the demuxer automatically proceeds to the next frame.
 *
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param trackIndex Index of the track from which the compressed frame is to be read.
 * @param sample Pointer to the OH_AVBuffer instance for storing the compressed frame data and related information.
 * @return {@link AV_ERR_OK}: The operation is successful.
 *     <br>{@link AV_ERR_INVALID_VAL}:
 *     <br>1. The value of **demuxer** is nullptr or does not point to a demuxer instance.
 *     <br>2. The value of **sample** is nullptr.
 *     <br>3. The track index is out of range.
 *     <br>4. The value of **sample** is null.
 *     <br>{@link AV_ERR_OPERATE_NOT_PERMIT}:
 *     <br>1. The track with the specified index is not selected.
 *     <br>2. The demuxer is not correctly initialized.
 *     <br>{@link AV_ERR_NO_MEMORY}: The sample capacity is insufficient to store all frame data.
 *     <br>{@link AV_ERR_UNKNOWN}: Failed to read or parse the frame from the file.
 * @since 11
 */
OH_AVErrCode OH_AVDemuxer_ReadSampleBuffer(OH_AVDemuxer *demuxer, uint32_t trackIndex,
    OH_AVBuffer *sample);

/**
 * @brief Seeks to the specified time for all the selected tracks based on a seek mode.
 *
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param millisecond Time to seek to, in milliseconds. The timestamp is relative to the start position of the file.
 * @param mode Seek mode.
 * @return {@link AV_ERR_OK}: The operation is successful.
 *     <br>{@link AV_ERR_INVALID_VAL}:
 *     <br>1. The value of **demuxer** is nullptr or does not point to a demuxer instance.
 *     <br>2. The value of **millisecond** is out of range.
 *     <br>{@link AV_ERR_OPERATE_NOT_PERMIT}:
 *     <br>1. The track with the specified index is not selected.
 *     <br>2. The demuxer is not correctly initialized.
 *     <br>3. The seek operation cannot be performed on the resource.
 *     <br>{@link AV_ERR_UNKNOWN}:
 *     <br>1. The seek operation fails.
 *     <br>2. **OH_AVSeekMode** is set to **SEEK_MODE_NEXT_SYNC** and there is no I-frame
 *     following the specified position.
 * @since 10
 */
OH_AVErrCode OH_AVDemuxer_SeekToTime(OH_AVDemuxer *demuxer, int64_t millisecond, OH_AVSeekMode mode);

/**
 * @brief Sets a callback for obtaining the media key system information.
 *
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param callback Callback function.
 * @return {@link AV_ERR_OK}: The operation is successful.
 *     <br>{@link AV_ERR_OPERATE_NOT_PERMIT}: The demuxer is not correctly initialized.
 *     <br>{@link AV_ERR_INVALID_VAL}: The value of **demuxer** is nullptr or does not point to a demuxer instance.
 * @deprecated since 14
 * @useinstead OH_AVDemuxer_SetDemuxerMediaKeySystemInfoCallback
 * @since 11
 */
OH_AVErrCode OH_AVDemuxer_SetMediaKeySystemInfoCallback(OH_AVDemuxer *demuxer,
    DRM_MediaKeySystemInfoCallback callback);

/**
 * @brief Sets a callback for obtaining the media key system information.
 *
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param callback Callback function.
 * @return {@link AV_ERR_OK}: The operation is successful.
 *     <br>{@link AV_ERR_OPERATE_NOT_PERMIT}: The demuxer is not correctly initialized.
 *     <br>{@link AV_ERR_INVALID_VAL}: The value of **demuxer** is nullptr or does not point to a demuxer instance.
 * @since 12
 */
OH_AVErrCode OH_AVDemuxer_SetDemuxerMediaKeySystemInfoCallback(OH_AVDemuxer *demuxer,
    Demuxer_MediaKeySystemInfoCallback callback);

/**
 * @brief Obtains the media key system information. The media key system information can be obtained only after
 * {@link Demuxer_MediaKeySystemInfoCallback} or {@link DRM_MediaKeySystemInfoCallback} is successfully invoked.
 *
 * @param demuxer Pointer to an OH_AVDemuxer instance.
 * @param mediaKeySystemInfo Pointer to the media key system information.
 * @return {@link AV_ERR_OK}: The operation is successful.
 *     <br>{@link AV_ERR_OPERATE_NOT_PERMIT}: The demuxer engine is not initialized or fails to be initialized.
 *     <br>{@link AV_ERR_INVALID_VAL}:
 *     <br>1. The value of **demuxer** is nullptr or does not point to a demuxer instance.
 *     <br>2. The value of **mediaKeySystemInfo** is nullptr.
 * @since 11
 */
OH_AVErrCode OH_AVDemuxer_GetMediaKeySystemInfo(OH_AVDemuxer *demuxer, DRM_MediaKeySystemInfo *mediaKeySystemInfo);

#ifdef __cplusplus
}
#endif

#endif // NATIVE_AVDEMUXER_H
/** @} */