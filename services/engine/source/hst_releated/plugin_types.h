/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_PLUGIN_TYPES_H
#define HISTREAMER_PLUGIN_TYPES_H

#include <cstdint> // NOLINT: using int32_t in this file

namespace OHOS {
namespace MediaAVCodec {
namespace Plugin {
/**
 * @enum Plugin running state.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct State : int32_t {
    CREATED = 0,     ///< Indicates the status of the plugin when it is constructed.
                     ///< The plug-in will not be restored in the entire life cycle.
    INITIALIZED = 1, ///< Plugin global resource initialization completion status.
    PREPARED = 2,    ///< Status of parameters required for plugin running.
    RUNNING = 3,     ///< The system enters the running state after call start().
    PAUSED = 4,      ///< Plugin temporarily stops processing data. This state is optional.
    DESTROYED = -1,  ///< Plugin destruction state. In this state, all resources are released.
    INVALID = -2,    ///< An error occurs in any state and the plugin enters the invalid state.
};

/**
 * @enum Enumerates types of Seek Mode.
 *
 * @brief Seek modes, Options that SeekTo() behaviour.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct SeekMode : uint32_t {
    SEEK_NEXT_SYNC = 0,     ///> sync to keyframes after the time point.
    SEEK_PREVIOUS_SYNC,     ///> sync to keyframes before the time point.
    SEEK_CLOSEST_SYNC,      ///> sync to closest keyframes.
    SEEK_CLOSEST,           ///> seek to frames closest the time point.
};

/**
 * @enum Seekable Status.
 *
 * @since 1.0
 * @version 1.0
 */
enum class Seekable : int32_t {
    INVALID = -1,
    UNSEEKABLE = 0,
    SEEKABLE = 1
};

enum struct CodecMode {
    HARDWARE, ///<  HARDWARE CODEC
    SOFTWARE, ///<  SOFTWARE CODEC
};

/**
 * @enum Plugin Type.
 *
 * @since 1.0
 * @version 1.0
 */
enum struct PluginType : int32_t {
    INVALID_TYPE = -1, ///< Invalid plugin
    SOURCE = 1,        ///< reference SourcePlugin
    DEMUXER,           ///< reference DemuxerPlugin
    AUDIO_DECODER,     ///< reference CodecPlugin
    AUDIO_ENCODER,     ///< reference CodecPlugin
    VIDEO_DECODER,     ///< reference CodecPlugin
    VIDEO_ENCODER,     ///< reference CodecPlugin
    AUDIO_SINK,        ///< reference AudioSinkPlugin
    VIDEO_SINK,        ///< reference VideoSinkPlugin
    MUXER,             ///< reference MuxerPlugin
    OUTPUT_SINK,       ///< reference OutputSinkPlugin
};

/*
 * @brief Audio RenderInfo, default ContentType::CONTENT_TYPE_UNKNOWN(0) and StreamUsage::STREAM_USAGE_UNKNOWN(0)
 *        combined into AudioStreamType::STREAM_MUSIC.
 */
struct AudioRenderInfo {
    int32_t contentType {0};
    int32_t streamUsage {0};
    int32_t rendererFlags {0};
};

enum AudioInterruptMode {
    SHARE_MODE,
    INDEPENDENT_MODE
};

enum class VideoScaleType {
    VIDEO_SCALE_TYPE_FIT,
    VIDEO_SCALE_TYPE_FIT_CROP,
};
} // namespace Plugin
} // namespace MediaAVCodec
} // namespace OHOS
#endif // HISTREAMER_PLUGIN_TYPES_H
