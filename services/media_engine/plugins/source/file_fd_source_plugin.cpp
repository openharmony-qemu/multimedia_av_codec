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
#include <sys/stat.h>
#include "common/log.h"
#include "osal/filesystem/file_system.h"
#include "file_fd_source_plugin.h"
#include "common/media_core.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace FileFdSource {
namespace {
constexpr size_t SAVE_BUFFER_SIZE = 5 * 1024 * 1024;
constexpr int32_t ONE_THOUSAND_MICROSECOUNDS = 1000;
constexpr int32_t TEN_THOUSAND_MICROSECOUNDS = 10 * 1000;
constexpr int32_t ONE_MILLION_MICROSECOUNDS = 1 * 1000 * 1000;
uint64_t GetFileSize(int32_t fd)
{
    uint64_t fileSize = 0;
    struct stat s {};
    if (fstat(fd, &s) == 0) {
        fileSize = static_cast<uint64_t>(s.st_size);
        return fileSize;
    }
    return fileSize;
}
}
Status FileFdSourceRegister(const std::shared_ptr<Register>& reg)
{
    SourcePluginDef definition;
    definition.name = "FileFdSource";
    definition.description = "File Fd source";
    definition.rank = 100; // 100: max rank
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

Status FileFdSourcePlugin::SetCallback(Callback* cb)
{
    MEDIA_LOG_D("IN");
    callback_ = cb;
    MEDIA_LOG_D("OUT");
    return Status::OK;
}

Status FileFdSourcePlugin::SetSource(std::shared_ptr<MediaSource> source)
{
    MEDIA_LOG_D("IN");
    auto err = ParseUriInfo(source->GetSourceUri());
    if (err != Status::OK) {
        MEDIA_LOG_E("Parse file name from uri fail, uri: " PUBLIC_LOG "s", source->GetSourceUri().c_str());
        return err;
    }
    MEDIA_LOG_D("OUT");
    return err;
}

void FileFdSourcePlugin::SubmitBufferingStart()
{
    MEDIA_LOG_I("SubmitBufferingStart in.");
    isBuffering_ = true;
    if (callback_ != nullptr && !isTaskCallback_) {
        MEDIA_LOG_I("Read OnEvent BUFFERING_START.");
        callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "start"});
    }
    if (downloadTask_ != nullptr) {
        downloadTask_->Start();
    }
    isTaskCallback_ = false;
}

void FileFdSourcePlugin::SubmitReadFail()
{
    if (callback_ != nullptr) {
        MEDIA_LOG_I("Read OnEvent read fail");
        callback_->OnEvent({PluginEventType::CLIENT_ERROR, {NetworkClientErrorCode::ERROR_TIME_OUT}, "read"});
    }
}

void FileFdSourcePlugin::StartTimerTask()
{
    readTime_ = 0;
    if (timerTask_ != nullptr) {
        timerTask_->Start();
    }
}

void FileFdSourcePlugin::PauseDownloadTask(bool isAsync)
{
    if (!isBuffering_) {
        return;
    }
    if (downloadTask_ == nullptr) {
        return;
    }
    isBuffering_ = false;
    if (isAsync) {
        downloadTask_->PauseAsync();
    } else {
        downloadTask_->Pause();
    }
}

void FileFdSourcePlugin::PauseTimerTask()
{
    if (timerTask_ != nullptr) {
        timerTask_->Pause();
    }
}

bool FileFdSourcePlugin::HandleBuffering()
{
    MEDIA_LOG_I("HandleBuffering in.");
    int32_t sleepTime = 0;
    while (sleepTime < ONE_MILLION_MICROSECOUNDS) {    // 1s
        if (!isBuffering_) {
            break;
        }
        usleep(TEN_THOUSAND_MICROSECOUNDS);
        sleepTime += TEN_THOUSAND_MICROSECOUNDS;
    }
    return isBuffering_;
}

