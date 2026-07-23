/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "preprocessor_encoder.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodeclist_impl.h"
#include "media_core.h"
#include "preprocessor_format_utils.h"
#include "statistics_event_reporter.h"
#include "event_info_extented_key.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "PreprocessorEncoder"};
constexpr size_t MAX_LENGTH = 255;
constexpr size_t MAX_CALLBACK_EVENT_COUNT = 64;
static std::mutex g_interfaceMutex;
} // namespace

namespace OHOS {
namespace MediaAVCodec {

} // namespace MediaAVCodec
} // namespace OHOS

namespace OHOS {
namespace MediaAVCodec {

class PreprocessorCallbackDispatcher {
public:
    using Task = std::function<void()>;

    explicit PreprocessorCallbackDispatcher(size_t maxEventCount)
        : core_(std::make_shared<Core>(maxEventCount)), worker_(&PreprocessorCallbackDispatcher::WorkerLoop, core_)
    {
    }

    ~PreprocessorCallbackDispatcher()
    {
        Shutdown();
        if (!worker_.joinable()) {
            return;
        }
        if (worker_.get_id() == std::this_thread::get_id()) {
            worker_.detach();
            return;
        }
        worker_.join();
    }

    void OpenGate()
    {
        std::lock_guard<std::mutex> lock(core_->mutex);
        if (core_->stopping) {
            return;
        }
        ++core_->epoch;
        core_->accepting = true;
    }

    void CloseGate()
    {
        std::deque<Event> dropped;
        {
            std::lock_guard<std::mutex> lock(core_->mutex);
            core_->accepting = false;
            ++core_->epoch;
            dropped.swap(core_->queue);
        }
    }

    void TryPost(Task task, Task onQueueFull = nullptr)
    {
        Task dropTask;
        {
            std::lock_guard<std::mutex> lock(core_->mutex);
            if (core_->stopping || !core_->accepting) {
                return;
            }
            if (core_->queue.size() >= core_->maxEventCount) {
                dropTask = std::move(onQueueFull);
            } else {
                core_->queue.push_back(Event{core_->epoch, std::move(task)});
                core_->cv.notify_one();
                return;
            }
        }
        if (dropTask) {
            dropTask();
        }
    }

    void Shutdown()
    {
        std::deque<Event> dropped;
        {
            std::lock_guard<std::mutex> lock(core_->mutex);
            if (core_->stopping) {
                return;
            }
            core_->accepting = false;
            core_->stopping = true;
            ++core_->epoch;
            dropped.swap(core_->queue);
        }
        core_->cv.notify_all();
    }

    void JoinWorker()
    {
        if (worker_.joinable() && worker_.get_id() != std::this_thread::get_id()) {
            worker_.join();
        }
    }

    void WaitForCallbackFinished() const
    {
        std::unique_lock<std::mutex> lock(core_->mutex);
        core_->callbackFinishedCv.wait(lock, [core = core_]() { return !core->callbackExecuting; });
    }

    void WaitForCallbackFinishedIfNeeded() const
    {
        if (worker_.joinable() && worker_.get_id() == std::this_thread::get_id()) {
            return;
        }
        WaitForCallbackFinished();
    }

    void WaitWorkerExit() const
    {
        std::unique_lock<std::mutex> lock(core_->exitMutex);
        core_->exitCv.wait(lock, [core = core_]() { return core->workerExited; });
    }

private:
    struct Event {
        uint64_t epoch{0};
        Task run{};
    };

    struct Core {
        explicit Core(size_t count) : maxEventCount(count) {}

        std::mutex mutex;
        std::condition_variable cv;
        std::deque<Event> queue;
        bool accepting{false};
        bool stopping{false};
        bool callbackExecuting{false};
        uint64_t epoch{0};
        const size_t maxEventCount;
        std::condition_variable callbackFinishedCv;

        std::mutex exitMutex;
        std::condition_variable exitCv;
        bool workerExited{false};
    };

