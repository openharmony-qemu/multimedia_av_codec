/*
* Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "FileFdSourcePlugin"

#include <cerrno>
#include <cstring>
#include <regex>
#include <memory>
#ifdef WIN32
#include <fcntl.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "common/log.h"
#include "osal/filesystem/file_system.h"
#include "file_fd_source_plugin.h"
#include "common/media_core.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "FileFdSourcePlugin" };
bool StrToLong(const std::string_view& str, int64_t& value)
{
    FALSE_RETURN_V_MSG_E(!str.empty() && (isdigit(str.front()) || (str.front() == '-')),
        false, "no valid string.");
    std::string valStr(str);
    char* end = nullptr;
    errno = 0;
    long long result = strtoll(valStr.c_str(), &end, 10); /* 10 means decimal */
    FALSE_RETURN_V_MSG_E(result >= LLONG_MIN && result <= LLONG_MAX, false,
        "call StrToLong func false,  input str is: %{public}s!", valStr.c_str());
    FALSE_RETURN_V_MSG_E(end != valStr.c_str() && end[0] == '\0' && errno != ERANGE, false,
        "call StrToLong func false,  input str is: %{public}s!", valStr.c_str());
    value = result;
    return true;
}
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace FileFdSource {
namespace {
constexpr int32_t FDPOS                         = 2;
constexpr int32_t READ_TIME                     = 3;
constexpr size_t CACHE_SIZE                     = 40 * 1024 * 1024;
constexpr size_t PER_CACHE_SIZE                 = 48 * 10 * 1024;;
constexpr int32_t TEN_MILLISECOUNDS             = 10;
constexpr int32_t ONE_SECONDS                   = 1 * 1000;
constexpr int32_t CACHE_TIME_DEFAULT            = 5;
constexpr int32_t SEEK_TIME_LOWER               = 20;
constexpr int32_t SEEK_TIME_UPPER               = 1000;
constexpr int32_t RECORD_TIME_INTERVAL          = 1 * 1000;
constexpr int32_t MILLISECOUND_TO_SECOND        = 1 * 1000;
constexpr int32_t RETRY_TIMES                   = 3;
constexpr int32_t TO_BYTE                       = 8;
constexpr int32_t PERCENT_100                   = 100;
constexpr int32_t MAX_RANK                      = 100;
constexpr int32_t READ_RETRY                    = 2;
constexpr int32_t READ_ERROR_IO                 = EIO;
constexpr int32_t READ_ERROR_NOMEM              = ENOMEM;
constexpr float CACHE_LEVEL_1                   = 0.3;

constexpr unsigned int HMDFS_IOC = 0xf2;
#define IOCTL_CLOUD 2
#define HMDFS_IOC_HAS_CACHE _IOW(HMDFS_IOC, 6, struct HmdfsHasCache)
#define HMDFS_IOC_GET_LOCATION _IOR(HMDFS_IOC, 7, __u32)
#define HMDFS_IOC_CANCEL_READ _IO(HMDFS_IOC, 8)
#define HMDFS_IOC_RESTORE_READ _IO(HMDFS_IOC, 9)

uint64_t GetFileSize(int32_t fd)
{
    uint64_t fileSize = 0;
    struct stat s {};
    int ret = fstat(fd, &s);
    if (ret == 0) {
        fileSize = static_cast<uint64_t>(s.st_size);
        FALSE_RETURN_V_MSG_E(fileSize != 0, fileSize, "fileSize 0, fstat ret 0");
        return fileSize;
    } else {
        MEDIA_LOG_W("GetFileSize error ret " PUBLIC_LOG_D32 ", errno " PUBLIC_LOG_D32, ret, errno);
    }
    return fileSize;
}
bool isNumber(const std::string& str)
{
    return str.find_first_not_of("0123456789") == std::string::npos;
}
}
Status FileFdSourceRegister(const std::shared_ptr<Register>& reg)
{
    MEDIA_LOG_I("fileSourceRegister is started");
    SourcePluginDef definition;
    definition.name = "FileFdSource";
    definition.description = "File Fd source";
    definition.rank = MAX_RANK; // 100: max rank
    Capability capability;
    capability.AppendFixedKey<std::vector<ProtocolType>>(Tag::MEDIA_PROTOCOL_TYPE, {ProtocolType::FD});
    definition.AddInCaps(capability);
    auto func = [](const std::string& name) -> std::shared_ptr<SourcePlugin> {
        return std::make_shared<FileFdSourcePlugin>(name);
    };
    definition.SetCreator(func);
    return reg->AddPlugin(definition);
}

PLUGIN_DEFINITION(FileFdSource, LicenseType::APACHE_V2, FileFdSourceRegister, [] {});

FileFdSourcePlugin::FileFdSourcePlugin(std::string name)
    : SourcePlugin(std::move(name))
{
}

FileFdSourcePlugin::~FileFdSourcePlugin()
{
    MEDIA_LOG_D("~FileFdSourcePlugin in.");
    steadyClock_.Reset();
    SetInterruptState(true);
    MEDIA_LOG_D("~FileFdSourcePlugin isInterrupted_ " PUBLIC_LOG_D32, isInterrupted_.load());
    FALSE_RETURN_MSG(downloadTask_ != nullptr, "~FileFdSourcePlugin out.");
    downloadTask_->Stop();
    MEDIA_LOG_D("~FileFdSourcePlugin out.");
}

Status FileFdSourcePlugin::SetCallback(Callback* cb)
{
    MEDIA_LOG_D("SetCallback in " PUBLIC_LOG_D32, cb != nullptr);
    callback_ = cb;
    return Status::OK;
}

Status FileFdSourcePlugin::SetSource(std::shared_ptr<MediaSource> source)
{
    MEDIA_LOG_I("SetSource in. %{private}s", source->GetSourceUri().c_str());
    auto err = ParseUriInfo(source->GetSourceUri());
    if (err != Status::OK) {
        MEDIA_LOG_E("Parse file name from uri fail, uri %{private}s", source->GetSourceUri().c_str());
        return err;
    }
    CheckFileType();
    if (isCloudFile_) {
        ringBuffer_ = std::make_shared<RingBuffer>(CACHE_SIZE);
        FALSE_RETURN_V_MSG_E(!(ringBuffer_ == nullptr || !ringBuffer_->Init()),
            Status::ERROR_NO_MEMORY, "memory is not enough ringBuffer_");
        downloadTask_ = std::make_shared<Task>(std::string("downloadTaskFD"));
        FALSE_RETURN_V_MSG_E(downloadTask_ != nullptr, Status::ERROR_NO_MEMORY, "memory is not enough");
        downloadTask_->RegisterJob([this] {
            CacheDataLoop();
            return 0;
        });
        downloadTask_->Start();
        steadyClock_.Reset();
    }
    return Status::OK;
}

Status FileFdSourcePlugin::Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    return Read(0, buffer, offset, expectedLen);
}