Status FileFdSourcePlugin::Read(std::shared_ptr<Buffer>& buffer, uint64_t offset, size_t expectedLen)
{
    if (isBuffering_) {
        MEDIA_LOG_D("buffer position " PUBLIC_LOG_U64 ", expectedLen " PUBLIC_LOG_ZU ", fileSize "
            PUBLIC_LOG_U64, position_, expectedLen, fileSize_);
        if (HandleBuffering()) {
            MEDIA_LOG_I("return error again.");
            return Status::ERROR_AGAIN;
        }
    }
    if (!buffer) {
        buffer = std::make_shared<Buffer>();
    }
    std::shared_ptr<Memory> bufData;
    if (buffer->IsEmpty()) {
        bufData = buffer->AllocMemory(nullptr, expectedLen);
    } else {
        bufData = buffer->GetMemory();
    }
    expectedLen = std::min(static_cast<size_t>(size_ + offset_ - position_), expectedLen);
    expectedLen = std::min(bufData->GetCapacity(), expectedLen);
    MEDIA_LOG_D("buffer position " PUBLIC_LOG_U64 ", expectedLen " PUBLIC_LOG_ZU, position_, expectedLen);

    if (isReadFrame_) {
        StartTimerTask();
    }
    auto size = read(fd_, bufData->GetWritableAddr(expectedLen), expectedLen);
    isReadSuccess_ = size >= 0 ? : false;
    if (isReadFrame_) {
        PauseTimerTask();
        MEDIA_LOG_D("size: " PUBLIC_LOG_D64  "isTaskCallback_: " PUBLIC_LOG_U64,
            static_cast<uint64_t>(size), static_cast<uint64_t>(isTaskCallback_));
        if ((size > 0 && static_cast<size_t>(size) < expectedLen) || isTaskCallback_) {
            SubmitBufferingStart();
        }
    }

    if (size <= 0) {
        MEDIA_LOG_I("return EOS, buffer position " PUBLIC_LOG_U64 ", expectedLen " PUBLIC_LOG_ZU ", fileSize "
            PUBLIC_LOG_U64, position_, expectedLen, fileSize_);
        if (size < 0) {
            MEDIA_LOG_E("fd read fail errno: " PUBLIC_LOG_D32, static_cast<int32_t>(errno));
        }
        return Status::END_OF_STREAM;
    }

    bufData->UpdateDataSize(size);
    position_ += bufData->GetSize();
    MEDIA_LOG_D("position_: " PUBLIC_LOG_U64 ", readSize: " PUBLIC_LOG_ZU,
        position_, buffer->GetMemory()->GetSize());
    return Status::OK;
}

Status FileFdSourcePlugin::GetSize(uint64_t& size)
{
    MEDIA_LOG_D("IN");
    size = size_;
    MEDIA_LOG_D("Size_: " PUBLIC_LOG_U64, size);
    return Status::OK;
}

Seekable FileFdSourcePlugin::GetSeekable()
{
    MEDIA_LOG_D("IN");
    return seekable_;
}

Status FileFdSourcePlugin::SeekTo(uint64_t offset)
{
    FALSE_RETURN_V_MSG_E(fd_ != -1 && seekable_ == Seekable::SEEKABLE,
        Status::ERROR_WRONG_STATE, "no valid fd or no seekable.");
    if (isBuffering_) {
        if (downloadTask_ != nullptr) {
            downloadTask_->Pause();
        }
        isBuffering_ = false;
        callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
    }
    int32_t ret = lseek(fd_, offset + static_cast<uint64_t>(offset_), SEEK_SET);
    if (ret == -1) {
        MEDIA_LOG_E("seek to " PUBLIC_LOG_U64 " failed due to " PUBLIC_LOG_S, offset, strerror(errno));
        return Status::ERROR_UNKNOWN;
    }
    position_ = offset + static_cast<uint64_t>(offset_);
    MEDIA_LOG_D("now seek to " PUBLIC_LOG_D32, ret);
    return Status::OK;
}

