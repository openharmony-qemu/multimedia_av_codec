/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef MEDIA_SOURCE_MOCK_H
#define MEDIA_SOURCE_MOCK_H

#include "gtest/gtest.h"
#include "source.h"

namespace OHOS {
namespace Media {
class MockMediaSource : public Plugins::MediaSource {
public:
    MockMediaSource(std::string uri)
        : Plugins::MediaSource(uri),
          uri_(std::move(uri)),
          type_(SourceType::SOURCE_TYPE_URI) {}

#ifndef OHOS_LITE
    MockMediaSource(std::shared_ptr<IMediaDataSource> dataSrc)
        : Plugins::MediaSource(dataSrc),
          type_(SourceType::SOURCE_TYPE_STREAM),
          dataSrc_(std::move(dataSrc)) {}
#endif
    MockMediaSource(std::string uri, std::map<std::string, std::string> header)
        : Plugins::MediaSource(uri, header),
          uri_(std::move(uri)),
          header_(std::move(header)) {}

    SourceType GetSourceType() const
    {
        return type_;
    }

    const std::string &GetSourceUri() const
    {
        return uri_;
    }

    const std::map<std::string, std::string> &GetSourceHeader() const
    {
        return header_;
    }
    void AddMediaStream(const std::shared_ptr<PlayMediaStream>& playMediaStream)
    {
        playMediaStreamVec_.push_back(playMediaStream);
    }
    MediaStreamList GetMediaStreamList() const
    {
        return playMediaStreamVec_;
    }

    void SetPlayStrategy(const std::shared_ptr<PlayStrategy>& playStrategy)
    {
        playStrategy_ = playStrategy;
    }

    std::shared_ptr<PlayStrategy> GetPlayStrategy() const
    {
        return playStrategy_;
    }

    void SetMimeType(const std::string& mimeType)
    {
        mimeType_ = mimeType;
    }

    std::string GetMimeType() const
    {
        return mimeType_;
    }

    void SetAppUid(int32_t appUid)
    {
        appUid_ = appUid;
    }

    int32_t GetAppUid()
    {
        return appUid_;
    }

    void SetSourceLoader(std::shared_ptr<IMediaSourceLoader> mediaSourceLoader)
    {
        sourceLoader_ = std::move(mediaSourceLoader);
    }

    std::shared_ptr<IMediaSourceLoader> GetSourceLoader() const
    {
        return sourceLoader_;
    }

    #ifndef OHOS_LITE
    std::shared_ptr<IMediaDataSource> GetDataSrc() const
    {
        return dataSrc_;
    }
    #endif
private:
    std::string uri_ {};
    std::string mimeType_ {};
    SourceType type_ {};
    std::map<std::string, std::string> header_ {};
    std::shared_ptr<PlayStrategy> playStrategy_ {};
    int32_t appUid_ {-1};
#ifndef OHOS_LITE
    std::shared_ptr<IMediaDataSource> dataSrc_ {};
#endif
    std::shared_ptr<IMediaSourceLoader> sourceLoader_ {};
    MediaStreamList playMediaStreamVec_;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_SOURCE_MOCK_H