/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "codec_buffer_circular.h"
#include <sstream>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "buffer/avbuffer.h"
#include "buffer/avsharedmemory.h"
#include "buffer_converter.h"
#include "meta/format.h"
#include "meta/meta.h"

using namespace OHOS::Media;
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecBufferCircular"};
constexpr int64_t MAX_TIMEOUT = std::chrono::time_point_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::time_point::max()
    ).time_since_epoch().count() / 2;
constexpr int64_t  LOG_INTERVAL_MS = 1000;   // 1s
constexpr uint32_t LOG_MAX_COUNT   = 5;      // max 5 times in logIntervalMs
} // namespace
namespace OHOS {
namespace MediaAVCodec {
CodecBufferCircular::~CodecBufferCircular()
{
    {
        std::lock_guard<std::mutex> lock(inMutex_);
        PrintCaches(false);
    }
    {
        std::lock_guard<std::mutex> lock(outMutex_);
        PrintCaches(true);
    }
}

void CodecBufferCircular::SetConverter(std::shared_ptr<BufferConverter> &converter)
{
    converter_ = converter;
}

int32_t CodecBufferCircular::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    std::scoped_lock lock(inMutex_, outMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(CanEnableMode<MODE_ASYNC>(), AVCS_ERR_INVALID_OPERATION,
                                      "Can not enable async mode");
    EnableMode<MODE_ASYNC>();
    callback_ = callback;
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    std::scoped_lock lock(inMutex_, outMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(CanEnableMode<MODE_ASYNC>(), AVCS_ERR_INVALID_OPERATION,
                                      "Can not enable async mode");
    EnableMode<MODE_ASYNC>();
    mediaCb_ = callback;
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback)
{
    std::scoped_lock lock(inMutex_, outMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(attrCb_ == nullptr, AVCS_ERR_INVALID_STATE,
                                      "Already set parameter with atrribute callback");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(CanEnableMode<MODE_ASYNC>(), AVCS_ERR_INVALID_OPERATION,
                                      "Can not enable async mode");
    EnableMode<MODE_ASYNC>();
    paramCb_ = callback;
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback)
{
    std::scoped_lock lock(inMutex_, outMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(paramCb_ == nullptr, AVCS_ERR_INVALID_STATE, "Already set parameter callback");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(CanEnableMode<MODE_ASYNC>(), AVCS_ERR_INVALID_OPERATION,
                                      "Can not enable async mode");
    EnableMode<MODE_ASYNC>();
    attrCb_ = callback;
    return AVCS_ERR_OK;
}

void CodecBufferCircular::SetIsRunning(bool isRunning)
{
    std::scoped_lock lock(inMutex_, outMutex_);
    if (isRunning) {
        AddFlag(FLAG_IS_RUNNING);
    } else {
        RemoveFlag(FLAG_IS_RUNNING);
        RemoveFlag(FLAG_INPUT_EOS);
        RemoveFlag(FLAG_OUTPUT_EOS);
    }
    inCond_.notify_all();
    outCond_.notify_all();
}

bool CodecBufferCircular::CanEnableSyncMode()
{
    std::scoped_lock lock(inMutex_, outMutex_);
    return CanEnableMode<MODE_SYNC>();
}

bool CodecBufferCircular::CanEnableAsyncMode()
{
    std::scoped_lock lock(inMutex_, outMutex_);
    return CanEnableMode<MODE_ASYNC>();
}

void CodecBufferCircular::EnableSyncMode()
{
    std::scoped_lock lock(inMutex_, outMutex_);
    CHECK_AND_RETURN_LOG_WITH_TAG(CanEnableMode<MODE_SYNC>(), "Can not enable sync mode");
    EnableMode<MODE_SYNC>();
}

void CodecBufferCircular::EnableAsyncMode()
{
    std::scoped_lock lock(inMutex_, outMutex_);
    CHECK_AND_RETURN_LOG_WITH_TAG(CanEnableMode<MODE_ASYNC>(), "Can not enable async mode");
    EnableMode<MODE_ASYNC>();
}

bool CodecBufferCircular::IsSyncMode()
{
    std::scoped_lock lock(inMutex_, outMutex_);
    return HasFlag(FLAG_IS_SYNC) && HasFlag(FLAG_SYNC_ASYNC_CONFIGURED);
}

void CodecBufferCircular::ResetFlag()
{
    std::scoped_lock lock(inMutex_, outMutex_);
    RemoveFlag(FLAG_IS_RUNNING);
    RemoveFlag(FLAG_IS_SYNC);
    RemoveFlag(FLAG_SYNC_ASYNC_CONFIGURED);
    RemoveFlag(FLAG_ERROR);
    RemoveFlag(FLAG_INPUT_EOS);
    RemoveFlag(FLAG_OUTPUT_EOS);
}

void CodecBufferCircular::ClearCaches()
{
    {
        std::lock_guard<std::mutex> lock(inMutex_);
        PrintCaches(false);
        inCache_.clear();
        EventQueue emptyQueue;
        std::swap(inQueue_, emptyQueue);
    }
    {
        std::lock_guard<std::mutex> lock(outMutex_);
        PrintCaches(true);
        outCache_.clear();
        EventQueue emptyQueue;
        std::swap(outQueue_, emptyQueue);
    }
}

void CodecBufferCircular::FlushCaches()
{
    {
        std::lock_guard<std::mutex> lock(inMutex_);
        PrintCaches(false);
        for (auto &val : inCache_) {
            val.second.owner = OWNED_BY_SERVER;
        }
        EventQueue emptyQueue;
        std::swap(inQueue_, emptyQueue);
    }
    {
        std::lock_guard<std::mutex> lock(outMutex_);
        PrintCaches(true);
        for (auto &val : outCache_) {
            val.second.owner = OWNED_BY_SERVER;
        }
        EventQueue emptyQueue;
        std::swap(outQueue_, emptyQueue);
    }
}

/******************************** Common ********************************/
int32_t CodecBufferCircular::HandleInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    // Api9
    std::lock_guard<std::mutex> lock(inMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!HasFlag(FLAG_IS_SYNC), AVCS_ERR_INVALID_OPERATION, "Not support sync mode");
    BufferCacheIter iter = inCache_.find(index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_RUNNING), AVCS_ERR_INVALID_STATE, "Not in running state");
    if (iter == inCache_.end()) {
        AVCODEC_LOGD_WITH_TAG("Index is invalid %{public}u", index);
        return AVCS_ERR_OK;
    }
    BufferItem &item = iter->second;
    item.owner = OWNED_BY_SERVER;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(item.buffer != nullptr, AVCS_ERR_INVALID_OPERATION, "Buffer is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(item.memory != nullptr, AVCS_ERR_INVALID_OPERATION, "Memory is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(converter_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Converter is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(item.buffer->memory_ != nullptr, AVCS_ERR_INVALID_OPERATION,
                                      "Get buffer memory is nullptr");
    item.buffer->pts_ = info.presentationTimeUs;
    item.buffer->flag_ = static_cast<uint32_t>(flag);
    item.buffer->memory_->SetOffset(info.offset);
    item.buffer->memory_->SetSize(info.size);

    item.pts = info.presentationTimeUs;
    item.flag = static_cast<uint32_t>(flag);
    item.size = info.size;

    converter_->WriteToBuffer(item.buffer, item.memory);
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::HandleInputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(inMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_RUNNING), AVCS_ERR_INVALID_STATE, "Not in running state");
    BufferCacheIter iter = inCache_.find(index);
    if (iter == inCache_.end()) {
        AVCODEC_LOGD_WITH_TAG("Index is invalid %{public}u", index);
        return HasFlag(FLAG_IS_SYNC) ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
    }
    BufferItem &item = iter->second;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(item.owner == OWNED_BY_USER, AVCS_ERR_INVALID_OPERATION,
                                      "Invalid ownership:%{public}s", OwnerToString(item.owner).c_str());
    item.owner = OWNED_BY_SERVER;
    if (item.buffer != nullptr) {
        item.pts = item.buffer->pts_;
        item.flag = item.buffer->flag_;
        item.size = (item.buffer->memory_ != nullptr) ? item.buffer->memory_->GetSize() : 0;
    }
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::HandleOutputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(outMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_RUNNING), AVCS_ERR_INVALID_STATE, "Not in running state");
    BufferCacheIter iter = outCache_.find(index);
    if (iter == outCache_.end()) {
        AVCODEC_LOGD_WITH_TAG("Index is invalid %{public}u", index);
        return HasFlag(FLAG_IS_SYNC) ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
    }
    BufferItem &item = iter->second;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(item.owner == OWNED_BY_USER, AVCS_ERR_INVALID_OPERATION,
                                      "Invalid ownership:%{public}s", OwnerToString(item.owner).c_str());
    item.owner = OWNED_BY_SERVER;
    if (item.buffer != nullptr) {
        item.pts = item.buffer->pts_;
        item.flag = item.buffer->flag_;
        item.size = (item.buffer->memory_ != nullptr) ? item.buffer->memory_->GetSize() : 0;
    }
    return AVCS_ERR_OK;
}

void CodecBufferCircular::QueueInputBufferDone(uint32_t index)
{
    std::lock_guard<std::mutex> lock(inMutex_);
    BufferCacheIter iter = inCache_.find(index);
    if (iter == inCache_.end()) {
        AVCODEC_LOGD_WITH_TAG("index=%{public}u", index);
        return;
    }
    // The current owner of buffer is server and cannot read info from this buffer
    BufferItem &item = iter->second;
    if (item.flag & AVCODEC_BUFFER_FLAG_EOS) {
        AddFlag(FLAG_INPUT_EOS);
        inCond_.notify_all();
    }
    AVCODEC_LOGD_WITH_TAG("index=%{public}u, size=%{public}d, flag=%{public}u, pts=%{public}" PRId64, index, item.size,
                          item.flag, item.pts);
}

void CodecBufferCircular::ReleaseOutputBufferDone(uint32_t index)
{
    std::lock_guard<std::mutex> lock(outMutex_);
    BufferCacheIter iter = outCache_.find(index);
    if (iter == outCache_.end()) {
        AVCODEC_LOGD_WITH_TAG("index=%{public}u", index);
        return;
    }
    // The current owner of buffer is server and cannot read info from this buffer
    BufferItem &item = iter->second;
    AVCODEC_LOGD_WITH_TAG("index=%{public}u, size=%{public}d, flag=%{public}u, pts=%{public}" PRId64, index, item.size,
                          item.flag, item.pts);
}

void CodecBufferCircular::NotifyEos()
{
    std::lock_guard<std::mutex> lock(inMutex_);
    AddFlag(FLAG_INPUT_EOS);
    inCond_.notify_all();
}

std::shared_ptr<Format> CodecBufferCircular::GetParameter(BufferCacheIter &iter)
{
    if (iter->second.parameter == nullptr) {
        iter->second.parameter = std::make_shared<Format>();
        iter->second.parameter->SetMetaPtr(iter->second.buffer->meta_);
    }
    return iter->second.parameter;
}

std::shared_ptr<Format> CodecBufferCircular::GetAttribute(BufferCacheIter &iter)
{
    if (iter->second.attribute == nullptr) {
        iter->second.attribute = std::make_shared<Format>();
    }
    iter->second.attribute->PutLongValue(Media::Tag::MEDIA_TIME_STAMP, iter->second.buffer->pts_);
    return iter->second.attribute;
}

inline bool CodecBufferCircular::HasFlag(const CodecCircularFlag flag)
{
    return flags_ & flag;
}

inline void CodecBufferCircular::AddFlag(const CodecCircularFlag flag)
{
    flags_ |= flag;
}

inline void CodecBufferCircular::RemoveFlag(const CodecCircularFlag flag)
{
    flags_ &= ~flag;
}

/******************************** DFX ********************************/
void CodecBufferCircular::PrintCaches(bool isOutput)
{
    BufferCache &cache = isOutput ? outCache_ : inCache_;
    std::array<std::vector<uint32_t>, 3> ownerArrays; // 3: [SERVER = 0, CLIENT = 1, USER = 2]
    for (const auto &[key, item] : cache) {
        ownerArrays[item.owner].emplace_back(key);
    }
    auto getCacheInfo = [](const std::vector<uint32_t> &keys, const char *ownerstr) {
        std::ostringstream oss;
        oss << ownerstr << "(";
        if (!keys.empty()) {
            auto it = keys.begin();
            oss << *it;
            for (++it; it != keys.end(); ++it) {
                oss << "," << *it;
            }
        }
        oss << ")";
        return oss.str();
    };
    const std::string userInfo = getCacheInfo(ownerArrays[OWNED_BY_USER], "user");
    const std::string clientInfo = HasFlag(FLAG_IS_SYNC) ? getCacheInfo(ownerArrays[OWNED_BY_CLIENT], ",client") : "";
    const std::string serverInfo = getCacheInfo(ownerArrays[OWNED_BY_SERVER], ",server");
    AVCODEC_LOGI_WITH_TAG("%{public}s cache:%{public}s%{public}s%{public}s", (isOutput ? "out" : "in"),
                          userInfo.c_str(), clientInfo.c_str(), serverInfo.c_str());
}

const std::string &CodecBufferCircular::OwnerToString(BufferOwner owner)
{
    static std::unordered_map<BufferOwner, const std::string> ownerStringMap = {
        {OWNED_BY_SERVER, "server"},
        {OWNED_BY_CLIENT, "client"},
        {OWNED_BY_USER, "user"},
    };
    static const std::string defaultString = "unknown";
    auto iter = ownerStringMap.find(owner);
    return iter == ownerStringMap.end() ? defaultString : iter->second;
}

void CodecBufferCircular::ClearOutputBufferOwnedByCodec()
{
    for (auto iter = outCache_.begin(); iter != outCache_.end();) {
        if (iter->second.owner == OWNED_BY_SERVER) {
            iter = outCache_.erase(iter);
        } else {
            ++iter;
        }
    }
}

/******************************** Callback ********************************/
void CodecBufferCircular::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    IsSyncMode() ? SyncOnError(errorType, errorCode) : AsyncOnError(errorType, errorCode);
}

void CodecBufferCircular::OnOutputFormatChanged(const Format &format)
{
    IsSyncMode() ? SyncOnOutputFormatChanged(format) : AsyncOnOutputFormatChanged(format);
}

void CodecBufferCircular::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG_WITH_TAG(buffer != nullptr, "buffer is nullptr");
    {
        std::unique_lock<std::mutex> lock(inMutex_);
        if (HasFlag(FLAG_INPUT_EOS)) {
            AVCODEC_LOGD_WITH_TAG("At eos state, no buffer available");
            return;
        }
    }
    IsSyncMode() ? SyncOnInputBufferAvailable(index, buffer) : AsyncOnInputBufferAvailable(index, buffer);
}

void CodecBufferCircular::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG_WITH_TAG(buffer != nullptr, "buffer is nullptr");
    IsSyncMode() ? SyncOnOutputBufferAvailable(index, buffer) : AsyncOnOutputBufferAvailable(index, buffer);
}