    static void WorkerLoop(const std::shared_ptr<Core>& core)
    {
        pthread_setname_np(pthread_self(), "OS_Preproc_CB");
        for (;;) {
            Event event;
            bool execute = false;
            {
                std::unique_lock<std::mutex> lock(core->mutex);
                core->cv.wait(lock, [core]() { return core->stopping || !core->queue.empty(); });
                if (core->stopping && core->queue.empty()) {
                    break;
                }
                event = std::move(core->queue.front());
                core->queue.pop_front();
                execute = core->accepting && event.epoch == core->epoch && event.run;
                core->callbackExecuting = execute;
            }
            if (execute) {
                event.run();
                {
                    std::lock_guard<std::mutex> lock(core->mutex);
                    core->callbackExecuting = false;
                }
                core->callbackFinishedCv.notify_all();
            }
        }
        {
            std::lock_guard<std::mutex> lock(core->exitMutex);
            core->workerExited = true;
        }
        core->exitCv.notify_all();
    }

    std::shared_ptr<Core> core_;
    std::thread worker_;
};

bool PreprocessorEncoder::IsPrimaryEncoderMagic(AVMagic magic)
{
    return magic == AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER;
}

bool PreprocessorEncoder::IsSecondaryEncoderMagic(AVMagic magic)
{
    return magic == AVMagic::AVCODEC_MAGIC_SECONDARY_VIDEO_ENCODER;
}

bool PreprocessorEncoder::IsPreprocEncoderMagic(AVMagic magic)
{
    return IsPrimaryEncoderMagic(magic) || IsSecondaryEncoderMagic(magic);
}

bool PreprocessorEncoder::IsValidPreprocEncoderMime(const char *mime)
{
    CHECK_AND_RETURN_RET_LOG(mime != nullptr, false, "Mime is nullptr!");
    size_t len = strnlen(mime, MAX_LENGTH);
    CHECK_AND_RETURN_RET_LOG(len > 0 && len < MAX_LENGTH, false, "Mime is too long!");
    std::string mimeStr(mime, len);
    AVCodecListImpl codecList;
    CHECK_AND_RETURN_RET_LOG(codecList.Init() == AVCS_ERR_OK, false, "Init AVCodecList failed!");
    CapabilityData *cap = codecList.GetCapability(mimeStr, true, AVCodecCategory::AVCODEC_NONE);
    CHECK_AND_RETURN_RET_LOG(cap != nullptr, false, "Video encoder capability is not found: %{public}s", mime);
    return cap->codecType == static_cast<int32_t>(AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER) && cap->mimeType == mimeStr;
}

std::string PreprocessorEncoder::GenerateEncoderId(const std::string &prefix)
{
    static std::atomic<uint64_t> counter{0};
    uint64_t id = counter.fetch_add(1);
    return prefix + "_" + std::to_string(id);
}

PreprocessorEncoder::PreprocessorEncoder(AVMagic magic) : OH_AVCodec(magic)
{
    AVCODEC_LOGI("PreprocessorEncoder created");
}

PreprocessorEncoder::~PreprocessorEncoder()
{
    ShutdownCallbackDispatcher();
    if (callbackDispatcher_) {
        callbackDispatcher_->JoinWorker();
    }
    DetachPeerEncoder();
    UnregisterEncoderFromManager();
    AVCODEC_LOGI("PreprocessorEncoder destroyed");
}

OH_AVErrCode PreprocessorEncoder::CreatePrimary(const char *mime, OH_AVCodec **codec)
{
    CHECK_AND_RETURN_RET_LOG(codec != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr! It must not be nullptr!");
    CHECK_AND_RETURN_RET_LOG(*codec == nullptr, AV_ERR_INVALID_VAL,
        "The pointer stored in codec is not nullptr! It must be nullptr!");
    CHECK_AND_RETURN_RET_LOG(IsValidPreprocEncoderMime(mime), AV_ERR_INVALID_VAL,
        "Invalid or unsupported video encoder mime!");
    auto encoder = std::make_unique<PreprocessorEncoder>(AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER);
    CHECK_AND_RETURN_RET_LOG(encoder != nullptr, AV_ERR_NO_MEMORY, "PreprocessorEncoder create failed!");

    OH_AVErrCode ret = encoder->InitAsPrimary(mime);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "InitAsPrimary failed!");

    Media::Meta meta;
    meta.SetData(Media::Tag::AV_CODEC_CALLER_PID, getprocpid());
    meta.SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "PreprocEnc");
    StatisticsEventReporter::Report(StatisticsEventType::ADVANCED_FEATURE_INFO, meta);

