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

#include "cpp_ext/type_traits_ext.h"
#include "common/log.h"
#include "osal/utils/util.h"
#include "common/media_source.h"
#include "source.h"

namespace OHOS {
namespace Media {
using namespace Plugins;
const size_t DEFAULT_READ_SIZE = 4096;

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
      pushData_(nullptr)
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

void Source::ClearData()
{
    protocol_.clear();
    uri_.clear();
    seekable_ = Seekable::INVALID;
    position_ = 0;
    mediaOffset_ = 0;
    isPluginReady_ = false;
    isAboveWaterline_ = false;
}

Status Source::SetSource(const std::shared_ptr<MediaSource>& source)
{
    MEDIA_LOG_I("SetSource entered.");
    if (source == nullptr) {
        MEDIA_LOG_E("Invalid source");
        return Status::ERROR_INVALID_PARAMETER;
    }
    ClearData();
    Status err = FindPlugin(source);
    if (err != Status::OK) {
        return err;
    }
    err = InitPlugin(source);
    if (err != Status::OK) {
        return err;
    }
    ActivateMode();
    return Status::OK;
}

Status Source::SetPushData(const std::shared_ptr<PushDataImpl>& data)
{
    pushData_ = data;
    return Status::OK;
}

Status Source::InitPlugin(const std::shared_ptr<MediaSource>& source)
{
    MEDIA_LOG_D("IN");
    Status err = plugin_->Init();
    if (err != Status::OK) {
        return err;
    }
    plugin_->SetCallback(this);
    return plugin_->SetSource(source);
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
        isPluginReady_ = true;
        if (isAboveWaterline_ && isPluginReady_) {
            MEDIA_LOG_D("media source send EVENT_READY");
            isPluginReady_ = false;
            isAboveWaterline_ = false;
        }
    }
    return ret;
}

Status Source::Start()
{
    MEDIA_LOG_I("Start entered.");
    return plugin_ ? plugin_->Start() : Status::ERROR_INVALID_OPERATION;
}

Status Source::PullData(uint64_t offset, size_t size, std::shared_ptr<Buffer>& data)
{
    MEDIA_LOG_DD("IN, offset: " PUBLIC_LOG_U64 ", size: " PUBLIC_LOG_ZU ", outPort: " PUBLIC_LOG_S,
        offset, size, outPort.c_str());
    if (!plugin_) {
        return Status::ERROR_INVALID_OPERATION;
    }
    Status err;
    auto readSize = size;
    if (seekable_ == Seekable::SEEKABLE) {
        uint64_t totalSize = 0;
        if ((plugin_->GetSize(totalSize) == Status::OK) && (totalSize != 0)) {
            if (offset >= totalSize) {
                MEDIA_LOG_W("Offset: " PUBLIC_LOG_U64 " is larger than totalSize: " PUBLIC_LOG_U64,
                    offset, totalSize);
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
            if (err != Status::OK) {
                MEDIA_LOG_E("Seek to " PUBLIC_LOG_U64 " fail", offset);
                return err;
            }
            position_ = offset;
        }
    }
    err = plugin_->Read(data, offset, readSize);
    if (err == Status::OK) {
        position_ += data->GetMemory()->GetSize();
    }
    return err;
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
    }
}

void Source::ActivateMode()
{
    MEDIA_LOG_D("ActivateMode");
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
        taskPtr_->Start();
    }
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
        if (taskPtr_) {
            taskPtr_->StopAsync();
        }
        data->flag |= BUFFER_FLAG_EOS;
        pushData_->PushData(data, mediaOffset_);
    } else if (err == Status::OK) {
        auto memory = data->GetMemory();
        auto size = memory->GetSize();
        if (!((size > 0) || (data->flag & BUFFER_FLAG_EOS))) {
            MEDIA_LOG_E("ReadLoop err");
            return;
        }
        if (data->flag & BUFFER_FLAG_EOS) {
            if (taskPtr_) {
                taskPtr_->StopAsync();
            }
        }

        pushData_->PushData(data, mediaOffset_);
        mediaOffset_ += size;
    } else {
        MEDIA_LOG_E("Read data failed (" PUBLIC_LOG_D32 ")", err);
    }
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
    MEDIA_LOG_I("protocol: " PUBLIC_LOG_S ", uri: " PUBLIC_LOG_S, protocol_.c_str(), uri_.c_str());
    return ret;
}

Status Source::CreatePlugin(const std::shared_ptr<PluginInfo>& info, const std::string& name,
    PluginManager& manager)
{
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
            for (auto supportProtocol : supportProtocols) {
                if (supportProtocol == g_protocolStringToType[protocol_]) {
                    if (CreatePlugin(info, name, pluginManager) == Status::OK) {
                        MEDIA_LOG_I("supportProtocol:" PUBLIC_LOG_S " CreatePlugin " PUBLIC_LOG_S " success",
                            protocol_.c_str(), name.c_str());
                        return Status::OK;
                    }
                }
            }
        }
    }
    MEDIA_LOG_E("Cannot find any plugin");
    return Status::ERROR_UNSUPPORTED_FORMAT;
}

Status Source::GetSize(uint64_t &fileSize)
{
    if (plugin_ == nullptr) {
        MEDIA_LOG_E("plugin_ is nullptr!");
        return Status::ERROR_INVALID_OPERATION;
    }
    return plugin_->GetSize(fileSize);
}
} // namespace Media
} // namespace OHOS