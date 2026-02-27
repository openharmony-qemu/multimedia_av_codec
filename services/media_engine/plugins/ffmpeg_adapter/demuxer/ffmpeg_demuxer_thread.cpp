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
#include "avcodec_log.h"
#include "common/log.h"
#include "meta/video_types.h"
#include "demuxer_log_compressor.h"
#include "ffmpeg_demuxer_plugin.h"
#include "meta/format.h"
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "FfmpegDemuxerThread" };
constexpr int64_t LOG_INTERVAL_MS = 2000; // 2s
constexpr uint32_t LOG_MAX_COUNT = 10; // 10 times
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
    if (ioContext->invokerType != InvokerType::READ) {
        switch (ioContext->avReadPacketStopState.load()) {
            case AVReadPacketStopState::TRUE:
                MEDIA_LOG_I("AVReadPacket stopped");
                return AV_READ_PACKET_READ_ERROR;
            case AVReadPacketStopState::FALSE:
                MEDIA_LOG_I("Read again and retry, offset:" PUBLIC_LOG_D64, ioContext->offset);
                std::this_thread::sleep_for(std::chrono::milliseconds(AV_READ_PACKET_SLEEP_TIME));
                return AV_READ_PACKET_READ_AGAIN;
            case AVReadPacketStopState::UNSET:
                ioContext->retry = true;
                ioContext->initErrorAgain = (ioContext->invokerType == InvokerType::INIT ? true : false);
                MEDIA_LOG_I("Read again, UNSET state, offset:" PUBLIC_LOG_D64, ioContext->offset);
                return AV_READ_PACKET_READ_ERROR;
            default:
                MEDIA_LOG_E("Invalid AVReadPacketStopState");
                break;
        }
    }
    tryCount++;
    if (tryCount >= AV_READ_PACKET_RETRY_UPPER_LIMIT) {
        std::unique_lock<std::mutex> readLock(readPacketMutex_);
        readCbCv_.wait(readLock, [ioContext]() {
            return (ioContext->readCbReady) || (ioContext->invokerType != InvokerType::READ);
        }); // Wait to be notified
        ioContext->readCbReady.store(false); // Reset the flag
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
        if (ioContext->initDownloadDataSize <= UINT64_MAX - static_cast<uint64_t>(dataSize)) {
            ioContext->initDownloadDataSize += static_cast<uint64_t>(dataSize);
        } else {
            AVCODEC_LOG_LIMIT_IN_TIME(AVCODEC_LOGW, LOG_INTERVAL_MS, LOG_MAX_COUNT,
                "DataSize " PUBLIC_LOG_U64 " is invalid", static_cast<uint64_t>(dataSize));
        }
    }
}

Status FFmpegDemuxerPlugin::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> sample, uint32_t timeout)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    Status ret;
    MediaAVCodec::AVCodecTrace trace(std::string("ReadSample_timeout_") + std::to_string(trackId));
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "AVBuffer or memory is nullptr");
    FALSE_RETURN_V_MSG_E((readModeFlags_.load(std::memory_order_acquire) & ReadModeToFlags(ReadMode::SYNC)) == 0,
        Status::ERROR_INVALID_OPERATION,
        "Cannot use sync and async Read together");
    readModeFlags_.fetch_or(ReadModeToFlags(ReadMode::ASYNC), std::memory_order_acq_rel);
    isPauseReadPacket_.store(false);
    if (ioContext_.invokerType != InvokerType::READ) {
        std::lock_guard<std::mutex> readLock(ioContext_.invokerTypeMutex);
        ioContext_.invokerType = InvokerType::READ;
    }
    trackId_ = trackId;
    if (!cacheQueue_.HasCache(trackId) && timeout == 0) {
        return Status::ERROR_WAIT_TIMEOUT;
    }
    if (!readThread_) {
        readThread_ = std::make_unique<std::thread>(&FFmpegDemuxerPlugin::FFmpegReadLoop, this);
    }
    ret = WaitForLoop(trackId, timeout);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Frame reading thread error, ret = " PUBLIC_LOG_D32, ret);
    
    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    auto samplePacket = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_NULL_POINTER, "Cache packet is nullptr");

    if (samplePacket->isEOS) {
        ret = SetEosSample(sample, false);
        if (ret == Status::OK) {
            DumpPacketInfo(trackId, Stage::FILE_END);
            cacheQueue_.Pop(trackId);
        }
        return ret;
    }
    SupplementFirstFrameIfPending(trackId, samplePacket);
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

