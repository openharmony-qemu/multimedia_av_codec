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

#include "muxer_engine_impl.h"
#include <set>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "securec.h"
#include "avcodec_log.h"
#include "muxer_factory.h"
#include "avcodec_trace.h"
#include "avcodec_sysevent.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "avcodec_dump_utils.h"
#include "buffer/avsharedmemorybase.h"
#include "data_sink_fd.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MuxerEngineImpl"};
    constexpr int32_t ERR_TRACK_INDEX = -1;
    constexpr uint32_t DUMP_MUXER_INFO_INDEX = 0x01010000;
    constexpr uint32_t DUMP_STATUS_INDEX = 0x01010100;
    constexpr uint32_t DUMP_OUTPUT_FORMAT_INDEX = 0x01010200;
    constexpr uint32_t DUMP_OFFSET_8 = 8;

    const std::map<OHOS::MediaAVCodec::OutputFormat, const std::string> OutputFormatStringMap = {
        { OHOS::MediaAVCodec::OutputFormat::OUTPUT_FORMAT_M4A, "m4a" },
        { OHOS::MediaAVCodec::OutputFormat::OUTPUT_FORMAT_MPEG_4, "mp4" },
    };

    const std::vector<std::pair<std::string_view, const std::string>> AUDIO_DUMP_TABLE = {
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME, "Codec_Mime" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, "Channel_Count" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_SAMPLE_RATE, "Sample_Rate" },
    };

    const std::vector<std::pair<std::string_view, const std::string>> VIDEO_DUMP_TABLE = {
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME, "Codec_Mime" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_WIDTH, "Width" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_HEIGHT, "Height" },
    };

    const std::vector<std::pair<std::string_view, const std::string>> IMAGE_DUMP_TABLE = {
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME, "Codec_Mime" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_WIDTH, "Width" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_HEIGHT, "Height" },
    };

    const std::map<OHOS::MediaAVCodec::MuxerEngineImpl::TrackMimeType,
        std::vector<std::pair<std::string_view, const std::string>>> MUXER_DUMP_TABLE = {
        { OHOS::MediaAVCodec::MuxerEngineImpl::TrackMimeType::TRACK_MIME_TYPE_AUDIO, AUDIO_DUMP_TABLE },
        { OHOS::MediaAVCodec::MuxerEngineImpl::TrackMimeType::TRACK_MIME_TYPE_VIDEO, VIDEO_DUMP_TABLE },
        { OHOS::MediaAVCodec::MuxerEngineImpl::TrackMimeType::TRACK_MIME_TYPE_IMAGE, IMAGE_DUMP_TABLE },
    };
}