void CodecBufferCircular::OnOutputBufferBinded(std::map<uint32_t, sptr<SurfaceBuffer>> &bufferMap)
{
    if (!IsSyncMode() && (mediaCb_ != nullptr)) {
        mediaCb_->OnOutputBufferBinded(bufferMap);
    }
}

void CodecBufferCircular::OnOutputBufferUnbinded()
{
    if (!IsSyncMode() && (mediaCb_ != nullptr)) {
        mediaCb_->OnOutputBufferUnbinded();
    }
}

/******************************** Async mode ********************************/
void CodecBufferCircular::AsyncOnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (errorType == AVCODEC_ERROR_FRAMEAORK_FAILED) {
        return;
    }
    std::shared_ptr<MediaCodecCallback> mediaCb = nullptr;
    std::shared_ptr<AVCodecCallback> callback = nullptr;
    {
        std::scoped_lock lock(inMutex_, outMutex_);
        mediaCb = mediaCb_;
        callback = callback_;
    }
    // AVBuffer callback
    if (mediaCb != nullptr) {
        mediaCb->OnError(errorType, errorCode);
        return;
    }
    // Api9 callback
    if (callback != nullptr) {
        callback->OnError(errorType, errorCode);
        return;
    }
}

void CodecBufferCircular::AsyncOnOutputFormatChanged(const Format &format)
{
    std::unique_lock<std::mutex> lock(outMutex_);
    ClearOutputBufferOwnedByCodec();
    // AVBuffer callback
    auto mediaCb = mediaCb_;
    if (mediaCb != nullptr) {
        lock.unlock();
        mediaCb->OnOutputFormatChanged(format);
        return;
    }
    // Api9 callback
    auto callback = callback_;
    if (callback != nullptr) {
        lock.unlock();
        callback->OnOutputFormatChanged(format);
        return;
    }
}

