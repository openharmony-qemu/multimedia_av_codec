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
#include "common/log.h"
#include "data_sink_fd.h"
#include "data_sink_file.h"

namespace {
using namespace OHOS::Media;
using namespace Plugins;

constexpr int32_t ERR_TRACK_INDEX = -1;
constexpr uint32_t MAX_BUFFER_COUNT = 10;

const std::map<OutputFormat, std::set<std::string>> MUX_FORMAT_INFO = {
    {OutputFormat::MPEG_4, {MimeType::AUDIO_MPEG, MimeType::AUDIO_AAC,
                            MimeType::VIDEO_AVC, MimeType::VIDEO_MPEG4,
                            MimeType::VIDEO_HEVC,
                            MimeType::IMAGE_JPG, MimeType::IMAGE_PNG,
                            MimeType::IMAGE_BMP}},
    {OutputFormat::M4A, {MimeType::AUDIO_AAC,
                         MimeType::IMAGE_JPG, MimeType::IMAGE_PNG,
                         MimeType::IMAGE_BMP}},
    {OutputFormat::AMR, {MimeType::AUDIO_AMR_NB, MimeType::AUDIO_AMR_WB}},
    {OutputFormat::MP3, {MimeType::AUDIO_MPEG, MimeType::IMAGE_JPG}},
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
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " instances create", FAKE_POINTER(this));
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
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " instances destroy", FAKE_POINTER(this));
}

Status MediaMuxer::Init(int32_t fd, Plugins::OutputFormat format)
{
    MEDIA_LOG_I("Init");
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(state_ == State::UNINITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not UNINITIALIZED, the current state is %{public}s.", StateConvert(state_).c_str());

    FALSE_RETURN_V_MSG_E(fd >= 0, Status::ERROR_INVALID_PARAMETER, "The fd %{public}d is error!", fd);
    uint32_t fdPermission = static_cast<uint32_t>(fcntl(fd, F_GETFL, 0));
    FALSE_RETURN_V_MSG_E((fdPermission & O_WRONLY) == O_WRONLY || (fdPermission & O_RDWR) == O_RDWR,
        Status::ERROR_INVALID_PARAMETER, "No permission to write fd.");
    FALSE_RETURN_V_MSG_E(lseek(fd, 0, SEEK_CUR) != -1, Status::ERROR_INVALID_PARAMETER,
        "The fd is not seekable.");
    format_ = format == Plugins::OutputFormat::DEFAULT ? Plugins::OutputFormat::MPEG_4 : format;
    muxer_ = CreatePlugin(format_);
    if (muxer_ != nullptr) {
        state_ = State::INITIALIZED;
        muxer_->SetCallback(this);
        MEDIA_LOG_I("The state is INITIALIZED");
    } else {
        MEDIA_LOG_E("The state is UNINITIALIZED");
    }
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is UNINITIALIZED");
    return muxer_->SetDataSink(std::make_shared<DataSinkFd>(fd));
}

Status MediaMuxer::Init(FILE *file, Plugins::OutputFormat format)
{
    MEDIA_LOG_I("Init");
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(state_ == State::UNINITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not UNINITIALIZED, the current state is %{public}s.", StateConvert(state_).c_str());

    FALSE_RETURN_V_MSG_E(file != nullptr, Status::ERROR_INVALID_PARAMETER, "The file handle is null!");
    FALSE_RETURN_V_MSG_E(fseek(file, 0L, SEEK_CUR) >= 0, Status::ERROR_INVALID_PARAMETER,
        "The file handle is not seekable.");
    format_ = format == Plugins::OutputFormat::DEFAULT ? Plugins::OutputFormat::MPEG_4 : format;
    muxer_ = CreatePlugin(format_);
    if (muxer_ != nullptr) {
        state_ = State::INITIALIZED;
        muxer_->SetCallback(this);
        MEDIA_LOG_I("The state is INITIALIZED");
    } else {
        MEDIA_LOG_E("The state is UNINITIALIZED");
    }
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
                         "The state is UNINITIALIZED");
    return muxer_->SetDataSink(std::make_shared<DataSinkFile>(file));
}

Status MediaMuxer::SetParameter(const std::shared_ptr<Meta> &param)
{
    MEDIA_LOG_I("SetParameter");
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not INITIALIZED, the interface must be called after constructor and before Start(). "
        "The current state is %{public}s.", StateConvert(state_).c_str());
    return muxer_->SetParameter(param);
}

