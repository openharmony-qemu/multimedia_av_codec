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
#ifndef MEDIA_AVCODEC_AV_COMMOM_H
#define MEDIA_AVCODEC_AV_COMMOM_H

#include <vector>
#include <string>
#include "meta/format.h"
#include "buffer/avsharedmemory.h"

namespace OHOS {
namespace MediaAVCodec {
using Format = Media::Format;
using AVSharedMemory = Media::AVSharedMemory;
/**
 * @brief Media type
 *
 * @since 3.1
 * @version 3.1
 */
enum MediaType : int32_t {
    /**
     * track is audio.
     */
    MEDIA_TYPE_AUD = 0,
    /**
     * track is video.
     */
    MEDIA_TYPE_VID = 1,
    /**
     * track is subtitle.
     */
    MEDIA_TYPE_SUBTITLE = 2,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 3.1
 */
enum class VideoPixelFormat : int32_t {
    UNKNOWN = -1,
    YUV420P = 0,
    /**
     * yuv 420 planar.
     */
    YUVI420 = 1,
    /**
     *  NV12. yuv 420 semiplanar.
     */
    NV12 = 2,
    /**
     *  NV21. yvu 420 semiplanar.
     */
    NV21 = 3,
    /**
     * format from surface.
     */
    SURFACE_FORMAT = 4,
    /**
     * RGBA.
     */
    RGBA = 5,
};

/**
 * @brief the struct of geolocation
 *
 * @param latitude float: latitude in degrees. Its value must be in the range [-90, 90].
 * @param longitude float: longitude in degrees. Its value must be in the range [-180, 180].
 * @since  3.1
 * @version 3.1
 */
struct Location {
    float latitude = 0;
    float longitude = 0;
};

/**
 * @brief Enumerates the seek mode.
 */
enum AVSeekMode : uint8_t {
    /* seek to sync sample after the time */
    SEEK_MODE_NEXT_SYNC = 0,
    /* seek to sync sample before the time */
    SEEK_MODE_PREVIOUS_SYNC,
    /* seek to sync sample closest to time */
    SEEK_MODE_CLOSEST_SYNC,
};

/**
 * @brief Enumerates the video rotation.
 *
 * @since 3.2
 * @version 3.2
 */
enum VideoRotation : uint32_t {
    /**
     * Video without rotation
     */
    VIDEO_ROTATION_0 = 0,
    /**
     * Video rotated 90 degrees
     */
    VIDEO_ROTATION_90 = 90,
    /**
     * Video rotated 180 degrees
     */
    VIDEO_ROTATION_180 = 180,
    /**
     * Video rotated 270 degrees
     */
    VIDEO_ROTATION_270 = 270,
};

/**
 * @brief Enumerates the state change reason.
 *
 * @since 3.2
 * @version 3.2
 */
enum StateChangeReason {
    /**
     * audio/video state change by user
     */
    USER = 1,
    /**
     * audio/video state change by system
     */
    BACKGROUND = 2,
};

/**
 * @brief Enumerates the output format.
 *
 * @since 10
 * @version 4.0
 */
enum OutputFormat : uint32_t {
    /**
     * output format default mp4
    */
    OUTPUT_FORMAT_DEFAULT = 0,
    /**
     * output format mp4
    */
    OUTPUT_FORMAT_MPEG_4 = 2,
    /**
     * output format m4a
    */
    OUTPUT_FORMAT_M4A = 6,
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_AV_COMMOM_H