void CodecBufferCircular::AsyncOnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> &buffer)
{
    std::unique_lock<std::mutex> lock(inMutex_);
    BufferCacheIter iter = inCache_.find(index);
    if (iter == inCache_.end()) {
        BufferItem item = {.buffer = buffer};
        iter = inCache_.emplace(index, item).first;
    } else {
        iter->second.buffer = buffer;
    }
    BufferItem &item = iter->second;
    item.owner = OWNED_BY_USER;
    // Encoder parameter with attribute callback
    auto attrCb = attrCb_;
    if (attrCb != nullptr) {
        auto attribute = GetAttribute(iter);
        auto parameter = GetParameter(iter);
        lock.unlock();
        attrCb->OnInputParameterWithAttrAvailable(index, attribute, parameter);
        return;
    }
    // Encoder parameter callback
    auto paramCb = paramCb_;
    if (paramCb != nullptr) {
        auto parameter = GetParameter(iter);
        lock.unlock();
        paramCb->OnInputParameterAvailable(index, parameter);
        return;
    }
    // AVBuffer callback
    auto mediaCb = mediaCb_;
    if (mediaCb != nullptr) {
        item.buffer->pts_ = 0;
        lock.unlock();
        mediaCb->OnInputBufferAvailable(index, item.buffer);
        return;
    }
    // Api9 callback
    auto callback = callback_;
    if (callback != nullptr) {
        item.buffer->pts_ = 0;
        ConvertToSharedMemory(item.buffer, item.memory);
        if (converter_ != nullptr) {
            converter_->SetInputBufferFormat(item.buffer);
        }
        lock.unlock();
        callback->OnInputBufferAvailable(index, item.memory);
        return;
    }
}