Status MediaMuxer::SetUserMeta(const std::shared_ptr<Meta> &userMeta)
{
    MEDIA_LOG_I("SetUserMeta");
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not INITIALIZED, the interface must be called after constructor and before Start(). "
        "The current state is %{public}s.", StateConvert(state_).c_str());
    return muxer_->SetUserMeta(userMeta);
}

Status MediaMuxer::AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc)
{
    MEDIA_LOG_I("AddTrack");
    std::lock_guard<std::mutex> lock(mutex_);
    trackIndex = ERR_TRACK_INDEX;
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not INITIALIZED, the interface must be called after constructor and before Start(). "
        "The current state is %{public}s.", StateConvert(state_).c_str());
    std::string mimeType = {};
    FALSE_RETURN_V_MSG_E(trackDesc->Get<Tag::MIME_TYPE>(mimeType), Status::ERROR_INVALID_DATA,
        "The track format does not contain mime.");
    FALSE_RETURN_V_MSG_E(CanAddTrack(mimeType), Status::ERROR_UNSUPPORTED_FORMAT,
        "The track mime is unsupported: %{public}s.", mimeType.c_str());
    FALSE_RETURN_V_MSG_E(CheckKeys(mimeType, trackDesc), Status::ERROR_INVALID_DATA,
        "The track format keys not contained.");

    int32_t trackId = -1;
    Status ret = muxer_->AddTrack(trackId, trackDesc);
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "AddTrack failed! %{public}s.", mimeType.c_str());
    FALSE_RETURN_V_MSG_E(trackId >= 0, Status::ERROR_INVALID_DATA,
        "The track index is greater than or equal to 0.");
    trackIndex = tracks_.size();
    sptr<Track> track = new Track();
    track->trackId_ = trackId;
    track->mimeType_ = mimeType;
    track->trackDesc_ = trackDesc;
    track->bufferQ_ = AVBufferQueue::Create(MAX_BUFFER_COUNT, MemoryType::UNKNOWN_MEMORY, mimeType);
    track->producer_ = track->bufferQ_->GetProducer();
    track->consumer_ = track->bufferQ_->GetConsumer();
    tracks_.emplace_back(track);
    return Status::NO_ERROR;
}

sptr<AVBufferQueueProducer> MediaMuxer::GetInputBufferQueue(uint32_t trackIndex)
{
    MEDIA_LOG_I("GetInputBufferQueue");
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(state_ == State::INITIALIZED, nullptr,
        "The state is not INITIALIZED, the interface must be called after AddTrack() and before Start(). "
        "The current state is %{public}s.", StateConvert(state_).c_str());
    FALSE_RETURN_V_MSG_E(trackIndex < tracks_.size(), nullptr,
        "The track index does not exist, the interface must be called after AddTrack() and before Start().");
    return tracks_[trackIndex]->producer_;
}

Status MediaMuxer::WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FALSE_RETURN_V_MSG_E(state_ == State::STARTED, Status::ERROR_WRONG_STATE,
        "The state is not STARTED, the interface must be called after Start() and before Stop(). "
        "The current state is %{public}s", StateConvert(state_).c_str());
    FALSE_RETURN_V_MSG_E(trackIndex < tracks_.size(), Status::ERROR_INVALID_DATA,
        "The track index does not exist, the interface must be called after AddTrack() and before Start().");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr, Status::ERROR_INVALID_DATA,
        "Invalid sample");
    MEDIA_LOG_D("WriteSample track:" PUBLIC_LOG_U32 ", pts:" PUBLIC_LOG_D64 ", size:" PUBLIC_LOG_D32
        ", flags:" PUBLIC_LOG_U32, trackIndex, sample->pts_, sample->memory_->GetSize(), sample->flag_);
    std::shared_ptr<AVBuffer> buffer = nullptr;
    AVBufferConfig avBufferConfig;
    avBufferConfig.size = sample->memory_->GetSize();
    avBufferConfig.memoryType = MemoryType::VIRTUAL_MEMORY;
    Status ret = tracks_[trackIndex]->producer_->RequestBuffer(buffer, avBufferConfig, -1);
    FALSE_RETURN_V_MSG_E(ret == Status::OK && buffer != nullptr, Status::ERROR_NO_MEMORY,
        "Request buffer failed.");
    buffer->pts_ = sample->pts_;
    buffer->dts_ = sample->dts_;
    buffer->flag_ = sample->flag_;
    buffer->duration_ = sample->duration_;
    *buffer->meta_.get() = *sample->meta_.get(); // copy meta
    if (sample->memory_ != nullptr && sample->memory_->GetSize() > 0) { // copy data
        int32_t retInt = buffer->memory_->Write(sample->memory_->GetAddr(), sample->memory_->GetSize(), 0);
        FALSE_RETURN_V_MSG_E(retInt > 0, Status::ERROR_NO_MEMORY, "Write sample in buffer failed.");
    }
    return tracks_[trackIndex]->producer_->PushBuffer(buffer, true);
}