Status FFmpegDemuxerPlugin::ReadSampleZeroCopy(uint32_t trackId, std::shared_ptr<AVBuffer> sample, uint32_t timeout)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    Status ret;
    MediaAVCodec::AVCodecTrace trace(std::string("ReadSampleZeroCopy_timeout_") + std::to_string(trackId));
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_NULL_POINTER, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_INVALID_PARAMETER, "AVBuffer is nullptr");
    FALSE_RETURN_V_MSG_E((readModeFlags_.load(std::memory_order_acquire) & ReadModeToFlags(ReadMode::SYNC)) == 0,
        Status::ERROR_INVALID_OPERATION,
        "Cannot use sync and async Read together");
    readModeFlags_.fetch_or(ReadModeToFlags(ReadMode::ASYNC), std::memory_order_acq_rel);
    isPauseReadPacket_.store(false);
    if (ioContext_.invokerType != InvokerType::READ) {
        std::lock_guard<std::mutex> readLock(ioContext_.invokerTypeMutex);
        ioContext_.invokerType = InvokerType::READ;
    }
    trackId_ = trackId;
    if (!cacheQueue_.HasCache(trackId) && timeout == 0) {
        return Status::ERROR_WAIT_TIMEOUT;
    }
    if (!readThread_) {
        readThread_ = std::make_unique<std::thread>(&FFmpegDemuxerPlugin::FFmpegReadLoop, this);
    }
    ret = WaitForLoop(trackId, timeout);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Frame reading thread error, ret = " PUBLIC_LOG_D32, ret);

    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    auto samplePacket = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_NULL_POINTER, "Cache packet is nullptr");

    if (samplePacket->isEOS) {
        ret = SetEosSample(sample, true);
        if (ret == Status::OK) {
            DumpPacketInfo(trackId, Stage::FILE_END);
            cacheQueue_.Pop(trackId);
        }
        return ret;
    }
    SupplementFirstFrameIfPending(trackId, samplePacket);
    ret = ConvertAVPacketToSampleMemory(sample, samplePacket);
    DumpPacketInfo(trackId, Stage::FIRST_READ);
    if (ret == Status::OK) {
        cacheQueue_.Pop(trackId);
    }
    return ret;
}