    std::scoped_lock interfaceLock(g_interfaceMutex);
    CHECK_AND_RETURN_RET_LOG(*codec == nullptr, AV_ERR_INVALID_VAL,
        "The pointer stored in codec is not nullptr! It must be nullptr!");
    *codec = encoder.release();
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::CreateSecondary(OH_AVCodec *primary, OH_AVCodec **secondary)
{
    std::scoped_lock interfaceLock(g_interfaceMutex);
    CHECK_AND_RETURN_RET_LOG(primary != nullptr, AV_ERR_INVALID_VAL, "Primary is nullptr!");
    CHECK_AND_RETURN_RET_LOG(secondary != nullptr, AV_ERR_INVALID_VAL, "Codec is nullptr! It must not be nullptr!");
    CHECK_AND_RETURN_RET_LOG(*secondary == nullptr, AV_ERR_INVALID_VAL,
        "The pointer stored in codec is not nullptr! It must be nullptr!");

    CHECK_AND_RETURN_RET_LOG(IsPrimaryEncoderMagic(primary->magic_), AV_ERR_INVALID_VAL, "Primary magic error!");
    auto *primaryEnc = reinterpret_cast<PreprocessorEncoder *>(primary);

    std::scoped_lock primaryLock(primaryEnc->instanceMutex_);
    CHECK_AND_RETURN_RET_LOG(primaryEnc->secondary_ == nullptr, AV_ERR_OPERATE_NOT_PERMIT,
        "Secondary encoder already exists!");

    auto encoder = std::make_unique<PreprocessorEncoder>(AVMagic::AVCODEC_MAGIC_SECONDARY_VIDEO_ENCODER);
    CHECK_AND_RETURN_RET_LOG(encoder != nullptr, AV_ERR_NO_MEMORY, "PreprocessorEncoder create failed!");

    OH_AVErrCode ret = encoder->InitAsSecondary(primaryEnc);
    CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, ret, "InitAsSecondary failed!");

    Media::Meta meta;
    meta.SetData(Media::Tag::AV_CODEC_CALLER_PID, getprocpid());
    meta.SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "OneInDualOut");
    StatisticsEventReporter::Report(StatisticsEventType::ADVANCED_FEATURE_INFO, meta);

    *secondary = encoder.release();
    return AV_ERR_OK;
}

void PreprocessorEncoder::Destroy(OH_AVCodec* codec)
{
    auto* preprocEnc = reinterpret_cast<PreprocessorEncoder*>(codec);
    if (preprocEnc == nullptr) {
        return;
    }

    std::vector<PreprocessorEncoder*> encodersToDestroy;
    std::vector<std::shared_ptr<PreprocessorCallbackDispatcher>> dispatchers;
    std::shared_ptr<PreprocessorCallbackDispatcher> existingDispatcher;
    bool isDestroying = false;
    {
        std::scoped_lock interfaceLock(g_interfaceMutex);
        isDestroying = preprocEnc->destroying_.exchange(true);
        if (isDestroying) {
            existingDispatcher = preprocEnc->callbackDispatcher_;
        } else {
            preprocEnc->CollectDestroyTargets(encodersToDestroy, dispatchers);
            for (auto* encoder : encodersToDestroy) {
                encoder->magic_ = AVMagic::AVCODEC_MAGIC_INVALID_ENTITY;
            }
        }
    }
    if (isDestroying) {
        if (existingDispatcher) {
            existingDispatcher->WaitForCallbackFinished();
            existingDispatcher->WaitWorkerExit();
        }
        return;
    }

    for (auto* encoder : encodersToDestroy) {
        encoder->PrepareCallbackShutdown();
        encoder->ShutdownCallbackDispatcher();
    }
    for (const auto& dispatcher : dispatchers) {
        if (dispatcher) {
            dispatcher->WaitForCallbackFinished();
        }
    }
    for (const auto& dispatcher : dispatchers) {
        if (dispatcher) {
            dispatcher->JoinWorker();
        }
    }
    for (auto* encoder : encodersToDestroy) {
        (void)encoder->Release();
    }
    for (auto iter = encodersToDestroy.rbegin(); iter != encodersToDestroy.rend(); ++iter) {
        delete *iter;
    }
}