Status FileFdSourcePlugin::Read(int32_t streamId, std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    FALSE_RETURN_V_MSG_E(fd_ != -1, Status::ERROR_WRONG_STATE, "no valid fd");
    if (isCloudFile_) {
        return ReadOnlineFile(0, buffer, offset, expectedLen);
    } else {
        return ReadOfflineFile(0, buffer, offset, expectedLen);
    }
}

Status FileFdSourcePlugin::ReadOfflineFile(int32_t streamId, std::shared_ptr<Buffer>& buffer,
    uint64_t offset, size_t expectedLen)
{
    std::shared_ptr<Memory> bufData = GetBufferPtr(buffer, expectedLen);
    FALSE_RETURN_V_MSG_E(bufData != nullptr, Status::ERROR_NO_MEMORY, "memory is not enough");
    int64_t lastSize = static_cast<size_t>(GetLastSize(position_));
    expectedLen = lastSize < 0 ?Len : std::min(static_cast<size_t>(lastSize), expectedLen);
    expectedLen = std::min(bufData->GetCapacity(), expectedLen);
    MEDIA_LOG_D("ReadLocal buffer pos: " PUBLIC_LOG_U64 " , len:" PUBLIC_LOG_ZU, position_.load(), expectedLen);

    int64_t offsetCur = lseek(fd_, 0, SEEK_CUR);
    if (offsetCur < 0) {
        MEDIA_LOG_E("Fd get offset failed ");
    } else if (static_cast<uint64_t>(offsetCur) != position_.load()) {
        MEDIA_LOG_E("Fd offsetCur has changed. offsetCur " PUBLIC_LOG_D64 ", offsetOld " PUBLIC_LOG_U64,
            offsetCur, position_.load());
    }
    auto size = read(fd_, bufData->GetWritableAddr(expectedLen), expectedLen);
    if (size <= 0) {
        HandleReadResult(expectedLen, size);
        FALSE_RETURN_V((loc_ != IOCTL_CLOUD || isEnableFdCache_ ||
            (errno != READ_ERROR_NOMEM && errno != READ_ERROR_IO)), Status::ERROR_INVALID_DATA);
        MEDIA_LOG_D("ReadLocal END_OF_STREAM");
        return Status::END_OF_STREAM;
    }
    bufData->UpdateDataSize(size);
    position_ += static_cast<uint64_t>(size);
    if (buffer->GetMemory() != nullptr) {
        MEDIA_LOG_D("ReadLocal position_ " PUBLIC_LOG_U64 ", readSize " PUBLIC_LOG_ZU,
            position_.load(), buffer->GetMemory()->GetSize());
    }
    return Status::OK;
}