void CodecBufferCircular::AsyncOnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> &buffer)
{
    std::unique_lock<std::mutex> lock(outMutex_);
    BufferCacheIter iter = outCache_.find(index);
    if (iter == outCache_.end()) {
        BufferItem item = {.buffer = buffer};
        iter = outCache_.emplace(index, item).first;
    } else {
        iter->second.buffer = buffer;
    }
    BufferItem &item = iter->second;
    item.owner = OWNED_BY_USER;
    // AVBuffer callback
    auto mediaCb = mediaCb_;
    if (mediaCb != nullptr) {
        lock.unlock();
        mediaCb->OnOutputBufferAvailable(index, item.buffer);
        return;
    }
    // Api9 callback
    auto callback = callback_;
    if (callback != nullptr) {
        ConvertToSharedMemory(item.buffer, item.memory);
        if (converter_ != nullptr) {
            converter_->SetOutputBufferFormat(item.buffer);
            converter_->ReadFromBuffer(item.buffer, item.memory);
        }
        AVCodecBufferFlag flag = static_cast<AVCodecBufferFlag>(item.buffer->flag_);
        AVCodecBufferInfo info;
        info.presentationTimeUs = item.buffer->pts_;
        if (item.buffer->memory_ != nullptr) {
            info.offset = item.buffer->memory_->GetOffset();
            info.size = item.buffer->memory_->GetSize();
        }
        lock.unlock();
        callback->OnOutputBufferAvailable(index, info, flag, item.memory);
        return;
    }
}