namespace OHOS {
namespace MediaAVCodec {
const std::map<uint32_t, std::set<std::string_view>> MUX_FORMAT_INFO = {
    {OUTPUT_FORMAT_MPEG_4, {CodecMimeType::AUDIO_MPEG, CodecMimeType::AUDIO_AAC,
                            CodecMimeType::VIDEO_AVC, CodecMimeType::VIDEO_MPEG4, CodecMimeType::VIDEO_HEVC,
                            CodecMimeType::IMAGE_JPG, CodecMimeType::IMAGE_PNG, CodecMimeType::IMAGE_BMP}},
    {OUTPUT_FORMAT_M4A, {CodecMimeType::AUDIO_AAC,
                         CodecMimeType::VIDEO_AVC, CodecMimeType::VIDEO_MPEG4,
                         CodecMimeType::IMAGE_JPG, CodecMimeType::IMAGE_PNG, CodecMimeType::IMAGE_BMP}},
};

const std::map<std::string_view, std::set<std::string_view>> MUX_MIME_INFO = {
    {CodecMimeType::AUDIO_MPEG, {MediaDescriptionKey::MD_KEY_SAMPLE_RATE, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT}},
    {CodecMimeType::AUDIO_AAC, {MediaDescriptionKey::MD_KEY_SAMPLE_RATE, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT}},
    {CodecMimeType::VIDEO_AVC, {MediaDescriptionKey::MD_KEY_WIDTH, MediaDescriptionKey::MD_KEY_HEIGHT}},
    {CodecMimeType::VIDEO_MPEG4, {MediaDescriptionKey::MD_KEY_WIDTH, MediaDescriptionKey::MD_KEY_HEIGHT}},
    {CodecMimeType::VIDEO_HEVC, {MediaDescriptionKey::MD_KEY_WIDTH, MediaDescriptionKey::MD_KEY_HEIGHT}},
    {CodecMimeType::IMAGE_JPG, {MediaDescriptionKey::MD_KEY_WIDTH, MediaDescriptionKey::MD_KEY_HEIGHT}},
    {CodecMimeType::IMAGE_PNG, {MediaDescriptionKey::MD_KEY_WIDTH, MediaDescriptionKey::MD_KEY_HEIGHT}},
    {CodecMimeType::IMAGE_BMP, {MediaDescriptionKey::MD_KEY_WIDTH, MediaDescriptionKey::MD_KEY_HEIGHT}},
};

std::shared_ptr<IMuxerEngine> IMuxerEngineFactory::CreateMuxerEngine(
    int32_t appUid, int32_t appPid, int32_t fd, OutputFormat format)
{
    AVCodecTrace trace("IMuxerEngineFactory::CreateMuxerEngine");
    CHECK_AND_RETURN_RET_LOG(fd >= 0, nullptr, "fd %{public}d is error!", fd);
    uint32_t fdPermission = static_cast<uint32_t>(fcntl(fd, F_GETFL, 0));
    CHECK_AND_RETURN_RET_LOG((fdPermission & O_RDWR) == O_RDWR, nullptr, "no permission to read and write fd");
    CHECK_AND_RETURN_RET_LOG(lseek(fd, 0, SEEK_CUR) != -1, nullptr, "the fd is not seekable");
    std::shared_ptr<MuxerEngineImpl> muxerEngineImpl = std::make_shared<MuxerEngineImpl>(appUid, appPid, fd, format);
    int32_t ret = muxerEngineImpl->Init();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init muxer engine implementation failed");
    return muxerEngineImpl;
}

MuxerEngineImpl::MuxerEngineImpl(int32_t appUid, int32_t appPid, int32_t fd, OutputFormat format)
    : appUid_(appUid), appPid_(appPid), fd_(fd), format_(format), que_("muxer_write_q"), pool_("muxer_buffer_p")
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MuxerEngineImpl::~MuxerEngineImpl()
{
    AVCODEC_LOGD("Destroy");

    if (state_ == State::STARTED) {
        que_.SetActive(false);
        StopThread();
    }

    appUid_ = -1;
    appPid_ = -1;
    muxer_ = nullptr;
    tracksDesc_.clear();
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t MuxerEngineImpl::Init()
{
    AVCodecTrace trace("MuxerEngine::Init");
    format_ = (format_ == OUTPUT_FORMAT_DEFAULT) ? OUTPUT_FORMAT_MPEG_4 : format_;
    muxer_ = Plugin::MuxerFactory::Instance().CreatePlugin(format_);
    if (muxer_ != nullptr) {
        state_ = State::INITIALIZED;
        AVCODEC_LOGI("state_ is INITIALIZED");
    } else {
        AVCODEC_LOGE("state_ is UNINITIALIZED");
        FaultEventWrite(FaultType::FAULT_TYPE_INNER_ERROR, AVCSErrorToString(AVCS_ERR_INVALID_STATE), "Muxer");
    }
    CHECK_AND_RETURN_RET_LOG(state_ == State::INITIALIZED, AVCS_ERR_INVALID_STATE, "state_ is UNINITIALIZED");
    if (fd_ >= 0) {
        Plugin::Status ret = muxer_->SetDataSink(std::make_shared<Plugin::DataSinkFd>(fd_));
        CHECK_AND_RETURN_RET_LOG(ret == Plugin::Status::NO_ERROR, TranslatePluginStatus(ret),
            "failed to set data sink fd");
    }
    return AVCS_ERR_OK;
}

int32_t MuxerEngineImpl::SetRotation(int32_t rotation)
{
    AVCodecTrace trace("MuxerEngine::SetRotation");
    AVCODEC_LOGI("SetRotation");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == State::INITIALIZED, AVCS_ERR_INVALID_OPERATION,
        "The state is not INITIALIZED, the interface must be called after constructor and before Start(). "
        "The current state is %{public}s", ConvertStateToString(state_).c_str());
    if (rotation != VIDEO_ROTATION_0 && rotation != VIDEO_ROTATION_90 &&
        rotation != VIDEO_ROTATION_180 && rotation != VIDEO_ROTATION_270) {
        AVCODEC_LOGW("Invalid rotation: %{public}d, keep default 0", rotation);
        return AVCS_ERR_INVALID_VAL;
    }
    Plugin::Status ret = muxer_->SetRotation(rotation);
    CHECK_AND_RETURN_RET_LOG(ret == Plugin::Status::NO_ERROR, TranslatePluginStatus(ret), "SetRotation failed");
    return AVCS_ERR_OK;
}

int32_t MuxerEngineImpl::AddTrack(int32_t &trackIndex, const MediaDescription &trackDesc)
{
    AVCodecTrace trace("MuxerEngine::AddTrack");
    AVCODEC_LOGI("AddTrack");
    std::lock_guard<std::mutex> lock(mutex_);
    trackIndex = ERR_TRACK_INDEX;
    CHECK_AND_RETURN_RET_LOG(state_ == State::INITIALIZED, AVCS_ERR_INVALID_OPERATION,
        "The state is not INITIALIZED, the interface must be called after constructor and before Start(). "
        "The current state is %{public}s", ConvertStateToString(state_).c_str());
    std::string mimeType = {};
    CHECK_AND_RETURN_RET_LOG(trackDesc.GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, mimeType),
        AVCS_ERR_INVALID_VAL, "track format does not contain mime");
    CHECK_AND_RETURN_RET_LOG(CanAddTrack(mimeType), AVCS_ERR_UNSUPPORT_FILE_TYPE,
        "track mime is unsupported: %{public}s", mimeType.c_str());
    CHECK_AND_RETURN_RET_LOG(CheckKeys(mimeType, trackDesc), AVCS_ERR_INVALID_VAL,
        "track format keys not contained");

