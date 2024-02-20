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

#define HST_LOG_TAG "Source"

#include "avcodec_trace.h"
#include "cpp_ext/type_traits_ext.h"
#include "common/log.h"
#include "osal/utils/util.h"
#include "common/media_source.h"
#include "source.h"

namespace OHOS {
namespace Media {
using namespace Plugins;
const size_t DEFAULT_READ_SIZE = 4096;

const int32_t READ_LOOP_RETRY_TIMES = 50;

static std::map<std::string, ProtocolType> g_protocolStringToType = {
    {"http", ProtocolType::HTTP},
    {"https", ProtocolType::HTTPS},
    {"file", ProtocolType::FILE},
    {"stream", ProtocolType::STREAM},
    {"fd", ProtocolType::FD}
};

Source::Source()
    : taskPtr_(nullptr),
      protocol_(),
      uri_(),
      seekable_(Seekable::INVALID),
      position_(0),
      plugin_(nullptr),
      isPluginReady_(false),
      isAboveWaterline_(false),
      pushData_(nullptr),
      mediaDemuxerCallback_(std::make_shared<CallbackImpl>())
{
    MEDIA_LOG_D("Source called");
}

Source::~Source()
{
    MEDIA_LOG_D("~Source called");
    if (plugin_) {
        plugin_->Deinit();
    }
    if (taskPtr_) {
        taskPtr_->Stop();
    }
}

void Source::SetCallback(Callback* callback)
{
    MEDIA_LOG_I("SetCallback entered.");
    if (callback == nullptr) {
        MEDIA_LOG_E("callback is nullptr");
        return;
    }
    if (mediaDemuxerCallback_ == nullptr) {
        MEDIA_LOG_E("mediaDemuxerCallback is nullptr");
        return;
    }
    mediaDemuxerCallback_->SetCallbackWrap(callback);
}

void Source::ClearData()
{
    protocol_.clear();
    uri_.clear();
    seekable_ = Seekable::INVALID;
    position_ = 0;
    mediaOffset_ = 0;
    isPluginReady_ = false;
    isAboveWaterline_ = false;
    isHls_ = false;
}

Status Source::SetSource(const std::shared_ptr<MediaSource>& source)
{
    MediaAVCodec::AVCodecTrace trace("Source::SetSource");
    MEDIA_LOG_I("SetSource enter.");
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "SetSource Invalid source");

    ClearData();
    Status ret = FindPlugin(source);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "SetSource FindPlugin failed");

    ret = InitPlugin(source);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "SetSource InitPlugin failed");

    if (plugin_ != nullptr) {
        isHls_ = plugin_->IsSeekToTimeSupported();
    }
    MEDIA_LOG_I("SetSource isHls: " PUBLIC_LOG_D32, isHls_);

    ActivateMode();

    MEDIA_LOG_I("SetSource exit.");
    return Status::OK;
}

Status Source::SetPushData(const std::shared_ptr<PushDataImpl>& data)
{
    pushData_ = data;
    return Status::OK;
}

Status Source::InitPlugin(const std::shared_ptr<MediaSource>& source)
{
    MediaAVCodec::AVCodecTrace trace("Source::InitPlugin");
    MEDIA_LOG_I("InitPlugin enter");
    FALSE_RETURN_V_MSG_E(plugin_ != nullptr, Status::ERROR_INVALID_OPERATION, "InitPlugin, Source plugin is nullptr");

    Status ret = plugin_->Init();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "InitPlugin failed");

    plugin_->SetCallback(this);
    ret = plugin_->SetSource(source);

    MEDIA_LOG_I("InitPlugin exit");
    return ret;
}

Status Source::Prepare()
{
    MEDIA_LOG_I("Prepare entered.");
    if (plugin_ == nullptr) {
        return Status::OK;
    }
    Status ret = plugin_->Prepare();
    if (ret == Status::OK) {
        MEDIA_LOG_D("media source send EVENT_READY");
    } else if (ret == Status::ERROR_DELAY_READY) {
        if (isAboveWaterline_) {
            MEDIA_LOG_D("media source send EVENT_READY");
            isPluginReady_ = false;
            isAboveWaterline_ = false;
        }
    }
    return ret;
}

bool Source::IsSeekToTimeSupported()
{
    return isHls_;
}

Status Source::Start()
{
    MEDIA_LOG_I("Start entered.");
    return plugin_ ? plugin_->Start() : Status::ERROR_INVALID_OPERATION;
}