OH_AVErrCode PreprocessorEncoder::InitAsPrimary(const char *mime)
{
    isPrimary_ = true;
    mime_ = mime;
    encoderId_ = GenerateEncoderId("primary");

    callbackDispatcher_ = std::make_shared<PreprocessorCallbackDispatcher>(MAX_CALLBACK_EVENT_COUNT);
    CHECK_AND_RETURN_RET_LOG(callbackDispatcher_ != nullptr, AV_ERR_NO_MEMORY,
        "CallbackDispatcher create failed!");

    auto videoEncoder = VideoEncoderFactory::CreateByMime(mime_);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, AV_ERR_NO_MEMORY, "VideoEncoder create failed!");

    videoEncoderObject_ = std::make_shared<VideoEncoderObject>(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(videoEncoderObject_ != nullptr, AV_ERR_NO_MEMORY, "VideoEncoderObject create failed!");

    preprocMgr_ = std::make_shared<PreprocessorManager>();
    CHECK_AND_RETURN_RET_LOG(preprocMgr_ != nullptr, AV_ERR_NO_MEMORY, "PreprocessorManager create failed!");

    preprocMgr_->CreateSharedSurface();

    auto preprocessor = std::make_shared<Preprocessor>(mime);
    CHECK_AND_RETURN_RET_LOG(preprocessor != nullptr, AV_ERR_NO_MEMORY, "Preprocessor create failed!");
    preprocessors_[encoderId_] = preprocessor;

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::InitAsSecondary(PreprocessorEncoder *primary)
{
    isPrimary_ = false;
    mime_ = primary->mime_;
    encoderId_ = GenerateEncoderId("secondary");

    callbackDispatcher_ = std::make_shared<PreprocessorCallbackDispatcher>(MAX_CALLBACK_EVENT_COUNT);
    CHECK_AND_RETURN_RET_LOG(callbackDispatcher_ != nullptr, AV_ERR_NO_MEMORY,
        "CallbackDispatcher create failed!");

    auto videoEncoder = VideoEncoderFactory::CreateByMime(mime_);
    CHECK_AND_RETURN_RET_LOG(videoEncoder != nullptr, AV_ERR_NO_MEMORY, "VideoEncoder create failed!");

    videoEncoderObject_ = std::make_shared<VideoEncoderObject>(videoEncoder);
    CHECK_AND_RETURN_RET_LOG(videoEncoderObject_ != nullptr, AV_ERR_NO_MEMORY, "VideoEncoderObject create failed!");

    auto preprocessor = std::make_shared<Preprocessor>(mime_);
    CHECK_AND_RETURN_RET_LOG(preprocessor != nullptr, AV_ERR_NO_MEMORY, "Preprocessor create failed!");
    preprocessors_[encoderId_] = preprocessor;

    preprocMgr_ = primary->preprocMgr_;
    primary_ = primary;
    primary_->secondary_ = this;

    return AV_ERR_OK;
}

void PreprocessorEncoder::CreateSharedSurface()
{
    if (preprocMgr_) {
        preprocMgr_->CreateSharedSurface();
    }
}

sptr<Surface> PreprocessorEncoder::GetSharedSurface()
{
    if (preprocMgr_) {
        return preprocMgr_->GetSharedSurface();
    }
    return nullptr;
}

void PreprocessorEncoder::RegisterEncoderToManager()
{
    if (preprocMgr_) {
        auto preprocessor = preprocessors_[encoderId_];
        auto streamChangedCb = [this](const Media::Format& format) {
            OnStreamChanged(format);
        };
        auto errorCb = [this](int32_t errorCode) {
            OnPreprocessError(errorCode);
        };
        preprocMgr_->RegisterEncoder(encoderId_, preprocessor, encoderSurface_, streamChangedCb, errorCb);
    }
}

void PreprocessorEncoder::UnregisterEncoderFromManager()
{
    if (preprocMgr_) {
        preprocMgr_->UnregisterEncoder(encoderId_);
    }
}

void PreprocessorEncoder::CollectDestroyTargets(
    std::vector<PreprocessorEncoder*>& encoders,
    std::vector<std::shared_ptr<PreprocessorCallbackDispatcher>>& dispatchers)
{
    encoders.push_back(this);
    if (callbackDispatcher_) {
        dispatchers.push_back(callbackDispatcher_);
    }
    if (!isPrimary_) {
        DetachFromPrimaryEncoder();
        return;
    }

    auto* secondary = secondary_;
    secondary_ = nullptr;
    if (secondary == nullptr || secondary->primary_ != this) {
        return;
    }
    secondary->primary_ = nullptr;
    if (secondary->destroying_.exchange(true)) {
        return;
    }
    encoders.push_back(secondary);
    if (secondary->callbackDispatcher_) {
        dispatchers.push_back(secondary->callbackDispatcher_);
    }
}

void PreprocessorEncoder::DetachPeerEncoder()
{
    if (isPrimary_) {
        auto *secondary = secondary_;
        secondary_ = nullptr;
        if (secondary != nullptr && secondary->primary_ == this) {
            secondary->primary_ = nullptr;
        }
        return;
    }

    DetachFromPrimaryEncoder();
}

void PreprocessorEncoder::DetachFromPrimaryEncoder()
{
    auto *primary = primary_;
    primary_ = nullptr;
    if (primary != nullptr && primary->secondary_ == this) {
        primary->secondary_ = nullptr;
    }
}

OH_AVErrCode PreprocessorEncoder::RegisterCallback(struct OH_AVCodecCallback callback, void *userData)
{
    std::scoped_lock lock(instanceMutex_);
    {
        std::unique_lock<std::shared_mutex> callbackLock(callbackMutex_);
        callback_ = callback;
        userData_ = userData;
    }

    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        if (videoEncoderObject_->callback_ != nullptr) {
            videoEncoderObject_->callback_->UpdateCallback(callback, userData);
        } else {
            videoEncoderObject_->callback_ = std::make_shared<NativeVideoEncoderCallback>(this, callback, userData);
        }
        int32_t ret = videoEncoderObject_->videoEncoder_->SetCallback(
            std::static_pointer_cast<MediaCodecCallback>(videoEncoderObject_->callback_));
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Video encoder register callback failed!");
    }
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::Configure(const Media::Format &format)
{
    std::scoped_lock lock(instanceMutex_);

    auto ret = ConfigureVideoEncoder(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Configure video encoder failed!");

    ret = ConfigurePreprocessor(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "Configure preprocessor failed!");

    if (encoderSurface_ == nullptr) {
        ret = CreateEncoderSurface();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AV_ERR_OPERATE_NOT_PERMIT, "Create internal surface failed!");
    }
    RegisterEncoderToManager();

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::SetParameter(const Media::Format &format)
{
    std::scoped_lock lock(instanceMutex_);
    auto preprocessor = GetPreprocessor();
    if (!preprocessor) {
        return AV_ERR_INVALID_VAL;
    }

    int32_t ret = AVCS_ERR_OK;
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        ret = videoEncoderObject_->videoEncoder_->SetParameter(format);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "PreprocessorEncoder set parameter failed!");
    }

    ret = preprocessor->SetParameter(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                             "PreprocessorEncoder set preprocessor parameter failed!");

    return AV_ERR_OK;
}