Status FileFdSourcePlugin::ReadOnlineFile(int32_t streamId, std::shared_ptr<Buffer>& buffer,
    uint64_t offset, size_t expectedLen)
{
    if (isBuffering_) {
        if (HandleBuffering()) {
            FALSE_RETURN_V_MSG_E(!isInterrupted_, Status::OK, "please not retry read, isInterrupted true");
            FALSE_RETURN_V_MSG_E(isReadBlocking_, Status::OK, "please not retry read, isReadBlocking false");
            MEDIA_LOG_I("is buffering, return error again.");
            return Status::ERROR_AGAIN;
        }
    }

    std::shared_ptr<Memory> bufData = GetBufferPtr(buffer, expectedLen);
    FALSE_RETURN_V_MSG_E(bufData != nullptr, Status::ERROR_NO_MEMORY, "memory is not enough");
    expectedLen = std::min(static_cast<size_t>(GetLastSize(position_)), expectedLen);
    expectedLen = std::min(bufData->GetCapacity(), expectedLen);

    // ringbuffer 0 after seek in 20ms, don't notify buffering
    curReadTime_ = steadyClock2_.ElapsedMilliseconds();
    if (isReadFrame_ &&  ringBuffer_->GetSize() < expectedLen && !HasCacheData(expectedLen, offset) &&
        static_cast<size_t>(GetLastSize(position_)) > expectedLen) {
        MEDIA_LOG_I("ringBuffer.size() " PUBLIC_LOG_ZU " curReadTime_ " PUBLIC_LOG_D64
            " lastReadTime_ " PUBLIC_LOG_D64 " pos_ " PUBLIC_LOG_U64 " cachePos_ " PUBLIC_LOG_U64
            " expectedLen  " PUBLIC_LOG_ZU " offset_ " PUBLIC_LOG_D64 " offset " PUBLIC_LOG_U64,
            ringBuffer_->GetSize(), curReadTime_, lastReadTime_, position_.load(), cachePosition_.load(),
            expectedLen, offset_, offset);
        CheckReadTime();
        FALSE_RETURN_V_MSG_E(!isInterrupted_, Status::OK, "please not retry read, isInterrupted true");
        FALSE_RETURN_V_MSG_E(isReadBlocking_, Status::OK, "please not retry read, isReadBlocking false");
        return Status::ERROR_AGAIN;
    }

    size_t size = ringBuffer_->ReadBuffer(bufData->GetWritableAddr(expectedLen), expectedLen, READ_RETRY);
    if (size == 0) {
        MEDIA_LOG_I("read size 0, fd " PUBLIC_LOG_D32 ", offset " PUBLIC_LOG_D64 ", size:" PUBLIC_LOG_U64 ", pos:"
            PUBLIC_LOG_U64 ", readBlock:" PUBLIC_LOG_D32, fd_, offset, size_, position_.load(), isReadBlocking_.load());
        FALSE_RETURN_V_MSG_E(GetLastSize(position_) != 0, Status::END_OF_STREAM, "ReadCloud END_OF_STREAM");
        bufData->UpdateDataSize(0);
        return Status::OK;
    }
    bufData->UpdateDataSize(size);
    int64_t ct = steadyClock2_.ElapsedMilliseconds() - curReadTime_;
    if (ct > READ_TIME) {
        MEDIA_LOG_I("ReadCloud buffer position " PUBLIC_LOG_U64 ", expectedLen " PUBLIC_LOG_ZU
        " costTime: " PUBLIC_LOG_U64, position_.load(), expectedLen, ct);
    }
    position_ += static_cast<uint64_t>(size);
    MEDIA_LOG_D("ringBuffer.size() " PUBLIC_LOG_ZU, ringBuffer_->GetSize());
    return Status::OK;
}

Status FileFdSourcePlugin::SeekTo(uint64_t offset)
{
    FALSE_RETURN_V_MSG_E(fd_ != -1 && seekable_ == Seekable::SEEKABLE,
        Status::ERROR_WRONG_STATE, "no valid fd or no seekable.");

    MEDIA_LOG_D("SeekTo offset: " PUBLIC_LOG_U64, offset);
    if (isCloudFile_) {
        return SeekToOnlineFile(offset);
    } else {
        return SeekToOfflineFile(offset);
    }
}

Status FileFdSourcePlugin::SeekToOfflineFile(uint64_t offset)
{
    int32_t ret = lseek(fd_, offset + static_cast<uint64_t>(offset_), SEEK_SET);
    if (ret == -1) {
        MEDIA_LOG_E("SeekLocal failed, fd " PUBLIC_LOG_D32 ", offset " PUBLIC_LOG_U64 ", errStr "
            PUBLIC_LOG_S, fd_, offset, strerror(errno));
        return Status::ERROR_UNKNOWN;
    }
    position_ = offset + static_cast<uint64_t>(offset_);
    MEDIA_LOG_D("SeekLocal end ret " PUBLIC_LOG_D32 ", position_ " PUBLIC_LOG_U64, ret, position_.load());
    return Status::OK;
}

