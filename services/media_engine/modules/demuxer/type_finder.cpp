/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "TypeFinder"

#include "type_finder.h"

#include <algorithm>
#include "avcodec_trace.h"
#include "common/log.h"
#include "meta/any.h"
#include "osal/utils/util.h"
#include "plugin/plugin_info.h"

namespace OHOS {
namespace Media {
using namespace Plugins;
namespace {
std::string GetUriSuffix(const std::string& uri)
{
    std::string suffix {""};
    auto const pos = uri.find_last_of('.');
    if (pos != std::string::npos) {
        suffix = uri.substr(pos + 1);
    }
    return suffix;
}

bool IsPluginSupportedExtension(Plugins::PluginInfo& pluginInfo, const std::string& extension)
{
    if (pluginInfo.pluginType != Plugins::PluginType::DEMUXER) {
        return false;
    }
    bool rtv = false;
    auto info = pluginInfo.extra[PLUGIN_INFO_EXTRA_EXTENSIONS];
    if (info.HasValue() && Any::IsSameTypeWith<std::vector<std::string>>(info)) {
        for (const auto& ext : AnyCast<std::vector<std::string>&>(info)) {
            if (ext == extension) {
                rtv = true;
                break;
            }
        }
    }
    return rtv;
}

void ToLower(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) { return std::tolower(ch); });
}
} // namespace

TypeFinder::TypeFinder()
    : sniffNeeded_(true),
      uri_(),
      mediaDataSize_(0),
      pluginName_(),
      plugins_(),
      pluginRegistryChanged_(true),
      task_(nullptr),
      checkRange_(),
      peekRange_(),
      typeFound_()
{
    MEDIA_LOG_D("TypeFinder ctor called...");
}

TypeFinder::~TypeFinder()
{
    MEDIA_LOG_D("TypeFinder dtor called...");
    if (task_) {
        task_->Stop();
    }
}

bool TypeFinder::IsSniffNeeded(std::string uri)
{
    if (uri.empty() || pluginRegistryChanged_) {
        return true;
    }

    ToLower(uri);
    return uri_ != uri;
}

void TypeFinder::Init(std::string uri, uint64_t mediaDataSize, std::function<bool(uint64_t, size_t)> checkRange,
    std::function<bool(uint64_t, size_t, std::shared_ptr<Buffer>&)> peekRange)
{
    mediaDataSize_ = mediaDataSize;
    checkRange_ = std::move(checkRange);
    peekRange_ = std::move(peekRange);
    sniffNeeded_ = IsSniffNeeded(uri);
    if (sniffNeeded_) {
        uri_.swap(uri);
        pluginName_.clear();
        if (GetPlugins()) {
            SortPlugins(GetUriSuffix(uri_));
        } else {
            MEDIA_LOG_E("TypeFinder Init failed due to no demuxer plugins...");
        }
    }
}

/**
 * FindMediaType for seekable source, is a sync interface.
 * @return plugin names for the found media type.
 */
std::string TypeFinder::FindMediaType()
{
    MediaAVCodec::AVCodecTrace trace("TypeFinder::FindMediaType");
    if (sniffNeeded_) {
        pluginName_ = SniffMediaType();
        if (!pluginName_.empty()) {
            sniffNeeded_ = false;
        }
    }
    return pluginName_;
}

Status TypeFinder::ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen)
{
    if (!buffer || expectedLen == 0 || !IsOffsetValid(offset)) {
        MEDIA_LOG_E("ReadAt failed, buffer empty: " PUBLIC_LOG_D32 ", expectedLen: " PUBLIC_LOG_ZU ", offset: "
            PUBLIC_LOG_D64, !buffer, expectedLen, offset);
        return Status::ERROR_INVALID_PARAMETER;
    }

    const int maxTryTimes = 3;
    int i = 0;
    while (!checkRange_(offset, expectedLen) && (i++ < maxTryTimes)) {
        OSAL::SleepFor(5); // 5 ms
    }
    if (i == maxTryTimes) {
        MEDIA_LOG_E("ReadAt exceed maximum allowed try times and failed.");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    FALSE_LOG_MSG(peekRange_(static_cast<uint64_t>(offset), expectedLen, buffer), "peekRange failed.");
    return Status::OK;
}

Status TypeFinder::GetSize(uint64_t& size)
{
    size = mediaDataSize_;
    return (mediaDataSize_ > 0) ? Status::OK : Status::ERROR_UNKNOWN;
}

Plugins::Seekable TypeFinder::GetSeekable()
{
    return Plugins::Seekable::INVALID;
}

std::string TypeFinder::SniffMediaType()
{
    MEDIA_LOG_I("SniffMediaType begin.");
    std::string pluginName;
    int maxProb = 0;
    auto dataSource = shared_from_this();
    int cnt = 0;
    for (const auto& plugin : plugins_) {
        auto prob = Plugins::PluginManager::Instance().Sniffer(plugin->name, dataSource);
        ++cnt;
        if (prob > maxProb) {
            maxProb = prob;
            pluginName = plugin->name;
        }
    }
    MEDIA_LOG_I("SniffMediaType end, sniffed plugin num = " PUBLIC_LOG_D32, cnt);
    return pluginName;
}

std::string TypeFinder::GuessMediaType() const
{
    std::string pluginName;
    std::string uriSuffix = GetUriSuffix(uri_);
    if (uriSuffix.empty()) {
        return "";
    }
    for (const auto& pluginInfo : plugins_) {
        if (IsPluginSupportedExtension(*pluginInfo, uriSuffix)) {
            pluginName = pluginInfo->name;
            break;
        }
    }
    return pluginName;
}

bool TypeFinder::IsOffsetValid(int64_t offset) const
{
    return (mediaDataSize_ == 0) || (static_cast<int64_t>(mediaDataSize_) == -1) ||
        offset < static_cast<int64_t>(mediaDataSize_);
}

bool TypeFinder::GetPlugins()
{
    MEDIA_LOG_I("TypeFinder::GetPlugins : " PUBLIC_LOG_D32 ", empty: " PUBLIC_LOG_D32,
        (pluginRegistryChanged_ == true), plugins_.empty());
    if (pluginRegistryChanged_) {
        pluginRegistryChanged_ = false;
        auto pluginNames = Plugins::PluginManager::Instance().ListPlugins(Plugins::PluginType::DEMUXER);
        for (auto& pluginName : pluginNames) {
            auto pluginInfo
                = Plugins::PluginManager::Instance().GetPluginInfo(Plugins::PluginType::DEMUXER, pluginName);
            if (!pluginInfo) {
                MEDIA_LOG_E("GetPlugins failed for plugin: " PUBLIC_LOG_S, pluginName.c_str());
                continue;
            }
            plugins_.emplace_back(std::move(pluginInfo));
        }
    }
    return !plugins_.empty();
}

void TypeFinder::SortPlugins(const std::string& uriSuffix)
{
    if (uriSuffix.empty()) {
        return;
    }
    std::stable_sort(
        plugins_.begin(), plugins_.end(),
        [&uriSuffix](const std::shared_ptr<Plugins::PluginInfo>& lhs,
            const std::shared_ptr<Plugins::PluginInfo>& rhs) {
            if (IsPluginSupportedExtension(*lhs, uriSuffix)) {
                return (lhs->rank >= rhs->rank) || !IsPluginSupportedExtension(*rhs, uriSuffix);
            } else {
                return (lhs->rank >= rhs->rank) && !IsPluginSupportedExtension(*rhs, uriSuffix);
            }
        });
}
} // namespace Media
} // namespace OHOS