Status MediaMuxer::Start()
{
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
    for (const auto& track : tracks_) {
        track->SetBufferAvailableListener(this);
        sptr<IConsumerListener> listener = track;
        track->consumer_->SetBufferAvailableListener(listener);
    }
    StartThread("OS_MUXER_WRITE");
    return Status::NO_ERROR;
}

void MediaMuxer::StartThread(const std::string &name)
{
    threadName_ = name;
    if (thread_ != nullptr) {
        MEDIA_LOG_W("Started already! [%{public}s]", threadName_.c_str());
        return;
    }
    isThreadExit_ = false;
    thread_ = std::make_unique<std::thread>(&MediaMuxer::ThreadProcessor, this);
    MEDIA_LOG_D("The thread started! [%{public}s]", threadName_.c_str());
}

Status MediaMuxer::Stop()
{
    MEDIA_LOG_I("Stop");
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ == State::STOPPED) {
        MEDIA_LOG_W("The current state is STOPPED!");
        return Status::ERROR_INVALID_OPERATION;
    }
    FALSE_RETURN_V_MSG_E(state_ == State::STARTED, Status::ERROR_WRONG_STATE,
        "The state is not STARTED. The current state is %{public}s.", StateConvert(state_).c_str());
    state_ = State::STOPPED;
    for (auto& track : tracks_) { // Stop the producer first
        sptr<IConsumerListener> listener = nullptr;
        track->consumer_->SetBufferAvailableListener(listener);
    }
    StopThread();
    Status ret = muxer_->Stop();
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "Stop failed!");
    return Status::NO_ERROR;
}

void MediaMuxer::StopThread()
{
    if (isThreadExit_) {
        MEDIA_LOG_D("Stopped already! [%{public}s]", threadName_.c_str());
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutexBufferAvailable_);
        isThreadExit_ = true;
        condBufferAvailable_.notify_all();
    }

    MEDIA_LOG_D("Stopping ! [%{public}s]", threadName_.c_str());
    if (thread_ != nullptr) {
        if (thread_->joinable()) {
            thread_->join();
        }
        thread_ = nullptr;
    }
}

Status MediaMuxer::Reset()
{
    MEDIA_LOG_I("Reset");
    if (state_ == State::STARTED) {
        Stop();
    }
    state_ = State::UNINITIALIZED;
    muxer_ = nullptr;
    tracks_.clear();

    return Status::NO_ERROR;
}