Status FileFdSourcePlugin::SeekToOnlineFile(uint64_t offset)
{
    FALSE_RETURN_V_MSG_E(ringBuffer_ != nullptr, Status::ERROR_WRONG_STATE, "SeekCloud ringBuffer_ is nullptr");
    MEDIA_LOG_D("SeekCloud, ringBuffer.size: " PUBLIC_LOG_ZU ", offset " PUBLIC_LOG_U64,
        ringBuffer_->GetSize(), offset);
    if (ringBuffer_->Seek(offset)) {
        position_ = offset + static_cast<uint64_t>(offset_);
        MEDIA_LOG_I("SeekCloud ringBuffer_ seek hit, offset " PUBLIC_LOG_U64, offset);
        return Status::OK;
    }
    // First clear buffer, avoid no available buffer then task pause never exit.
    ringBuffer_->SetActive(false);
    inSeek_ = true;
    if (downloadTask_ != nullptr) {
        downloadTask_->Pause();
        inSeek_ = false;
    }
    ringBuffer_->Clear();
    ringBuffer_->SetMediaOffset(offset);
    {
        std::lock_guard<std::mutex> lock(interruptMutex_);
        FALSE_RETURN_V(!isInterrupted_, Status::OK);
        ringBuffer_->SetActive(true);
    }

    int32_t ret = lseek(fd_, offset + static_cast<uint64_t>(offset_), SEEK_SET);
    if (ret == -1) {
        MEDIA_LOG_E("SeekCloud failed, fd_ " PUBLIC_LOG_D32 ", offset " PUBLIC_LOG_U64 ", errStr "
            PUBLIC_LOG_S, fd_, offset, strerror(errno));
        return Status::ERROR_UNKNOWN;
    }
    position_ = offset + static_cast<uint64_t>(offset_);
    cachePosition_ = position_.load();

    MEDIA_LOG_D("SeekCloud end, fd_ " PUBLIC_LOG_D32 ", size_ " PUBLIC_LOG_U64 ", offset_ " PUBLIC_LOG_D64
        ", position_ " PUBLIC_LOG_U64, fd_, size_, offset_, position_.load());
    if (downloadTask_ != nullptr) {
        downloadTask_->Start();
    }
    return Status::OK;
}

