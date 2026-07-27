/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "http_source_plugin_stream_resource_parser.h"
#include "hls/m3u8.h"
#include "dash/dash_mpd_downloader.h"
#include "common/log.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

StreamResourceParser* StreamResourceParser::Parse(
    const uint8_t* data, size_t size, StreamProtocolType protocol, const std::string& rootUrl)
{
    auto parser = new StreamResourceParser();
    parser->protocol_ = protocol;
    parser->rootUrl_ = rootUrl;

    if (data == nullptr || size == 0) {
        return parser;
    }

    switch (protocol) {
        case StreamProtocolType::HLS:
            parser->ParseHls(data, size);
            break;
        case StreamProtocolType::DASH:
            parser->ParseDash(data, size);
            break;
        case StreamProtocolType::HTTP:
        default:
            parser->ParseHttp(data, size);
            break;
    }
    return parser;
}

bool StreamResourceParser::HasNext() const
{
    return currentIndex_ < resources_.size();
}

StreamResourceInfo StreamResourceParser::Next()
{
    if (currentIndex_ < resources_.size()) {
        return resources_[currentIndex_++];
    }
    return {"", false, StreamProtocolType::HTTP};
}

std::vector<StreamResourceInfo> StreamResourceParser::NextBatch(size_t batchSize)
{
    std::vector<StreamResourceInfo> batch;
    for (size_t i = 0; i < batchSize && HasNext(); ++i) {
        batch.push_back(Next());
    }
    return batch;
}

std::vector<StreamResourceInfo> StreamResourceParser::GetAll()
{
    std::vector<StreamResourceInfo> remaining(resources_.begin() + currentIndex_, resources_.end());
    currentIndex_ = resources_.size();
    return remaining;
}

size_t StreamResourceParser::GetTotalCount() const
{
    return resources_.size();
}

size_t StreamResourceParser::GetProcessedCount() const
{
    return currentIndex_;
}

void StreamResourceParser::ParseHls(const uint8_t* data, size_t size)
{
    std::string content(reinterpret_cast<const char*>(data), size);
    M3U8MasterPlaylist masterPlaylist(content, rootUrl_);
    masterPlaylist.StartParsing();

    if (!masterPlaylist.isSimple_) {
        for (const auto& variant : masterPlaylist.variants_) {
            if (!variant->uri_.empty()) {
                resources_.push_back({variant->uri_, true, StreamProtocolType::HLS});
            }
        }
    } else {        // 二级m3u8
        auto variant = masterPlaylist.variants_.front();
        if (!variant->m3u8_->fmp4HeaderUri_.empty()) {
            resources_.push_back({variant->m3u8_->fmp4HeaderUri_, false, StreamProtocolType::HLS});
        }
        for (const auto& file : variant->m3u8_->files_) {
            if (!file->uri_.empty()) {
                resources_.push_back({file->uri_, false, StreamProtocolType::HLS});
            }
        }
    }
}

void StreamResourceParser::ParseDash(const uint8_t* data, size_t size)
{
    auto urls = DashMpdDownloader::ParseMpdResources(data, size, rootUrl_);
    for (const auto& url : urls) {
        resources_.push_back({url, false, StreamProtocolType::DASH});
    }
}

void StreamResourceParser::ParseHttp(const uint8_t* data, size_t size)
{
    (void)data;
    (void)size;
    resources_.push_back({"", false, StreamProtocolType::HTTP});
}

} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS

extern "C" {
OHOS::Media::Plugins::HttpPlugin::StreamResourceParser* GetStreamResourceParser(
    const uint8_t* data, size_t size, OHOS::Media::Plugins::HttpPlugin::StreamProtocolType protocol,
    const std::string& rootUrl)
{
    return OHOS::Media::Plugins::HttpPlugin::StreamResourceParser::Parse(data, size, protocol, rootUrl);
}
}