void MediaMuxer::ThreadProcessor()
{
    MEDIA_LOG_D("Enter ThreadProcessor [%{public}s]", threadName_.c_str());
    constexpr int32_t timeoutMs = 500;
    constexpr uint32_t nameSizeMax = 15;
    pthread_setname_np(pthread_self(), threadName_.substr(0, nameSizeMax).c_str());
    int32_t trackCount = static_cast<int32_t>(tracks_.size());
    for (;;) {
        if (isThreadExit_ && bufferAvailableCount_ <= 0) {
            MEDIA_LOG_D("Exit ThreadProcessor [%{public}s]", threadName_.c_str());
            return;
        }
        std::unique_lock<std::mutex> lock(mutexBufferAvailable_);
        condBufferAvailable_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
            [this] { return isThreadExit_ || bufferAvailableCount_ > 0; });
        int32_t trackIdx = -1;
        std::shared_ptr<AVBuffer> buffer1 = nullptr;
        for (int i = 0; i < trackCount; ++i) {
            std::shared_ptr<AVBuffer> buffer2 = tracks_[i]->GetBuffer();
            if ((buffer1 != nullptr && buffer2 != nullptr && buffer1->pts_ > buffer2->pts_) ||
                (buffer1 == nullptr && buffer2 != nullptr)) {
                buffer1 = buffer2;
                trackIdx = i;
            }
        }
        if (buffer1 != nullptr) {
            muxer_->WriteSample(tracks_[trackIdx]->trackId_, tracks_[trackIdx]->curBuffer_);
            tracks_[trackIdx]->ReleaseBuffer();
            --bufferAvailableCount_;
        }
        MEDIA_LOG_D("Track " PUBLIC_LOG_S " 2 bufferAvailableCount_ :" PUBLIC_LOG_D32,
            threadName_.c_str(), bufferAvailableCount_.load());
    }
}

void MediaMuxer::OnBufferAvailable()
{
    ++bufferAvailableCount_;
    condBufferAvailable_.notify_one();
    MEDIA_LOG_D("Track " PUBLIC_LOG_S " 1 bufferAvailableCount_ :" PUBLIC_LOG_D32,
        threadName_.c_str(), bufferAvailableCount_.load());
}

std::shared_ptr<AVBuffer> MediaMuxer::Track::GetBuffer()
{
    if (curBuffer_ == nullptr && bufferAvailableCount_ > 0) {
        consumer_->AcquireBuffer(curBuffer_);
    }
    return curBuffer_;
}

void MediaMuxer::Track::ReleaseBuffer()
{
    if (curBuffer_ != nullptr) {
        consumer_->ReleaseBuffer(curBuffer_);
        curBuffer_ = nullptr;
        --bufferAvailableCount_;
    }
}

void MediaMuxer::Track::SetBufferAvailableListener(MediaMuxer *listener)
{
    listener_ = listener;
}

void MediaMuxer::Track::OnBufferAvailable()
{
    ++bufferAvailableCount_;
    MEDIA_LOG_D("Track " PUBLIC_LOG_S " bufferAvailableCount_ :" PUBLIC_LOG_D32,
        mimeType_.c_str(), bufferAvailableCount_.load());
    listener_->OnBufferAvailable();
}

std::shared_ptr<Plugins::MuxerPlugin> MediaMuxer::CreatePlugin(Plugins::OutputFormat format)
{
    static const std::unordered_map<Plugins::OutputFormat, std::string> table = {
        {Plugins::OutputFormat::DEFAULT, MimeType::MEDIA_MP4},
        {Plugins::OutputFormat::MPEG_4, MimeType::MEDIA_MP4},
        {Plugins::OutputFormat::M4A, MimeType::MEDIA_M4A},
        {Plugins::OutputFormat::AMR, MimeType::MEDIA_AMR},
        {Plugins::OutputFormat::MP3, MimeType::MEDIA_MP3},
    };
    FALSE_RETURN_V_MSG_E(table.find(format) != table.end(), nullptr,
        "The output format %{public}d is not supported!", format);

    auto names = Plugins::PluginManager::Instance().ListPlugins(Plugins::PluginType::MUXER);
    std::string pluginName = "";
    uint32_t maxProb = 0;
    for (auto& name : names) {
        auto info = Plugins::PluginManager::Instance().GetPluginInfo(Plugins::PluginType::MUXER, name);
        if (info == nullptr) {
            continue;
        }
        for (const auto& cap : info->outCaps) {
            if (cap.mime == table.at(format) && info->rank > maxProb) {
                maxProb = info->rank;
                pluginName = name;
                break;
            }
        }
    }
    MEDIA_LOG_I("The max probability is %{public}d, and the plugin name is %{public}s.", maxProb, pluginName.c_str());
    if (!pluginName.empty()) {
        auto plugin = Plugins::PluginManager::Instance().CreatePlugin(pluginName, Plugins::PluginType::MUXER);
        return std::reinterpret_pointer_cast<Plugins::MuxerPlugin>(plugin);
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

void MediaMuxer::OnEvent(const PluginEvent &event)
{
    MEDIA_LOG_D("OnEvent");
}
} // Media
} // OHOS
