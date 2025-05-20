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
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(CanSetIsAsyncMode(true), AVCS_ERR_INVALID_OPERATION, "Enable async mode failed");
    callback_ = callback;
    SetIsAsyncMode(true);
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(CanSetIsAsyncMode(true), AVCS_ERR_INVALID_OPERATION, "Enable async mode failed");
    mediaCb_ = callback;
    SetIsAsyncMode(true);
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(attrCb_ == nullptr, AVCS_ERR_INVALID_STATE,
                                      "Already set parameter with atrribute callback!");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(CanSetIsAsyncMode(true), AVCS_ERR_INVALID_OPERATION, "Enable async mode failed");
    paramCb_ = callback;
    SetIsAsyncMode(true);
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(paramCb_ == nullptr, AVCS_ERR_INVALID_STATE, "Already set parameter callback!");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(CanSetIsAsyncMode(true), AVCS_ERR_INVALID_OPERATION, "Enable async mode failed");
    attrCb_ = callback;
    SetIsAsyncMode(true);
    return AVCS_ERR_OK;
}

void CodecBufferCircular::SetIsRunning(bool isRunning)
{
    std::scoped_lock lock(inMutex_, outMutex_);
    if (isRunning) {
        flag_ |= FLAG_IS_RUNNING;
    } else {
        flag_ &= ~FLAG_IS_RUNNING;
    }
    inCond_.notify_all();
    outCond_.notify_all();
}

void CodecBufferCircular::SetIsAsyncMode(bool isAsyncMode)
{
    if (!CanSetIsAsyncMode(isAsyncMode)) {
        return;
    }
    if (isAsyncMode) {
        flag_ |= FLAG_IS_ASYNC;
    } else {
        flag_ |= FLAG_IS_SYNC;
    }
    isConfigured_ = true;
}

bool CodecBufferCircular::GetIsAsyncMode()
{
    return isConfigured_ && (flag_ & FLAG_IS_ASYNC);
}

bool CodecBufferCircular::CanSetIsAsyncMode(bool isAsyncMode)
{
    if (isConfigured_) {
        if (isAsyncMode && (flag_ & FLAG_IS_SYNC)) {
            AVCODEC_LOGE_WITH_TAG("Sync mode active, cannot enable async");
            return false;
        } else if (!isAsyncMode && (flag_ & FLAG_IS_ASYNC)) {
            AVCODEC_LOGE_WITH_TAG("Async mode active, cannot enable sync");
            return false;
        }
    }
    return true;
}

void CodecBufferCircular::ResetFlag()
{
    flag_ &= ~FLAG_IS_RUNNING;
    flag_ &= ~FLAG_STREAM_CHANGED;
    flag_ &= ~FLAG_ERROR;
}

void CodecBufferCircular::ClearCaches()
{
    {
        std::lock_guard<std::mutex> lock(inMutex_);
        PrintCaches(false);
        inCache_.clear();
        std::queue<uint32_t> emptyQueue;
        std::swap(inQueue_, emptyQueue);
    }
    {
        std::lock_guard<std::mutex> lock(outMutex_);
        PrintCaches(true);
        outCache_.clear();
        std::queue<uint32_t> emptyQueue;
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
    }
    {
        std::lock_guard<std::mutex> lock(outMutex_);
        PrintCaches(true);
        for (auto &val : outCache_) {
            val.second.owner = OWNED_BY_SERVER;
        }
    }
}

