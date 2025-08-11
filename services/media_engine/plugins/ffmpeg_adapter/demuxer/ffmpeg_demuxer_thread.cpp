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
#include "qos.h"
#include "res_type.h"
#include "res_sched_client.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "FfmpegDemuxerThread" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
const int32_t AV_READ_PACKET_READ_ERROR = -1;
const int32_t AV_READ_PACKET_READ_AGAIN = -2;
const int32_t AV_READ_PACKET_RETRY_UPPER_LIMIT = 9;
const int32_t AV_READ_PACKET_SLEEP_TIME = 50;
constexpr uint32_t RES_TYPE = OHOS::ResourceSchedule::ResType::RES_TYPE_THREAD_QOS_CHANGE;
constexpr uint32_t RES_VALUE = 0;

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
    int tryCount = 0;
    do {
        auto result = ioContext->dataSource->ReadAt(ioContext->offset, buffer, static_cast<size_t>(bufSize));
        int dataSize = static_cast<int>(buffer->GetMemory()->GetSize());
        UpdateInitDownloadData(ioContext, dataSize);
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
                break;
            case Status::ERROR_AGAIN:
                ret = HandleReadAgain(ioContext, dataSize, tryCount);
                break;
            case Status::END_OF_STREAM:
                ret = HandleReadEOS(ioContext);
                break;
            default:
                ret = HandleReadError(static_cast<int>(result));
                break;
        }
    } while (ret == AV_READ_PACKET_READ_AGAIN);
    return ret;
}

int FFmpegDemuxerPlugin::HandleReadOK(IOContext* ioContext, int dataSize)
{
    if (dataSize > 0) {
        ioContext->offset += dataSize;
        return dataSize;
    }
    return AV_READ_PACKET_READ_ERROR;
}

int FFmpegDemuxerPlugin::HandleReadAgain(IOContext* ioContext, int dataSize, int& tryCount)
{
    if (dataSize > 0) {
        ioContext->offset += dataSize;
        return dataSize;
    }
    if (ioContext->invokerType != READ) {
        ioContext->retry = true;
        ioContext->initErrorAgain = (ioContext->invokerType == INIT ? true : false);
        MEDIA_LOG_I("Read again, invokerType!=READ, offset:" PUBLIC_LOG_D64, ioContext->offset);
        return AV_READ_PACKET_READ_ERROR;
    }
    tryCount++;
    if (tryCount >= AV_READ_PACKET_RETRY_UPPER_LIMIT) {
        std::unique_lock<std::mutex> readLock(readPacketMutex_);
        readCbCv_.wait(readLock, [ioContext]() {return ioContext->readCbReady;}); // Wait to be notified
        ioContext->readCbReady = false; // Reset the flag
        tryCount = 0;
    } else {
        MEDIA_LOG_I("Read again, retry count: " PUBLIC_LOG_D32 ", offset:" PUBLIC_LOG_D64, tryCount, ioContext->offset);
        std::this_thread::sleep_for(std::chrono::milliseconds(AV_READ_PACKET_SLEEP_TIME));
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
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    Status ret;
    MediaAVCodec::AVCodecTrace trace("ReadSample_timeout");
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_!=nullptr, Status::ERROR_INVALID_PARAMETER,
        "AVBuffer or memory is nullptr");
    FALSE_RETURN_V_MSG_E(readModeMap_.find(0) == readModeMap_.end(), Status::ERROR_INVALID_OPERATION,
        "Cannot use sync and async Read together");
    readModeMap_[1] = 1;
    isPauseReadPacket_ = false;
    if (ioContext_.invokerType != InvokerType::READ) {
        std::lock_guard<std::mutex> readLock(ioContext_.invokerTypeMutex);
        ioContext_.invokerType = InvokerType::READ;
    }
    trackId_ = trackId;
    if (!readThread_) {
        readThread_ = std::make_unique<std::thread>(&FFmpegDemuxerPlugin::FFmpegReadLoop, this);
    }
    ret = WaitForLoop(trackId, timeout);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Frame reading thread error, ret = " PUBLIC_LOG_D32, ret);
    
    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    auto samplePacket = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_NULL_POINTER, "Cache packet is nullptr");

    if (samplePacket->isEOS) {
        ret = SetEosSample(sample);
        if (ret == Status::OK) {
            DumpPacketInfo(trackId, Stage::FILE_END);
            cacheQueue_.Pop(trackId);
        }
        return ret;
    }

    ret = ConvertAVPacketToSample(sample, samplePacket);
    DumpPacketInfo(trackId, Stage::FIRST_READ);
    if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
        return Status::OK;
    } else if (ret == Status::OK) {
        MEDIA_LOG_D("All partial sample has been copied");
        cacheQueue_.Pop(trackId);
    }
    return ret;
}

bool FFmpegDemuxerPlugin::ShouldWaitForRead(uint32_t trackId)
{
    if ((NeedCombineFrame(trackId) && cacheQueue_.GetCacheSize(trackId) <= 1) ||
        (!NeedCombineFrame(trackId) && !cacheQueue_.HasCache(trackId))) {
        return true;
    }
    return false;
}