    int32_t trackId = -1;
    Plugin::Status ret = muxer_->AddTrack(trackId, trackDesc);
    CHECK_AND_RETURN_RET_LOG(ret == Plugin::Status::NO_ERROR, TranslatePluginStatus(ret), "AddTrack failed");
    CHECK_AND_RETURN_RET_LOG(trackId >= 0, AVCS_ERR_INVALID_OPERATION,
        "The track index is greater than or equal to 0.");
    trackIndex = trackId;
    tracksDesc_.emplace(trackIndex, MediaDescription(trackDesc));

    return AVCS_ERR_OK;
}

int32_t MuxerEngineImpl::Start()
{
    AVCodecTrace trace("MuxerEngine::Start");
    AVCODEC_LOGI("Start");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == State::INITIALIZED, AVCS_ERR_INVALID_OPERATION,
        "The state is not INITIALIZED, the interface must be called after AddTrack() and before WriteSample(). "
        "The current state is %{public}s", ConvertStateToString(state_).c_str());
    CHECK_AND_RETURN_RET_LOG(tracksDesc_.size() > 0, AVCS_ERR_INVALID_OPERATION,
        "The track count is error, count is %{public}zu", tracksDesc_.size());
    Plugin::Status ret = muxer_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == Plugin::Status::NO_ERROR, TranslatePluginStatus(ret), "Start failed");
    state_ = State::STARTED;
    StartThread("OS_MuxerWrite");
    return AVCS_ERR_OK;
}

int32_t MuxerEngineImpl::WriteSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
    AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    AVCodecTrace trace("MuxerEngine::WriteSample");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(state_ == State::STARTED, AVCS_ERR_INVALID_OPERATION,
        "The state is not STARTED, the interface must be called after Start() and before Stop(). "
        "The current state is %{public}s", ConvertStateToString(state_).c_str());
    CHECK_AND_RETURN_RET_LOG(tracksDesc_.find(trackIndex) != tracksDesc_.end(), AVCS_ERR_INVALID_VAL,
        "The track index does not exist");
    CHECK_AND_RETURN_RET_LOG(sample != nullptr && info.offset >= 0 && info.size >= 0 &&
        sample->GetSize() >= (info.offset + info.size), AVCS_ERR_INVALID_VAL, "Invalid memory");

    std::shared_ptr<uint8_t> buffer = pool_.AcquireBuffer(info.size);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AVCS_ERR_NO_MEMORY, "AcquireBuffer from pool failed");
    errno_t rc = memcpy_s(buffer.get(), info.size, sample->GetBase() + info.offset, info.size);
    CHECK_AND_RETURN_RET_LOG(rc == EOK, AVCS_ERR_UNKNOWN, "memcpy_s failed");

    std::shared_ptr<BlockBuffer> blockBuffer = std::make_shared<BlockBuffer>();
    blockBuffer->trackIndex_ = trackIndex;
    blockBuffer->buffer_ = buffer;
    blockBuffer->info_ = info;
    blockBuffer->flag_ = flag;
    que_.Push(blockBuffer);

    return AVCS_ERR_OK;
}