void CodecBufferCircular::PrintCaches(bool isOutput)
{
    BufferCache &cache = isOutput ? outCache_ : inCache_;
    std::array<std::vector<uint32_t>, 3> ownerArrays; // [SERVER, CLIENT, USER]
    for (const auto &[key, item] : cache) {
        ownerArrays[item.owner].emplace_back(key);
    }
    auto buildCacheStr = [](const std::vector<uint32_t> &keys, const char *ownerstr) {
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
    const std::string serverStr = buildCacheStr(ownerArrays[OWNED_BY_SERVER], "server");
    const std::string clientStr = (flag_ & FLAG_IS_SYNC) ? buildCacheStr(ownerArrays[OWNED_BY_CLIENT], ",client") : "";
    const std::string userStr = buildCacheStr(ownerArrays[OWNED_BY_USER], ",user");
    AVCODEC_LOGI_WITH_TAG("%{public}s caches:%{public}s%{public}s%{public}s", (isOutput ? "out" : "in"),
                          serverStr.c_str(), clientStr.c_str(), userStr.c_str());
}

int32_t CodecBufferCircular::HandleInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    // Api9
    std::lock_guard<std::mutex> lock(inMutex_);
    BufferCacheIter iter = inCache_.find(index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_RUNNING, AVCS_ERR_INVALID_STATE, "Not in running state");
    if (iter == inCache_.end()) {
        AVCODEC_LOGW_WITH_TAG("Index is invalid %{publlic}u", index);
        return (flag_ & FLAG_IS_SYNC) ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
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

    converter_->WriteToBuffer(item.buffer, item.memory);

    EXPECT_AND_LOGD_WITH_TAG(
        item.buffer != nullptr, "index=%{public}d, size=%{public}d, flag=%{public}u, pts=%{public}" PRId64, index,
        item.buffer->memory_ != nullptr ? item.buffer->memory_->GetSize() : 0, item.buffer->flag_, item.buffer->pts_);
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::HandleInputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(inMutex_);
    BufferCacheIter iter = inCache_.find(index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_RUNNING, AVCS_ERR_INVALID_STATE, "Not in running state");
    if (iter == inCache_.end()) {
        AVCODEC_LOGW_WITH_TAG("Index is invalid %{publlic}u", index);
        return (flag_ & FLAG_IS_SYNC) ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
    }
    BufferItem &item = iter->second;
    if (flag_ & FLAG_IS_SYNC) {
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(item.owner == OWNED_BY_USER, AVCS_ERR_INVALID_OPERATION,
                                          "Invalid ownership:%{public}s", OwnerToString(item.owner).c_str());
    }
    item.owner = OWNED_BY_SERVER;
    EXPECT_AND_LOGD_WITH_TAG(
        item.buffer != nullptr, "index=%{public}d, size=%{public}d, flag=%{public}u, pts=%{public}" PRId64, index,
        item.buffer->memory_ != nullptr ? item.buffer->memory_->GetSize() : 0, item.buffer->flag_, item.buffer->pts_);
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::HandleOutputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(outMutex_);
    BufferCacheIter iter = outCache_.find(index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_RUNNING, AVCS_ERR_INVALID_STATE, "Not in running state");
    if (iter == inCache_.end()) {
        AVCODEC_LOGW_WITH_TAG("Index is invalid %{publlic}u", index);
        return (flag_ & FLAG_IS_SYNC) ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
    }
    BufferItem &item = iter->second;
    if (flag_ & FLAG_IS_SYNC) {
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(item.owner == OWNED_BY_USER, AVCS_ERR_INVALID_OPERATION,
                                          "Invalid ownership:%{public}s", OwnerToString(item.owner).c_str());
    }
    item.owner = OWNED_BY_SERVER;
    return AVCS_ERR_OK;
}

std::shared_ptr<Format> CodecBufferCircular::GetParameter(BufferCacheIter &iter)
{
    if (iter->second.parameter == nullptr && iter->second.buffer != nullptr) {
        iter->second.parameter = std::make_shared<Format>();
        iter->second.parameter->SetMetaPtr(iter->second.buffer->meta_);
    }
    return iter->second.parameter;
}

std::shared_ptr<Format> CodecBufferCircular::GetAttribute(BufferCacheIter &iter)
{
    if (iter->second.attribute == nullptr && iter->second.buffer != nullptr) {
        iter->second.attribute = std::make_shared<Format>();
        iter->second.attribute->PutLongValue(Media::Tag::MEDIA_TIME_STAMP, iter->second.buffer->pts_);
    }
    return iter->second.attribute;
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

/******************************** Callback ********************************/
void CodecBufferCircular::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (flag_ & FLAG_IS_SYNC) ? SyncOnError(errorType, errorCode) : AsyncOnError(errorType, errorCode);
}

void CodecBufferCircular::OnOutputFormatChanged(const Format &format)
{
    (flag_ & FLAG_IS_SYNC) ? SyncOnOutputFormatChanged(format) : AsyncOnOutputFormatChanged(format);
}

void CodecBufferCircular::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    (flag_ & FLAG_IS_SYNC) ? SyncOnInputBufferAvailable(index, buffer) : AsyncOnInputBufferAvailable(index, buffer);
}

void CodecBufferCircular::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    (flag_ & FLAG_IS_SYNC) ? SyncOnOutputBufferAvailable(index, buffer) : AsyncOnOutputBufferAvailable(index, buffer);
}

/******************************** Async mode ********************************/
void CodecBufferCircular::AsyncOnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (errorType == static_cast<AVCodecErrorType>(AVCODEC_ERROR_EXTEND_START + 1)) {
        return;
    }
    if (mediaCb_ != nullptr) {
        mediaCb_->OnError(errorType, errorCode);
    } else if (callback_ != nullptr) {
        callback_->OnError(errorType, errorCode);
    }
}