Status FFmpegDemuxerPlugin::ConvertAVPacketToSampleMemory(
    std::shared_ptr<AVBuffer> sample, std::shared_ptr<SamplePacket> samplePacket)
{
    Status bufferIsValid = BufferIsValid(sample, samplePacket);
    FALSE_RETURN_V_MSG_E(bufferIsValid == Status::OK, bufferIsValid, "AVBuffer or packet is invalid");
    WriteBufferAttr(sample, samplePacket);

    // convert
    Plugins::AVPacketWrapperPtr tempPktWrapper = CombinePackets(samplePacket);
    AVPacket *tempPkt = (tempPktWrapper != nullptr) ? tempPktWrapper->GetAVPacket() : nullptr;
    FALSE_RETURN_V_MSG_E(tempPkt != nullptr, Status::ERROR_INVALID_OPERATION, "Combine packets failed");
    bool combined = tempPktWrapper != samplePacket->pkts[0];
    if (cacheQueue_.ResetInfo(samplePacket) == false) {
        MEDIA_LOG_D("Reset info failed");
    }

    MaybeInitSeekCalibAfterRead(tempPktWrapper->GetStreamIndex(), tempPkt);

    Status ret = ConvertToAnnexbAndUpdateSample(sample, samplePacket, tempPktWrapper);
    if (combined) {
        tempPktWrapper.reset();
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
    FALSE_RETURN_V_MSG_E(readThread_ != nullptr, Status::ERROR_UNKNOWN, "Read thread is nullptr");
    if (ShouldWaitForRead(trackId)) {
        isWaitingForReadThread_.store(true);
        if (threadState_ == READING) {
            std::lock_guard<std::mutex> readLock(readPacketMutex_);
            ioContext_.readCbReady.store(true);
            readCbCv_.notify_one();
        }
        if (threadState_ == WAITING) {
            std::lock_guard<std::mutex> readLock(ioContext_.invokerTypeMutex);
            threadReady_.store(true);
            readLoopCv_.notify_one();
        }
        {
            std::unique_lock<std::mutex> readLock(readSampleMutex_);
            if (!readCacheCv_.wait_for(readLock, std::chrono::milliseconds(timeout),
                [this, trackId] { return !ShouldWaitForRead(trackId);})) {
                isWaitingForReadThread_.store(false);
                FALSE_RETURN_V_MSG_E(readLoopStatus_ == Status::OK, readLoopStatus_, "read thread abnoraml end");
                return Status::ERROR_WAIT_TIMEOUT;
            }
        }
    }
    isWaitingForReadThread_.store(false);
    return Status::OK;
}

void FFmpegDemuxerPlugin::FFmpegReadLoop()
{
    MEDIA_LOG_I("Read loop start");
    if (isAsyncReadThreadPrioritySet_.load()) {
        UpdateAsyncReadThreadPriority();
    }
    threadState_ = READING;
    Plugins::AVPacketWrapperPtr pktWrapper = nullptr;
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
        if (!EnsurePacketAllocated(pktWrapper)) {
            break;
        }
        if (!ReadAndProcessFrame(pktWrapper)) {
            break;
        }
        pktWrapper = nullptr;
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
               (!cacheQueue_.HasCache(trackId_) && !isPauseReadPacket_) ||
               (isWaitingForReadThread_.load() && cacheQueue_.GetCacheSize(trackId_) <= 1);
    });
    threadState_ = READING;
    threadReady_.store(false);
}

bool FFmpegDemuxerPlugin::EnsurePacketAllocated(Plugins::AVPacketWrapperPtr& pktWrapper)
{
    if (!pktWrapper) {
        pktWrapper = std::make_shared<Plugins::AVPacketWrapper>();
        if (pktWrapper == nullptr || pktWrapper->GetAVPacket() == nullptr) {
            MEDIA_LOG_E("Create AVPacketWrapper failed");
            readLoopStatus_ = Status::ERROR_NULL_POINTER;
            return false;
        }
    } else if (pktWrapper->GetAVPacket() == nullptr) {
        MEDIA_LOG_E("AVPacketWrapper holds null AVPacket");
        readLoopStatus_ = Status::ERROR_NULL_POINTER;
        return false;
    }
    return true;
}

bool FFmpegDemuxerPlugin::ReadAndProcessFrame(Plugins::AVPacketWrapperPtr& pktWrapper)
{
    AVPacket *pkt = (pktWrapper != nullptr) ? pktWrapper->GetAVPacket() : nullptr;
    FALSE_RETURN_V_MSG_E(pkt != nullptr, false, "Pkt is nullptr");
    {
        readLoopStatus_ = Status::OK;
        int ffmpegRet = AVReadFrameLimit(pkt);
        std::unique_lock<std::mutex> sLock(syncMutex_);
        UpdMinTsPacketInfo(pktWrapper->GetAVPacket());
        if (ffmpegRet == AVERROR_EOF) {
            HandleAVPacketEndOfStream(pktWrapper);
            return true;
        }
        if (ffmpegRet < 0) {
            HandleAVPacketReadError(pktWrapper, ffmpegRet);
            return false;
        }
    }
    auto trackId = pkt->stream_index;
    if (!TrackIsSelected(trackId)) {
        av_packet_unref(pkt);
        return true;
    }
    AVStream* avStream = formatContext_->streams[trackId];
    UpdateCachedDrmInfoFromStream(avStream); // Extract and update DRM info
    bool isWebVTT = IsWebvttMP4(avStream);
    if (isWebVTT && WebvttPktProcess(pkt)) {
        return true;
    }
    Status ret = AddPacketToCacheQueue(pktWrapper);
    if (ret != Status::OK) {
        MEDIA_LOG_E("Add cache failed");
        readLoopStatus_ = ret;
        return false;
    }
    if (isWebVTT) {
        Plugins::AVPacketWrapperPtr vtteWrapper = nullptr;
        if (!EnsurePacketAllocated(vtteWrapper)) {
            readLoopStatus_ = Status::ERROR_NULL_POINTER;
            return false;
        }
        if (!ReadOnePacketAndProcessWebVTT(vtteWrapper)) {
            return false;
        }
    }
    if (!NeedCombineFrame(trackId_) || cacheQueue_.GetCacheSize(trackId_) != 1) {
        std::lock_guard<std::mutex> readLock(readSampleMutex_);
        readCacheCv_.notify_one();
    }
    return true;
}