Status Source::PullData(uint64_t offset, int64_t seekTime, size_t size, std::shared_ptr<Plugins::Buffer>& data)
{
    MEDIA_LOG_DD("IN, offset: " PUBLIC_LOG_U64 ", size: " PUBLIC_LOG_ZU ", seekTime: " PUBLIC_LOG_D64
        ", position: " PUBLIC_LOG_U64, offset, size, seekTime, position_);
    if (!plugin_) {
        return Status::ERROR_INVALID_OPERATION;
    }
    Status err;
    auto readSize = size;
    if (seekable_ != Seekable::SEEKABLE) {
        err = plugin_->Read(data, offset, readSize);
        FALSE_LOG_MSG(err == Status::OK, "Unseekable, plugin read failed.");
        return err;
    }
    if (isHls_) {
        err = plugin_->Read(data, offset, readSize);
        FALSE_LOG_MSG(err == Status::OK, "hls, plugin read failed.");
        return err;
    }

    uint64_t totalSize = 0;
    if ((plugin_->GetSize(totalSize) == Status::OK) && (totalSize != 0)) {
        if (offset >= totalSize) {
            MEDIA_LOG_W("Offset: " PUBLIC_LOG_U64 " is larger than totalSize: " PUBLIC_LOG_U64, offset, totalSize);
            return Status::END_OF_STREAM;
        }
        if ((offset + readSize) > totalSize) {
            readSize = totalSize - offset;
        }
        if (data->GetMemory() != nullptr) {
            auto realSize = data->GetMemory()->GetCapacity();
            readSize = (readSize > realSize) ? realSize : readSize;
        }
        MEDIA_LOG_DD("TotalSize_: " PUBLIC_LOG_U64, totalSize);
    }
    if (position_ != offset) {
        err = plugin_->SeekTo(offset);
        FALSE_RETURN_V_MSG_E(err == Status::OK, err, "Seek to " PUBLIC_LOG_U64 " fail", offset);
        position_ = offset;
    }

    err = plugin_->Read(data, offset, readSize);
    if (err == Status::OK) {
        position_ += data->GetMemory()->GetSize();
    }
    return err;
}

Status Source::GetBitRates(std::vector<uint32_t>& bitRates)
{
    MEDIA_LOG_I("GetBitRates");
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("GetBitRates failed, plugin_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->GetBitRates(bitRates);
}

Status Source::SelectBitRate(uint32_t bitRate)
{
    MEDIA_LOG_I("SelectBitRate");
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("SelectBitRate failed, plugin_ is nullptr");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->SelectBitRate(bitRate);
}

Status Source::SeekToTime(int64_t seekTime)
{
    int64_t timeNs;
    if (Plugins::Ms2HstTime(seekTime, timeNs)) {
        return plugin_->SeekToTime(timeNs);
    } else {
        return Status::ERROR_INVALID_PARAMETER;
    }
}

bool Source::IsNeedPreDownload()
{
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("IsNeedPreDownload failed, plugin_ is nullptr");
        return false;
    }
    return plugin_->IsNeedPreDownload();
}

Status Source::Stop()
{
    MEDIA_LOG_I("Stop entered.");
    mediaOffset_ = 0;
    seekable_ = Seekable::INVALID;
    protocol_.clear();
    uri_.clear();
    return plugin_->Stop();
}

Status Source::Pause()
{
    MEDIA_LOG_I("Pause entered.");
    mediaOffset_ = 0;
    if (taskPtr_) {
        taskPtr_->Stop();
    }
    return Status::OK;
}

Status Source::Resume()
{
    MEDIA_LOG_I("Resume entered.");
    if (taskPtr_) {
        taskPtr_->Start();
    }
    return Status::OK;
}

Status Source::SetReadBlockingFlag(bool isReadBlockingAllowed)
{
    MEDIA_LOG_D("SetReadBlockingFlag entered, IsReadBlockingAllowed %{public}d", isReadBlockingAllowed);
    FALSE_RETURN_V(plugin_ != nullptr, Status::OK);
    return plugin_->SetReadBlockingFlag(isReadBlockingAllowed);
}

void Source::OnEvent(const Plugins::PluginEvent& event)
{
    MEDIA_LOG_D("OnEvent");
    if (event.type == PluginEventType::ABOVE_LOW_WATERLINE) {
        if (isPluginReady_ && isAboveWaterline_) {
            isAboveWaterline_ = false;
            isPluginReady_ = false;
        }
    } else if (event.type == PluginEventType::CLIENT_ERROR || event.type == PluginEventType::SERVER_ERROR) {
        MEDIA_LOG_I("Error happened, need notify client by OnEvent");
        if (mediaDemuxerCallback_ != nullptr) {
            mediaDemuxerCallback_->OnEvent(event);
        }
    } else if (event.type == PluginEventType::SOURCE_DRM_INFO_UPDATE) {
        MEDIA_LOG_I("Drminfo updates from source");
        if (mediaDemuxerCallback_ != nullptr) {
            mediaDemuxerCallback_->OnEvent(event);
        }
    }
}