void CodecBufferCircular::ConvertToSharedMemory(const std::shared_ptr<AVBuffer> &buffer,
                                                std::shared_ptr<AVSharedMemory> &memory)
{
    // Api9
    using Flags = AVSharedMemory::Flags;
    std::shared_ptr<AVMemory> &bufferMem = buffer->memory_;
    if (bufferMem == nullptr || memory != nullptr) {
        return;
    }
    MemoryType type = bufferMem->GetMemoryType();
    int32_t capacity = bufferMem->GetCapacity();
    if (type == MemoryType::SHARED_MEMORY) {
        std::string name = std::string("SharedMem_") + std::to_string(buffer->GetUniqueId());
        int32_t fd = bufferMem->GetFileDescriptor();
        bool isReadable = bufferMem->GetMemoryFlag() == MemoryFlag::MEMORY_READ_ONLY;
        uint32_t flag = isReadable ? Flags::FLAGS_READ_ONLY : Flags::FLAGS_READ_WRITE;
        memory = AVSharedMemoryBase::CreateFromRemote(fd, capacity, flag, name);
    } else {
        std::string name = std::string("SharedMem_") + std::to_string(buffer->GetUniqueId());
        memory = AVSharedMemoryBase::CreateFromLocal(capacity, Flags::FLAGS_READ_WRITE, name);
        if (memory == nullptr) {
            AVCODEC_LOGW_WITH_TAG("Create shared memory from local failed");
        }
    }
}

