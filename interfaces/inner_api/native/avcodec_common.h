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
#ifndef MEDIA_AVCODEC_COMMOM_H
#define MEDIA_AVCODEC_COMMOM_H

#include <string>
#include <vector>
#include <map>
#include "av_common.h"
#include "buffer/avbuffer.h"
#include "meta/format.h"

namespace OHOS {
namespace MediaAVCodec {
using AVBuffer = OHOS::Media::AVBuffer;
using AVSharedMemory = OHOS::Media::AVSharedMemory;
using Format = OHOS::Media::Format;
/**
 * @brief Error type of AVCodec
 *
 * @since 3.1
 * @version 3.1
 */
enum AVCodecErrorType : int32_t {
    /* internal errors, error code passed by the errorCode, and definition see "AVCodecServiceErrCode" */
    AVCODEC_ERROR_INTERNAL,
    /* extend error start. The extension error code agreed upon by the plug-in and
       the application will be transparently transmitted by the service. */
    AVCODEC_ERROR_EXTEND_START = 0X10000,
};

enum class API_VERSION : int32_t {
    API_VERSION_10 = 10,
    API_VERSION_11 = 11
};

/**
 * @brief Flag of AVCodecBuffer.
 *
 * @since 3.1
 */
enum AVCodecBufferFlag : uint32_t {
    AVCODEC_BUFFER_FLAG_NONE = 0,
    /** This signals the end of stream. */
    AVCODEC_BUFFER_FLAG_EOS = 1 << 0,
    /** This indicates that the buffer contains the data for a sync frame. */
    AVCODEC_BUFFER_FLAG_SYNC_FRAME = 1 << 1,
    /** This indicates that the buffer only contains part of a frame. */
    AVCODEC_BUFFER_FLAG_PARTIAL_FRAME = 1 << 2,
    /** This indicated that the buffer contains codec specific data. */
    AVCODEC_BUFFER_FLAG_CODEC_DATA = 1 << 3,
    /** Flag is used to discard packets which are required to maintain valid decoder state but are not required
     * for output and should be dropped after decoding.
     * @since 12
     */
    AVCODEC_BUFFER_FLAG_DISCARD = 1 << 4,
    /** Flag is used to indicate packets that contain frames that can be discarded by the decoder,
     * I.e. Non-reference frames.
     * @since 12
     */
    AVCODEC_BUFFER_FLAG_DISPOSABLE = 1 << 5,
    /** Indicates that the frame is an extended discardable frame. It is not on the main reference path and
     * is referenced only by discardable frames or extended discardable frames. When subsequent frames on the branch
     * reference path are discarded by decoder, the frame can be further discarded.
     * @since 12
     */
    AVCODEC_BUFFER_FLAG_DISPOSABLE_EXT = 1 << 6,
};

struct AVCodecBufferInfo {
    /* The presentation timestamp in microseconds for the buffer */
    int64_t presentationTimeUs = 0;
    /* The amount of data (in bytes) in the buffer */
    int32_t size = 0;
    /* The start-offset of the data in the buffer */
    int32_t offset = 0;
};

class AVCodecCallback {
public:
    virtual ~AVCodecCallback() = default;
    /**
     * Called when an error occurred.
     *
     * @param errorType Error type. For details, see {@link AVCodecErrorType}.
     * @param errorCode Error code.
     * @since 3.1
     * @version 3.1
     */
    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;

    /**
     * Called when the output format has changed.
     *
     * @param format The new output format.
     * @since 3.1
     * @version 3.1
     */
    virtual void OnOutputFormatChanged(const Format &format) = 0;

    /**
     * Called when an input buffer becomes available.
     *
     * @param index The index of the available input buffer.
     * @param buffer A {@link AVSharedMemory} object for a input buffer index that contains the data.
     * @since 3.1
     * @version 4.0
     */
    virtual void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) = 0;

    /**
     * Called when an output buffer becomes available.
     *
     * @param index The index of the available output buffer.
     * @param info The info of the available output buffer. For details, see {@link AVCodecBufferInfo}
     * @param flag The flag of the available output buffer. For details, see {@link AVCodecBufferFlag}
     * @param buffer A {@link AVSharedMemory} object for a output buffer index that contains the data.
     * @since 3.1
     * @version 4.0
     */
    virtual void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                         std::shared_ptr<AVSharedMemory> buffer) = 0;
};

