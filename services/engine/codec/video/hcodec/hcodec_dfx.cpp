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
#include "hitrace_meter.h"
#include "hcodec.h"
#include "hcodec_log.h"
#include "hcodec_dfx.h"
#include "hcodec_utils.h"

namespace OHOS::MediaAVCodec {
using namespace std;

ScopedTrace::ScopedTrace(const std::string &value)
{
    StartTrace(HITRACE_TAG_ZMEDIA, value);
}

ScopedTrace::~ScopedTrace()
{
    FinishTrace(HITRACE_TAG_ZMEDIA);
}

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
    std::chrono::time_point<std::chrono::steady_clock> lastOwnerChangeTime;
    msg.param->GetValue(KEY_LAST_OWNER_CHANGE_TIME, lastOwnerChangeTime);
    if ((lastOwnerChangeTime == lastOwnerChangeTime_) &&
        (circulateWarnPrintedTimes_ < MAX_CIRCULATE_WARN_TIMES)) {
        HLOGW("buffer circulate stoped");
        UpdateOwner();
        PrintAllBufferInfo();
        circulateWarnPrintedTimes_++;
    }
    ParamSP param = make_shared<ParamBundle>();
    param->SetValue(KEY_LAST_OWNER_CHANGE_TIME, lastOwnerChangeTime_);
    SendAsyncMsg(MsgWhat::PRINT_ALL_BUFFER_OWNER, param,
                 THREE_SECONDS_IN_US * (circulateWarnPrintedTimes_ + 1));
}

void HCodec::PrintAllBufferInfo()
{
    auto now = chrono::steady_clock::now();
    PrintAllBufferInfo(true, now);
    PrintAllBufferInfo(false, now);
}

void HCodec::PrintAllBufferInfo(bool isInput, std::chrono::time_point<std::chrono::steady_clock> now)
{
    const char* inOutStr = isInput ? " in" : "out";
    bool eos = isInput ? inputPortEos_ : outputPortEos_;
    uint64_t cnt = isInput ? inTotalCnt_ : outRecord_.totalCnt;
    const std::array<int, OWNER_CNT>& arr = isInput ? inputOwner_ : outputOwner_;
    const vector<BufferInfo>& pool = isInput ? inputBufferPool_ : outputBufferPool_;

    std::stringstream s;
    for (const BufferInfo& info : pool) {
        int64_t holdMs = chrono::duration_cast<chrono::milliseconds>(now - info.lastOwnerChangeTime).count();
        s << info.bufferId << ":" << ToString(info.owner) << "(" << holdMs << "), ";
    }
    HLOGI("%s: eos=%d, cnt=%" PRIu64 ", %d/%d/%d/%d, %s", inOutStr, eos, cnt,
          arr[OWNED_BY_US], arr[OWNED_BY_USER], arr[OWNED_BY_OMX], arr[OWNED_BY_SURFACE], s.str().c_str());
}

std::string HCodec::OnGetHidumperInfo()
{
    auto now = chrono::steady_clock::now();
    std::stringstream s;
    s << endl;
    s << "        " << compUniqueStr_ << "[" << currState_->GetName() << "]" << endl;
    s << "        " << "------------INPUT-----------" << endl;
    s << "        " << "eos:" << inputPortEos_ << ", etb:" << inTotalCnt_ << endl;
    for (const BufferInfo& info : inputBufferPool_) {
        int64_t holdMs = chrono::duration_cast<chrono::milliseconds>(now - info.lastOwnerChangeTime).count();
        s << "        " << "inBufId = " << info.bufferId << ", owner = " << ToString(info.owner)
          << ", holdMs = " << holdMs << endl;
    }
    s << "        " << "----------------------------" << endl;
    s << "        " << "------------OUTPUT----------" << endl;
    s << "        " << "eos:" << outputPortEos_ << ", fbd:" << outRecord_.totalCnt << endl;
    for (const BufferInfo& info : outputBufferPool_) {
        int64_t holdMs = chrono::duration_cast<chrono::milliseconds>(now - info.lastOwnerChangeTime).count();
        s << "        " << "outBufId = " << info.bufferId << ", owner = " << ToString(info.owner)
          << ", holdMs = " << holdMs << endl;
    }
    s << "        " << "----------------------------" << endl;
    return s.str();
}

void HCodec::UpdateOwner()
{
    UpdateOwner(true);
    UpdateOwner(false);
}

void HCodec::UpdateOwner(bool isInput)
{
    std::array<int, OWNER_CNT>& arr = isInput ? inputOwner_ : outputOwner_;
    const std::array<std::string, OWNER_CNT>& ownerStr = isInput ? inputOwnerStr_ : outputOwnerStr_;
    const vector<BufferInfo>& pool = isInput ? inputBufferPool_ : outputBufferPool_;

    arr.fill(0);
    for (const BufferInfo &info : pool) {
        arr[info.owner]++;
    }
    for (uint32_t owner = 0; owner < static_cast<uint32_t>(OWNER_CNT); owner++) {
        CountTrace(HITRACE_TAG_ZMEDIA, ownerStr[owner], arr[owner]);
    }
}