/******************************** Sync mode ********************************/
void CodecBufferCircular::SyncOnError(AVCodecErrorType errorType, int32_t errorCode)
{
    std::scoped_lock lock(inMutex_, outMutex_);
    lastError_ = errorCode;
    AddFlag(FLAG_ERROR);
    outCond_.notify_all();
    inCond_.notify_all();
}

void CodecBufferCircular::SyncOnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::mutex> lock(outMutex_);
    outQueue_.push(Event({.type = EVENT_STREAM_CHANGED}));
    ClearOutputBufferOwnedByCodec();
    outCond_.notify_all();
}

void CodecBufferCircular::SyncOnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> &buffer)
{
    std::lock_guard<std::mutex> lock(inMutex_);
    BufferCacheIter iter = inCache_.find(index);
    if (iter == inCache_.end()) {
        BufferItem item = {.buffer = buffer};
        iter = inCache_.emplace(index, item).first;
    } else {
        iter->second.buffer = buffer;
    }
    iter->second.buffer->pts_ = 0;
    iter->second.owner = OWNED_BY_CLIENT;
    inQueue_.push(Event({.type = EVENT_INPUT_BUFFER, .index = index}));
    inCond_.notify_all();
}

void CodecBufferCircular::SyncOnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> &buffer)
{
    std::lock_guard<std::mutex> lock(outMutex_);
    BufferCacheIter iter = outCache_.find(index);
    if (iter == outCache_.end()) {
        BufferItem item = {.buffer = buffer};
        iter = outCache_.emplace(index, item).first;
    } else {
        iter->second.buffer = buffer;
    }
    iter->second.owner = OWNED_BY_CLIENT;
    outQueue_.push(Event({.type = EVENT_OUTPUT_BUFFER, .index = index}));
    outCond_.notify_all();
}

int32_t CodecBufferCircular::QueryInputBuffer(uint32_t &index, int64_t timeoutUs)
{
    return QueryInputIndex(index, timeoutUs);
}

int32_t CodecBufferCircular::QueryOutputBuffer(uint32_t &index, int64_t timeoutUs)
{
    return QueryOutputIndex(index, timeoutUs);
}

int32_t CodecBufferCircular::QueryInputIndex(uint32_t &index, int64_t timeoutUs)
{
    std::unique_lock<std::mutex> lock(inMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_SYNC), AVCS_ERR_INVALID_OPERATION, "Need enable sync mode");
    bool isNotTimeout = WaitForInputBuffer(lock, timeoutUs);

    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_RUNNING), AVCS_ERR_INVALID_STATE, "Not in running state");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!HasFlag(FLAG_INPUT_EOS), AVCS_ERR_INVALID_STATE, "End-of-stream pushed");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!HasFlag(FLAG_ERROR), lastError_, "%{public}s",
                                      AVCSErrorToString(static_cast<AVCodecServiceErrCode>(lastError_)).c_str());
    if (!isNotTimeout) {
        return AVCS_ERR_TRY_AGAIN;
    }
    Event event = inQueue_.front();
    inQueue_.pop();
    index = event.index;
    BufferCacheIter iter = inCache_.find(index);
    if (iter != inCache_.end()) {
        iter->second.owner = OWNED_BY_USER;
    }
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::QueryOutputIndex(uint32_t &index, int64_t timeoutUs)
{
    std::unique_lock<std::mutex> lock(outMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_SYNC), AVCS_ERR_INVALID_OPERATION, "Need enable sync mode");
    bool isNotTimeout = WaitForOutputBuffer(lock, timeoutUs);

    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_RUNNING), AVCS_ERR_INVALID_STATE, "Not in running state");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!HasFlag(FLAG_OUTPUT_EOS), AVCS_ERR_INVALID_STATE, "End-of-stream reached");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!HasFlag(FLAG_ERROR), lastError_, "%{public}s",
                                      AVCSErrorToString(static_cast<AVCodecServiceErrCode>(lastError_)).c_str());
    if (!isNotTimeout) {
        return AVCS_ERR_TRY_AGAIN;
    }
    Event event = outQueue_.front();
    outQueue_.pop();
    if (event.type == EVENT_STREAM_CHANGED) {
        AVCODEC_LOGI_WITH_TAG("Output format changed");
        return AVCS_ERR_STREAM_CHANGED;
    }
    index = event.index;
    BufferCacheIter iter = outCache_.find(index);
    if (iter == outCache_.end()) {
        return AVCS_ERR_OK;
    }
    BufferItem &item = iter->second;
    item.owner = OWNED_BY_USER;
    if (item.buffer != nullptr) {
        item.flag = item.buffer->flag_;
    }
    if (item.flag & AVCODEC_BUFFER_FLAG_EOS) {
        AddFlag(FLAG_OUTPUT_EOS);
        outCond_.notify_all();
    }
    return AVCS_ERR_OK;
}