void Source::ActivateMode()
{
    MediaAVCodec::AVCodecTrace trace("Source::ActivateMode");
    MEDIA_LOG_I("ActivateMode enter");
    int32_t retry {0};
    do {
        if (plugin_) {
            seekable_ = plugin_->GetSeekable();
        }
        retry++;
        if (seekable_ == Seekable::INVALID) {
            if (retry >= 20) { // 20 means retry times
                break;
            }
            OSAL::SleepFor(10); // 10 means sleep time pre retry
        }
    } while (seekable_ == Seekable::INVALID);

    FALSE_LOG(seekable_ != Seekable::INVALID);
    if (seekable_ == Seekable::UNSEEKABLE) {
        if (taskPtr_ == nullptr) {
            taskPtr_ = std::make_shared<Task>("DataReader");
            taskPtr_->RegisterJob([this] { ReadLoop(); });
        }
        MEDIA_LOG_I("ActivateMode Source task start");
        taskPtr_->Start();
    }
    MEDIA_LOG_I("ActivateMode exit");
}

Plugins::Seekable Source::GetSeekable()
{
    return plugin_->GetSeekable();
}

std::string Source::GetUriSuffix(const std::string& uri)
{
    MEDIA_LOG_D("IN");
    std::string suffix;
    auto const pos = uri.find_last_of('.');
    if (pos != std::string::npos) {
        suffix = uri.substr(pos + 1);
    }
    return suffix;
}

void Source::ReadLoop()
{
    std::shared_ptr<Buffer> data = std::make_shared<Buffer>();
    Status err = plugin_->Read(data, -1, DEFAULT_READ_SIZE);
    if (err == Status::END_OF_STREAM) {
        MEDIA_LOG_I("ReadLoop current is end of stream");
        if (taskPtr_) {
            taskPtr_->StopAsync();
        }
        data->flag |= BUFFER_FLAG_EOS;
        pushData_->PushData(data, mediaOffset_);
        return;
    }
    if (err != Status::OK) {
        MEDIA_LOG_E("Read data failed (" PUBLIC_LOG_D32 ")", err);
        return;
    }
    auto memory = data->GetMemory();
    auto size = memory->GetSize();
    if (size <= 0) {
        if (retryTimes_ >= READ_LOOP_RETRY_TIMES) {
            MEDIA_LOG_E("ReadLoop retry time reach to max times, retryTimes %{public}d", retryTimes_);
            if (taskPtr_) {
                taskPtr_->StopAsync();
            }
            retryTimes_ = 0;
            data->flag |= BUFFER_FLAG_EOS;
            pushData_->PushData(data, mediaOffset_);
        } else {
            retryTimes_++;
            OSAL::SleepFor(1); // Read data failure, sleep 1ms then retry
            return;
        }
    } else {
        retryTimes_ = 0; // Read data success, reset retry times
    }
    if (data->flag & BUFFER_FLAG_EOS) {
        if (taskPtr_) {
            MEDIA_LOG_I("ReadLoop eos buffer, stop task");
            taskPtr_->StopAsync();
        }
    }

    MEDIA_LOG_D("Read data mediaOffset_: " PUBLIC_LOG_D64, mediaOffset_ + size);
    pushData_->PushData(data, mediaOffset_);
    mediaOffset_ += size;
}

bool Source::GetProtocolByUri()
{
    auto ret = true;
    auto const pos = uri_.find("://");
    if (pos != std::string::npos) {
        auto prefix = uri_.substr(0, pos);
        protocol_.append(prefix);
    } else {
        protocol_.append("file");
        std::string fullPath;
        ret = OSAL::ConvertFullPath(uri_, fullPath); // convert path to full path
        if (ret && !fullPath.empty()) {
            uri_ = fullPath;
        }
    }
    return ret;
}