void CodecBufferCircular::AsyncOnOutputFormatChanged(const Format &format)
{
    {
        std::lock_guard<std::mutex> lock(outMutex_);
        outCache_.clear();
    }
    if (mediaCb_ != nullptr) {
        mediaCb_->OnOutputFormatChanged(format);
    } else if (callback_ != nullptr) {
        callback_->OnOutputFormatChanged(format);
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
    // Encoder parameter callback
    if (attrCb_ != nullptr) {
        auto attribute = GetAttribute(iter);
        auto parameter = GetParameter(iter);
        lock.unlock();
        attrCb_->OnInputParameterWithAttrAvailable(index, attribute, parameter);
        return;
    }
    if (paramCb_ != nullptr) {
        auto parameter = GetParameter(iter);
        lock.unlock();
        paramCb_->OnInputParameterAvailable(index, parameter);
        return;
    }
    // AVBuffer callback
    if (mediaCb_ != nullptr) {
        lock.unlock();
        mediaCb_->OnInputBufferAvailable(index, item.buffer);
        return;
    }
    // Api9 callback
    if (callback_ != nullptr) {
        ConvertToSharedMemory(item.buffer, item.memory);
        if (converter_ != nullptr) {
            converter_->SetInputBufferFormat(item.buffer);
        }
        lock.unlock();
        callback_->OnInputBufferAvailable(index, item.memory);
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
    if (mediaCb_ != nullptr) {
        lock.unlock();
        mediaCb_->OnOutputBufferAvailable(index, item.buffer);
        return;
    }
    // Api9 callback
    if (callback_ != nullptr) {
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
        callback_->OnOutputBufferAvailable(index, info, flag, item.memory);
        return;
    }
}

void CodecBufferCircular::ConvertToSharedMemory(const std::shared_ptr<AVBuffer> &buffer,
                                                std::shared_ptr<AVSharedMemory> &memory)
{
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
        CHECK_AND_RETURN_LOG_WITH_TAG(memory != nullptr, "Create shared memory from local failed");
    }
}

/******************************** Sync mode ********************************/
void CodecBufferCircular::SyncOnError(AVCodecErrorType errorType, int32_t errorCode)
{
    std::scoped_lock lock(inMutex_, outMutex_);
    lastError_ = errorCode;
    flag_ |= FLAG_ERROR;
    outCond_.notify_all();
    inCond_.notify_all();
}

void CodecBufferCircular::SyncOnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::mutex> lock(outMutex_);
    flag_ |= FLAG_STREAM_CHANGED;
    std::queue<uint32_t> emptyQueue;
    std::swap(outQueue_, emptyQueue);
    outCache_.clear();
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
    iter->second.owner = OWNED_BY_CLIENT;
    inQueue_.push(index);
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
    outQueue_.push(index);
    outCond_.notify_all();
}

int32_t CodecBufferCircular::QueryInputParameterWithAttr(uint32_t &index, int64_t timeoutUs)
{
    return QueryInputIndex(index, timeoutUs);
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
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_SYNC, AVCS_ERR_INVALID_OPERATION, "Not support async mode");
    std::unique_lock<std::mutex> lock(inMutex_);
    bool isNotTimeout = WaitForBuffer(lock, inCond_, timeoutUs, false);

    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_RUNNING, AVCS_ERR_INVALID_STATE, "Not in running state");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(flag_ & FLAG_ERROR), lastError_, "%{public}s",
                                      AVCSErrorToString(static_cast<AVCodecServiceErrCode>(lastError_)).c_str());
    if (!isNotTimeout) {
        return AVCS_ERR_VIDEO_TRY_AGAIN_LATER;
    }
    index = inQueue_.front();
    inQueue_.pop();
    BufferCacheIter iter = inCache_.find(index);
    if (iter != inCache_.end()) {
        iter->second.owner = OWNED_BY_USER;
    }
    return AVCS_ERR_OK;
}

int32_t CodecBufferCircular::QueryOutputIndex(uint32_t &index, int64_t timeoutUs)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_SYNC, AVCS_ERR_INVALID_OPERATION, "Not support async mode");
    std::unique_lock<std::mutex> lock(outMutex_);
    bool isNotTimeout = WaitForBuffer(lock, outCond_, timeoutUs, true);

    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_RUNNING, AVCS_ERR_INVALID_STATE, "Not in running state");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(flag_ & FLAG_ERROR), lastError_, "%{public}s",
                                      AVCSErrorToString(static_cast<AVCodecServiceErrCode>(lastError_)).c_str());
    if (flag_ & FLAG_STREAM_CHANGED) {
        AVCODEC_LOGI_WITH_TAG("Output format changed");
        flag_ &= ~FLAG_STREAM_CHANGED;
        return AVCS_ERR_VIDEO_STREAM_CHANGED;
    }
    if (!isNotTimeout) {
        return AVCS_ERR_VIDEO_TRY_AGAIN_LATER;
    }
    index = outQueue_.front();
    outQueue_.pop();
    BufferCacheIter iter = outCache_.find(index);
    if (iter != outCache_.end()) {
        iter->second.owner = OWNED_BY_USER;
    }
    return AVCS_ERR_OK;
}

