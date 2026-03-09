/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "plugin/plugin_manager_v2.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "TypeFinder" };
}

namespace OHOS {
namespace Media {
using namespace Plugins;
namespace {

const int32_t WAIT_TIME = 5;
const uint32_t DEFAULT_SNIFF_SIZE = 4096 * 4;
constexpr int32_t MAX_TRY_TIMES = 5;

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
    MEDIA_LOG_D("In");
}

TypeFinder::~TypeFinder()
{
    MEDIA_LOG_D("In");
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

void TypeFinder::Init(std::string uri, uint64_t mediaDataSize,
    std::function<Status(int32_t, uint64_t, size_t)> checkRange,
    std::function<Status(int32_t, uint64_t, size_t, std::shared_ptr<Buffer>&, bool)> peekRange, int32_t streamId)
{
    streamID_ = streamId;
    mediaDataSize_ = mediaDataSize;
    checkRange_ = std::move(checkRange);
    peekRange_ = std::move(peekRange);
    sniffNeeded_ = IsSniffNeeded(uri);
    if (sniffNeeded_) {
        uri_.swap(uri);
        pluginName_.clear();
    }
}

uint64_t TypeFinder::GetSniffSize()
{
    return sniffSize_;
}

void TypeFinder::SetSniffSize(uint64_t sniffSize)
{
    sniffSize_ = sniffSize;
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
        MEDIA_LOG_E("Buffer empty: " PUBLIC_LOG_D32 ", expectedLen: " PUBLIC_LOG_ZU ", offset: "
            PUBLIC_LOG_D64, !buffer, expectedLen, offset);
        return Status::ERROR_INVALID_PARAMETER;
    }

    const int maxTryTimes = 3;
    int i = 0;
    while ((checkRange_(streamID_, offset, expectedLen) != Status::OK) && (i < maxTryTimes) &&
           !isInterruptNeeded_.load()) {
        i++;
        std::unique_lock<std::mutex> lock(mutex_);
        readCond_.wait_for(lock, std::chrono::milliseconds(WAIT_TIME), [&] { return isInterruptNeeded_.load(); });
    }
    FALSE_RETURN_V_MSG_E(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE,
        "ReadAt interrupt " PUBLIC_LOG_D32 " " PUBLIC_LOG_U64 " " PUBLIC_LOG_ZU, streamID_, offset, expectedLen);
    if (i == maxTryTimes) {
        MEDIA_LOG_E("ReadAt failed try 5 times");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    FALSE_LOG_MSG(peekRange_(streamID_, static_cast<uint64_t>(offset), expectedLen, buffer, true) == Status::OK,
        "PeekRange failed");
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
    std::string pluginName;
    auto dataSource = shared_from_this();
    std::vector<uint8_t> buff(DEFAULT_SNIFF_SIZE);
    auto buffer = std::make_shared<Buffer>();
    FALSE_RETURN_V_MSG_E(buffer != nullptr, "", "Alloc failed");
    auto bufData = buffer->WrapMemory(buff.data(), DEFAULT_SNIFF_SIZE, DEFAULT_SNIFF_SIZE);
    FALSE_RETURN_V_MSG_E(
        buffer->GetMemory() != nullptr, "", "Alloc failed, sniffSize " PUBLIC_LOG_U32, DEFAULT_SNIFF_SIZE);
    int32_t tryCnt = 0;
    Status ret = Status::OK;
    size_t getDataSize = 0;
    while (tryCnt < MAX_TRY_TIMES) {
        ret = dataSource->ReadAt(0, buffer, DEFAULT_SNIFF_SIZE);
        getDataSize = buffer->GetMemory()->GetSize();
        if (ret == Status::OK && getDataSize == DEFAULT_SNIFF_SIZE) {
            MEDIA_LOG_D("SniffMediaType ReadAt ok");
            break;
        }
        MEDIA_LOG_D("SniffMediaType ReadAt failed, tryCnt: " PUBLIC_LOG_D32 " ret " PUBLIC_LOG_D32
            " got size: " PUBLIC_LOG_ZU, tryCnt, ret, getDataSize);
        ++tryCnt;
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK && getDataSize > 0, "", "Not data for sniff " PUBLIC_LOG_ZU, getDataSize);
    pluginName = Plugins::PluginManagerV2::Instance().SnifferPlugin(PluginType::DEMUXER, dataSource);
    return pluginName;
}

bool TypeFinder::IsOffsetValid(int64_t offset) const
{
    return (mediaDataSize_ == 0) || (static_cast<int64_t>(mediaDataSize_) == -1) ||
        offset < static_cast<int64_t>(mediaDataSize_);
}

int32_t TypeFinder::GetStreamID()
{
    return streamID_;
}

void TypeFinder::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("TypeFinder OnInterrupted %{public}d", isInterruptNeeded);
    std::unique_lock<std::mutex> lock(mutex_);
    isInterruptNeeded_ = isInterruptNeeded;
    readCond_.notify_all();
}
} // namespace Media
} // namespace OHOS