sptr<Surface> PreprocessorEncoder::GetSurface()
{
    std::scoped_lock lock(instanceMutex_);
    return GetSharedSurface();
}

int32_t PreprocessorEncoder::ConfigureVideoEncoder(const Media::Format &format)
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t bitrateMode = -1;
        bool bitrateModeExist = format.GetIntValue(OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, bitrateMode);
        if (bitrateModeExist && bitrateMode == SQR) {
            int64_t bitrate;
            int64_t maxBitrate;
            bool bitrateExist = format.GetLongValue(OH_MD_KEY_BITRATE, bitrate);
            bool maxBitrateExist = format.GetLongValue(OH_MD_KEY_MAX_BITRATE, maxBitrate);
            if (bitrateExist && !maxBitrateExist) {
                AVCODEC_LOGW("In SQR bitrate mode, param %{public}s is not set, param %{public}s will be used instead",
                             OH_MD_KEY_MAX_BITRATE, OH_MD_KEY_BITRATE);
                Media::Format formatTemp = format;
                formatTemp.PutLongValue(OH_MD_KEY_MAX_BITRATE, bitrate);
                return videoEncoderObject_->videoEncoder_->Configure(formatTemp);
            }
        }
        int32_t ret = videoEncoderObject_->videoEncoder_->Configure(format);
        return ret;
    }
    return AVCS_ERR_OK;
}

int32_t PreprocessorEncoder::ConfigurePreprocessor(const Media::Format &format)
{
    auto preprocessor = GetPreprocessor();
    if (preprocessor) {
        return preprocessor->Configure(format);
    }
    return AVCS_ERR_OK;
}