Status FFmpegDemuxerPlugin::WaitForLoop(const uint32_t trackId, const uint32_t timeout)
{
    if (ShouldWaitForRead(trackId)) {
        if (threadState_ == READING) {
            std::lock_guard<std::mutex> readLock(readPacketMutex_);
            ioContext_.readCbReady = true;
            readCbCv_.notify_one();
        }
        if (threadState_ == WAITING) {
            std::lock_guard<std::mutex> readLock(ioContext_.invokerTypeMutex);
            threadReady_ = true;
            readLoopCv_.notify_one();
        }
        {
            std::unique_lock<std::mutex> readLock(readSampleMutex_);
            if (!readCacheCv_.wait_for(readLock, std::chrono::milliseconds(timeout),
                [this, trackId] { return !ShouldWaitForRead(trackId);})) {
                FALSE_RETURN_V_MSG_E(readLoopStatus_ == Status::OK, readLoopStatus_, "read thread abnoraml end");
                return Status::ERROR_WAIT_TIMEOUT;
            }
        }
    }
    return Status::OK;
}

void FFmpegDemuxerPlugin::FFmpegReadLoop()
{
    if (isAsyncReadThreadPrioritySet_) {
        UpdateAsyncReadThreadPriority();
    }
    threadState_ = READING;
    AVPacket *pkt = nullptr;
    bool continueRead = true;
    while (continueRead) {
        if ((!NeedCombineFrame(trackId_) || cacheQueue_.GetCacheSize(trackId_) != 1) && NeedWaitForRead()) {
            HandleReadWait();
        }
        {
            std::lock_guard<std::mutex> readLock(readSampleMutex_);
            readCacheCv_.notify_one();
        }
        if (ioContext_.invokerType == InvokerType::DESTORY) {
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
    {
        std::lock_guard<std::mutex> readLock(readSampleMutex_);
        readCacheCv_.notify_one();
    }
    {
        std::lock_guard<std::mutex> seekWaitLock(seekWaitMutex_);
        threadState_ = NOT_STARTED;
        seekWaitCv_.notify_one();
    }
    MEDIA_LOG_I("Read loop end");
}

bool FFmpegDemuxerPlugin::NeedWaitForRead()
{
    return (cacheQueue_.HasCache(trackId_) || isPauseReadPacket_) && ioContext_.invokerType != InvokerType::DESTORY;
}

void FFmpegDemuxerPlugin::HandleReadWait()
{
    std::unique_lock<std::mutex> readLock(ioContext_.invokerTypeMutex);
    {
        std::unique_lock<std::mutex> seekWaitLock(seekWaitMutex_);
        threadState_ = WAITING;
        seekWaitCv_.notify_one();
    }
    readLoopCv_.wait(readLock, [this]() {
        return (threadReady_) || (ioContext_.invokerType == InvokerType::DESTORY) ||
               (!cacheQueue_.HasCache(trackId_) && !isPauseReadPacket_);
    });
    threadState_ = READING;
    threadReady_ = false;
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
        if (ffmpegRet == AVERROR_EOF) {
            HandleAVPacketEndOfStream(pkt);
            return true;
        }
        if (ffmpegRet < 0) {
            HandleAVPacketReadError(pkt, ffmpegRet);
            return false;
        }
    }
    auto trackId = pkt->stream_index;
    if (!TrackIsSelected(trackId)) {
        av_packet_unref(pkt);
        return true;
    }
    AVStream* avStream = formatContext_->streams[trackId];
    bool isWebVTT = IsWebvttMP4(avStream);
    if (isWebVTT && WebvttPktProcess(pkt)) {
        return true;
    }
    Status ret = AddPacketToCacheQueue(pkt);
    if (ret != Status::OK) {
        MEDIA_LOG_E("Add cache failed");
        readLoopStatus_ = ret;
        return false;
    }
    if (isWebVTT) {
        AVPacket* vttePkt = nullptr;
        if (!EnsurePacketAllocated(vttePkt)) {
            readLoopStatus_ = Status::ERROR_NULL_POINTER;
            return false;
        }
        if (!ReadOnePacketAndProcessWebVTT(vttePkt)) {
            return false;
        }
    }
    if (!NeedCombineFrame(trackId_) || cacheQueue_.GetCacheSize(trackId_) != 1) {
        std::lock_guard<std::mutex> readLock(readSampleMutex_);
        readCacheCv_.notify_one();
    }
    return true;
}

void FFmpegDemuxerPlugin::HandleAVPacketEndOfStream(AVPacket* pkt)
{
    WebvttMP4EOSProcess(pkt);
    av_packet_free(&pkt);
    Status ret = PushEOSToAllCache();
    if (ret != Status::OK) {
        readLoopStatus_ = ret;
        return;
    }
    readLoopStatus_ = Status::END_OF_STREAM;
    {
        std::lock_guard<std::mutex> readLock(readSampleMutex_);
        readCacheCv_.notify_one();
    }
}

void FFmpegDemuxerPlugin::HandleAVPacketReadError(AVPacket* pkt, int ffmpegRet)
{
    FALSE_RETURN_MSG(pkt != nullptr, "AVPacket is nullptr");
    av_packet_free(&pkt);
    MEDIA_LOG_E("Call av_read_frame failed:" PUBLIC_LOG_S ", retry: " PUBLIC_LOG_D32,
        AVStrError(ffmpegRet).c_str(), int(ioContext_.retry));
    readLoopStatus_ = Status::ERROR_UNKNOWN;
}

bool FFmpegDemuxerPlugin::ReadOnePacketAndProcessWebVTT(AVPacket* pkt)
{
    std::unique_lock<std::mutex> sLock(syncMutex_);
    readLoopStatus_ = Status::OK;
    int ffmpegRet = AVReadFrameLimit(pkt);
    if (ffmpegRet == AVERROR_EOF) {
        HandleAVPacketEndOfStream(pkt);
        return false;
    }
    if (ffmpegRet < 0) {
        HandleAVPacketReadError(pkt, ffmpegRet);
        return false;
    }
    if (WebvttPktProcess(pkt)) {
        MEDIA_LOG_D("Webvtt packet processed successfully");
    }
    return true;
}
void FFmpegDemuxerPlugin::ReleaseFFmpegReadLoop()
{
    if (ioContext_.invokerType != InvokerType::DESTORY) {
        std::lock_guard<std::mutex> destoryLock(ioContext_.invokerTypeMutex);
        ioContext_.invokerType = InvokerType::DESTORY;
        readLoopCv_.notify_one();
    }
    std::unique_lock<std::mutex> readLock(readPacketMutex_);
    ioContext_.readCbReady = true;
    readCbCv_.notify_one();
    readLock.unlock();
    if (readThread_ != nullptr && readThread_->joinable()) {
        readThread_->join();
    }
    readThread_ = nullptr;
    readLoopStatus_ = Status::OK; // Reset readLoopStatus_ to OK after destroy
}

Status FFmpegDemuxerPlugin::GetNextSampleSize(uint32_t trackId, int32_t& size, uint32_t timeout)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("GetNextSampleSize_timeout");
    MEDIA_LOG_D("In, track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_UNKNOWN, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_UNKNOWN, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(readModeMap_.find(0) == readModeMap_.end(), Status::ERROR_INVALID_OPERATION,
        "Cannot use sync and async Read together");
    readModeMap_[1] = 1;
    isPauseReadPacket_ = false;
    if (ioContext_.invokerType != InvokerType::READ) {
        std::lock_guard<std::mutex> readLock(ioContext_.invokerTypeMutex);
        ioContext_.invokerType = InvokerType::READ;
    }
    trackId_ = trackId;
    if (!readThread_) {
        readThread_ = std::make_unique<std::thread>(&FFmpegDemuxerPlugin::FFmpegReadLoop, this);
    }

    Status ret = WaitForLoop(trackId, timeout);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Frame reading thread error, ret = " PUBLIC_LOG_D32, ret);

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

Status FFmpegDemuxerPlugin::Pause()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    isPauseReadPacket_ = true;
    return Status::OK;
}