bool CodecBufferCircular::WaitForInputBuffer(std::unique_lock<std::mutex> &lock, int64_t timeoutUs)
{
    const auto predicate = [this] {
        return !HasFlag(FLAG_IS_RUNNING) || // [1] Not in running state
               HasFlag(FLAG_INPUT_EOS) ||   // [2] End-of-stream pushed
               HasFlag(FLAG_ERROR) ||       // [3] Error state detected
               inQueue_.size() > 0;         // [4] Input buffer available
    };

    if (timeoutUs < 0) {
        inCond_.wait(lock, predicate);
        return true; // Always returns true after wait
    }
    if (timeoutUs == 0) {
        return predicate(); // Immediate status check
    }
    if (timeoutUs > MAX_TIMEOUT) {
        timeoutUs = MAX_TIMEOUT;
    }
    // Returns true if predicate satisfied
    return inCond_.wait_for(lock, std::chrono::microseconds(timeoutUs), predicate);
}

bool CodecBufferCircular::WaitForOutputBuffer(std::unique_lock<std::mutex> &lock, int64_t timeoutUs)
{
    const auto predicate = [this] {
        return !HasFlag(FLAG_IS_RUNNING) || // [1] Not in running state
               HasFlag(FLAG_OUTPUT_EOS) ||  // [2] End-of-stream reached
               HasFlag(FLAG_ERROR) ||       // [3] Error state detected
               outQueue_.size() > 0;        // [4] Output buffer available | Stream description changed
    };

    if (timeoutUs < 0) {
        outCond_.wait(lock, predicate);
        return true;
    }
    if (timeoutUs == 0) {
        return predicate();
    }
    if (timeoutUs > MAX_TIMEOUT) {
        timeoutUs = MAX_TIMEOUT;
    }
    return outCond_.wait_for(lock, std::chrono::microseconds(timeoutUs), predicate);
}

std::shared_ptr<AVBuffer> CodecBufferCircular::GetInputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(inMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_SYNC), nullptr, "Need enable sync mode");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_RUNNING), nullptr, "Not in running state");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!HasFlag(FLAG_ERROR), nullptr, "%{public}s",
                                      AVCSErrorToString(static_cast<AVCodecServiceErrCode>(lastError_)).c_str());
    BufferCacheIter iter = inCache_.find(index);
    CHECK_AND_RETURN_RET_LOG_LIMIT_IN_TIME_WITH_TAG(iter != inCache_.end(), nullptr,
        LOG_INTERVAL_MS, LOG_MAX_COUNT, "Index is invalid %{public}u", index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(iter->second.owner == OWNED_BY_USER, nullptr, "Invalid ownership:%{public}s",
                                      OwnerToString(iter->second.owner).c_str());
    return iter->second.buffer;
}

std::shared_ptr<AVBuffer> CodecBufferCircular::GetOutputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(outMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_SYNC), nullptr, "Need enable sync mode");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(HasFlag(FLAG_IS_RUNNING), nullptr, "Not in running state");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!HasFlag(FLAG_ERROR), nullptr, "%{public}s",
                                      AVCSErrorToString(static_cast<AVCodecServiceErrCode>(lastError_)).c_str());
    BufferCacheIter iter = outCache_.find(index);
    CHECK_AND_RETURN_RET_LOG_LIMIT_IN_TIME_WITH_TAG(iter != outCache_.end(), nullptr,
        LOG_INTERVAL_MS, LOG_MAX_COUNT, "Index is invalid %{public}u", index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(iter->second.owner == OWNED_BY_USER, nullptr, "Invalid ownership:%{public}s",
                                      OwnerToString(iter->second.owner).c_str());
    return iter->second.buffer;
}
} // namespace MediaAVCodec
} // namespace OHOS