bool Source::ParseProtocol(const std::shared_ptr<MediaSource>& source)
{
    bool ret = true;
    SourceType srcType = source->GetSourceType();
    MEDIA_LOG_D("sourceType = " PUBLIC_LOG_D32, CppExt::to_underlying(srcType));
    if (srcType == SourceType::SOURCE_TYPE_URI) {
        uri_ = source->GetSourceUri();
        ret = GetProtocolByUri();
    } else if (srcType == SourceType::SOURCE_TYPE_FD) {
        protocol_.append("fd");
        uri_ = source->GetSourceUri();
    } else if (srcType == SourceType::SOURCE_TYPE_STREAM) {
        protocol_.append("stream");
        uri_.append("stream://");
    }
    MEDIA_LOG_I("protocol: " PUBLIC_LOG_S ", uri: %{private}s", protocol_.c_str(), uri_.c_str());
    return ret;
}

Status Source::CreatePlugin(const std::shared_ptr<PluginInfo>& info, const std::string& name,
    PluginManager& manager)
{
    MediaAVCodec::AVCodecTrace trace("Source::CreatePlugin");
    if ((plugin_ != nullptr) && (pluginInfo_ != nullptr)) {
        if (info->name == pluginInfo_->name && plugin_->Reset() == Status::OK) {
            MEDIA_LOG_I("Reuse last plugin: " PUBLIC_LOG_S, name.c_str());
            return Status::OK;
        }
        if (plugin_->Deinit() != Status::OK) {
            MEDIA_LOG_E("Deinit last plugin: " PUBLIC_LOG_S " error", pluginInfo_->name.c_str());
        }
    }
    plugin_ = std::static_pointer_cast<SourcePlugin>(manager.CreatePlugin(name, PluginType::SOURCE));
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("PluginManager CreatePlugin " PUBLIC_LOG_S " fail", name.c_str());
        return Status::OK;
    }
    pluginInfo_ = info;
    MEDIA_LOG_I("Create new plugin: \"" PUBLIC_LOG_S "\" success", pluginInfo_->name.c_str());
    return Status::OK;
}

Status Source::FindPlugin(const std::shared_ptr<MediaSource>& source)
{
    MediaAVCodec::AVCodecTrace trace("Source::FindPlugin");
    if (!ParseProtocol(source)) {
        MEDIA_LOG_E("Invalid source!");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (protocol_.empty()) {
        MEDIA_LOG_E("protocol_ is empty");
        return Status::ERROR_INVALID_PARAMETER;
    }
    PluginManager& pluginManager = PluginManager::Instance();
    auto nameList = pluginManager.ListPlugins(PluginType::SOURCE);
    for (const std::string& name : nameList) {
        std::shared_ptr<PluginInfo> info = pluginManager.GetPluginInfo(PluginType::SOURCE, name);
        if (info == nullptr) {
            MEDIA_LOG_W("info is null.");
            continue;
        }
        MEDIA_LOG_I("name: " PUBLIC_LOG_S ", info->name: " PUBLIC_LOG_S, name.c_str(), info->name.c_str());
        auto val = info->extra[PLUGIN_INFO_EXTRA_PROTOCOL];
        if (Any::IsSameTypeWith<std::vector<ProtocolType>>(val)) {
            MEDIA_LOG_I("supportProtocol:" PUBLIC_LOG_S " CreatePlugin " PUBLIC_LOG_S,
                            protocol_.c_str(), name.c_str());
            auto supportProtocols = AnyCast<std::vector<ProtocolType>>(val);
            bool result = std::any_of(supportProtocols.begin(), supportProtocols.end(),
                [&](const auto& supportProtocol) {
                return supportProtocol == g_protocolStringToType[protocol_] && CreatePlugin(info,
                    name, pluginManager) == Status::OK;
            });
            if (result) {
                MEDIA_LOG_I("supportProtocol:" PUBLIC_LOG_S " CreatePlugin " PUBLIC_LOG_S " success",
                    protocol_.c_str(), name.c_str());
                return Status::OK;
            }
        }
    }
    MEDIA_LOG_E("Cannot find any plugin");
    return Status::ERROR_UNSUPPORTED_FORMAT;
}

int64_t Source::GetDuration()
{
    FALSE_RETURN_V_MSG_W(isHls_, Plugins::HST_TIME_NONE, "Source GetDuration return -1 for isHls false.");
    int64_t duration;
    Status ret = plugin_->GetDuration(duration);
    FALSE_RETURN_V_MSG_W(ret == Status::OK, Plugins::HST_TIME_NONE, "Source GetDuration from source plugin failed.");
    return duration;
}

Status Source::GetSize(uint64_t &fileSize)
{
    FALSE_RETURN_V_MSG_W(plugin_ != nullptr, Status::ERROR_INVALID_OPERATION, "GetSize Source plugin is nullptr!");
    return plugin_->GetSize(fileSize);
}
} // namespace Media
} // namespace OHOS