bool CodecBufferCircular::WaitForBuffer(std::unique_lock<std::mutex> &lock, std::condition_variable &cond,
                                        int64_t timeoutUs, bool isOutput)
{
    const auto timeout = std::chrono::microseconds(timeoutUs);
    const auto func = [this, &isOutput] {
        return (isOutput ? outQueue_ : inQueue_).size() > 0 || // get new buffer
               (flag_ & FLAG_ERROR) ||                         // on error
               ((flag_ & FLAG_STREAM_CHANGED) && isOutput) ||  // on output format changed
               !(flag_ & FLAG_IS_RUNNING);                     // stop running
    };
    if (timeoutUs < 0) {
        cond.wait(lock, func);
        return true;
    } else if (timeoutUs == 0) {
        return func();
    }
    return cond.wait_for(lock, timeout, func); // true: is not timeout
}

std::shared_ptr<Format> CodecBufferCircular::GetInputParameter(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_SYNC, nullptr, "Not support async mode");
    std::lock_guard<std::mutex> lock(inMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_RUNNING, nullptr, "Not in running state");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(flag_ & FLAG_ERROR), nullptr, "%{public}s",
                                      AVCSErrorToString(static_cast<AVCodecServiceErrCode>(lastError_)).c_str());
    BufferCacheIter iter = inCache_.find(index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(iter != inCache_.end(), nullptr, "Index is invalid %{publlic}u", index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(iter->second.owner == OWNED_BY_USER, nullptr, "Invalid ownership:%{public}s",
                                      OwnerToString(iter->second.owner).c_str());
    return GetParameter(iter);
}

std::shared_ptr<Format> CodecBufferCircular::GetInputAttribute(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_SYNC, nullptr, "Not support async mode");
    std::lock_guard<std::mutex> lock(inMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_RUNNING, nullptr, "Not in running state");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(flag_ & FLAG_ERROR), nullptr, "%{public}s",
                                      AVCSErrorToString(static_cast<AVCodecServiceErrCode>(lastError_)).c_str());
    BufferCacheIter iter = inCache_.find(index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(iter != inCache_.end(), nullptr, "Index is invalid %{publlic}u", index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(iter->second.owner == OWNED_BY_USER, nullptr, "Invalid ownership:%{public}s",
                                      OwnerToString(iter->second.owner).c_str());
    return GetAttribute(iter);
}

std::shared_ptr<AVBuffer> CodecBufferCircular::GetInputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_SYNC, nullptr, "Not support async mode");
    std::lock_guard<std::mutex> lock(inMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_RUNNING, nullptr, "Not in running state");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(flag_ & FLAG_ERROR), nullptr, "%{public}s",
                                      AVCSErrorToString(static_cast<AVCodecServiceErrCode>(lastError_)).c_str());
    BufferCacheIter iter = inCache_.find(index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(iter != inCache_.end(), nullptr, "Index is invalid %{publlic}u", index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(iter->second.owner == OWNED_BY_USER, nullptr, "Invalid ownership:%{public}s",
                                      OwnerToString(iter->second.owner).c_str());
    return iter->second.buffer;
}

std::shared_ptr<AVBuffer> CodecBufferCircular::GetOutputBuffer(uint32_t index)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_SYNC, nullptr, "Not support async mode");
    std::lock_guard<std::mutex> lock(outMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(flag_ & FLAG_IS_RUNNING, nullptr, "Not in running state");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(flag_ & FLAG_ERROR), nullptr, "%{public}s",
                                      AVCSErrorToString(static_cast<AVCodecServiceErrCode>(lastError_)).c_str());
    BufferCacheIter iter = outCache_.find(index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(iter != outCache_.end(), nullptr, "Index is invalid %{publlic}u", index);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(iter->second.owner == OWNED_BY_USER, nullptr, "Invalid ownership:%{public}s",
                                      OwnerToString(iter->second.owner).c_str());
    return iter->second.buffer;
}
} // namespace MediaAVCodec
} // namespace OHOS