int32_t PreprocessorEncoder::CreateEncoderSurface()
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        auto surface = videoEncoderObject_->videoEncoder_->CreateInputSurface();
        if (surface == nullptr && encoderSurface_ == nullptr) {
            return AVCS_ERR_NO_MEMORY;
        }

        if (surface != nullptr) {
            encoderSurface_ = surface;
        }
    }

    return AVCS_ERR_OK;
}

OHOS::sptr<OH_AVBuffer> PreprocessorEncoder::GetTransData(const uint32_t& index,
                                                            std::shared_ptr<AVBuffer>& buffer,
                                                            bool isOutput)
{
    CHECK_AND_RETURN_RET_LOG(videoEncoderObject_ && videoEncoderObject_->videoEncoder_, nullptr,
                             "Context video encoder is nullptr!");
    OHOS::sptr<OH_AVBuffer> object;
    videoEncoderObject_->GetTransData(index, buffer, isOutput, object);
    return object;
}


std::shared_ptr<Preprocessor> PreprocessorEncoder::GetPreprocessor()
{
    auto it = preprocessors_.find(encoderId_);
    if (it != preprocessors_.end()) {
        return it->second;
    }
    return nullptr;
}

OH_AVErrCode PreprocessorEncoder::Start()
{
    std::scoped_lock lock(instanceMutex_);
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->Start();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Video encoder start failed!");
    }
    auto preprocessor = GetPreprocessor();
    if (preprocessor) {
        preprocessor->FlushTimeStamp();
    }
    if (callbackDispatcher_) {
        callbackDispatcher_->OpenGate();
    }
    if (preprocMgr_) {
        int32_t ret = preprocMgr_->SetEncoderRunning(encoderId_, true);
        if (ret != AVCS_ERR_OK) {
            CloseCallbackGate();
            if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
                (void)videoEncoderObject_->videoEncoder_->Stop();
                videoEncoderObject_->ClearBufferList();
            }
            return AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
        }
    }
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::Stop()
{
    CloseAndWaitCallbackIfNeeded();
    std::scoped_lock lock(instanceMutex_);
    if (preprocMgr_) {
        preprocMgr_->NotifyEosNowIfPending(encoderId_);
        (void)preprocMgr_->SetEncoderRunning(encoderId_, false);
    }
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->Stop();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Video encoder stop failed!");
        videoEncoderObject_->ClearBufferList();
    }
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::Flush()
{
    CloseAndWaitCallbackIfNeeded();
    std::scoped_lock lock(instanceMutex_);
    if (preprocMgr_) {
        preprocMgr_->NotifyEosNowIfPending(encoderId_);
        (void)preprocMgr_->SetEncoderRunning(encoderId_, false);
    }
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->Flush();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Video encoder flush failed!");
        videoEncoderObject_->ClearBufferList();
    }
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::Reset()
{
    CloseAndWaitCallbackIfNeeded();
    std::scoped_lock lock(instanceMutex_);
    if (preprocMgr_) {
        preprocMgr_->NotifyEosNowIfPending(encoderId_);
        (void)preprocMgr_->SetEncoderRunning(encoderId_, false);
    }
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->Reset();
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Video encoder reset failed!");
        videoEncoderObject_->ClearBufferList();
    }
    if (preprocMgr_) {
        UnregisterEncoderFromManager();
    }
    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::Release()
{
    std::scoped_lock lock(instanceMutex_);
    CloseCallbackGate();
    if (preprocMgr_) {
        (void)preprocMgr_->SetEncoderRunning(encoderId_, false);
    }

    OH_AVErrCode result = AV_ERR_OK;
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        // Stop the fixed Native callback bridge before releasing the underlying encoder.
        videoEncoderObject_->StopCallback();
        int32_t ret = videoEncoderObject_->videoEncoder_->Release();
        videoEncoderObject_->ClearBufferList();
        if (ret != AVCS_ERR_OK) {
            result = AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret));
            AVCODEC_LOGE("PreprocessorEncoder release failed!");
        }
        videoEncoderObject_.reset();
    }

    return result;
}

