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

#define MEDIA_PLUGIN
#define HST_LOG_TAG "FfmpegDemuxerPlugin"
#include <unistd.h>
#include <chrono>
#include "avcodec_trace.h"
#include "securec.h"
#include "ffmpeg_format_helper.h"
#include "ffmpeg_utils.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "common/log.h"
#include "meta/video_types.h"
#include "demuxer_log_compressor.h"
#include "ffmpeg_demuxer_plugin.h"
#include "meta/format.h"
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "FfmpegDemuxerPlugin" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
const int32_t AV_READ_PACKET_READ_ERROR = -1;
const int32_t AV_READ_PACKET_READ_AGAIN = -2;
const int32_t AV_READ_PACKET_RETRY_UPPER_LIMIT = 9;
const int32_t AV_READ_PACKET_SLEEP_TIME = 50;

std::condition_variable FFmpegDemuxerPlugin::readCbCv_;
std::mutex FFmpegDemuxerPlugin::readPacketMutex_;

// Write packet data into the buffer provided by ffmpeg
int FFmpegDemuxerPlugin::AVReadPacket(void* opaque, uint8_t* buf, int bufSize)
{
    int ret = CheckContextIsValid(opaque, bufSize);
    FALSE_RETURN_V(ret == 0, ret);
    ret = -1;
    auto ioContext = static_cast<IOContext*>(opaque);
    FALSE_RETURN_V_MSG_E(ioContext != nullptr, ret, "IOContext is nullptr");
    auto buffer = std::make_shared<Buffer>();
    FALSE_RETURN_V_MSG_E(buffer != nullptr, ret, "Buffer is nullptr");
    auto bufData = buffer->WrapMemory(buf, bufSize, 0);
    FALSE_RETURN_V_MSG_E(buffer->GetMemory() != nullptr, ret, "Memory is nullptr");
    MediaAVCodec::AVCodecTrace trace("AVReadPacket_ReadAt");
    std::unique_lock<std::mutex> readLock(readPacketMutex_);
    int tryCount = 0;
    bool needBlockWait = false;
    do {
        auto result = ioContext->dataSource->ReadAt(ioContext->offset, buffer, static_cast<size_t>(bufSize));
        int dataSize = static_cast<int>(buffer->GetMemory()->GetSize());
        MEDIA_LOG_D("Want:" PUBLIC_LOG_D32 ", Get:" PUBLIC_LOG_D32 ", offset:" PUBLIC_LOG_D64 ", index:" PUBLIC_LOG_D32,
            bufSize, dataSize, ioContext->offset, readatIndex_.load());
    #ifdef BUILD_ENG_VERSION
        DumpParam dumpParam {DumpMode(DUMP_READAT_INPUT & ioContext->dumpMode), buf, -1, ioContext->offset,
            dataSize, readatIndex_++, -1, -1};
        Dump(dumpParam);
    #endif
        switch (result) {
            case Status::OK:
                ret = HandleReadOK(ioContext, dataSize);
                UpdateInitDownloadData(ioContext, dataSize);
                return ret;
            case Status::ERROR_AGAIN:
                ret = HandleReadAgain(ioContext, dataSize, tryCount, needBlockWait);
                if (ret != AV_READ_PACKET_READ_AGAIN) {
                    return ret;
                }
                break;
            case Status::END_OF_STREAM:
                ret = HandleReadEOS(ioContext);
                return ret;
            default:
                ret = HandleReadError(static_cast<int>(result));
                return ret;
        }
        HandleReadWaitOrSleep(needBlockWait, tryCount, readLock);
    } while (true);
    int dataSize = static_cast<int>(buffer->GetMemory()->GetSize());
    UpdateInitDownloadData(ioContext, dataSize);
    return ret;
}

void FFmpegDemuxerPlugin::HandleReadWaitOrSleep(bool& needBlockWait, int& tryCount,
                                                std::unique_lock<std::mutex>& readLock)
{
    if (needBlockWait) {
        readCbCv_.wait(readLock); // Wait to be notified
        needBlockWait = false;
        tryCount = 0;
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(AV_READ_PACKET_SLEEP_TIME));
    }
}

