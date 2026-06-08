/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
 * @addtogroup CodecBase
 * @{
 *
 * @brief The CodecBase module provides variables, properties, and functions
 * for audio and video muxer, demuxer, and basic encoding and decoding functions.
 *
 * @since 9
 */
/**
 * @file native_avcodec_videobase.h
 *
 * @brief Declare the Native API used for basic video encoding and decoding functions, 
 * as well as video-specific configurations and parameters.
 *
 * @kit AVCodecKit
 * @library libnative_media_codecbase.so
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 26.0.0
 */

#ifndef NATIVE_AVCODEC_VIDEOBASE_H
#define NATIVE_AVCODEC_VIDEOBASE_H
#include <stdint.h>
#include "native_avformat.h"
#include "native_averrors.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Key for describing the top-coordinate (y) of a single ROI rectangle, value type is int32_t.
 *
 * The origin of the coordinate system is the top-left corner of the video.
 * The value range is [0, {@link OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM}).
 * This is a mandatory key used when configuring ROI parameters.
 *
 * @since 26.0.0
 */
extern const char *OH_MD_KEY_VIDEO_METADATA_ROI_TOP;

/**
 * @brief Key for describing the left-coordinate (x) of a single ROI rectangle, value type is int32_t.
 *
 * The origin of the coordinate system is the top-left corner of the video.
 * The value range is [0, {@link OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT}).
 * This is a mandatory key used when configuring ROI parameters.
 *
 * @since 26.0.0
 */
extern const char *OH_MD_KEY_VIDEO_METADATA_ROI_LEFT;

/**
 * @brief Key for describing the bottom-coordinate (y) of a single ROI rectangle, value type is int32_t.
 *
 * The origin of the coordinate system is the top-left corner of the video.
 * The value range is ({@link OH_MD_KEY_VIDEO_METADATA_ROI_TOP}, {@link OH_MD_KEY_VIDEO_HEIGHT}].
 * This is a mandatory key used when configuring ROI parameters.
 *
 * @since 26.0.0
 */
extern const char *OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM;

/**
 * @brief Key for describing the right-coordinate (x) of a single ROI rectangle, value type is int32_t.
 *
 * The origin of the coordinate system is the top-left corner of the video.
 * The value range is ({@link OH_MD_KEY_VIDEO_METADATA_ROI_LEFT}, {@link OH_MD_KEY_VIDEO_WIDTH}].
 * This is a mandatory key used when configuring ROI parameters. 
 *
 * @since 26.0.0
 */
extern const char *OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT;

/**
 * @brief Key for describing the quantization parameter offset (delta-qp) of a single ROI, value type is int32_t.
 *
 * The value range is [-51, 51].
 * This is an optional key used when configuring ROI parameters.
 *
 * @since 26.0.0
 */
extern const char *OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP;

/**
 * @brief Key for describing the semantic label (sem_label) of a single ROI, value type is int32_t.
 *
 * The value must correspond to {@link OH_VideoMetadataRoiSemanticLabel}.
 * This is an optional key used when configuring ROI parameters.
 *
 * @since 26.0.0
 */
extern const char *OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL;

/**
 * @brief The semantic labels for Region of Interest (ROI) in video encoding.
 *
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 26.0.0
 */
typedef enum OH_VideoMetadataRoiSemanticLabel {
    /**
     * Indicates an unspecified or unknown region.
     *
     * @since 26.0.0
     */
    VIDEO_METADATA_ROI_SEM_LABEL_OTHER = 0,
    /**
     * Indicates that the ROI contains a human face.
     *
     * @since 26.0.0
     */
    VIDEO_METADATA_ROI_SEM_LABEL_FACE = 1
} OH_VideoMetadataRoiSemanticLabel;

/**
 * @brief Formats the region of interest (ROI) configuration from an OH_AVFormat handle
 * into a string and appends it to the target string.
 *
 * This function extracts ROI properties (such as coordinates, delta quantization parameter,
 * and semantic label) from the provided format handle, constructs the standard ROI string
 * representation, and seamlessly appends it to the string pointed to by roiStrInOut.
 *
 * If *roiStrInOut is NULL, this function will allocate memory for a new string.
 * If *roiStrInOut is not NULL, this function will reallocate the memory to append
 * the new configuration safely.
 *
 * @note The caller takes ownership of the memory allocated for *roiStrInOut.
 * To prevent memory leaks, the caller is fully responsible for freeing the
 * final string using the standard free() function when it is no longer needed.
 *
 * @param roiStrInOut A double pointer to the target string. The pointer itself must not be NULL.
 * If *roiStrInOut is NULL, a new string is allocated.
 * @param format The OH_AVFormat handle containing the ROI parameters to be appended.
 * Must not be NULL.
 * @return Returns AV_ERR_OK if the string is successfully formatted and appended.
 * Returns AV_ERR_INVALID_VAL if the roiStrInOut pointer or format handle is NULL,
 * or if the format lacks required ROI keys.
 * Returns AV_ERR_NO_MEMORY if internal memory allocation or reallocation fails.
 * @since 26.0.0
 */
OH_AVErrCode OH_VideoMetadata_AppendRoiString(char **roiStrInOut, OH_AVFormat *format);

/**
 * @brief Pre-parses the ROI string to obtain the number of valid ROI regions contained within it.
 * * This interface is decoupled from specific backend capacity limits and accurately returns
 * the number of valid regions identified in the string based on syntax rules.
 *
 * @param roiStr The input ROI configuration string.
 * @param outCount [OUT] Returns the number of valid ROI regions parsed from the string.
 * @return AV_ERR_OK if the operation is successful; returns a specific error code otherwise.
 * @since 26.0.0
 */
OH_AVErrCode OH_VideoMetadata_GetRoiCount(const char *roiStr, uint32_t *outCount);

/**
 * @brief Parses the ROI string and populates the caller-provided OH_AVFormat array.
 *
 * The caller is responsible for providing a properly sized pointer array and explicitly
 * destroying the successfully created OH_AVFormat handles using OH_AVFormat_Destroy()
 * to prevent memory leaks.
 *
 * @param roiStr The input ROI configuration string.
 * @param outFormats [OUT] A pointer array allocated by the caller to receive the parsed OH_AVFormat handles.
 * @param maxCapacity [IN] Indicates the maximum physical capacity of the outFormats array to prevent
 * out-of-bounds writes.
 * @param outCount [OUT] Returns the actual number of ROIs successfully parsed and populated into the array.
 * @return AV_ERR_OK if the operation is successful; returns a specific error code otherwise.
 * @since 26.0.0
 */
OH_AVErrCode OH_VideoMetadata_ParseRoiString(const char *roiStr, OH_AVFormat **outFormats, uint32_t maxCapacity,
                                             uint32_t *outCount);

#ifdef __cplusplus
}
#endif
#endif // NATIVE_AVCODEC_VIDEOBASE_H
/** @} */