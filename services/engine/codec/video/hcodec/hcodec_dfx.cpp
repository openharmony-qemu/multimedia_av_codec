/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <fstream>
#include <sstream>
#include "hcodec.h"
#include "hcodec_log.h"
#include "hcodec_dfx.h"
#include "hcodec_utils.h"
#include "hisysevent.h"

namespace OHOS::MediaAVCodec {
using namespace std;

FuncTracker::FuncTracker(std::string value) : value_(std::move(value))
{
    PLOGI("%s >>", value_.c_str());
}

FuncTracker::~FuncTracker()
{
    PLOGI("%s <<", value_.c_str());
}

void HCodec::OnPrintAllBufferOwner(const MsgInfo& msg)
{
    TimePoint lastOwnerChangeTime;
    msg.param->GetValue(KEY_LAST_OWNER_CHANGE_TIME, lastOwnerChangeTime);
    if (lastOwnerChangeTime == lastOwnerChangeTime_) {
        UpdateOwner();  // update all owner count trace in case of systrace is catched too late
        if (!circulateHasStopped_) {
            HLOGW("buffer circulate stoped");
            PrintAllBufferInfo();
            circulateHasStopped_ = true;
        }
    }
    ParamSP param = make_shared<ParamBundle>();
    param->SetValue(KEY_LAST_OWNER_CHANGE_TIME, lastOwnerChangeTime_);
    SendAsyncMsg(MsgWhat::PRINT_ALL_BUFFER_OWNER, param, THREE_SECONDS_IN_US);
}

void HCodec::PrintAllBufferInfo()
{
    auto now = chrono::steady_clock::now();
    PrintAllBufferInfo(now, OMX_DirInput);
    PrintAllBufferInfo(now, OMX_DirOutput);
}

void HCodec::PrintAllBufferInfo(const TimePoint& now, OMX_DIRTYPE port)
{
    const Record& record = record_[port];
    const char* inOutStr = (port == OMX_DirInput) ? " in" : "out";
    bool eos = (port == OMX_DirInput) ? inputPortEos_ : outputPortEos_;
    const std::array<int, OWNER_CNT>& arr = record.currOwner_;
    const vector<BufferInfo>& pool = (port == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;

    std::stringstream s;
    for (const BufferInfo& info : pool) {
        int64_t holdMs = chrono::duration_cast<chrono::milliseconds>(now - info.lastOwnerChangeTime).count();
        s << info.bufferId << ":" << ToString(info.owner) << "(" << holdMs << "), ";
    }
    HLOGI("%s: eos=%d, cnt=%" PRIu64 ", pts=%" PRId64 ", %d/%d/%d/%d, %s", inOutStr, eos,
          record.frameCntTotal_, record.lastPts_,
          arr[OWNED_BY_US], arr[OWNED_BY_USER], arr[OWNED_BY_OMX], arr[OWNED_BY_SURFACE], s.str().c_str());
}

std::string HCodec::OnGetHidumperInfo()
{
    auto now = chrono::steady_clock::now();
    std::stringstream s;
    auto getbufferCapacity = [](const std::vector<BufferInfo> &pool) -> int32_t {
        IF_TRUE_RETURN_VAL(pool.empty(), 0);
        IF_TRUE_RETURN_VAL(pool.front().surfaceBuffer, pool.front().surfaceBuffer->GetSize());
        IF_TRUE_RETURN_VAL(!(pool.front().avBuffer && pool.front().avBuffer->memory_), 0);
        return pool.front().avBuffer->memory_->GetCapacity();
    };

    s << "        " << "[" << compUniqueStr_ << "][" << currState_->GetName() << "]" << endl;
    s << "        " << "------------INPUT-----------" << endl;
    s << "        " << "eos:" << inputPortEos_ << ", etb:" << record_[OMX_DirInput].frameCntTotal_
      << ", bufferCapacity:" << getbufferCapacity(inputBufferPool_) << endl;
    for (const BufferInfo& info : inputBufferPool_) {
        int64_t holdMs = chrono::duration_cast<chrono::milliseconds>(now - info.lastOwnerChangeTime).count();
        s << "        " << "inBufId = " << info.bufferId << ", owner = " << ToString(info.owner);
        if (info.hasSwapedOut) {
            s << ", hasSwapedOut = " << info.hasSwapedOut << ", nextOwner = " << ToString(info.nextStepOwner);
        }
        s << ", holdMs = " << holdMs << endl;
    }
    s << "        " << "----------------------------" << endl;
    s << "        " << "------------OUTPUT----------" << endl;
    s << "        " << "eos:" << outputPortEos_ << ", fbd:" << record_[OMX_DirOutput].frameCntTotal_
      << ", bufferCapacity:" << getbufferCapacity(outputBufferPool_) << endl;
    for (const BufferInfo& info : outputBufferPool_) {
        int fd = info.surfaceBuffer == nullptr ? -1 : info.surfaceBuffer->GetFileDescriptor();
        int64_t holdMs = chrono::duration_cast<chrono::milliseconds>(now - info.lastOwnerChangeTime).count();
        s << "        " << "outBufId = " << info.bufferId << ", fd = " << fd << ", owner = " << ToString(info.owner);
        if (info.hasSwapedOut) {
            s << ", hasSwapedOut = " << info.hasSwapedOut << ", nextOwner = " << ToString(info.nextStepOwner);
        }
        s << ", holdMs = " << holdMs << endl;
    }
    s << "        " << "----------------------------" << endl << endl;
    return s.str();
}

void HCodec::UpdateOwner()
{
    UpdateOwner(OMX_DirInput);
    UpdateOwner(OMX_DirOutput);
}

void HCodec::FaultEventWrite(const string& faultType, const std::string& msg)
{
    HiSysEventWrite(HISYSEVENT_DOMAIN_HCODEC, "FAULT",
                    OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
                    "MODULE", "HardwareDecoder",
                    "FAULTTYPE", faultType,
                    "MSG", msg);
}

void HCodec::UpdateOwner(OMX_DIRTYPE port)
{
    std::array<int, OWNER_CNT>& arr = record_[port].currOwner_;
    const vector<BufferInfo>& pool = (port == OMX_DirInput) ? inputBufferPool_ : outputBufferPool_;

    arr.fill(0);
    for (const BufferInfo &info : pool) {
        arr[info.owner]++;
    }
    for (uint32_t owner = 0; owner < static_cast<uint32_t>(OWNER_CNT); owner++) {
        CountTrace(HITRACE_TAG_ZMEDIA, record_[port].ownerTraceTag_[owner], arr[owner]);
    }
}

void HCodec::ReduceOwner(OMX_DIRTYPE port, BufferOwner owner)
{
    Record& record = record_[port];
    record.currOwner_[owner]--;
    CountTrace(HITRACE_TAG_ZMEDIA, record.ownerTraceTag_[owner], record.currOwner_[owner]);
}

void HCodec::ChangeOwner(BufferInfo& info, BufferOwner newOwner)
{
    auto now = chrono::steady_clock::now();
    OMX_DIRTYPE port = info.isInput ? OMX_DirInput : OMX_DirOutput;
    Record& record = record_[port];
    std::array<int, OWNER_CNT>& currOwner = record.currOwner_;
    BufferOwner oldOwner = info.owner;
    if (!record.beginOfInterval_.has_value()) {
        record.ResetInterval(now);
    }

    UpdateHoldCnt(now, port, oldOwner);
    UpdateHoldCnt(now, port, newOwner);
    UpdateHoldTime(now, info, newOwner);

    // now change owner
    currOwner[oldOwner]--;
    currOwner[newOwner]++;
    info.owner = newOwner;
    info.lastOwnerChangeTime = now;
    record.lastOwnerChangeTime_[oldOwner] = now;
    record.lastOwnerChangeTime_[newOwner] = now;
    lastOwnerChangeTime_ = now;
    if (circulateHasStopped_) {
        HLOGI("circulate resume, %s, %s -> %s, pts=%" PRId64,
            (info.isInput ? "in" : "out"), ToString(oldOwner), ToString(newOwner), info.omxBuffer->pts);
        circulateHasStopped_ = false;
    }

    CountTrace(HITRACE_TAG_ZMEDIA, record.ownerTraceTag_[oldOwner], currOwner[oldOwner]);
    CountTrace(HITRACE_TAG_ZMEDIA, record.ownerTraceTag_[newOwner], currOwner[newOwner]);

    if (info.isInput && oldOwner == OWNED_BY_US && newOwner == OWNED_BY_OMX) {
        record.frameCntTotal_++;
        record.frameCntInterval_++;
        record.frameMbitsInterval_ += info.omxBuffer->filledLen * BYTE_TO_MBIT;
        record.lastPts_ = info.omxBuffer->pts;
        debugMode_ ? UpdateInputRecord(now, info) : PrintStatistic(now, port);
    }
    if (!info.isInput && oldOwner == OWNED_BY_US && newOwner == OWNED_BY_USER) {
        record.frameCntTotal_++;
        record.frameCntInterval_++;
        record.frameMbitsInterval_ += info.omxBuffer->filledLen * BYTE_TO_MBIT;
        record.lastPts_ = info.omxBuffer->pts;
        debugMode_ ? UpdateOutputRecord(now, info) : PrintStatistic(now, port);
    }
}

// now, on this port, this owner's hold cnt is gonna change
void HCodec::UpdateHoldCnt(const TimePoint& now, OMX_DIRTYPE port, BufferOwner owner)
{
    Record& record = record_[port];
    if (!record.lastOwnerChangeTime_[owner].has_value()) {
        return;
    }
    auto holdUs = chrono::duration_cast<chrono::microseconds>(
        now - record.lastOwnerChangeTime_[owner].value()).count();
    if (holdUs < 0) {
        HLOGW("steady clock has go backwards, %ld us", holdUs);
        record.ResetInterval(now);
        return;
    }
    TotalEvent& holdCnt = record.holdCntInterval_[owner];
    holdCnt.eventCnt += static_cast<uint64_t>(holdUs);
    holdCnt.eventSum += (static_cast<uint64_t>(holdUs) *
                         static_cast<uint64_t>(record.currOwner_[owner]));
}

// now, this buffer is gonna change to new owner
void HCodec::UpdateHoldTime(const TimePoint& now, const BufferInfo& info, BufferOwner newOwner)
{
    Record& record = record_[info.isInput ? OMX_DirInput : OMX_DirOutput];
    auto holdUs = chrono::duration_cast<chrono::microseconds>(now - info.lastOwnerChangeTime).count();
    if (holdUs < 0) {
        HLOGW("steady clock has go backwards, %ld us", holdUs);
        record.ResetInterval(now);
        return;
    }
    BufferOwner oldOwner = info.owner;
    TotalEvent& oldOwnerHoldTime = record.holdTimeInterval_[oldOwner];
    oldOwnerHoldTime.eventCnt++;
    oldOwnerHoldTime.eventSum += static_cast<uint64_t>(holdUs);
    if (debugMode_) {
        std::array<int, OWNER_CNT> currOwner = record.currOwner_;
        currOwner[oldOwner]--;
        currOwner[newOwner]++;
        HLOGI("%s = %u, after hold %.1f ms, %s -> %s, %d/%d/%d/%d", (info.isInput ? "inBufId" : "outBufId"),
            info.bufferId, holdUs / US_TO_MS, ToString(oldOwner), ToString(newOwner),
            currOwner[OWNED_BY_US], currOwner[OWNED_BY_USER], currOwner[OWNED_BY_OMX], currOwner[OWNED_BY_SURFACE]);
    }
}

bool HCodec::CalculateInterval(const TimePoint& now, OMX_DIRTYPE port, IntervalAverage& ave)
{
    Record& record = record_[port];
    if (!record.beginOfInterval_.has_value()) {
        return false;
    }
    auto fromBeginOfIntervalToNowUs = chrono::duration_cast<chrono::microseconds>(
        now - record.beginOfInterval_.value()).count();
    if (fromBeginOfIntervalToNowUs < 0) {
        HLOGW("steady clock has go backwards, %ld us", fromBeginOfIntervalToNowUs);
        record.ResetInterval(now);
        return false;
    }
    if (fromBeginOfIntervalToNowUs == 0) {
        return false;
    }

    ave.fps = record.frameCntInterval_ * US_TO_S / fromBeginOfIntervalToNowUs;
    ave.mbps = record.frameMbitsInterval_ * US_TO_S / fromBeginOfIntervalToNowUs;
    for (uint32_t owner = 0; owner < static_cast<uint32_t>(OWNER_CNT); owner++) {
        const TotalEvent& holdCnt = record.holdCntInterval_[owner];
        ave.holdCnt[owner] = (holdCnt.eventCnt == 0) ? 0 :
            static_cast<double>(holdCnt.eventSum) / holdCnt.eventCnt;
        const TotalEvent& holdTime = record.holdTimeInterval_[owner];
        ave.holdMs[owner] = (holdTime.eventCnt == 0) ? -1 :
            (holdTime.eventSum / US_TO_MS / holdTime.eventCnt);
    }
    return true;
}

void HCodec::PrintStatistic(const TimePoint& now, OMX_DIRTYPE port)
{
    Record& record = record_[port];
    if (record.frameCntInterval_ % PRINT_PER_FRAME != 0) {
        return;
    }
    IntervalAverage ave;
    bool ret = CalculateInterval(now, port, ave);
    if (!ret) {
        return;
    }
    const char* inOutStr = (port == OMX_DirInput) ? " in" : "out";
    HLOGI("%s: fps=%.1f, Mbps=%.1f, cnt=%" PRIu64 ", pts=%" PRId64 ", %.1f/%.1f/%.1f/%.1f, %.0f/%.0f/%.0f/%.0f, "
        "fence %.3f, discard %.0f%%", inOutStr, ave.fps, ave.mbps, record.frameCntTotal_, record.lastPts_,
        ave.holdCnt[OWNED_BY_US], ave.holdCnt[OWNED_BY_USER], ave.holdCnt[OWNED_BY_OMX], ave.holdCnt[OWNED_BY_SURFACE],
        ave.holdMs[OWNED_BY_US], ave.holdMs[OWNED_BY_USER], ave.holdMs[OWNED_BY_OMX], ave.holdMs[OWNED_BY_SURFACE],
        record.waitFenceCostUsInterval_ / US_TO_MS / PRINT_PER_FRAME,
        static_cast<double>(record.discardCntInterval_) / PRINT_PER_FRAME * 100); // 100: percent
    record.ResetInterval(now);
}

void HCodec::UpdateInputRecord(const TimePoint& now, const BufferInfo& info)
{
    if (!info.IsValidFrame()) {
        return;
    }
    inTimeMap_[info.omxBuffer->pts] = now;
    Record& record = record_[info.isInput ? OMX_DirInput : OMX_DirOutput];
    auto fromBeginOfIntervalToNowUs = chrono::duration_cast<chrono::microseconds>(
        now - record.beginOfInterval_.value()).count();
    if (fromBeginOfIntervalToNowUs < 0) {
        HLOGW("steady clock has go backwards, %ld us", fromBeginOfIntervalToNowUs);
        record.ResetInterval(now);
        return;
    }
    if (fromBeginOfIntervalToNowUs == 0) {
        HLOGI("pts = %" PRId64 ", len = %u, flags = 0x%x",
              info.omxBuffer->pts, info.omxBuffer->filledLen, info.omxBuffer->flag);
    } else {
        double inFps = record.frameCntInterval_ * US_TO_S / fromBeginOfIntervalToNowUs;
        HLOGI("pts = %" PRId64 ", len = %u, flags = 0x%x, in fps %.2f",
              info.omxBuffer->pts, info.omxBuffer->filledLen, info.omxBuffer->flag, inFps);
    }
}

void HCodec::UpdateOutputRecord(const TimePoint& now, const BufferInfo& info)
{
    if (!info.IsValidFrame()) {
        return;
    }
    auto it = inTimeMap_.find(info.omxBuffer->pts);
    if (it == inTimeMap_.end()) {
        return;
    }
    Record& record = record_[info.isInput ? OMX_DirInput : OMX_DirOutput];
    auto fromInToOut = chrono::duration_cast<chrono::microseconds>(now - it->second).count();
    inTimeMap_.erase(it);
    if (fromInToOut < 0) {
        HLOGW("steady clock has go backwards, %ld us", fromInToOut);
        record.ResetInterval(now);
        return;
    }
    onePtsInToOutTotalCostUs_ += static_cast<uint64_t>(fromInToOut);
    double oneFrameCostMs = fromInToOut / US_TO_MS;
    double averageCostMs = onePtsInToOutTotalCostUs_ / US_TO_MS / record.frameCntInterval_;

    auto fromBeginOfIntervalToNowUs = chrono::duration_cast<chrono::microseconds>(
        now - record.beginOfInterval_.value()).count();
    if (fromBeginOfIntervalToNowUs < 0) {
        HLOGW("steady clock has go backwards, %ld us", fromBeginOfIntervalToNowUs);
        record.ResetInterval(now);
        return;
    }
    if (fromBeginOfIntervalToNowUs == 0) {
        HLOGI("pts = %" PRId64 ", len = %u, flags = 0x%x, "
              "cost %.2f ms (%.2f ms)",
              info.omxBuffer->pts, info.omxBuffer->filledLen, info.omxBuffer->flag,
              oneFrameCostMs, averageCostMs);
    } else {
        double outFps = record.frameCntInterval_ * US_TO_S / fromBeginOfIntervalToNowUs;
        HLOGI("pts = %" PRId64 ", len = %u, flags = 0x%x, "
              "cost %.2f ms (%.2f ms), out fps %.2f",
              info.omxBuffer->pts, info.omxBuffer->filledLen, info.omxBuffer->flag,
              oneFrameCostMs, averageCostMs, outFps);
    }
}

bool HCodec::BufferInfo::IsValidFrame() const
{
    if (omxBuffer->flag & OMX_BUFFERFLAG_EOS) {
        return false;
    }
    if (omxBuffer->flag & OMX_BUFFERFLAG_CODECCONFIG) {
        return false;
    }
    if (omxBuffer->filledLen == 0) {
        return false;
    }
    return true;
}

#ifdef BUILD_ENG_VERSION
void HCodec::BufferInfo::Dump(const string& prefix, uint64_t cnt, DumpMode dumpMode, bool isEncoder) const
{
    if (isInput) {
        if (((dumpMode & DUMP_ENCODER_INPUT) && isEncoder) ||
            ((dumpMode & DUMP_DECODER_INPUT) && !isEncoder)) {
            Dump(prefix + "_Input", cnt);
        }
    } else {
        if (((dumpMode & DUMP_ENCODER_OUTPUT) && isEncoder) ||
            ((dumpMode & DUMP_DECODER_OUTPUT) && !isEncoder)) {
            Dump(prefix + "_Output", cnt);
        }
    }
}

void HCodec::BufferInfo::Dump(const string& prefix, uint64_t cnt) const
{
    if (surfaceBuffer) {
        DumpSurfaceBuffer(prefix, cnt);
    } else {
        DumpLinearBuffer(prefix);
    }
}

void HCodec::BufferInfo::DumpSurfaceBuffer(const std::string& prefix, uint64_t cnt) const
{
    if (omxBuffer->filledLen == 0) {
        return;
    }
    const char* va = reinterpret_cast<const char*>(surfaceBuffer->GetVirAddr());
    IF_TRUE_RETURN_VOID_WITH_MSG(va == nullptr, "null va");
    int w = surfaceBuffer->GetWidth();
    int h = surfaceBuffer->GetHeight();
    int byteStride = surfaceBuffer->GetStride();
    IF_TRUE_RETURN_VOID_WITH_MSG(byteStride == 0, "stride 0");
    int alignedH = h;
    uint32_t totalSize = surfaceBuffer->GetSize();
    uint32_t seq = surfaceBuffer->GetSeqNum();
    GraphicPixelFormat graphicFmt = static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat());
    std::optional<PixelFmt> fmt = TypeConverter::GraphicFmtToFmt(graphicFmt);
    IF_TRUE_RETURN_VOID_WITH_MSG(!fmt.has_value(), "unknown fmt %d", graphicFmt);

    string suffix;
    bool dumpAsVideo = true;
    DecideDumpInfo(alignedH, totalSize, suffix, dumpAsVideo);

    char name[128];
    int ret = 0;
    if (dumpAsVideo) {
        ret = sprintf_s(name, sizeof(name), "%s/%s_%dx%d(%dx%d)_fmt%s.%s",
                        DUMP_PATH, prefix.c_str(), w, h, byteStride, alignedH,
                        fmt->strFmt.c_str(), suffix.c_str());
    } else {
        ret = sprintf_s(name, sizeof(name), "%s/%s_%" PRIu64 "_%dx%d(%d)_fmt%s_pts%" PRId64 "_seq%u.%s",
                        DUMP_PATH, prefix.c_str(), cnt, w, h, byteStride,
                        fmt->strFmt.c_str(), omxBuffer->pts, seq, suffix.c_str());
    }
    if (ret > 0) {
        ofstream ofs(name, ios::binary | ios::app);
        if (ofs.is_open()) {
            ofs.write(va, totalSize);
        } else {
            LOGW("cannot open %s", name);
        }
    }
    // if we unmap here, flush cache will fail
}

void HCodec::BufferInfo::DecideDumpInfo(int& alignedH, uint32_t& totalSize, string& suffix, bool& dumpAsVideo) const
{
    int h = surfaceBuffer->GetHeight();
    int byteStride = surfaceBuffer->GetStride();
    GraphicPixelFormat fmt = static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat());
    switch (fmt) {
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010: {
            OH_NativeBuffer_Planes *planes = nullptr;
            GSError err = surfaceBuffer->GetPlanesInfo(reinterpret_cast<void**>(&planes));
            if (err != GSERROR_OK || planes == nullptr) { // compressed
                suffix = "bin";
                dumpAsVideo = false;
                return;
            }
            alignedH = static_cast<int32_t>(static_cast<int64_t>(planes->planes[1].offset) / byteStride);
            totalSize = GetYuv420Size(byteStride, alignedH);
            suffix = "yuv";
            break;
        }
        case GRAPHIC_PIXEL_FMT_RGBA_1010102:
        case GRAPHIC_PIXEL_FMT_RGBA_8888: {
            totalSize = static_cast<uint32_t>(byteStride * h);
            suffix = "rgba";
            break;
        }
        default: {
            suffix = "bin";
            dumpAsVideo = false;
            break;
        }
    }
}

void HCodec::BufferInfo::DumpLinearBuffer(const string& prefix) const
{
    if (omxBuffer->filledLen == 0) {
        return;
    }
    if (avBuffer == nullptr || avBuffer->memory_ == nullptr) {
        LOGW("invalid avbuffer");
        return;
    }
    const char* va = reinterpret_cast<const char*>(avBuffer->memory_->GetAddr());
    if (va == nullptr) {
        LOGW("null va");
        return;
    }

    char name[128];
    int ret = sprintf_s(name, sizeof(name), "%s/%s.bin", DUMP_PATH, prefix.c_str());
    if (ret <= 0) {
        LOGW("sprintf_s failed");
        return;
    }
    ofstream ofs(name, ios::binary | ios::app);
    if (ofs.is_open()) {
        ofs.write(va, omxBuffer->filledLen);
    } else {
        LOGW("cannot open %s", name);
    }
}
#endif // BUILD_ENG_VERSION
}