void FFmpegDemuxerPlugin::HandleAVPacketEndOfStream(Plugins::AVPacketWrapperPtr& pktWrapper)
{
    AVPacket *pkt = (pktWrapper != nullptr) ? pktWrapper->GetAVPacket() : nullptr;
    WebvttMP4EOSProcess(pkt);
    pktWrapper.reset();
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

void FFmpegDemuxerPlugin::HandleAVPacketReadError(Plugins::AVPacketWrapperPtr& pktWrapper, int ffmpegRet)
{
    AVPacket *pkt = (pktWrapper != nullptr) ? pktWrapper->GetAVPacket() : nullptr;
    FALSE_RETURN_MSG(pkt != nullptr, "AVPacket is nullptr");
    pktWrapper.reset();
    MEDIA_LOG_E("Call av_read_frame failed:" PUBLIC_LOG_S ", retry: " PUBLIC_LOG_D32,
        AVStrError(ffmpegRet).c_str(), int(ioContext_.retry));
    readLoopStatus_ = Status::ERROR_UNKNOWN;
}

bool FFmpegDemuxerPlugin::ReadOnePacketAndProcessWebVTT(Plugins::AVPacketWrapperPtr& pktWrapper)
{
    AVPacket *pkt = (pktWrapper != nullptr) ? pktWrapper->GetAVPacket() : nullptr;
    readLoopStatus_ = Status::OK;
    int ffmpegRet = AVReadFrameLimit(pkt);
    std::unique_lock<std::mutex> sLock(syncMutex_);
    UpdMinTsPacketInfo(pktWrapper->GetAVPacket());
    if (ffmpegRet == AVERROR_EOF) {
        HandleAVPacketEndOfStream(pktWrapper);
        return false;
    }
    if (ffmpegRet < 0) {
        HandleAVPacketReadError(pktWrapper, ffmpegRet);
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
    ioContext_.readCbReady.store(true);
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
    MediaAVCodec::AVCodecTrace trace(std::string("GetNextSampleSize_timeout_") + std::to_string(trackId));
    MEDIA_LOG_D("In, track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(formatContext_ != nullptr, Status::ERROR_UNKNOWN, "AVFormatContext is nullptr");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_UNKNOWN, "Track has not been selected");
    FALSE_RETURN_V_MSG_E((readModeFlags_.load(std::memory_order_acquire) & ReadModeToFlags(ReadMode::SYNC)) == 0,
        Status::ERROR_INVALID_OPERATION,
        "Cannot use sync and async Read together");
    readModeFlags_.fetch_or(ReadModeToFlags(ReadMode::ASYNC), std::memory_order_acq_rel);
    isPauseReadPacket_.store(false);
    if (ioContext_.invokerType != InvokerType::READ) {
        std::lock_guard<std::mutex> readLock(ioContext_.invokerTypeMutex);
        ioContext_.invokerType = InvokerType::READ;
    }
    trackId_ = trackId;
    if (!cacheQueue_.HasCache(trackId) && timeout == 0) {
        return Status::ERROR_WAIT_TIMEOUT;
    }
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
    for (const auto &pktWrapper : samplePacket->pkts) {
        AVPacket *pkt = (pktWrapper != nullptr) ? pktWrapper->GetAVPacket() : nullptr;
        FALSE_RETURN_V_MSG_E(pkt != nullptr, Status::ERROR_UNKNOWN, "Packet in sample is nullptr");
        totalSize += pkt->size;
    }

    const auto* snapshot = GetStreamSnapshot(trackId);
    FALSE_RETURN_V_MSG_E(snapshot != nullptr && snapshot->valid,
        Status::ERROR_UNKNOWN, "Stream snapshot is invalid");
    if ((std::count(g_streamContainedXPS.begin(), g_streamContainedXPS.end(), snapshot->codecId) > 0)) {
        Plugins::AVPacketWrapperPtr firstWrapper = samplePacket->pkts[0];
        if (firstWrapper != nullptr && firstWrapper->GetAVPacket() != nullptr &&
            (static_cast<uint32_t>(firstWrapper->GetFlags()) & static_cast<uint32_t>(AV_PKT_FLAG_KEY))) {
            totalSize += snapshot->extradataSize;
        }
    }
    size = totalSize;
    return Status::OK;
}

Status FFmpegDemuxerPlugin::Pause()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    isPauseReadPacket_.store(true);
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

Status FFmpegDemuxerPlugin::BoostReadThreadPriority()
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
    isAsyncReadThreadPrioritySet_.store(true);
    MEDIA_LOG_I("Boost read thread priority to user interactive");
    return Status::OK;
}

void FFmpegDemuxerPlugin::UpdateAsyncReadThreadPriority()
{
    auto ret = SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    if (ret != 0) {
        MEDIA_LOG_E("Set thread qos failed, ret = " PUBLIC_LOG_D32, ret);
    } else {
        MEDIA_LOG_I("Set thread qos success, level = " PUBLIC_LOG_U32, OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    }
}

Status FFmpegDemuxerPlugin::SetAVReadPacketStopState(bool state)
{
    ioContext_.avReadPacketStopState.store(state ? AVReadPacketStopState::TRUE : AVReadPacketStopState::FALSE);
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ConvertHevcToAnnexb(AVPacket& pkt, const ConvertToAnnexbParams &params)
{
    size_t cencInfoSize = 0;
    AVPacket *firstPkt = (params.samplePacket->pkts[0] != nullptr) ?
                          params.samplePacket->pkts[0]->GetAVPacket() : nullptr;
    FALSE_RETURN_V_MSG_E(firstPkt != nullptr, Status::ERROR_INVALID_OPERATION, "First pkt is nullptr");
    uint8_t *cencInfo = av_packet_get_side_data(firstPkt, AV_PKT_DATA_ENCRYPTION_INFO, &cencInfoSize);
    PacketConvertToBufferInfo convertInfo(params.outDataSize);
    convertInfo.srcData = pkt.data;
    convertInfo.srcDataSize = pkt.size;
    convertInfo.outBuffer = params.outBuffer;
    convertInfo.outBufferSize = params.outBufferSize;
    convertInfo.sideData = cencInfo;
    convertInfo.sideDataSize = cencInfoSize;
    auto convertRet = streamParsers_->ConvertPacketToAnnexb(pkt.stream_index, convertInfo);
    if (NeedCombineFrame(firstPkt->stream_index)) {
        const uint8_t *syncFrameData = convertRet ? params.outBuffer : pkt.data;
        int32_t syncFrameDataSize = convertRet ? params.outDataSize : pkt.size;
        if (streamParsers_->IsSyncFrame(pkt.stream_index, syncFrameData, syncFrameDataSize)) {
            pkt.flags = static_cast<int32_t>(static_cast<uint32_t>(pkt.flags) | static_cast<uint32_t>(AV_PKT_FLAG_KEY));
        }
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ConvertVvcToAnnexb(AVPacket& pkt, const ConvertToAnnexbParams &params)
{
    PacketConvertToBufferInfo convertInfo(params.outDataSize);
    convertInfo.srcData = pkt.data;
    convertInfo.srcDataSize = pkt.size;
    convertInfo.outBuffer = params.outBuffer;
    convertInfo.outBufferSize = params.outBufferSize;
    convertInfo.sideData = nullptr;
    convertInfo.sideDataSize = 0;
    auto convertRet = streamParsers_->ConvertPacketToAnnexb(pkt.stream_index, convertInfo);
    if (!convertRet) {
        MEDIA_LOG_DD("Convert packet to annexb failed");
    }
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ConvertPacketToAnnexb(Plugins::AVPacketWrapperPtr srcWrapper,
    std::shared_ptr<SamplePacket> samplePacket, Plugins::AVPacketWrapperPtr& outWrapper)
{
    AVPacket *srcAVPacket = (srcWrapper != nullptr) ? srcWrapper->GetAVPacket() : nullptr;
    FALSE_RETURN_V_MSG_E(srcAVPacket != nullptr, Status::ERROR_NULL_POINTER, "srcAVPacket is nullptr");
    FALSE_RETURN_V_MSG_E(samplePacket != nullptr, Status::ERROR_NULL_POINTER, "samplePacket is nullptr");
    const AVStreamSnapshot* snapshot = GetStreamSnapshot(static_cast<uint32_t>(srcAVPacket->stream_index));
    FALSE_RETURN_V_MSG_E(snapshot != nullptr && snapshot->valid,
        Status::ERROR_INVALID_OPERATION, "Stream snapshot is invalid");
    auto codecId = snapshot->codecId;
    // H.264 (AVC)
    if (codecId == AV_CODEC_ID_H264 && avbsfContexts_.count(srcAVPacket->stream_index) > 0 &&
        avbsfContexts_[srcAVPacket->stream_index] != nullptr) {
        Status ret = ConvertAvcToAnnexb(*srcAVPacket);
        outWrapper = (ret == Status::OK) ? srcWrapper : nullptr;
        return ret;
    }
    // HEVC and VVC
    if (codecId != AV_CODEC_ID_HEVC && codecId != AV_CODEC_ID_VVC) {
        outWrapper = nullptr;
        return Status::OK;
    }
    if (streamParsers_ == nullptr || !streamParsers_->ParserIsInited(srcAVPacket->stream_index)) {
        outWrapper = nullptr;
        return Status::OK;
    }
    return ConvertPacketToAnnexbWithParser(srcAVPacket, samplePacket, outWrapper, snapshot);
}

Status FFmpegDemuxerPlugin::ConvertPacketToAnnexbWithParser(AVPacket* srcAVPacket,
    std::shared_ptr<SamplePacket> samplePacket, Plugins::AVPacketWrapperPtr& outWrapper,
    const AVStreamSnapshot* snapshot)
{
    int32_t xpsDataSize = 0;
    if (std::count(g_streamContainedXPS.begin(), g_streamContainedXPS.end(), snapshot->codecId) > 0) {
        xpsDataSize = snapshot->extradataSize;
    }
    int32_t estimatedSize = srcAVPacket->size;
    if (xpsDataSize > 0) {
        FALSE_RETURN_V_MSG_E(estimatedSize <= INT32_MAX - xpsDataSize, Status::ERROR_INVALID_DATA,
            "size overflow srcSize=" PUBLIC_LOG_D32 " extradataSize=" PUBLIC_LOG_D32, srcAVPacket->size, xpsDataSize);
        estimatedSize += xpsDataSize;
    }
    outWrapper = std::make_shared<Plugins::AVPacketWrapper>();
    FALSE_RETURN_V_MSG_E(outWrapper != nullptr && outWrapper->GetAVPacket() != nullptr,
        Status::ERROR_INVALID_OPERATION, "Create output AVPacketWrapper failed");
    AVPacket *outPkt = outWrapper->GetAVPacket();
    int ret = av_new_packet(outPkt, estimatedSize);
    FALSE_RETURN_V_MSG_E(ret >= 0, Status::ERROR_INVALID_OPERATION, "Call av_new_packet failed");
    av_packet_copy_props(outPkt, srcAVPacket);
    int32_t outDataSize = 0;
    ConvertToAnnexbParams params(outDataSize);
    params.samplePacket = samplePacket;
    params.outBuffer = outPkt->data;
    params.outBufferSize = estimatedSize;
    Status status = (snapshot->codecId == AV_CODEC_ID_HEVC) ? ConvertHevcToAnnexb(*srcAVPacket, params) :
                                                              ConvertVvcToAnnexb(*srcAVPacket, params);
    if (status != Status::OK) {
        MEDIA_LOG_W("Convert to annexb failed, status=" PUBLIC_LOG_D32, static_cast<int32_t>(status));
        outWrapper = nullptr;
        return status;
    }
    if (outDataSize == 0) {
        outWrapper = nullptr;
        return Status::OK;
    }
    if (outDataSize > estimatedSize) {
        MEDIA_LOG_E("conver overflow outSize=" PUBLIC_LOG_D32 " estimated=" PUBLIC_LOG_D32, outDataSize, estimatedSize);
        outWrapper = nullptr;
        return Status::ERROR_INVALID_OPERATION;
    }
    outPkt->size = outDataSize;
    return Status::OK;
}

Status FFmpegDemuxerPlugin::ConvertToAnnexbAndUpdateSample(std::shared_ptr<AVBuffer> sample,
    std::shared_ptr<SamplePacket> samplePacket, Plugins::AVPacketWrapperPtr& tempPktWrapper)
{
    Plugins::AVPacketWrapperPtr outPktWrapper = nullptr;
    Status ret = ConvertPacketToAnnexb(tempPktWrapper, samplePacket, outPktWrapper);
    FALSE_RETURN_V_MSG_D(ret == Status::OK, ret, "Convert packet to annexb failed");

    tempPktWrapper = (outPktWrapper != nullptr) ? outPktWrapper : tempPktWrapper;
    AVPacket *tempPkt = (tempPktWrapper != nullptr) ? tempPktWrapper->GetAVPacket() : nullptr;
    FALSE_RETURN_V_MSG_E(tempPkt != nullptr, Status::ERROR_INVALID_OPERATION, "Output packet is nullptr");
    const AVStreamSnapshot* snapshot = GetStreamSnapshot(static_cast<uint32_t>(tempPkt->stream_index));
    if (snapshot != nullptr && snapshot->valid &&
        (snapshot->codecId == AV_CODEC_ID_HEVC || snapshot->codecId == AV_CODEC_ID_H264)) {
        SetDropTag(*tempPkt, sample, snapshot->codecId);
    }
    if (cacheQueue_.SetInfo(samplePacket) == false) {
        MEDIA_LOG_D("Set info failed");
    }

    SetDrmCencInfo(sample, samplePacket);
    sample->flag_ = ConvertFlagsFromFFmpeg(*tempPkt, false);
    std::shared_ptr<AVPacketWrapper> pktWrapper = std::make_shared<AVPacketWrapper>(av_packet_clone(tempPkt));
    FALSE_RETURN_V_MSG_E(pktWrapper != nullptr, Status::ERROR_INVALID_OPERATION, "Clone packet failed");
    std::shared_ptr<AVPacketMemory> avPacketMemory = std::make_shared<AVPacketMemory>(pktWrapper);
    FALSE_RETURN_V_MSG_E(avPacketMemory != nullptr, Status::ERROR_INVALID_OPERATION, "Create AVPacketMemory failed");
    sample->memory_ = std::static_pointer_cast<Media::AVMemory>(avPacketMemory);
    MEDIA_LOG_D("AVMemory created, size=" PUBLIC_LOG_U32, avPacketMemory->GetSize());
    MEDIA_LOG_D("CurrentBuffer: [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "/" PUBLIC_LOG_U32 "]",
        sample->pts_, sample->duration_, sample->flag_);
    if (!samplePacket->isEOS) {
        UpdateLastPacketInfo(tempPkt->stream_index, sample->pts_, tempPkt->pos, sample->duration_);
    }
#ifdef BUILD_ENG_VERSION
    DumpParam dumpParam {DumpMode(DUMP_AVBUFFER_OUTPUT & dumpMode_), tempPkt->data + samplePacket->offset,
        tempPkt->stream_index, -1, tempPkt->size, trackDfxInfoMap_[tempPkt->stream_index].frameIndex++, tempPkt->pts,
        -1};
    Dump(dumpParam);
#endif
    return Status::OK;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