void HCodec::ReduceOwner(bool isInput, BufferOwner owner)
{
    std::array<int, OWNER_CNT>& arr = isInput ? inputOwner_ : outputOwner_;
    const std::array<std::string, OWNER_CNT>& ownerStr = isInput ? inputOwnerStr_ : outputOwnerStr_;
    arr[owner]--;
    CountTrace(HITRACE_TAG_ZMEDIA, ownerStr[owner], arr[owner]);
}

void HCodec::PrintStatistic(bool isInput, std::chrono::time_point<std::chrono::steady_clock> now)
{
    int64_t fromFirstToNow = chrono::duration_cast<chrono::microseconds>(
        now - (isInput ? firstInTime_ : firstOutTime_)).count();
    if (fromFirstToNow == 0) {
        return;
    }
    double fps = PRINT_PER_FRAME * US_TO_S / fromFirstToNow;
    const std::array<int, OWNER_CNT>& arr = isInput ? inputOwner_ : outputOwner_;
    double aveHoldMs[OWNER_CNT];
    for (uint32_t owner = 0; owner < static_cast<uint32_t>(OWNER_CNT); owner++) {
        TotalCntAndCost& holdRecord = isInput ? inputHoldTimeRecord_[owner] : outputHoldTimeRecord_[owner];
        aveHoldMs[owner] = (holdRecord.totalCnt == 0) ? -1 : (holdRecord.totalCostUs / US_TO_MS / holdRecord.totalCnt);
    }
    HLOGI("%s:%.0f; %d/%d/%d/%d, %.0f/%.0f/%.0f/%.0f", (isInput ? " in" : "out"), fps,
        arr[OWNED_BY_US], arr[OWNED_BY_USER], arr[OWNED_BY_OMX], arr[OWNED_BY_SURFACE],
        aveHoldMs[OWNED_BY_US], aveHoldMs[OWNED_BY_USER], aveHoldMs[OWNED_BY_OMX], aveHoldMs[OWNED_BY_SURFACE]);
}

void HCodec::ChangeOwner(BufferInfo& info, BufferOwner newOwner)
{
    debugMode_ ? ChangeOwnerDebug(info, newOwner) : ChangeOwnerNormal(info, newOwner);
}

void HCodec::ChangeOwnerNormal(BufferInfo& info, BufferOwner newOwner)
{
    std::array<int, OWNER_CNT>& arr = info.isInput ? inputOwner_ : outputOwner_;
    const std::array<std::string, OWNER_CNT>& ownerStr = info.isInput ? inputOwnerStr_ : outputOwnerStr_;

    BufferOwner oldOwner = info.owner;
    auto now = chrono::steady_clock::now();
    lastOwnerChangeTime_ = now;
    circulateWarnPrintedTimes_ = 0;
    arr[oldOwner]--;
    arr[newOwner]++;
    CountTrace(HITRACE_TAG_ZMEDIA, ownerStr[oldOwner], arr[oldOwner]);
    CountTrace(HITRACE_TAG_ZMEDIA, ownerStr[newOwner], arr[newOwner]);

    uint64_t holdUs = static_cast<uint64_t>(
        chrono::duration_cast<chrono::microseconds>(now - info.lastOwnerChangeTime).count());
    TotalCntAndCost& holdRecord = info.isInput ? inputHoldTimeRecord_[oldOwner] :
                                                outputHoldTimeRecord_[oldOwner];
    holdRecord.totalCnt++;
    holdRecord.totalCostUs += holdUs;

    // now change owner
    info.lastOwnerChangeTime = now;
    info.owner = newOwner;

    if (info.isInput && oldOwner == OWNED_BY_US && newOwner == OWNED_BY_OMX) {
        if (inTotalCnt_ == 0) {
            firstInTime_ = now;
        }
        inTotalCnt_++;
        if (inTotalCnt_ % PRINT_PER_FRAME == 0) {
            PrintStatistic(info.isInput, now);
            firstInTime_ = now;
            inputHoldTimeRecord_.fill({0, 0});
        }
    }
    if (!info.isInput && oldOwner == OWNED_BY_OMX && newOwner == OWNED_BY_US) {
        if (outRecord_.totalCnt == 0) {
            firstOutTime_ = now;
        }
        outRecord_.totalCnt++;
        if (outRecord_.totalCnt % PRINT_PER_FRAME == 0) {
            PrintStatistic(info.isInput, now);
            firstOutTime_ = now;
            outputHoldTimeRecord_.fill({0, 0});
        }
    }
}

