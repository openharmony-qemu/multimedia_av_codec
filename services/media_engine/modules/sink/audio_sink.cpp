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

#include "audio_sink.h"

#define INPUT_BUFFER_QUEUE_NAME "AudioSinkInputBufferQueue"

namespace OHOS {
namespace Media {

Status AudioSink::Init(std::shared_ptr<Meta>& meta)
{
    state_ = Pipeline::FilterState::INITIALIZED;
    plugin_ = CreatePlugin(meta);
    plugin_->Init();
    plugin_->SetParameter(meta);
    plugin_->Prepare();
    return Status::OK;
}

sptr<AVBufferQueueProducer> AudioSink::GetInputBufferQueue()
{
    if (state_ != Pipeline::FilterState::READY) {
        return nullptr;
    }
    return inputBufferQueueProducer_;
}

Status AudioSink::SetParameter(const std::shared_ptr<Meta>& meta)
{
    plugin_->SetParameter(meta);
    return Status::OK;
}

Status AudioSink::GetParameter(std::shared_ptr<Meta>& meta) {
    return plugin_->GetParameter(meta);
}

Status AudioSink::Prepare() {
    state_ = Pipeline::FilterState::PREPARING;
    Status ret = Status::OK;
    ret = PrepareInputBufferQueue();
    if (ret != Status::OK) {
        state_ = Pipeline::FilterState::INITIALIZED;
        return ret;
    }
    state_ = Pipeline::FilterState::READY;
    return ret;
}

Status AudioSink::Start() {
    Status ret = Status::OK;
    ret = plugin_->Start();
    if (ret != Status::OK) {
        return ret;
    }
    state_ = Pipeline::FilterState::RUNNING;
    return ret;
}

Status AudioSink::Stop() {
    Status ret = Status::OK;
    ret = plugin_->Stop();
    if (ret != Status::OK) {
        return ret;
    }
    state_ = Pipeline::FilterState::INITIALIZED;
    return ret;
}

Status AudioSink::Pause() {
    Status ret = Status::OK;
    ret = plugin_->Pause();
    if (ret != Status::OK) {
        return ret;
    }
    state_ = Pipeline::FilterState::PAUSED;
    return ret;
}

Status AudioSink::Resume() {
    return plugin_->Resume();
}

Status AudioSink::Release() {

    return plugin_->Deinit();
}

Status AudioSink::SetVolume(float volume)
{
    if (plugin_ == nullptr) {
        return Status::ERROR_NULL_POINTER;
    }
    if (volume < 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    return plugin_->SetVolume(volume);
}

Status AudioSink::PrepareInputBufferQueue()
{
    if (inputBufferQueue_ != nullptr && inputBufferQueue_-> GetQueueSize() > 0) {
        MEDIA_LOG_I("InputBufferQueue already create");
        return Status::ERROR_INVALID_OPERATION;
    }
    int inputBufferSize = 8;
    MemoryType memoryType = MemoryType::SHARED_MEMORY;
#ifndef MEDIA_OHOS
    memoryType = MemoryType::VIRTUAL_MEMORY;
#endif
    inputBufferQueue_ = AVBufferQueue::Create(inputBufferSize, memoryType, INPUT_BUFFER_QUEUE_NAME);
    inputBufferQueueProducer_ = inputBufferQueue_->GetProducer();
    inputBufferQueueConsumer_ = inputBufferQueue_->GetConsumer();
    auto audioSink = std::make_shared<AudioSink>();
    sptr<IConsumerListener> listener = new AVBufferAvailableListener(shared_from_this());
    inputBufferQueueConsumer_->SetBufferAvailableListener(listener);
    return Status::OK;
}

std::shared_ptr<Plugin::AudioSinkPlugin> AudioSink::CreatePlugin(std::shared_ptr<Meta> meta)
{
    Plugin::PluginType pluginType = Plugin::PluginType::AUDIO_SINK;
    auto names = Plugin::PluginManager::Instance().ListPlugins(pluginType);
    std::string pluginName = "";
    for (auto& name : names) {
        auto info = Plugin::PluginManager::Instance().GetPluginInfo(pluginType, name);
        pluginName = name;
        break;
    }
    MEDIA_LOG_I("pluginName %{public}s", pluginName.c_str());
    if (!pluginName.empty()) {
        auto plugin = Plugin::PluginManager::Instance().CreatePlugin(pluginName, pluginType);
        return std::reinterpret_pointer_cast<Plugin::AudioSinkPlugin>(plugin);
    } else {
        MEDIA_LOG_E("No plugins matching output format");
    }
    return nullptr;
}

void AudioSink::ProcessOutputBuffer() {
    Status ret;
    std::shared_ptr<AVBuffer> filledOutputBuffer = nullptr;
    if (plugin_ == nullptr || inputBufferQueueConsumer_ == nullptr) {
        return;
    }
    ret = inputBufferQueueConsumer_->AcquireBuffer(filledOutputBuffer);
    if (ret != Status::OK) {
        return;
    }
    if (state_ != Pipeline::FilterState::RUNNING) {
        inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
        return;
    }
    plugin_->Write(filledOutputBuffer);
    inputBufferQueueConsumer_->ReleaseBuffer(filledOutputBuffer);
}


} //namespace MEDIA
} //namespace OHOS