int32_t MuxerEngineImpl::Stop()
{
    AVCodecTrace trace("MuxerEngine::Stop");
    AVCODEC_LOGI("Stop");
    std::unique_lock<std::mutex> lock(mutex_);
    if (state_ == State::STOPPED) {
        AVCODEC_LOGW("current state is STOPPED!");
        return AVCS_ERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(state_ == State::STARTED, AVCS_ERR_INVALID_OPERATION,
        "The state is not STARTED. The current state is %{public}s", ConvertStateToString(state_).c_str());
    state_ = State::STOPPED;
    que_.SetActive(false, false);
    cond_.wait(lock, [this] { return que_.Empty(); });
    StopThread();
    Plugin::Status ret = muxer_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == Plugin::Status::NO_ERROR, TranslatePluginStatus(ret), "Stop failed");
    return AVCS_ERR_OK;
}

int32_t MuxerEngineImpl::DumpInfo(int32_t fd)
{
    AVCodecDumpControler dumpControler;
    dumpControler.AddInfo(DUMP_MUXER_INFO_INDEX, "Muxer_Info");
    dumpControler.AddInfo(DUMP_STATUS_INDEX, "Status", ConvertStateToString(state_));
    dumpControler.AddInfo(DUMP_OUTPUT_FORMAT_INDEX,
        "Output_Format", OutputFormatStringMap.at(format_));

    int32_t dumpTrackIndex = 3;
    int mediaDescIdx = 0;
    for (auto it = tracksDesc_.begin(); it != tracksDesc_.end(); ++it) {
        auto mediaDesc = it->second;
        int32_t dumpInfoIndex = 1;
        std::string codecMime;
        bool ret = mediaDesc.GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, codecMime);
        CHECK_AND_CONTINUE_LOG(ret, "Get codec mime from format failed.");
        TrackMimeType mimeType = GetTrackMimeType(codecMime);
        auto &dumpTable = MUXER_DUMP_TABLE.at(mimeType);

        dumpControler.AddInfo(DUMP_MUXER_INFO_INDEX + (dumpTrackIndex << DUMP_OFFSET_8),
            std::string("Track_") + std::to_string(mediaDescIdx++) + "_Info");
        for (auto iter : dumpTable) {
            dumpControler.AddInfoFromFormat(
                DUMP_MUXER_INFO_INDEX + (dumpTrackIndex << DUMP_OFFSET_8) + dumpInfoIndex,
                mediaDesc, iter.first, iter.second);
            dumpInfoIndex++;
        }
        dumpTrackIndex++;
    }

    std::string dumpString;
    dumpControler.GetDumpString(dumpString);
    CHECK_AND_RETURN_RET_LOG(fd != -1, AVCS_ERR_INVALID_VAL, "Get a invalid fd.");
    write(fd, dumpString.c_str(), dumpString.size());

    return AVCS_ERR_OK;
}

int32_t MuxerEngineImpl::StartThread(const std::string &name)
{
    AVCodecTrace trace("MuxerEngine::StartThread");
    threadName_ = name;
    if (thread_ != nullptr) {
        AVCODEC_LOGW("Started already! [%{public}s]", threadName_.c_str());
        return AVCS_ERR_OK;
    }
    isThreadExit_ = false;
    thread_ = std::make_unique<std::thread>(&MuxerEngineImpl::ThreadProcessor, this);
    AVCodecTrace::TraceBegin("muxer_write_thread", FAKE_POINTER(thread_.get()));
    AVCODEC_LOGD("thread started! [%{public}s]", threadName_.c_str());
    return AVCS_ERR_OK;
}

int32_t MuxerEngineImpl::StopThread() noexcept
{
    if (isThreadExit_) {
        AVCODEC_LOGD("Stopped already! [%{public}s]", threadName_.c_str());
        return AVCS_ERR_OK;
    }
    if (std::this_thread::get_id() == thread_->get_id()) {
        AVCODEC_LOGD("Stop at the task thread, reject");
        return AVCS_ERR_INVALID_OPERATION;
    }

    std::unique_ptr<std::thread> t;
    isThreadExit_ = true;
    cond_.notify_all();
    std::swap(thread_, t);
    if (t != nullptr && t->joinable()) {
        t->join();
    }
    thread_ = nullptr;
    return AVCS_ERR_OK;
}