OH_AVErrCode PreprocessorEncoder::NotifyEos()
{
    std::scoped_lock lock(instanceMutex_);
    CHECK_AND_RETURN_RET_LOG(preprocMgr_, AV_ERR_OPERATE_NOT_PERMIT, "PreprocessorManager is not found");
    if (preprocMgr_) {
        std::shared_ptr<VideoEncoderObject> videoEncoderObject = videoEncoderObject_;
        std::string encoderId = encoderId_;
        bool requestRet = preprocMgr_->RequestNotifyEos(encoderId_, [videoEncoderObject, encoderId]() {
            if (!(videoEncoderObject && videoEncoderObject->videoEncoder_)) {
                AVCODEC_LOGE("Video encoder is nullptr when async notify eos: %{public}s", encoderId.c_str());
                return;
            }
            int32_t ret = videoEncoderObject->videoEncoder_->NotifyEos();
            if (ret != AV_ERR_OK) {
                AVCODEC_LOGE("Async notify eos failed for %{public}s, ret=%{public}d", encoderId.c_str(), ret);
            }
        });
        CHECK_AND_RETURN_RET_LOG(requestRet, AV_ERR_INVALID_STATE, "Request async notify eos failed!");
    }

    return AV_ERR_OK;
}

OH_AVErrCode PreprocessorEncoder::ReleaseOutputBuffer(uint32_t index)
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        int32_t ret = videoEncoderObject_->videoEncoder_->ReleaseOutputBuffer(index);
        CHECK_AND_RETURN_RET_LOG(ret == AV_ERR_OK, AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(ret)),
                                 "Preprocessor release output buffer failed!");
    }
    return AV_ERR_OK;
}

PreprocessorEncoder::CallbackSnapshot PreprocessorEncoder::GetCallbackSnapshot() const
{
    std::shared_lock<std::shared_mutex> lock(callbackMutex_);
    return {callback_, userData_};
}

void PreprocessorEncoder::CloseCallbackGate()
{
    if (callbackDispatcher_) {
        callbackDispatcher_->CloseGate();
    }
}

void PreprocessorEncoder::CloseAndWaitCallbackIfNeeded()
{
    auto dispatcher = callbackDispatcher_;
    if (dispatcher) {
        dispatcher->CloseGate();
        dispatcher->WaitForCallbackFinishedIfNeeded();
    }
}

void PreprocessorEncoder::PrepareCallbackShutdown()
{
    std::scoped_lock lock(instanceMutex_);
    CloseCallbackGate();
    if (preprocMgr_) {
        (void)preprocMgr_->SetEncoderRunning(encoderId_, false);
    }
    if (videoEncoderObject_) {
        videoEncoderObject_->StopCallback();
    }
}

void PreprocessorEncoder::ShutdownCallbackDispatcher()
{
    if (callbackDispatcher_) {
        callbackDispatcher_->Shutdown();
    }
}

void PreprocessorEncoder::OnStreamChanged(const Media::Format& format)
{
    CallbackSnapshot snapshot = GetCallbackSnapshot();
    if (snapshot.callback.onStreamChanged == nullptr || !callbackDispatcher_) {
        return;
    }

    OHOS::sptr<OH_AVFormat> object = new (std::nothrow) OH_AVFormat(format);
    if (object == nullptr) {
        AVCODEC_LOGE("OH_AVFormat create failed in OnStreamChanged");
        return;
    }
    auto* codec = static_cast<OH_AVCodec*>(this);
    callbackDispatcher_->TryPost([codec, snapshot, object]() {
        snapshot.callback.onStreamChanged(
            codec, reinterpret_cast<OH_AVFormat*>(object.GetRefPtr()), snapshot.userData);
    });
}

void PreprocessorEncoder::PostError(int32_t errorCode)
{
    CallbackSnapshot snapshot = GetCallbackSnapshot();
    if (snapshot.callback.onError == nullptr || !callbackDispatcher_) {
        return;
    }
    OH_AVErrCode avErrorCode = AVCSErrorToOHAVErrCode(static_cast<AVCodecServiceErrCode>(errorCode));
    auto* codec = static_cast<OH_AVCodec*>(this);
    callbackDispatcher_->TryPost([codec, snapshot, avErrorCode]() {
        snapshot.callback.onError(codec, static_cast<int32_t>(avErrorCode), snapshot.userData);
    });
}

void PreprocessorEncoder::OnPreprocessError(int32_t errorCode)
{
    PostError(errorCode);
}

void PreprocessorEncoder::OnEncoderError(int32_t errorCode)
{
    PostError(errorCode);
}