class AVDemuxerCallback {
public:
    virtual ~AVDemuxerCallback() = default;

    /**
     * Called when an drm info updated.
     *
     * @param drmInfo Drm Info.
     * @since 4.1
     * @version 4.1
     */
    virtual void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo) = 0;
};

class MediaCodecCallback {
public:
    virtual ~MediaCodecCallback() = default;
    /**
     * Called when an error occurred.
     *
     * @param errorType Error type. For details, see {@link AVCodecErrorType}.
     * @param errorCode Error code.
     * @since 4.1
     */
    virtual void OnError(AVCodecErrorType errorType, int32_t errorCode) = 0;

    /**
     * Called when the output format has changed.
     *
     * @param format The new output format.
     * @since 4.1
     */
    virtual void OnOutputFormatChanged(const Format &format) = 0;

    /**
     * Called when an input buffer becomes available.
     *
     * @param index The index of the available input buffer.
     * @param buffer A {@link AVBuffer} object for a input buffer index that contains the data.
     * @since 4.1
     */
    virtual void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) = 0;

    /**
     * Called when an output buffer becomes available.
     *
     * @param index The index of the available output buffer.
     * @param buffer A {@link AVBuffer} object for a output buffer index that contains the data.
     * @since 4.1
     */
    virtual void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) = 0;
};

class MediaCodecParameterCallback {
public:
    virtual ~MediaCodecParameterCallback() = default;
    /**
     * Called when an error occurred.
     *
     * @param index The index of the available input parmaeter.
     * @param parameter A {@link Format} object for a input parmaeter index that contains the data.
     * @since 5.0
     */
    virtual void OnInputParameterAvailable(uint32_t index, std::shared_ptr<Format> parameter) = 0;
};

class SurfaceBufferExtratDataKey {
public:
    /**
     * Key for timeStamp in surface's extraData, value type is int64
     */
    static constexpr std::string_view ED_KEY_TIME_STAMP = "timeStamp";

    /**
     * Key for endOfStream in surface's extraData, value type is bool
     */
    static constexpr std::string_view ED_KEY_END_OF_STREAM = "endOfStream";

private:
    SurfaceBufferExtratDataKey() = delete;
    ~SurfaceBufferExtratDataKey() = delete;
};

class AVSourceFormat {
public:
    static constexpr std::string_view SOURCE_TITLE         = "title";            // string, title
    static constexpr std::string_view SOURCE_ARTIST        = "artist";           // string, artist
    static constexpr std::string_view SOURCE_ALBUM         = "album";            // string, album
    static constexpr std::string_view SOURCE_ALBUM_ARTIST  = "album_artist";     // string, album artist
    static constexpr std::string_view SOURCE_DATE          = "date";             // string, media date,
                                                                                 // format: YYYY-MM-DD
    static constexpr std::string_view SOURCE_COMMENT       = "comment";          // string, comment
    static constexpr std::string_view SOURCE_GENRE         = "genre";            // string, genre
    static constexpr std::string_view SOURCE_COPYRIGHT     = "copyright";        // string, copyright
    static constexpr std::string_view SOURCE_LANGUAGE      = "language";         // string, language
    static constexpr std::string_view SOURCE_DESCRIPTION   = "description";      // string, description
    static constexpr std::string_view SOURCE_LYRICS        = "lyrics";           // string, cyrics

    static constexpr std::string_view SOURCE_FILE_TYPE     = "file_type";        // string, type
    static constexpr std::string_view SOURCE_HAS_VIDEO     = "has_video";        // bool, contain video tracks
    static constexpr std::string_view SOURCE_HAS_AUDIO     = "has_audio";        // bool, contain audio tracks
    static constexpr std::string_view SOURCE_HAS_SUBTITLE  = "has_subtitle";     // bool, contain subtitle tracks
    static constexpr std::string_view SOURCE_AUTHOR        = "author";           // string, autbor
    static constexpr std::string_view SOURCE_COMPOSER      = "composer";         // string, composer
private:
    AVSourceFormat() = delete;
    ~AVSourceFormat() = delete;
};

enum VideoBitStreamFormat {
    UNKNOWN = 0,
    AVCC,
    HVCC,
    ANNEXB
};

struct CUVVConfigBox {
    uint16_t cuva_version_map;
    uint16_t terminal_provide_code;
    uint16_t terminal_provide_oriented_code;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_COMMOM_H