void MuxerEngineImpl::ThreadProcessor()
{
    AVCODEC_LOGD("Enter ThreadProcessor [%{public}s]", threadName_.c_str());
    constexpr uint32_t nameSizeMax = 15;
    pthread_setname_np(pthread_self(), threadName_.substr(0, nameSizeMax).c_str());
    int32_t taskId = FAKE_POINTER(thread_.get());
    for (;;) {
        AVCodecTrace trace(threadName_);
        if (isThreadExit_) {
            AVCodecTrace::TraceEnd("muxer_write_thread", taskId);
            AVCODEC_LOGD("Exit ThreadProcessor [%{public}s]", threadName_.c_str());
            return;
        }
        auto buffer = que_.Pop();
        if (buffer != nullptr) {
            (void)muxer_->WriteSample(buffer->trackIndex_, buffer->buffer_.get(), buffer->info_, buffer->flag_);
            pool_.ReleaseBuffer(buffer->buffer_);
        }
        if (que_.Empty()) {
            cond_.notify_all();
        }
    }
}

bool MuxerEngineImpl::CanAddTrack(const std::string &mimeType)
{
    auto it = MUX_FORMAT_INFO.find(format_);
    if (it == MUX_FORMAT_INFO.end()) {
        return false;
    }
    return it->second.find(mimeType) != it->second.end();
}

bool MuxerEngineImpl::CheckKeys(const std::string &mimeType, const MediaDescription &trackDesc)
{
    bool ret = true;
    auto it = MUX_MIME_INFO.find(mimeType);
    if (it == MUX_MIME_INFO.end()) {
        return ret; // 不做检查
    }

    for (auto &key : it->second) {
        if (!trackDesc.ContainKey(key)) {
            ret = false;
            AVCODEC_LOGE("the MediaDescriptionKey %{public}s not contained", key.data());
        }
    }
    return ret;
}

std::string MuxerEngineImpl::ConvertStateToString(State state)
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

int32_t MuxerEngineImpl::TranslatePluginStatus(Plugin::Status error)
{
    const static std::unordered_map<Plugin::Status, int32_t> g_transTable = {
        {Plugin::Status::END_OF_STREAM, AVCodecServiceErrCode::AVCS_ERR_OK},
        {Plugin::Status::OK, AVCodecServiceErrCode::AVCS_ERR_OK},
        {Plugin::Status::NO_ERROR, AVCodecServiceErrCode::AVCS_ERR_OK},
        {Plugin::Status::ERROR_UNKNOWN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Plugin::Status::ERROR_PLUGIN_ALREADY_EXISTS, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Plugin::Status::ERROR_INCOMPATIBLE_VERSION, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Plugin::Status::ERROR_NO_MEMORY, AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY},
        {Plugin::Status::ERROR_WRONG_STATE, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
        {Plugin::Status::ERROR_UNIMPLEMENTED, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT},
        {Plugin::Status::ERROR_INVALID_PARAMETER, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Plugin::Status::ERROR_INVALID_DATA, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Plugin::Status::ERROR_MISMATCHED_TYPE, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Plugin::Status::ERROR_TIMED_OUT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Plugin::Status::ERROR_UNSUPPORTED_FORMAT, AVCodecServiceErrCode::AVCS_ERR_UNSUPPORT_FILE_TYPE},
        {Plugin::Status::ERROR_NOT_ENOUGH_DATA, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Plugin::Status::ERROR_NOT_EXISTED, AVCodecServiceErrCode::AVCS_ERR_OPEN_FILE_FAILED},
        {Plugin::Status::ERROR_AGAIN, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Plugin::Status::ERROR_PERMISSION_DENIED, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Plugin::Status::ERROR_NULL_POINTER, AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL},
        {Plugin::Status::ERROR_INVALID_OPERATION, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION},
        {Plugin::Status::ERROR_CLIENT, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Plugin::Status::ERROR_SERVER, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN},
        {Plugin::Status::ERROR_DELAY_READY, AVCodecServiceErrCode::AVCS_ERR_OK},
    };
    auto ite = g_transTable.find(error);
    if (ite == g_transTable.end()) {
        return AVCS_ERR_UNKNOWN;
    }
    return ite->second;
}

MuxerEngineImpl::TrackMimeType MuxerEngineImpl::GetTrackMimeType(const std::string &mime)
{
    TrackMimeType type;
    if (mime.find("audio") != mime.npos) {
        type = TRACK_MIME_TYPE_AUDIO;
    } else if (mime.find("video") != mime.npos) {
        type = TRACK_MIME_TYPE_VIDEO;
    } else {
        type = TRACK_MIME_TYPE_IMAGE;
    }

    return type;
}
} // MediaAVCodec
} // OHOS