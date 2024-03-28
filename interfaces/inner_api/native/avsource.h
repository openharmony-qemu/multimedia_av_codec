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

#ifndef MEDIA_AVCODEC_AVSOURCE_H
#define MEDIA_AVCODEC_AVSOURCE_H

#include <vector>
#include <memory>
#include <string>
#include "meta/format.h"
#include "media_demuxer.h"

namespace OHOS {
namespace MediaAVCodec {
class AVSource {
public:
    virtual ~AVSource() = default;

    /**
     * @brief Get the format info of source.
     * @param format The Format handle pointer to get format info.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 4.0
     */
    virtual int32_t GetSourceFormat(OHOS::Media::Format &format) = 0;

    /**
     * @brief Gets the parameters of the source.
     * @param format The Format handle pointer to get format info.
     * @param trackIndex The track index to get format.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 4.0
     */
    virtual int32_t GetTrackFormat(OHOS::Media::Format &format, uint32_t trackIndex) = 0;

    /**
     * @brief Gets the user meta for media.
     * @param format The Format handle pointer to get format info.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 5.0
     */
    virtual int32_t GetUserMeta(OHOS::Media::Format &format) = 0;

    std::string sourceUri;
    std::shared_ptr<Media::MediaDemuxer> demuxerEngine = nullptr;
};

class __attribute__((visibility("default"))) AVSourceFactory {
public:
#ifdef UNSUPPORT_SOURCE
    static std::shared_ptr<AVSource> CreateWithURI(const std::string &uri)
    {
        (void)uri;
        return nullptr;
    }

    static std::shared_ptr<AVSource> CreateWithFD(int32_t fd, int64_t offset, int64_t size)
    {
        (void)uri;
        return nullptr;
    }

    static std::shared_ptr<AVSource> CreateWithDataSource(const std::shared_ptr<Media::IMediaDataSource> &dataSource)
    {
        (void)uri;
        return nullptr;
    }

#else
    /**
     * @brief Instantiate the preferred source of the uri.
     * @param uri The file's uri.
     * @return Returns the preferred source.
     * @since 4.0
     */
    static std::shared_ptr<AVSource> CreateWithURI(const std::string &uri);

    /**
     * @brief Instantiate the preferred source of the fd.
     * @param fd The fileDescriptor data source.
     * @param offset The offset into the file to start reading.
     * @param size the length in bytes to read.
     * @return Returns the preferred source.
     * @since 4.0
     */
    static std::shared_ptr<AVSource> CreateWithFD(int32_t fd, int64_t offset, int64_t size);

    static std::shared_ptr<AVSource> CreateWithDataSource(const std::shared_ptr<Media::IMediaDataSource> &dataSource);

#endif
private:
    AVSourceFactory() = default;
    ~AVSourceFactory() = default;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_AVSOURCE_H