void HCodec::ChangeOwnerDebug(BufferInfo& info, BufferOwner newOwner)
{
    std::array<int, OWNER_CNT>& arr = info.isInput ? inputOwner_ : outputOwner_;
    const std::array<std::string, OWNER_CNT>& ownerStr = info.isInput ? inputOwnerStr_ : outputOwnerStr_;

    BufferOwner oldOwner = info.owner;
    auto now = chrono::steady_clock::now();
    lastOwnerChangeTime_ = now;
    circulateWarnPrintedTimes_ = 0;
    arr[oldOwner]--;
    arr[newOwner]++;
    CountTrace(HITRACE_TAG_ZMEDIA, ownerStr[oldOwner], arr[oldOwner]);
    CountTrace(HITRACE_TAG_ZMEDIA, ownerStr[newOwner], arr[newOwner]);

    const char* oldOwnerStr = ToString(oldOwner);
    const char* newOwnerStr = ToString(newOwner);
    const char* idStr = info.isInput ? "inBufId" : "outBufId";

    uint64_t holdUs = static_cast<uint64_t>
        (chrono::duration_cast<chrono::microseconds>(now - info.lastOwnerChangeTime).count());
    double holdMs = holdUs / US_TO_MS;
    TotalCntAndCost& holdRecord = info.isInput ? inputHoldTimeRecord_[oldOwner] :
                                                outputHoldTimeRecord_[oldOwner];
    holdRecord.totalCnt++;
    holdRecord.totalCostUs += holdUs;
    double aveHoldMs = holdRecord.totalCostUs / US_TO_MS / holdRecord.totalCnt;

    // now change owner
    info.lastOwnerChangeTime = now;
    info.owner = newOwner;
    HLOGI("%s = %u, after hold %.1f ms (%.1f ms), %s -> %s, %d/%d/%d/%d",
          idStr, info.bufferId, holdMs, aveHoldMs, oldOwnerStr, newOwnerStr,
          arr[OWNED_BY_US], arr[OWNED_BY_USER], arr[OWNED_BY_OMX], arr[OWNED_BY_SURFACE]);

    if (info.isInput && oldOwner == OWNED_BY_US && newOwner == OWNED_BY_OMX) {
        UpdateInputRecord(info, now);
    }
    if (!info.isInput && oldOwner == OWNED_BY_OMX && newOwner == OWNED_BY_US) {
        UpdateOutputRecord(info, now);
    }
}

void HCodec::UpdateInputRecord(const BufferInfo& info, std::chrono::time_point<std::chrono::steady_clock> now)
{
    if (!info.IsValidFrame()) {
        return;
    }
    inTimeMap_[info.omxBuffer->pts] = now;
    if (inTotalCnt_ == 0) {
        firstInTime_ = now;
    }
    inTotalCnt_++;

    uint64_t fromFirstInToNow = static_cast<uint64_t>
        (chrono::duration_cast<chrono::microseconds>(now - firstInTime_).count());
    if (fromFirstInToNow == 0) {
        HLOGI("pts = %" PRId64 ", len = %u, flags = 0x%x",
              info.omxBuffer->pts, info.omxBuffer->filledLen, info.omxBuffer->flag);
    } else {
        double inFps = inTotalCnt_ * US_TO_S / fromFirstInToNow;
        HLOGI("pts = %" PRId64 ", len = %u, flags = 0x%x, in fps %.2f",
              info.omxBuffer->pts, info.omxBuffer->filledLen, info.omxBuffer->flag, inFps);
    }
}

void HCodec::UpdateOutputRecord(const BufferInfo& info, std::chrono::time_point<std::chrono::steady_clock> now)
{
    if (!info.IsValidFrame()) {
        return;
    }
    auto it = inTimeMap_.find(info.omxBuffer->pts);
    if (it == inTimeMap_.end()) {
        return;
    }
    if (outRecord_.totalCnt == 0) {
        firstOutTime_ = now;
    }
    outRecord_.totalCnt++;

    uint64_t fromInToOut = static_cast<uint64_t>
        (chrono::duration_cast<chrono::microseconds>(now - it->second).count());
    inTimeMap_.erase(it);
    outRecord_.totalCostUs += fromInToOut;
    double oneFrameCostMs = fromInToOut / US_TO_MS;
    double averageCostMs = outRecord_.totalCostUs / US_TO_MS / outRecord_.totalCnt;

    uint64_t fromFirstOutToNow = static_cast<uint64_t>
        (chrono::duration_cast<chrono::microseconds>(now - firstOutTime_).count());
    if (fromFirstOutToNow == 0) {
        HLOGI("pts = %" PRId64 ", len = %u, flags = 0x%x, "
              "cost %.2f ms (%.2f ms)",
              info.omxBuffer->pts, info.omxBuffer->filledLen, info.omxBuffer->flag,
              oneFrameCostMs, averageCostMs);
    } else {
        double outFps = outRecord_.totalCnt * US_TO_S / fromFirstOutToNow;
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
        case GRAPHIC_PIXEL_FMT_RGBA_8888: {
            totalSize = byteStride * h;
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