void PreprocessorEncoder::OnOutputBufferAvailable(uint32_t index, const std::shared_ptr<AVBuffer>& buffer)
{
    CallbackSnapshot snapshot = GetCallbackSnapshot();
    if (snapshot.callback.onNewOutputBuffer == nullptr || !callbackDispatcher_ || !buffer) {
        return;
    }

    auto bufferCopy = buffer;
    OHOS::sptr<OH_AVBuffer> object = GetTransData(index, bufferCopy, true);
    if (object == nullptr) {
        AVCODEC_LOGE("OH_AVBuffer transform failed in OnOutputBufferAvailable");
        return;
    }
    auto* codec = static_cast<OH_AVCodec*>(this);
    std::weak_ptr<VideoEncoderObject> weakVideoEncoderObject = videoEncoderObject_;
    callbackDispatcher_->TryPost([codec, snapshot, index, object]() {
        snapshot.callback.onNewOutputBuffer(
            codec, index, reinterpret_cast<OH_AVBuffer*>(object.GetRefPtr()), snapshot.userData);
    }, [weakVideoEncoderObject, index]() {
        auto videoEncoderObject = weakVideoEncoderObject.lock();
        if (videoEncoderObject && videoEncoderObject->videoEncoder_) {
            (void)videoEncoderObject->videoEncoder_->ReleaseOutputBuffer(index);
        }
    });
}

OH_AVFormat* PreprocessorEncoder::GetOutputDescription()
{
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        Format format;
        int32_t ret = videoEncoderObject_->videoEncoder_->GetOutputFormat(format);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr,
                                 "PreprocessorEncoder get output description failed!");

        OH_AVFormat* avFormat = OH_AVFormat_Create();
        CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "PreprocessorEncoder create output format failed!");
        avFormat->format_ = format;
        return avFormat;
    }
    return nullptr;
}

OH_AVFormat* PreprocessorEncoder::GetInputDescription()
{
    auto preprocessor = GetPreprocessor();
    CHECK_AND_RETURN_RET_LOG(preprocessor != nullptr, nullptr, "Preprocessor not found for GetInputDescription!");

    Format format;
    // Step 1: Obtain underlying encoder InputDescription (width, height, pixel_format, stride, etc.)
    if (videoEncoderObject_ && videoEncoderObject_->videoEncoder_) {
        Format encoderFormat;
        int32_t ret = videoEncoderObject_->videoEncoder_->GetInputFormat(encoderFormat);
        if (ret == AVCS_ERR_OK) {
            format = encoderFormat;  // Copy entire encoder format as base
        }
    }
    // Step 2: Override input data fields from PreprocessorManager buffer (width, height, pixel_format, stride)
    if (preprocMgr_) {
        PreprocessorManager::BufferDataInfo bufInfo;
        if (preprocMgr_->PeekPendingBufferData(encoderId_, bufInfo)) {
            format.PutIntValue(Media::Tag::VIDEO_WIDTH, bufInfo.width);
            format.PutIntValue(Media::Tag::VIDEO_HEIGHT, bufInfo.height);
            format.PutIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, bufInfo.format);
            VideoPixelFormat mappedFmt;
            if (GraphicPixelFmtToVideoPixelFmt(static_cast<GraphicPixelFormat>(bufInfo.format), mappedFmt)) {
                format.PutIntValue(Media::Tag::VIDEO_PIXEL_FORMAT, static_cast<int32_t>(mappedFmt));
            }
            if (bufInfo.stride > 0) {
                format.PutIntValue(OHOS::Media::Tag::VIDEO_STRIDE, bufInfo.stride);
            }
            if (bufInfo.height > 0) {
                format.PutIntValue(OHOS::Media::Tag::VIDEO_SLICE_HEIGHT, bufInfo.height);
            }
        }
    }
    // Step 3: Append preprocessing metadata (always)
    auto config = preprocessor->GetConfigSnapshot();
    if (config.cropEnabled) {
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, config.cropLeft);
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, config.cropRight);
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, config.cropTop);
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, config.cropBottom);
    }
    if (config.downsamplingEnabled) {
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, config.downsamplingWidth);
        format.PutIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT, config.downsamplingHeight);
    }

    OH_AVFormat* avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "PreprocessorEncoder create input format failed!");
    avFormat->format_ = format;
    return avFormat;
}

} // namespace MediaAVCodec
} // namespace OHOS