int FFmpegDemuxerPlugin::HandleReadOK(IOContext* ioContext, int dataSize)
{
    if (dataSize > 0) {
        ioContext->offset += dataSize;
        return dataSize;
    }
    return AV_READ_PACKET_READ_ERROR;
}

int FFmpegDemuxerPlugin::HandleReadAgain(IOContext* ioContext, int dataSize,
                                         int& tryCount, bool& needBlockWait)
{
    if (dataSize > 0) {
        ioContext->offset += dataSize;
        UpdateInitDownloadData(ioContext, dataSize);
        return dataSize;
    }
    if (ioContext->invokerType != READ) {
        ioContext->retry = true;
        ioContext->initErrorAgain = (ioContext->invokerType == INIT ? true : false);
        MEDIA_LOG_I("Read again, set retry, invokerType!=READ");
        return AV_READ_PACKET_READ_ERROR;
    }
    tryCount++;
    if (tryCount >= AV_READ_PACKET_RETRY_UPPER_LIMIT) {
        MEDIA_LOG_I("Read again 9 times, block wait for wakeup");
        needBlockWait = true;
    } else {
        MEDIA_LOG_I("Read again, retry count: " PUBLIC_LOG_D32, tryCount);
    }
    return AV_READ_PACKET_READ_AGAIN;
}

int FFmpegDemuxerPlugin::HandleReadEOS(IOContext* ioContext)
{
    MEDIA_LOG_I("Read end");
    ioContext->eos = true;
    return AVERROR_EOF;
}

int FFmpegDemuxerPlugin::HandleReadError(int result)
{
    MEDIA_LOG_I("Read failed " PUBLIC_LOG_D32, static_cast<int>(result));
    return AV_READ_PACKET_READ_ERROR;
}

void FFmpegDemuxerPlugin::UpdateInitDownloadData(IOContext* ioContext, int dataSize)
{
    if (!ioContext->initCompleted) {
        if (ioContext->initDownloadDataSize <= UINT32_MAX - static_cast<uint32_t>(dataSize)) {
            ioContext->initDownloadDataSize += static_cast<uint32_t>(dataSize);
        } else {
            MEDIA_LOG_W("DataSize " PUBLIC_LOG_U32 " is invalid", static_cast<uint32_t>(dataSize));
        }
    }
}

Status FFmpegDemuxerPlugin::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample, uint32_t timeout)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    Status ret;
    MediaAVCodec::AVCodecTrace trace("ReadSample_timeout");
    MEDIA_LOG_I("In");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(versionMap_.find(0) == versionMap_.end(), Status::ERROR_INVALID_OPERATION,
        "old version has been used");
    versionMap_[1] = 1;
    isPauseReadPacket_ = true;
    if (ioContext_.invokerType != READ) {
        std::lock_guard<std::mutex> readLock(ioContext_.invorkTypeMutex);
        ioContext_.invokerType = READ;
    }
    trackId_ = trackId;
    if (!readThread_) {
        readThread_ = std::make_unique<std::thread>(&FFmpegDemuxerPlugin::FFmpegReadLoop, this);
    }
    ret = WaitForLoop(trackId, timeout);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Asyn read ffmpeg thread abnoraml end");
    
    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    auto samplePacket = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_NULL_POINTER, "Cache packet is nullptr");

    if (samplePacket->isEOS) {
        ret = SetEosSample(sample);
        if (ret == Status::OK) {
            MEDIA_LOG_I("Track:" PUBLIC_LOG_D32 " eos [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "]",
                trackId, trackDfxInfoMap_[trackId].lastPts,
                trackDfxInfoMap_[trackId].lastDurantion, trackDfxInfoMap_[trackId].lastPos);
            cacheQueue_.Pop(trackId);
        }
        return ret;
    }

    ret = ConvertAVPacketToSample(sample, samplePacket);
    if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
        return Status::OK;
    } else if (ret == Status::OK) {
        MEDIA_LOG_D("All partial sample has been copied");
        cacheQueue_.Pop(trackId);
    }
    return ret;
}