Status FileFdSourcePlugin::ParseUriInfo(const std::string& uri)
{
    if (uri.empty()) {
        MEDIA_LOG_E("uri is empty");
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::smatch fdUriMatch;
    FALSE_RETURN_V_MSG_E(std::regex_match(uri, fdUriMatch, std::regex("^fd://(.*)\\?offset=(.*)&size=(.*)")) ||
        std::regex_match(uri, fdUriMatch, std::regex("^fd://(.*)")),
        Status::ERROR_INVALID_PARAMETER, "Invalid fd uri format");
    FALSE_RETURN_V_MSG_E(fdUriMatch.size() >= FDPOS && isNumber(fdUriMatch[1].str()),
        Status::ERROR_INVALID_PARAMETER, "Invalid fd uri format");
    fd_ = std::stoi(fdUriMatch[1].str()); // 1: sub match fd subscript
    FALSE_RETURN_V_MSG_E(fd_ != -1 && FileSystem::IsRegularFile(fd_),
        Status::ERROR_INVALID_PARAMETER, "Invalid fd: " PUBLIC_LOG_D32, fd_);
    fileSize_ = GetFileSize(fd_);
    if (fdUriMatch.size() == 4) { // 4：4 sub match
        std::string offsetStr = fdUriMatch[2].str(); // 2: sub match offset subscript
        FALSE_RETURN_V_MSG_E(StrToLong(offsetStr, offset_), Status::ERROR_INVALID_PARAMETER,
            "Failed to read offset.");
        if (static_cast<uint64_t>(offset_) > fileSize_) {
            offset_ = static_cast<int64_t>(fileSize_);
        }
        size_ = static_cast<uint64_t>(std::stoll(fdUriMatch[3].str())); // 3: sub match size subscript
        uint64_t remainingSize = fileSize_ - static_cast<uint64_t>(offset_);
        if (size_ > remainingSize) {
            size_ = remainingSize;
        }
    } else {
        size_ = fileSize_;
        offset_ = 0;
    }
    position_ = offset_;
    seekable_ = FileSystem::IsSeekable(fd_) ? Seekable::SEEKABLE : Seekable::UNSEEKABLE;
    if (seekable_ == Seekable::SEEKABLE) {
        NOK_LOG(SeekTo(0));
    }
    MEDIA_LOG_I("Fd: " PUBLIC_LOG_D32 ", offset: " PUBLIC_LOG_D64 ", size: " PUBLIC_LOG_U64, fd_, offset_, size_);
    return Status::OK;
}

void FileFdSourcePlugin::CacheDataLoop()
{
    if (isInterrupted_) {
        MEDIA_LOG_E("CacheData break");
        WaitForInterrupt(TEN_MILLISECOUNDS);
        return;
    }

    int64_t curTime = steadyClock_.ElapsedMilliseconds();
    GetCurrentSpeed(curTime);
    int64_t lastSize = GetLastSize(cachePosition_.load());
    size_t bufferSize = lastSize < 0 ? PER_CACHE_SIZE : min(PER_CACHE_SIZE, static_cast<size_t>(lastSize));
    char* cacheBuffer = new char[bufferSize];
    if (cacheBuffer == nullptr) {
        MEDIA_LOG_E("CacheData memory is not enough bufferSize " PUBLIC_LOG_ZU, bufferSize);
        WaitForInterrupt(TEN_MILLISECOUNDS);
        return;
    }
    int size = read(fd_, cacheBuffer, bufferSize);
    if (size <= 0) {
        int64_t ct = steadyClock2_.ElapsedMilliseconds() - curTime;
        MEDIA_LOG_I("read failed, cost time: " PUBLIC_LOG_U64, ct);
        DeleteCacheBuffer(cacheBuffer, bufferSize);
        HandleReadResult(bufferSize, size);
        return;
    }
    MEDIA_LOG_D("Cache fd: " PUBLIC_LOG_D32 ", cachePos_ " PUBLIC_LOG_U64 ", ringBuffer_.size() " PUBLIC_LOG_ZU
        ", size_ " PUBLIC_LOG_U64, fd_, cachePosition_.load(), ringBuffer_->GetSize(), size_);
    while (!ringBuffer_->WriteBuffer(cacheBuffer, size)) {
        MEDIA_LOG_I("CacheData ringbuffer write failed");
        if (inSeek_ || isInterrupted_) {
            DeleteCacheBuffer(cacheBuffer, bufferSize);
            return;
        }
        WaitForInterrupt(TEN_MILLISECOUNDS);
    }
    cachePosition_ += static_cast<uint64_t>(size);
    downloadSize_ += static_cast<uint64_t>(size);

    int64_t ct = steadyClock2_.ElapsedMilliseconds() - curTime;
    if (ct > READ_TIME) {
        MEDIA_LOG_I("Cache fd:" PUBLIC_LOG_D32 " pos:" PUBLIC_LOG_U64 " rb:" PUBLIC_LOG_ZU
        " size:" PUBLIC_LOG_U64 " ct:" PUBLIC_LOG_U64, fd_, cachePosition_.load(), ringBuffer_->GetSize(),
        size_, ct);
    }
    
    DeleteCacheBuffer(cacheBuffer, bufferSize);

    CheckAndNotifyBufferingEnd();
}

void FileFdSourcePlugin::CheckAndNotifyBufferingEnd()
{
    if (isBuffering_ && (static_cast<int64_t>(ringBuffer_->GetSize()) > waterLineAbove_ ||
        GetLastSize(cachePosition_.load()) == 0)) {
        NotifyBufferingEnd();
    }
}

bool FileFdSourcePlugin::HasCacheData(size_t bufferSize, uint64_t offset)
{
    HmdfsHasCache ioctlData;
    ioctlData.offset = static_cast<int64_t>(offset);
    ioctlData.readSize = static_cast<int64_t>(bufferSize);
    int32_t ioResult = ioctl(fd_, HMDFS_IOC_HAS_CACHE, &ioctlData); // 0 has, -1 hasnot

    ioctlData.offset = static_cast<int64_t>(cachePosition_);
    ioctlData.readSize = static_cast<int64_t>(PER_CACHE_SIZE);
    int32_t ioCacheResult = ioctl(fd_, HMDFS_IOC_HAS_CACHE, &ioctlData);
    // ioctl has cache
    if (ioResult == 0 && ioCacheResult == 0) {
        return true;
    } else {
        MEDIA_LOG_I("ioctl has no cache with errno " PUBLIC_LOG_D32 " cachePosition_ " PUBLIC_LOG_U64
            " offset " PUBLIC_LOG_U64 " bufferSize " PUBLIC_LOG_ZU, errno, cachePosition_.load(), offset, bufferSize);
    }
    return false;
}

Status FileFdSourcePlugin::Stop()
{
    MEDIA_LOG_I("Stop enter.");
    isInterrupted_ = true;
    MEDIA_LOG_I("Stop isInterrupted_ " PUBLIC_LOG_D32, isInterrupted_.load());
    FALSE_RETURN_V(downloadTask_ != nullptr, Status::OK);
    downloadTask_->StopAsync();
    return Status::OK;
}

Status FileFdSourcePlugin::Reset()
{
    MEDIA_LOG_I("Reset enter.");
    isInterrupted_ = true;
    MEDIA_LOG_I("Reset isInterrupted_ " PUBLIC_LOG_D32, isInterrupted_.load());
    FALSE_RETURN_V(downloadTask_ != nullptr, Status::OK);
    downloadTask_->StopAsync();
    return Status::OK;
}

void FileFdSourcePlugin::PauseDownloadTask(bool isAsync)
{
    FALSE_RETURN(downloadTask_ != nullptr);
    if (isAsync) {
        downloadTask_->PauseAsync();
    } else {
        downloadTask_->Pause();
    }
}

bool FileFdSourcePlugin::HandleBuffering()
{
    MEDIA_LOG_I("HandleBuffering in.");
    int32_t sleepTime = 0;
    // return error again 1 time 1s, avoid ffmpeg error
    while (sleepTime < ONE_SECONDS && !isInterrupted_ && isReadBlocking_) {
        NotifyBufferingPercent();
        if (!isBuffering_) {
            break;
        }
        MEDIA_LOG_I("isBuffering.");
        WaitForInterrupt(TEN_MILLISECOUNDS);
        sleepTime += TEN_MILLISECOUNDS;
    }
    MEDIA_LOG_I("HandleBuffering out.");
    return isBuffering_;
}

void FileFdSourcePlugin::HandleReadResult(size_t bufferSize, int size)
{
    MEDIA_LOG_I("HandleReadResult size " PUBLIC_LOG_D32 ", fd " PUBLIC_LOG_D32 ", cachePosition_" PUBLIC_LOG_U64
        ", position_ " PUBLIC_LOG_U64 ", bufferSize " PUBLIC_LOG_ZU ", size_ " PUBLIC_LOG_U64 ", offset_ "
        PUBLIC_LOG_D64, size, fd_, cachePosition_.load(), position_.load(), bufferSize, size_, offset_);
    if (size < 0) {
        // errno EIO  5
        MEDIA_LOG_E("read fail, errno " PUBLIC_LOG_D32, errno);

        // read fail with errno, retry 3 * 10ms
        retryTimes_++;
        if (retryTimes_ >= RETRY_TIMES || isInterrupted_.load()) {
            NotifyReadFail();
            SetInterruptState(true);
        }
        WaitForInterrupt(TEN_MILLISECOUNDS);
    } else {
        cachePosition_ = 0;
        PauseDownloadTask(false);
    }
}

void FileFdSourcePlugin::NotifyBufferingStart()
{
    MEDIA_LOG_I("NotifyBufferingStart, ringBuffer.size() " PUBLIC_LOG_ZU
        ", waterLineAbove_ " PUBLIC_LOG_U64, ringBuffer_->GetSize(), waterLineAbove_);
    isBuffering_ = true;
    if (callback_ != nullptr && !isInterrupted_) {
        MEDIA_LOG_I("Read OnEvent BUFFERING_START.");
        callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
    } else {
        MEDIA_LOG_E("BUFFERING_START callback_ is nullptr or isInterrupted_ is true");
    }
}

void FileFdSourcePlugin::NotifyBufferingPercent()
{
    if (waterLineAbove_ != 0) {
        int64_t bp = static_cast<float>(ringBuffer_->GetSize()) / waterLineAbove_ * PERCENT_100;
        bp = bp > PERCENT_100 ? PERCENT_100 : bp;
        if (isBuffering_ && callback_ != nullptr && !isInterrupted_) {
            MEDIA_LOG_I("NotifyBufferingPercent, ringBuffer.size() " PUBLIC_LOG_ZU ", waterLineAbove_ " PUBLIC_LOG_U64
                ", PERCENT " PUBLIC_LOG_D32, ringBuffer_->GetSize(), waterLineAbove_, static_cast<int32_t>(bp));
            callback_->OnEvent({PluginEventType::EVENT_BUFFER_PROGRESS,
                {BufferingInfoType::BUFFERING_PERCENT}, std::to_string(bp)});
        } else {
            MEDIA_LOG_E("EVENT_BUFFER_PROGRESS callback_ is nullptr or isInterrupted_ \
                is true or isBuffering_ is false");
        }
    }
}

void FileFdSourcePlugin::NotifyBufferingEnd()
{
    NotifyBufferingPercent();
    MEDIA_LOG_I("NotifyBufferingEnd, ringBuffer.size() " PUBLIC_LOG_ZU
        ", waterLineAbove_ " PUBLIC_LOG_U64, ringBuffer_->GetSize(), waterLineAbove_);
    isBuffering_ = false;
    lastReadTime_ = 0;
    if (callback_ != nullptr && !isInterrupted_) {
        MEDIA_LOG_I("NotifyBufferingEnd success .");
        callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
    } else {
        MEDIA_LOG_E("BUFFERING_END callback_ is nullptr or isInterrupted_ is true");
    }
}

void FileFdSourcePlugin::NotifyReadFail()
{
    MEDIA_LOG_I("NotifyReadFail in.");
    if (callback_ != nullptr && !isInterrupted_) {
        MEDIA_LOG_I("Read OnEvent read fail");
        callback_->OnEvent({PluginEventType::CLIENT_ERROR,
            static_cast<int32_t>(NetworkClientErrorCode::ERROR_TIME_OUT), "read"});
    } else {
        MEDIA_LOG_E("CLIENT_ERROR callback_ is nullptr or isInterrupted_ is true");
    }
}

void FileFdSourcePlugin::SetDemuxerState(int32_t streamId)
{
    MEDIA_LOG_I("SetDemuxerState");
    isReadFrame_ = true;
}

Status FileFdSourcePlugin::SetCurrentBitRate(int32_t bitRate, int32_t streamID)
{
    currentBitRate_ = bitRate / TO_BYTE; // 8b
    MEDIA_LOG_I("currentBitRate: " PUBLIC_LOG_D32, currentBitRate_);
    // default cache 0.3s
    waterLineAbove_ = CACHE_LEVEL_1 * currentBitRate_;
    return Status::OK;
}

void FileFdSourcePlugin::SetBundleName(const std::string& bundleName)
{
    MEDIA_LOG_I("SetBundleName bundleName: " PUBLIC_LOG_S, bundleName.c_str());
}

Status FileFdSourcePlugin::SetReadBlockingFlag(bool isAllowed)
{
    MEDIA_LOG_I("SetReadBlockingFlag entered, IsReadBlockingAllowed %{public}d", isAllowed);
    if (ringBuffer_) {
        ringBuffer_->SetReadBlocking(isAllowed);
    }
    isReadBlocking_ = isAllowed;
    return Status::OK;
}

void FileFdSourcePlugin::SetInterruptState(bool isInterruptNeeded)
{
    bool isInterruptAllowed = true;
    MEDIA_LOG_I("SetInterruptState isInterrupted_" PUBLIC_LOG_D32, isInterruptNeeded);
    {
        std::lock_guard<std::mutex> lock(interruptMutex_);
        isInterruptAllowed = !(isInterrupted_ && isInterruptNeeded);
        isInterrupted_ = isInterruptNeeded;
        bufferCond_.notify_all();
    }
    if (ringBuffer_ != nullptr) {
        if (isInterrupted_) {
            ringBuffer_->SetActive(false);
        } else {
            ringBuffer_->SetActive(true);
        }
    }
    
    if (isInterrupted_ && isInterruptAllowed && isCloudFile_) {
        if (downloadTask_ != nullptr) {
            downloadTask_->StopAsync();
        }
        int ret = ioctl(fd_, HMDFS_IOC_CANCEL_READ);
        MEDIA_LOG_I("ioctl break read, fd %{public}d, ret %{public}d, errno %{public}d", fd_, ret, errno);
    }
}

Status FileFdSourcePlugin::GetSize(uint64_t& size)
{
    size = size_;
    return Status::OK;
}

Seekable FileFdSourcePlugin::GetSeekable()
{
    MEDIA_LOG_D("GetSeekable in");
    return seekable_;
}

void FileFdSourcePlugin::CheckFileType()
{
    int loc; // 1本地，2云端
    int ioResult = ioctl(fd_, HMDFS_IOC_GET_LOCATION, &loc);
    MEDIA_LOG_I("SetSource ioctl loc, ret " PUBLIC_LOG_D32 ", loc " PUBLIC_LOG_D32 ", errno"
        PUBLIC_LOG_D32, ioResult, loc, errno);

    if (ioResult == 0) {
        loc_ = loc;
    }
    if (!isEnableFdCache_) {
        isCloudFile_ = false;
        return;
    }

    if (ioResult == 0) {
        if (loc == IOCTL_CLOUD) {
            isCloudFile_ = true;
            MEDIA_LOG_I("ioctl file is cloud");
            int ret = ioctl(fd_, HMDFS_IOC_RESTORE_READ);
            MEDIA_LOG_I("ioctl restore fd, fd %{public}d, ret %{public}d, errno %{public}d", fd_, ret, errno);
            return;
        } else {
            isCloudFile_ = false;
            MEDIA_LOG_I("ioctl file is local");
        }
    } else {
        isCloudFile_ = false;
        MEDIA_LOG_I("ioctl failed to get file type");
    }
}

std::shared_ptr<Memory> FileFdSourcePlugin::GetBufferPtr(std::shared_ptr<Buffer>& buffer, size_t expectedLen)
{
    if (!buffer) {
        buffer = std::make_shared<Buffer>();
    }
    std::shared_ptr<Memory> bufData;
    if (buffer->IsEmpty()) {
        bufData = buffer->AllocMemory(nullptr, expectedLen);
    } else {
        bufData = buffer->GetMemory();
    }
    return bufData;
}

int64_t FileFdSourcePlugin::GetLastSize(uint64_t position)
{
    int64_t ret = static_cast<int64_t>(size_) + offset_ - static_cast<int64_t>(position);
    if (ret < 0) {
        MEDIA_LOG_E("GetLastSize error, fd_ " PUBLIC_LOG_D32 ", offset_ " PUBLIC_LOG_D64 ", size_ "
            PUBLIC_LOG_U64 ", position " PUBLIC_LOG_U64, fd_, offset_, size_, position);
    }
    return ret;
}

void FileFdSourcePlugin::GetCurrentSpeed(int64_t curTime)
{
    if ((curTime - lastCheckTime_) > RECORD_TIME_INTERVAL) {
        MEDIA_LOG_I("CacheDataLoop curTime_: " PUBLIC_LOG_U64 " lastCheckTime_: "
        PUBLIC_LOG_U64 " downloadSize_: " PUBLIC_LOG_U64, curTime, lastCheckTime_, downloadSize_);
        float duration = static_cast<double>(curTime - lastCheckTime_) / MILLISECOUND_TO_SECOND;
        avgDownloadSpeed_ = downloadSize_ / duration; //b/s
        MEDIA_LOG_I("downloadDuration: " PUBLIC_LOG_F " avgDownloadSpeed_: " PUBLIC_LOG_F,
            duration, avgDownloadSpeed_);
        downloadSize_ = 0;
        lastCheckTime_ = curTime;
        FALSE_RETURN(currentBitRate_ > 0);
        UpdateWaterLineAbove();
    }
}

void FileFdSourcePlugin::UpdateWaterLineAbove()
{
    FALSE_RETURN_MSG(currentBitRate_ > 0, "currentBitRate_ <= 0");
    float cacheTime = GetCacheTime(avgDownloadSpeed_ / currentBitRate_);
    MEDIA_LOG_I("cacheTime: " PUBLIC_LOG_F " avgDownloadSpeed_: " PUBLIC_LOG_F
        " currentBitRate: " PUBLIC_LOG_D32, cacheTime, avgDownloadSpeed_, currentBitRate_);
    waterLineAbove_ = cacheTime * currentBitRate_ > GetLastSize(cachePosition_.load()) ?
        GetLastSize(cachePosition_.load()) : cacheTime * currentBitRate_;
    MEDIA_LOG_I("waterLineAbove_: " PUBLIC_LOG_U64, waterLineAbove_);
}

float FileFdSourcePlugin::GetCacheTime(float num)
{
    MEDIA_LOG_I("GetCacheTime with num: " PUBLIC_LOG_F, num);
    if (num < 0) {
        return CACHE_LEVEL_1;
    }
    if (num > 0 && num < 0.5) { // (0, 0.5)
        return CACHE_TIME_DEFAULT;
    } else if (num >= 0.5 && num < 1) { //[0.5, 1)
        return CACHE_TIME_DEFAULT;
    } else if (num >= 1) { //[1, 2)
        return CACHE_LEVEL_1;
    }
    return CACHE_TIME_DEFAULT;
}

void FileFdSourcePlugin::DeleteCacheBuffer(char* buffer, size_t bufferSize)
{
    if (buffer != nullptr && bufferSize >= 0) {
        delete[] buffer;
    }
}

void FileFdSourcePlugin::CheckReadTime()
{
    if (IsValidTime(curReadTime_, lastReadTime_)) {
        NotifyBufferingStart();
        lastReadTime_ = 0;
    } else {
        if (lastReadTime_ == 0) {
            lastReadTime_ = curReadTime_;
        }
    }
}

bool FileFdSourcePlugin::IsValidTime(int64_t curTime, int64_t lastTime)
{
    return lastReadTime_ != 0 && curReadTime_ - lastReadTime_ < SEEK_TIME_UPPER &&
        curReadTime_ - lastReadTime_ > SEEK_TIME_LOWER;
}

void FileFdSourcePlugin::SetEnableOnlineFdCache(bool isEnableFdCache)
{
    isEnableFdCache_ = isEnableFdCache;
}

void FileFdSourcePlugin::WaitForInterrupt(int32_t waitTimeMS)
{
    std::unique_lock<std::mutex> lock(interruptMutex_);
    bufferCond_.wait_for(lock, std::chrono::milliseconds(waitTimeMS), [&] { return isInterrupted_.load(); });
}

bool FileFdSourcePlugin::IsLocalFd()
{
    return !isCloudFile_;
}
} // namespace FileFdSource
} // namespace Plugin
} // namespace Media
} // namespace OHOS