Status FFmpegDemuxerPlugin::GetLastPTSByTrackId(uint32_t trackId, int64_t &lastPTS)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    lastPTS = INT64_MIN;
    auto ret = cacheQueue_.GetLastPTSByTrackId(trackId, lastPTS);
    if (ret != Status::OK) {
        MEDIA_LOG_E("Get last pts failed");
    }
    return ret;
}

Status FFmpegDemuxerPlugin::SetAsyncReadThreadPriority(const uint32_t newPriority, const std::string &strBundleName)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    if (threadState_ != ThreadState::NOT_STARTED && readThread_ != nullptr) {
        MEDIA_LOG_W("Async read thread is running, cannot set priority now");
        return Status::ERROR_WRONG_STATE;
    }
    if (isAsyncReadThreadPrioritySet_) {
        MEDIA_LOG_W("Async read thread priority has been set, cannot set again");
        return Status::ERROR_WRONG_STATE;
    }
    isAsyncReadThreadPrioritySet_ = true;
    asyncReadThreadPriority_ = newPriority;
    bundleName_ = strBundleName;
    MEDIA_LOG_I("Set async read thread priority to " PUBLIC_LOG_U32 " for bundle: %s",
        asyncReadThreadPriority_.load(), bundleName_.c_str());
    return Status::OK;
}

void FFmpegDemuxerPlugin::UpdateAsyncReadThreadPriority()
{
    MEDIA_LOG_I("Update async read thread priority to " PUBLIC_LOG_U32 " for bundle: " PUBLIC_LOG_S,
        asyncReadThreadPriority_.load(), bundleName_.c_str());
    std::unordered_map<std::string, std::string> mapPayload;
    mapPayload["bundleName"] = bundleName_;
    mapPayload["pid"] = std::to_string(getpid());
    mapPayload[std::to_string(gettid())] = std::to_string(asyncReadThreadPriority_.load());
    OHOS::ResourceSchedule::ResSchedClient::GetInstance().ReportData(RES_TYPE, RES_VALUE, mapPayload);
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