Status FFmpegDemuxerPlugin::WaitForLoop(const uint32_t trackId, const uint32_t timeout)
{
    std::unique_lock<std::mutex> readLock(readSampleMutex_);
    if (!cacheQueue_.HasCache(trackId)) {
        if (threadState_ == READING) {
            readCbCv_.notify_one();
        }
        if (threadState_ == WAITING) {
            readLoopCv_.notify_one();
        }
        if (!readCacheCv_.wait_for(readLock, std::chrono::milliseconds(timeout),
            [this, trackId] { return cacheQueue_.HasCache(trackId); })) {
            FALSE_RETURN_V_MSG_E(readLoopStatus_ == Status::OK, readLoopStatus_, "ffmpegReadLoop thread abnoraml end");
            return Status::ERROR_WAIT_TIMEOUT;
        }
    }
    return Status::OK;
}

void FFmpegDemuxerPlugin::FFmpegReadLoop()
{
    threadState_ = READING;
    AVPacket *pkt = nullptr;
    bool continueRead = true;
    while (continueRead) {
        std::unique_lock<std::mutex> readLock(ioContext_.invorkTypeMutex);
        if (NeedWaitForRead()) {
            HandleReadWait(readLock);
        }
        readCacheCv_.notify_one();
        if (ioContext_.invokerType == DESTORY) {
            MEDIA_LOG_I("DESTORY trackId=" PUBLIC_LOG_D32, trackId_);
            break;
        }
        if (!EnsurePacketAllocated(pkt)) {
            break;
        }
        if (!ReadAndProcessFrame(pkt)) {
            break;
        }
        pkt = nullptr;
    }
    threadState_ = NOT_STARTED;
    readCacheCv_.notify_one();
    MEDIA_LOG_I("Read loop end");
}

bool FFmpegDemuxerPlugin::NeedWaitForRead()
{
    return (cacheQueue_.HasCache(trackId_) || !isPauseReadPacket_) && ioContext_.invokerType != DESTORY;
}

void FFmpegDemuxerPlugin::HandleReadWait(std::unique_lock<std::mutex>& readLock)
{
    threadState_ = WAITING;
    seekWaitCv_.notify_one();
    readLoopCv_.wait(readLock, [this]() {
        return (ioContext_.invokerType == DESTORY) || (!cacheQueue_.HasCache(trackId_) && isPauseReadPacket_);
    });
    threadState_ = READING;
}

bool FFmpegDemuxerPlugin::EnsurePacketAllocated(AVPacket*& pkt)
{
    if (pkt == nullptr) {
        pkt = av_packet_alloc();
        if (pkt == nullptr) {
            MEDIA_LOG_E("Call av_packet_alloc failed");
            readLoopStatus_ = Status::ERROR_NULL_POINTER;
            return false;
        }
    }
    return true;
}

bool FFmpegDemuxerPlugin::ReadAndProcessFrame(AVPacket* pkt)
{
    {
        std::unique_lock<std::mutex> sLock(syncMutex_);
        readLoopStatus_ = Status::OK;
        int ffmpegRet = AVReadFrameLimit(pkt);
        if (ffmpegRet == AVERROR_EOF) { // eos
            WebvttMP4EOSProcess(pkt);
            av_packet_free(&pkt);
            Status ret = PushEOSToAllCache();
            if (ret != Status::OK) {
                readLoopStatus_ = ret;
                return false;
            }
            readLoopStatus_ = Status::END_OF_STREAM;
            readCacheCv_.notify_one();
            return true;
        }
        if (ffmpegRet < 0) { // failed
            av_packet_free(&pkt);
            MEDIA_LOG_E("Call av_read_frame failed:" PUBLIC_LOG_S ", retry: " PUBLIC_LOG_D32,
                AVStrError(ffmpegRet).c_str(), int(ioContext_.retry));
            readLoopStatus_ = Status::ERROR_UNKNOWN;
            return false;
        }
    }
    auto trackId = pkt->stream_index;
    if (!TrackIsSelected(trackId)) {
        av_packet_unref(pkt);
        return true;
    }
    AVStream *avStream = formatContext_->streams[trackId];
    if (IsWebvttMP4(avStream) && WebvttPktProcess(pkt)) {
        return true;
    }
    Status ret = AddPacketToCacheQueue(pkt);
    if (ret != Status::OK) {
        MEDIA_LOG_E("Add cache failed");
        readLoopStatus_ = ret;
        return false;
    }
    readCacheCv_.notify_one();
    return true;
}

