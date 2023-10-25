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

#include "media_muxer.h"
#include <set>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <unordered_map>
#include "securec.h"
#include "codec_mime_type.h"
#include "muxer_factory.h"
#include "data_sink_fd.h"
#include "avcodec_log.h"
#include "avcodec_dfx.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaMuxer"};
}

namespace {
using namespace OHOS::Media;
using namespace Plugin;
constexpr int32_t ERR_TRACK_INDEX = -1;
constexpr uint32_t DUMP_MUXER_INFO_INDEX = 0x01010000;
constexpr uint32_t DUMP_STATUS_INDEX = 0x01010100;
constexpr uint32_t DUMP_OUTPUT_FORMAT_INDEX = 0x01010200;
constexpr uint32_t DUMP_OFFSET_8 = 8;

const std::map<uint32_t, std::set<std::string>> MUX_FORMAT_INFO = {
    {OUTPUT_FORMAT_MPEG_4, {CodecMimeType::AUDIO_MPEG, CodecMimeType::AUDIO_AAC,
                            CodecMimeType::VIDEO_AVC, CodecMimeType::VIDEO_MPEG4,
                            CodecMimeType::VIDEO_HEVC,
                            CodecMimeType::IMAGE_JPG, CodecMimeType::IMAGE_PNG,
                            CodecMimeType::IMAGE_BMP}},
    {OUTPUT_FORMAT_M4A, {CodecMimeType::AUDIO_AAC,
                         CodecMimeType::IMAGE_JPG, CodecMimeType::IMAGE_PNG,
                         CodecMimeType::IMAGE_BMP}},
};
const std::map<std::string, std::set<std::string>> MUX_MIME_INFO = {
    {CodecMimeType::AUDIO_MPEG, {Tag::AUDIO_SAMPLE_RATE, Tag::AUDIO_CHANNEL_COUNT}},
    {CodecMimeType::AUDIO_AAC, {Tag::AUDIO_SAMPLE_RATE, Tag::AUDIO_CHANNEL_COUNT}},
    {CodecMimeType::VIDEO_AVC, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
    {CodecMimeType::VIDEO_MPEG4, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
    {CodecMimeType::VIDEO_HEVC, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
    {CodecMimeType::IMAGE_JPG, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
    {CodecMimeType::IMAGE_PNG, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
    {CodecMimeType::IMAGE_BMP, {Tag::VIDEO_WIDTH, Tag::VIDEO_HEIGHT}},
};
}

namespace OHOS {
namespace Media {
MediaMuxer::MediaMuxer(int32_t appUid, int32_t appPid)
    : appUid_(appUid), appPid_(appPid)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaMuxer::~MediaMuxer()
{
    AVCODEC_LOGD("Destroy");
    if (state_ == State::STARTED) {
        Stop();
    }

    appUid_ = -1;
    appPid_ = -1;
    muxer_ = nullptr;
    tracks_.clear();
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

Status MediaMuxer::Init(int32_t fd, Plugin::OutputFormat format)
{
    AVCodecTrace trace("MediaMuxer::Init");
    AVCODEC_LOGI("Init");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == State::UNINITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not UNINITIALIZED, the current state is %{public}s", StateConvert(state_).c_str());
    CHECK_AND_RETURN_RET_LOG(fd >= 0, Status::ERROR_INVALID_PARAMETER,
        "fd %{public}d is error!", fd);
    uint32_t fdPermission = static_cast<uint32_t>(fcntl(fd, F_GETFL, 0));
    CHECK_AND_RETURN_RET_LOG((fdPermission & O_RDWR) == O_RDWR, Status::ERROR_INVALID_PARAMETER,
        "no permission to read and write fd");
    CHECK_AND_RETURN_RET_LOG(lseek(fd, 0, SEEK_CUR) != -1, Status::ERROR_INVALID_PARAMETER,
        "the fd is not seekable");
    fd_ = fd;
    format_ = (format == Plugin::OutputFormat::DEFAULT) ? Plugin::OutputFormat::MPEG_4 : Plugin::OutputFormat::M4A;
    muxer_ = Plugin::MuxerFactory::Instance().CreatePlugin(format_);
    if (muxer_ != nullptr) {
        state_ = State::INITIALIZED;
        AVCODEC_LOGI("state_ is INITIALIZED");
    } else {
        AVCODEC_LOGE("state_ is UNINITIALIZED");
    }
    CHECK_AND_RETURN_RET_LOG(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "state_ is UNINITIALIZED");
    return muxer_->SetDataSink(std::make_shared<DataSinkFd>(fd_));
}

Status MediaMuxer::SetParameter(const std::shared_ptr<Meta> &param)
{
    AVCodecTrace trace("MediaMuxer::SetParameter");
    AVCODEC_LOGI("SetParameter");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not INITIALIZED, the interface must be called after constructor and before Start(). "
        "The current state is %{public}s", StateConvert(state_).c_str());
    return muxer_->SetParameter(param);
}

Status MediaMuxer::AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc)
{
    AVCodecTrace trace("MediaMuxer::AddTrack");
    AVCODEC_LOGI("AddTrack");
    std::lock_guard<std::mutex> lock(mutex_);
    trackIndex = ERR_TRACK_INDEX;
    CHECK_AND_RETURN_RET_LOG(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not INITIALIZED, the interface must be called after constructor and before Start(). "
        "The current state is %{public}s", StateConvert(state_).c_str());
    std::string mimeType = {};
    CHECK_AND_RETURN_RET_LOG(trackDesc->Get(Tag::CODEC_MIME_TYPE, mimeType), Status::ERROR_INVALID_DATA,
        "track format does not contain mime");
    CHECK_AND_RETURN_RET_LOG(CanAddTrack(mimeType), Status::ERROR_INVALID_PARAMETER,
        "track mime is unsupported: %{public}s", mimeType.c_str());
    CHECK_AND_RETURN_RET_LOG(CheckKeys(mimeType, trackDesc), Status::ERROR_INVALID_DATA,
        "track format keys not contained");

    int32_t trackId = -1;
    Status ret = muxer_->AddTrack(trackId, trackDesc);
    CHECK_AND_RETURN_RET_LOG(ret == Status::NO_ERROR, ret, "AddTrack failed");
    CHECK_AND_RETURN_RET_LOG(trackId >= 0, Status::ERROR_INVALID_DATA,
        "The track index is greater than or equal to 0.");
    trackIndex = tracks_.size();
    auto track = std::make_shared<Track>();
    track->trackId_ = trackId;
    track->mimeType_ = mimeType;
    track->trackDesc_ = trackDesc;
    track->bufferQ_ = std::make_shared<AVBufferQueue>(mimeType, 10);
    track->producer_ = track->bufferQ_->GetProducer();
    track->consumer_ = track->bufferQ_->GetConsumer();
    track->muxer_ = muxer_;
    tracks_.emplace_back(track);
    return Status::NO_ERROR;
}

std::shared_ptr<AVBufferQueueProducer> MediaMuxer::GetInputBufferQueue(uint32_t trackIndex)
{
    AVCodecTrace trace("MediaMuxer::GetInputBufferQueue");
    AVCODEC_LOGI("GetInputBufferQueue");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not INITIALIZED, the interface must be called after AddTrack() and before Start(). "
        "The current state is %{public}s", StateConvert(state_).c_str());
    CHECK_AND_RETURN_RET_LOG(trackIndex < tracks_.size(), nullptr,
        "The track index does not exist, the interface must be called after AddTrack() and before Start().");
    return tracks_[trackIndex].producer_;
}

Status MediaMuxer::Start()
{
    AVCodecTrace trace("MediaMuxer::Start");
    AVCODEC_LOGI("Start");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == State::INITIALIZED, Status::ERROR_WRONG_STATE,
        "The state is not INITIALIZED, the interface must be called after AddTrack() and before WriteSample(). "
        "The current state is %{public}s", StateConvert(state_).c_str());
    CHECK_AND_RETURN_RET_LOG(tracks_.size() > 0, Status::ERROR_INVALID_OPERATION,
        "The track count is error, count is %{public}zu", tracks_.size());
    Status ret = muxer_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == Status::NO_ERROR, ret, "Start failed");
    state_ = State::STARTED;
    for (auto& track : tracks_) {
        track->consumer_->SetBufferAvailableListener(track);
        track->Start();
    }
    return Status::NO_ERROR;
}

Status MediaMuxer::Stop()
{
    AVCodecTrace trace("MediaMuxer::Stop");
    AVCODEC_LOGI("Stop");
    std::unique_lock<std::mutex> lock(mutex_);
    if (state_ == State::STOPPED) {
        AVCODEC_LOGW("current state is STOPPED!");
        return Status::NO_ERROR;
    }
    CHECK_AND_RETURN_RET_LOG(state_ == State::STARTED, Status::ERROR_WRONG_STATE,
        "The state is not STARTED. The current state is %{public}s", StateConvert(state_).c_str());
    state_ = State::STOPPED;
    for (auto& track : tracks_) {
        track->consumer_->SetBufferAvailableListener(nullptr);
        track->Stop();
    }
    Status ret = muxer_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == Status::NO_ERROR, ret, "Stop failed");
    return Status::NO_ERROR;
}

Status MediaMuxer::Reset()
{
    if (state_ == State::STARTED) {
        Stop();
    }
    state_ = State::UNINITIALIZED
    muxer_ = nullptr;
    tracks_.clear();

    return Status::NO_ERROR;
}

MediaMuxer::Track::~Track()
{
    if (thread_ != nullptr) {
        StopThread();
    }
}

Status MediaMuxer::Track::Start()
{
    AVCodecTrace trace("MediaMuxer::Track::StartThread");
    if (thread_ != nullptr) {
        AVCODEC_LOGW("Started already! [%{public}s]", mimeType_.c_str());
        return;
    }
    isThreadExit_ = false;
    thread_ = std::make_unique<std::thread>(&MediaMuxer::Track::ThreadProcessor, this);
    AVCodecTrace::TraceBegin(mimeType_, FAKE_POINTER(thread_.get()));
    AVCODEC_LOGD("thread started! [%{public}s]", mimeType_.c_str());
}

void MediaMuxer::Track::Stop() noexcept
{
    if (isThreadExit_) {
        AVCODEC_LOGD("Stopped already! [%{public}s]", mimeType_.c_str());
        return;
    }

    isThreadExit_ = true;
    SetBufferAvailable(true);
    condBufferEmpty_.wait(lock, [this] { return isEmpty_; });

    if (std::this_thread::get_id() == thread_->get_id()) {
        AVCODEC_LOGD("Stop at the task thread, reject");
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
    AVCODEC_LOGD("Enter ThreadProcessor [%{public}s]", mimeType_.c_str());
    constexpr uint32_t nameSizeMax = 15;
    pthread_setname_np(pthread_self(), mimeType_.c_str());
    int32_t taskId = FAKE_POINTER(thread_.get());
    for (;;) {
        AVCodecTrace trace(mimeType_ + " write");
        if (isThreadExit_ && !isBufferAvailable_) {
            AVCodecTrace::TraceEnd(mimeType_, taskId);
            AVCODEC_LOGD("Exit ThreadProcessor [%{public}s]", mimeType_.c_str());
            return;
        }
        std::unique_lock<std::mutex> lock(mutex_);
        condBufferAvailable_.wait(lock, [this] { return isBufferAvailable_; });
        std::shared_ptr<AVBuffer> buffer = nullptr;
        if (consumer_->AcquireBuffer(buffer) == 0 && buffer != nullptr) {
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

void MediaMuxer::Track::OnBufferAvailable(std::shared_ptr<AVBufferQueue>& outBuffer)
{
    SetBufferAvailable(true);
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
        if (trackDesc->Find(key) == trackDesc->end()) {
            ret = false;
            AVCODEC_LOGE("the Meta Key %{public}s not contained", key.data());
        }
    }
    return ret;
}

std::string MediaMuxer::StateConvert(State state)
{
    std::string stateInfo {};
    switch (state) {
        case State::UNINITIALIZED:
            stateInfo = "UNINITIALIZED";
            break;
        case State::INITIALIZED:
            stateInfo = "INITIALIZED";
            break;
        case State::STARTED:
            stateInfo = "STARTED";
            break;
        case State::STOPPED:
            stateInfo = "STOPPED";
            break;
        default:
            break;
    }
    return stateInfo;
}
} // Media
} // OHOS