Status FileFdSourcePlugin::ParseUriInfo(const std::string& uri)
{
    if (uri.empty()) {
        MEDIA_LOG_E("uri is empty");
        return Status::ERROR_INVALID_PARAMETER;
    }
    MEDIA_LOG_D("uri: " PUBLIC_LOG_S, uri.c_str());
    std::smatch fdUriMatch;
    FALSE_RETURN_V_MSG_E(std::regex_match(uri, fdUriMatch, std::regex("^fd://(.*)?offset=(.*)&size=(.*)")) ||
        std::regex_match(uri, fdUriMatch, std::regex("^fd://(.*)")),
        Status::ERROR_INVALID_PARAMETER, "Invalid fd uri format: " PUBLIC_LOG_S, uri.c_str());
    fd_ = std::stoi(fdUriMatch[1].str()); // 1: sub match fd subscript
    FALSE_RETURN_V_MSG_E(fd_ != -1 && FileSystem::IsRegularFile(fd_),
        Status::ERROR_INVALID_PARAMETER, "Invalid fd: " PUBLIC_LOG_D32, fd_);
    fileSize_ = GetFileSize(fd_);
    if (fdUriMatch.size() == 4) { // 4：4 sub match
        offset_ = std::stoll(fdUriMatch[2].str()); // 2: sub match offset subscript
        if (static_cast<uint64_t>(offset_) > fileSize_) {
            offset_ = fileSize_;
        }
        size_ = static_cast<uint64_t>(std::stoll(fdUriMatch[3].str())); // 3: sub match size subscript
        uint64_t remainingSize = fileSize_ - offset_;
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
    MEDIA_LOG_D("Fd: " PUBLIC_LOG_D32 ", offset: " PUBLIC_LOG_D64 ", size: " PUBLIC_LOG_U64, fd_, offset_, size_);
    return Status::OK;
}

Status FileFdSourcePlugin::Reset()
{
    return Status::OK;
}

int64_t FileFdSourcePlugin::ReadTimer()
{
    if (readTime_ > TEN_THOUSAND_MICROSECOUNDS) {   // 10ms
        if (callback_ != nullptr && !isTaskCallback_) {
            MEDIA_LOG_I("ReadTimer OnEvent BUFFERING_START readTime_: " PUBLIC_LOG_U64, readTime_);
            isBuffering_ = true;
            isTaskCallback_ = true;
            callback_->OnEvent({PluginEventType::BUFFERING_START, {BufferingInfoType::BUFFERING_START}, "pause"});
            if (timerTask_ != nullptr) {
                timerTask_->PauseAsync();
            }
        } else {
            MEDIA_LOG_D("BUFFERING_START callback_ is nullptr or isTaskCallback_ is null.");
        }
    }
    readTime_ += ONE_THOUSAND_MICROSECOUNDS;
    return ONE_THOUSAND_MICROSECOUNDS;
}

void FileFdSourcePlugin::SetDemuxerState()
{
    MEDIA_LOG_I("SetDemuxerState");
    isReadFrame_ = true;
}

void FileFdSourcePlugin::SetBundleName(const std::string& bundleName)
{
    MEDIA_LOG_I("SetBundleName bundleName: " PUBLIC_LOG_S, bundleName.c_str());
    bundleName_ = bundleName;
    timerTask_ = std::make_shared<Task>(std::string("timerTask"), "", TaskType::SINGLETON);
    timerTask_->SetEnableStateChangeLog(false);
    timerTask_->RegisterJob([this] { return ReadTimer(); });
    downloadTask_ = std::make_shared<Task>(std::string("downloadTask"), "", TaskType::SINGLETON);
    downloadTask_->RegisterJob([this] {
        CacheData();
        return 0;
    });
}

void FileFdSourcePlugin::HandleReadFail()
{
    PauseDownloadTask(true);
    SubmitReadFail();
}

void FileFdSourcePlugin::CacheData()
{
    size_t readSize = std::min(SAVE_BUFFER_SIZE, static_cast<size_t>(size_ + offset_ - position_));
    MEDIA_LOG_D("SAVE_BUFFER_SIZE: " PUBLIC_LOG_ZU ", readSize: " PUBLIC_LOG_ZU ", position_: " PUBLIC_LOG_U64,
        SAVE_BUFFER_SIZE, readSize, position_);
    size_t avaiableReadSize = readSize;
    char* cacheBuffer = new char[avaiableReadSize];

    while (avaiableReadSize > 0 && isReadSuccess_) {
        auto downloadReadSize = read(fd_, cacheBuffer, avaiableReadSize);
        if (downloadReadSize == 0) {
            MEDIA_LOG_D("fd read fail, cache data " PUBLIC_LOG_D64, static_cast<int64_t>(readSize - avaiableReadSize));
            break;
        } else if (downloadReadSize < 0) {
            isReadSuccess_ = false;
            MEDIA_LOG_I("buffer position " PUBLIC_LOG_U64 ", SAVE_BUFFER_SIZE " PUBLIC_LOG_ZU ", fileSize "
                PUBLIC_LOG_U64, position_, SAVE_BUFFER_SIZE, fileSize_);
            MEDIA_LOG_E("fd read fail errno: " PUBLIC_LOG_D32, static_cast<int32_t>(errno));
            break;
        } else {
            avaiableReadSize -= static_cast<size_t>(downloadReadSize);
        }
    }

    if (avaiableReadSize == 0) {
        MEDIA_LOG_I("Cache data over.");
    }

    if (cacheBuffer != nullptr) {
        delete[] cacheBuffer;
    }
    lseek(fd_, position_, SEEK_SET);

    if (!isReadSuccess_) {
        HandleReadFail();
    }

    if (callback_ != nullptr && isReadSuccess_) {
        MEDIA_LOG_I("ReadTimer OnEvent BUFFERING_END.");
        callback_->OnEvent({PluginEventType::BUFFERING_END, {BufferingInfoType::BUFFERING_END}, "end"});
    } else {
        MEDIA_LOG_I("BUFFERING_END callback_ is nullptr or isReadFail is true.");
    }

    PauseDownloadTask(true);
}
} // namespace FileFdSource
} // namespace Plugin
} // namespace Media
} // namespace OHOS