void FFmpegDemuxerPlugin::ReleaseFFmpegReadLoop()
{
    if (ioContext_.invokerType != DESTORY) {
        std::lock_guard<std::mutex> DestoryLock(ioContext_.invorkTypeMutex);
        ioContext_.invokerType = DESTORY;
        readLoopCv_.notify_one();
    }
    readCbCv_.notify_one();
    if (readThread_ != nullptr && readThread_->joinable()) {
        readThread_->join();
    }
    readThread_ = nullptr;
}

Status FFmpegDemuxerPlugin::GetNextSampleSize(uint32_t trackId, int32_t& size, uint32_t timeout)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("GetNextSampleSize_timeout");
    MEDIA_LOG_D("In, track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_UNKNOWN, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_UNKNOWN, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(versionMap_.find(0) == versionMap_.end(), Status::ERROR_INVALID_OPERATION,
        "old version has been used");

    versionMap_[1] = 1;
    if (ioContext_.invokerType != READ) {
        std::lock_guard<std::mutex> ReadLock(ioContext_.invorkTypeMutex);
        ioContext_.invokerType = READ;
    }
    trackId_ = trackId;
    if (!readThread_) {
        readThread_ = std::make_unique<std::thread>(&FFmpegDemuxerPlugin::FFmpegReadLoop, this);
    }

    Status ret = WaitForLoop(trackId, timeout);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Asyn read ffmpeg thread abnoraml end");

    std::shared_ptr<SamplePacket> samplePacket = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_UNKNOWN, "Cache sample is nullptr");
    if (samplePacket->isEOS) {
        MEDIA_LOG_I("Track " PUBLIC_LOG_D32 " eos", trackId);
        return Status::END_OF_STREAM;
    }
    FALSE_RETURN_V_MSG_E(samplePacket->pkts.size() > 0, Status::ERROR_UNKNOWN, "Cache sample is empty");
    int totalSize = 0;
    for (auto pkt : samplePacket->pkts) {
        FALSE_RETURN_V_MSG_E(pkt != nullptr, Status::ERROR_UNKNOWN, "Packet in sample is nullptr");
        totalSize += pkt->size;
    }

    FALSE_RETURN_V_MSG_E(trackId < formatContext_->nb_streams, Status::ERROR_UNKNOWN, "Track is out of range");
    AVStream* avStream = formatContext_->streams[trackId];
    FALSE_RETURN_V_MSG_E(avStream != nullptr && avStream->codecpar != nullptr,
        Status::ERROR_UNKNOWN, "AVStream is nullptr");
    if ((std::count(g_streamContainedXPS.begin(), g_streamContainedXPS.end(), avStream->codecpar->codec_id) > 0) &&
        static_cast<uint32_t>(samplePacket->pkts[0]->flags) & static_cast<uint32_t>(AV_PKT_FLAG_KEY)) {
        totalSize += avStream->codecpar->extradata_size;
    }
    size = totalSize;
    return Status::OK;
}

Status FFmpegDemuxerPlugin::PauseFFmpegReadLoop()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    isPauseReadPacket_ = false;
    return Status::OK;
}

Status FFmpegDemuxerPlugin::GetLastPTSByTrackId(uint32_t trackId, int64_t &lastPTS)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    auto ret = cacheQueue_.GetLastPTSByTrackId(trackId, lastPTS);
    if (ret != Status::OK) {
        MEDIA_LOG_E("Get last pts failed");
    }
    return ret;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
