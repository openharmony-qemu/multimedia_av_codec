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

void HCodec::PrintAllBufferInfo()
{
    auto now = chrono::steady_clock::now();
    PrintAllBufferInfo(true, now);
    PrintAllBufferInfo(false, now);
}

void HCodec::PrintAllBufferInfo(bool isInput, std::chrono::time_point<std::chrono::steady_clock> now)
{
    std::array<uint32_t, OWNER_CNT> arr;
    arr.fill(0);
    std::stringstream s;
    const vector<BufferInfo>& pool = isInput ? inputBufferPool_ : outputBufferPool_;
    for (const BufferInfo& info : pool) {
        arr[info.owner]++;
        int64_t holdMs = chrono::duration_cast<chrono::milliseconds>(now - info.lastOwnerChangeTime).count();
        s << info.bufferId << ":" << ToString(info.owner) << "(" << holdMs << "), ";
    }
    HLOGI("%s: %u/%u/%u/%u, %s", (isInput ? " in" : "out"),
          arr[OWNED_BY_US], arr[OWNED_BY_USER], arr[OWNED_BY_OMX], arr[OWNED_BY_SURFACE],
          s.str().c_str());
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

std::array<uint32_t, HCodec::OWNER_CNT> HCodec::CountOwner(bool isInput)
{
    std::array<uint32_t, OWNER_CNT> arr;
    arr.fill(0);
    const vector<BufferInfo>& pool = isInput ? inputBufferPool_ : outputBufferPool_;
    for (const BufferInfo &info : pool) {
        arr[info.owner]++;
    }
    return arr;
}

void HCodec::TraceOwner(const std::array<uint32_t, OWNER_CNT>& arr, bool isInput)
{
    string inOutStr = isInput ? "in" : "out";
    CountTrace(HITRACE_TAG_ZMEDIA, compUniqueStr_ + inOutStr + "_us", arr[OWNED_BY_US]);
    CountTrace(HITRACE_TAG_ZMEDIA, compUniqueStr_ + inOutStr + "_user", arr[OWNED_BY_USER]);
    CountTrace(HITRACE_TAG_ZMEDIA, compUniqueStr_ + inOutStr + "_omx", arr[OWNED_BY_OMX]);
    CountTrace(HITRACE_TAG_ZMEDIA, compUniqueStr_ + inOutStr + "_surface", arr[OWNED_BY_SURFACE]);
}

void HCodec::PrintStatistic(bool isInput, std::chrono::time_point<std::chrono::steady_clock> now)
{
    int64_t fromFirstToNow = chrono::duration_cast<chrono::microseconds>(
        now - (isInput ? firstInTime_ : firstOutTime_)).count();
    if (fromFirstToNow == 0) {
        return;
    }
    double fps = (isInput ? inTotalCnt_ : outRecord_.totalCnt) * US_TO_S / fromFirstToNow;
    std::array<uint32_t, OWNER_CNT> arr = CountOwner(isInput);
    double aveHoldMs[OWNER_CNT];
    for (uint32_t owner = 0; owner < static_cast<uint32_t>(OWNER_CNT); owner++) {
        TotalCntAndCost& holdRecord = isInput ? inputHoldTimeRecord_[owner] : outputHoldTimeRecord_[owner];
        aveHoldMs[owner] = (holdRecord.totalCnt == 0) ? -1 : (holdRecord.totalCostUs / US_TO_MS / holdRecord.totalCnt);
    }
    HLOGI("%s:%.0f; %u/%u/%u/%u, %.0f/%.0f/%.0f/%.0f", (isInput ? " in" : "out"), fps,
        arr[OWNED_BY_US], arr[OWNED_BY_USER], arr[OWNED_BY_OMX], arr[OWNED_BY_SURFACE],
        aveHoldMs[OWNED_BY_US], aveHoldMs[OWNED_BY_USER], aveHoldMs[OWNED_BY_OMX], aveHoldMs[OWNED_BY_SURFACE]);
}

void HCodec::ChangeOwner(BufferInfo& info, BufferOwner newOwner)
{
    debugMode_ ? ChangeOwnerDebug(info, newOwner) : ChangeOwnerNormal(info, newOwner);
}

void HCodec::ChangeOwnerNormal(BufferInfo& info, BufferOwner newOwner)
{
    BufferOwner oldOwner = info.owner;
    auto now = chrono::steady_clock::now();
    uint64_t holdUs = static_cast<uint64_t>(
        chrono::duration_cast<chrono::microseconds>(now - info.lastOwnerChangeTime).count());
    TotalCntAndCost& holdRecord = info.isInput ? inputHoldTimeRecord_[oldOwner] :
                                                outputHoldTimeRecord_[oldOwner];
    holdRecord.totalCnt++;
    holdRecord.totalCostUs += holdUs;
    info.lastOwnerChangeTime = now;
    info.owner = newOwner;

    static constexpr uint64_t PRINT_PER_FRAME = 200;
    if (info.isInput && oldOwner == OWNED_BY_US && newOwner == OWNED_BY_OMX) {
        if (inTotalCnt_ == 0) {
            firstInTime_ = now;
        }
        inTotalCnt_++;
        if (inTotalCnt_ % PRINT_PER_FRAME == 0) {
            PrintStatistic(info.isInput, now);
            inTotalCnt_ = 0;
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
            outRecord_.totalCnt = 0;
            outputHoldTimeRecord_.fill({0, 0});
        }
    }
}

void HCodec::ChangeOwnerDebug(BufferInfo& info, BufferOwner newOwner)
{
    BufferOwner oldOwner = info.owner;
    const char* oldOwnerStr = ToString(oldOwner);
    const char* newOwnerStr = ToString(newOwner);
    const char* idStr = info.isInput ? "inBufId" : "outBufId";

    // calculate hold time
    auto now = chrono::steady_clock::now();
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
    std::array<uint32_t, OWNER_CNT> arr = CountOwner(info.isInput);
    HLOGI("%s = %u, after hold %.1f ms (%.1f ms), %s -> %s, "
          "%u/%u/%u/%u",
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

void HCodec::BufferInfo::Dump(const string& prefix, DumpMode dumpMode, bool isEncoder) const
{
    if (isInput) {
        if (((dumpMode & DUMP_ENCODER_INPUT) && isEncoder) ||
            ((dumpMode & DUMP_DECODER_INPUT) && !isEncoder)) {
            Dump(prefix + "_Input");
        }
    } else {
        if (((dumpMode & DUMP_ENCODER_OUTPUT) && isEncoder) ||
            ((dumpMode & DUMP_DECODER_OUTPUT) && !isEncoder)) {
            Dump(prefix + "_Output");
        }
    }
}

void HCodec::BufferInfo::Dump(const string& prefix) const
{
    if (surfaceBuffer) {
        DumpSurfaceBuffer(prefix);
    } else {
        DumpLinearBuffer(prefix);
    }
}

void HCodec::BufferInfo::DumpSurfaceBuffer(const std::string& prefix) const
{
    const char* va = reinterpret_cast<const char*>(surfaceBuffer->GetVirAddr());
    if (va == nullptr) {
        LOGW("surface buffer has null va");
        return;
    }
    bool eos = (omxBuffer->flag & OMX_BUFFERFLAG_EOS);
    if (eos || omxBuffer->filledLen == 0) {
        return;
    }
    int w = surfaceBuffer->GetWidth();
    int h = surfaceBuffer->GetHeight();
    int alignedW = surfaceBuffer->GetStride();
    uint32_t totalSize = surfaceBuffer->GetSize();
    if (w <= 0 || h <= 0 || alignedW <= 0 || w > alignedW) {
        LOGW("invalid buffer dimension");
        return;
    }
    std::optional<PixelFmt> fmt = TypeConverter::GraphicFmtToFmt(
        static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat()));
    if (fmt == nullopt) {
        LOGW("invalid fmt=%d", surfaceBuffer->GetFormat());
        return;
    }
    optional<uint32_t> assumeAlignedH;
    string suffix;
    bool dumpAsVideo = true;  // we could only save it as individual image if we don't know aligned height
    DecideDumpInfo(assumeAlignedH, suffix, dumpAsVideo);

    char name[128];
    int ret = 0;
    if (dumpAsVideo) {
        ret = sprintf_s(name, sizeof(name), "%s/%s_%dx%d(%dx%u)_fmt%s.%s",
                        DUMP_PATH, prefix.c_str(), w, h, alignedW, assumeAlignedH.value_or(h),
                        fmt->strFmt.c_str(), suffix.c_str());
    } else {
        ret = sprintf_s(name, sizeof(name), "%s/%s_%dx%d(%d)_fmt%s_pts%" PRId64 ".%s",
                        DUMP_PATH, prefix.c_str(), w, h, alignedW, fmt->strFmt.c_str(), omxBuffer->pts, suffix.c_str());
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

void HCodec::BufferInfo::DecideDumpInfo(optional<uint32_t>& assumeAlignedH, string& suffix, bool& dumpAsVideo) const
{
    int h = surfaceBuffer->GetHeight();
    int alignedW = surfaceBuffer->GetStride();
    if (alignedW <= 0) {
        return;
    }
    uint32_t totalSize = surfaceBuffer->GetSize();
    GraphicPixelFormat fmt = static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat());
    switch (fmt) {
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010: {
            suffix = "yuv";
            if (GetYuv420Size(alignedW, h) == totalSize) {
                break;
            }
            // 2 bytes per pixel for UV, 3 bytes per pixel for YUV
            uint32_t alignedH = totalSize * 2 / 3 / static_cast<uint32_t>(alignedW);
            if (GetYuv420Size(alignedW, alignedH) == totalSize) {
                dumpAsVideo = true;
                assumeAlignedH = alignedH;
            } else {
                dumpAsVideo = false;
            }
            break;
        }
        case GRAPHIC_PIXEL_FMT_RGBA_8888: {
            suffix = "rgba";
            if (static_cast<uint32_t>(alignedW * h) != totalSize) {
                dumpAsVideo = false;
            }
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
    if (avBuffer == nullptr || avBuffer->memory_ == nullptr) {
        LOGW("invalid avbuffer");
        return;
    }
    const char* va = reinterpret_cast<const char*>(avBuffer->memory_->GetAddr());
    if (va == nullptr) {
        LOGW("null va");
        return;
    }
    bool eos = (omxBuffer->flag & OMX_BUFFERFLAG_EOS);
    if (eos || omxBuffer->filledLen == 0) {
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
}