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

#define HST_LOG_TAG "MediaMuxer"

#include "media_muxer.h"

#include <set>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <unordered_map>

#include "securec.h"
#include "meta/mime_type.h"
#include "plugin/plugin_manager.h"
#include "data_sink_fd.h"
#include "common/log.h"

namespace {
using namespace OHOS::Media;
using namespace Plugin;

constexpr int32_t ERR_TRACK_INDEX = -1;

const std::map<OutputFormat, std::set<std::string>> MUX_FORMAT_INFO = {
    {OutputFormat::MPEG_4, {MimeType::AUDIO_MPEG, MimeType::AUDIO_AAC,
                            MimeType::VIDEO_AVC, MimeType::VIDEO_MPEG4,
                            MimeType::VIDEO_HEVC,
                            MimeType::IMAGE_JPG, MimeType::IMAGE_PNG,
                            MimeType::IMAGE_BMP}},
    {OutputFormat::M4A, {MimeType::AUDIO_AAC,
                         MimeType::IMAGE_JPG, MimeType::IMAGE_PNG,
                         MimeType::IMAGE_BMP}},
};

const std::map<std::string, std::set<std::string>> MUX_MIME_INFO = {
    {MimeType::AUDIO_MPEG, {Tag::AUDIO_SAMPLE_RATE, Tag::AUDIO_CHANNEL_COUNT}},
    {MimeType::AUDIO_AAC, {Tag::AUDIO_SAMPLE_RATE, Tag::AUDIO_CHANNEL_COUNT}},
    {MimeType::VIDEO_AVC, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
    {MimeType::VIDEO_MPEG4, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
    {MimeType::VIDEO_HEVC, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
    {MimeType::IMAGE_JPG, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
    {MimeType::IMAGE_PNG, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
    {MimeType::IMAGE_BMP, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
};
}

namespace OHOS {
namespace Media {
MediaMuxer::MediaMuxer(int32_t appUid, int32_t appPid)
    : appUid_(appUid), appPid_(appPid)
{
    MEDIA_LOG_D("0x%{public}06X instances create", FAKE_POINTER(this));
}

MediaMuxer::~MediaMuxer()
{
    MEDIA_LOG_D("Destroy");
    if (state_ == State::STARTED) {
        Stop();
    }

    appUid_ = -1;
    appPid_ = -1;
    muxer_ = nullptr;
    tracks_.clear();
    MEDIA_LOG_D("0x%{public}06X instances destroy", FAKE_POINTER(this));
}

Status MediaMuxer::Init(int32_t fd, Plugin::OutputFormat format)
{
    // AVCodecTrace trace("MediaMuxer::Init");
    MEDIA_LOG_I("Init");
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(state_ == State::UNINITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not UNINITIALIZED, the current state is %{public}s.", StateConvert(state_).c_str());
#ifndef _WIN32
    FALSE_RETURN_V_MSG_E(fd >= 0, Status::ERROR_INVALID_PARAMETER,
        "The fd %{public}d is error!", fd);
    uint32_t fdPermission = static_cast<uint32_t>(fcntl(fd, F_GETFL, 0));
    FALSE_RETURN_V_MSG_E((fdPermission & O_RDWR) == O_RDWR, Status::ERROR_INVALID_PARAMETER,
        "No permission to read and write fd.");
    FALSE_RETURN_V_MSG_E(lseek(fd, 0, SEEK_CUR) != -1, Status::ERROR_INVALID_PARAMETER,
        "The fd is not seekable.");
#endif
    fd_ = fd;
    format_ = format == Plugin::OutputFormat::DEFAULT ? Plugin::OutputFormat::MPEG_4 : format;
    muxer_ = CreatePlugin(format_);
    if (muxer_ != nullptr) {
        state_ = State::INITIALIZED;
        MEDIA_LOG_I("The state is INITIALIZED");
    } else {
        MEDIA_LOG_E("The state is UNINITIALIZED");
    }
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is UNINITIALIZED");
    return muxer_->SetDataSink(std::make_shared<DataSinkFd>(fd_));
}

Status MediaMuxer::SetParameter(const std::shared_ptr<Meta> &param)
{
    // AVCodecTrace trace("MediaMuxer::SetParameter");
    MEDIA_LOG_I("SetParameter");
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not INITIALIZED, the interface must be called after constructor and before Start(). "
        "The current state is %{public}s.", StateConvert(state_).c_str());
    return muxer_->SetParameter(param);
}

Status MediaMuxer::AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc)
{
    // AVCodecTrace trace("MediaMuxer::AddTrack");
    MEDIA_LOG_I("AddTrack");
    std::lock_guard<std::mutex> lock(mutex_);
    trackIndex = ERR_TRACK_INDEX;
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not INITIALIZED, the interface must be called after constructor and before Start(). "
        "The current state is %{public}s.", StateConvert(state_).c_str());
    std::string mimeType = {};
    FALSE_RETURN_V_MSG_E(trackDesc->Get<Tag::MIME_TYPE>(mimeType), Status::ERROR_INVALID_DATA,
        "The track format does not contain mime.");
    FALSE_RETURN_V_MSG_E(CanAddTrack(mimeType), Status::ERROR_INVALID_PARAMETER,
        "The track mime is unsupported: %{public}s.", mimeType.c_str());
    FALSE_RETURN_V_MSG_E(CheckKeys(mimeType, trackDesc), Status::ERROR_INVALID_DATA,
        "The track format keys not contained.");

    int32_t trackId = -1;
    Status ret = muxer_->AddTrack(trackId, trackDesc);
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "AddTrack failed!");
    FALSE_RETURN_V_MSG_E(trackId >= 0, Status::ERROR_INVALID_DATA,
        "The track index is greater than or equal to 0.");
    trackIndex = tracks_.size();
    sptr<Track> track = new Track();
    track->trackId_ = trackId;
    track->mimeType_ = mimeType;
    track->trackDesc_ = trackDesc;
    track->bufferQ_ = AVBufferQueue::Create(10,MemoryType::UNKNOWN_MEMORY, mimeType);
    track->producer_ = track->bufferQ_->GetProducer();
    track->consumer_ = track->bufferQ_->GetConsumer();
    track->muxer_ = muxer_;
    tracks_.emplace_back(track);
    return Status::NO_ERROR;
}

sptr<AVBufferQueueProducer> MediaMuxer::GetInputBufferQueue(uint32_t trackIndex)
{
    // AVCodecTrace trace("MediaMuxer::GetInputBufferQueue");
    MEDIA_LOG_I("GetInputBufferQueue");
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, nullptr,
        "The state is not INITIALIZED, the interface must be called after AddTrack() and before Start(). "
        "The current state is %{public}s.", StateConvert(state_).c_str());
    FALSE_RETURN_V_MSG_E(trackIndex < tracks_.size(), nullptr,
        "The track index does not exist, the interface must be called after AddTrack() and before Start().");
    return tracks_[trackIndex]->producer_;
}

Status MediaMuxer::Start()
{
    // AVCodecTrace trace("MediaMuxer::Start");
    MEDIA_LOG_I("Start");
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not INITIALIZED, the interface must be called after AddTrack() and before WriteSample(). "
        "The current state is %{public}s.", StateConvert(state_).c_str());
    FALSE_RETURN_V_MSG_E(tracks_.size() > 0, Status::ERROR_INVALID_OPERATION,
        "The track count is error, count is %{public}zu.", tracks_.size());
    Status ret = muxer_->Start();
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "Start failed!");
    state_ = State::STARTED;
    for (auto& track : tracks_) {
        sptr<IConsumerListener> listener = track;
        track->consumer_->SetBufferAvailableListener(listener);
        track->Start();
    }
    return Status::NO_ERROR;
}

Status MediaMuxer::Stop()
{
    // AVCodecTrace trace("MediaMuxer::Stop");
    MEDIA_LOG_I("Stop");
    std::unique_lock<std::mutex> lock(mutex_);
    if (state_ == State::STOPPED) {
        MEDIA_LOG_W("The current state is STOPPED!");
        return Status::NO_ERROR;
    }
    FALSE_RETURN_V_MSG_E(state_ == State::STARTED, Status::ERROR_WRONG_STATE,
        "The state is not STARTED. The current state is %{public}s.", StateConvert(state_).c_str());
    state_ = State::STOPPED;
    for (auto& track : tracks_) { // Stop the producer first
        sptr<IConsumerListener> listener = nullptr;
        track->consumer_->SetBufferAvailableListener(listener);
    }
    for (auto& track : tracks_) { // Stop the consumer second (all data needs to be consumed)
        track->Stop();
    }
    Status ret = muxer_->Stop();
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "Stop failed!");
    return Status::NO_ERROR;
}

Status MediaMuxer::Reset()
{
    // AVCodecTrace trace("MediaMuxer::Reset");
    MEDIA_LOG_I("Reset");
    if (state_ == State::STARTED) {
        Stop();
    }
    state_ = State::UNINITIALIZED;
    muxer_ = nullptr;
    tracks_.clear();

    return Status::NO_ERROR;
}

MediaMuxer::Track::~Track()
{
    if (thread_ != nullptr) {
        Stop();
    }
}

void MediaMuxer::Track::Start()
{
    // AVCodecTrace trace("MediaMuxer::Track::StartThread");
    if (thread_ != nullptr) {
        MEDIA_LOG_W("Started already! [%{public}s]", mimeType_.c_str());
        return;
    }
    isThreadExit_ = false;
    thread_ = std::make_unique<std::thread>(&MediaMuxer::Track::ThreadProcessor, this);
    // AVCodecTrace::TraceBegin(mimeType_, FAKE_POINTER(thread_.get()));
    MEDIA_LOG_D("The thread started! [%{public}s]", mimeType_.c_str());
}

void MediaMuxer::Track::Stop() noexcept
{
    if (isThreadExit_) {
        MEDIA_LOG_D("Stopped already! [%{public}s]", mimeType_.c_str());
        return;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    isThreadExit_ = true;
    SetBufferAvailable(true);
    condBufferEmpty_.wait(lock, [this] { return isEmpty_.load(); });

    if (std::this_thread::get_id() == thread_->get_id()) {
        MEDIA_LOG_D("Stop at the task thread, reject!");
        return;
    }

    std::unique_ptr<std::thread> t;
    condBufferEmpty_.notify_all();
    condBufferAvailable_.notify_all();
    std::swap(thread_, t);
    if (t != nullptr && t->joinable()) {
        t->join();
    }
    thread_ = nullptr;
}

void MediaMuxer::Track::ThreadProcessor()
{
    MEDIA_LOG_D("Enter ThreadProcessor [%{public}s]", mimeType_.c_str());
    // constexpr uint32_t nameSizeMax = 15;
    pthread_setname_np(pthread_self(), mimeType_.c_str());
    // int32_t taskId = FAKE_POINTER(thread_.get());
    for (;;) {
        // AVCodecTrace trace(mimeType_ + " write");
        if (isThreadExit_ && !isBufferAvailable_) {
            // AVCodecTrace::TraceEnd(mimeType_, taskId);
            MEDIA_LOG_D("Exit ThreadProcessor [%{public}s]", mimeType_.c_str());
            return;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        condBufferAvailable_.wait(lock, [this] { return isBufferAvailable_.load(); });
        std::shared_ptr<AVBuffer> buffer = nullptr;
        if (consumer_->AcquireBuffer(buffer) == Status::OK && buffer != nullptr) {
            muxer_->WriteSample(trackId_, buffer);
            consumer_->ReleaseBuffer(buffer);
        } else {
            SetBufferAvailable(false);
        }
    }
}

void MediaMuxer::Track::SetBufferAvailable(bool isAvailable)
{
    std::lock_guard<std::mutex> lock(mutex_);
    isBufferAvailable_ = isAvailable;
    isEmpty_ = !isAvailable;
    condBufferAvailable_.notify_one();
    condBufferEmpty_.notify_one();
}

void MediaMuxer::Track::OnBufferAvailable()
{
    SetBufferAvailable(true);
}

std::shared_ptr<Plugin::MuxerPlugin> MediaMuxer::CreatePlugin(Plugin::OutputFormat format)
{
    static const std::unordered_map<Plugin::OutputFormat, std::string> table = {
        {Plugin::OutputFormat::DEFAULT, MimeType::CONTAINER_MP4},
        {Plugin::OutputFormat::MPEG_4, MimeType::CONTAINER_MP4},
        {Plugin::OutputFormat::M4A, MimeType::CONTAINER_M4A},
    };
    FALSE_RETURN_V_MSG_E(table.find(format) != table.end(), nullptr,
        "The output format %{public}d is not supported!", format);

    auto names = Plugin::PluginManager::Instance().ListPlugins(Plugin::PluginType::MUXER);
    std::string pluginName = "";
    int32_t maxProb = 0;
    for (auto& name : names) {
        auto info = Plugin::PluginManager::Instance().GetPluginInfo(Plugin::PluginType::MUXER, name);
        if (info == nullptr) {
            continue;
        }
        for (auto& cap : info->outCaps) {
            if (cap.mime == table.at(format) && info->rank > maxProb) {
                maxProb = info->rank;
                pluginName = name;
                break;
            }
        }
    }
    MEDIA_LOG_I("The maxProb is %{public}d, and pluginName is %{public}s.", maxProb, pluginName.c_str());
    if (!pluginName.empty()) {
        auto plugin = Plugin::PluginManager::Instance().CreatePlugin(pluginName, Plugin::PluginType::MUXER);
        return nullptr;
        // return reinterpret_cast<Plugin::MuxerPlugin>(plugin);
    } else {
        MEDIA_LOG_E("No plugins matching output format - %{public}d", format);
    }
    return nullptr;
}

bool MediaMuxer::CanAddTrack(const std::string &mimeType)
{
    auto it = MUX_FORMAT_INFO.find(format_);
    if (it == MUX_FORMAT_INFO.end()) {
        return false;
    }
    return it->second.find(mimeType) != it->second.end();
}

bool MediaMuxer::CheckKeys(const std::string &mimeType, const std::shared_ptr<Meta> &trackDesc)
{
    bool ret = true;
    auto it = MUX_MIME_INFO.find(mimeType);
    if (it == MUX_MIME_INFO.end()) {
        return ret; // 不做检查
    }

    for (auto &key : it->second) {
        if (trackDesc->Find(key.c_str()) == trackDesc->end()) {
            ret = false;
            MEDIA_LOG_E("The format key %{public}s not contained.", key.data());
        }
    }
    return ret;
}

std::string MediaMuxer::StateConvert(State state)
{
    static const std::map<State, std::string> table = {
        {State::UNINITIALIZED, "UNINITIALIZED"},
        {State::INITIALIZED, "INITIALIZED"},
        {State::STARTED, "STARTED"},
        {State::STOPPED, "STOPPED"},
    };
    auto it = table.find(state);
    if (it != table.end()) {
        return it->second;
    }
    return "";
}
} // Media
} // OHOS