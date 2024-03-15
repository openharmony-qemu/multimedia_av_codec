/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "tester_capi.h"
#include "common/native_mfmagic.h"
#include "hcodec_log.h"
#include "native_avcodec_videodecoder.h"
#include "native_avcodec_videoencoder.h"
#include "native_window.h"

namespace OHOS::MediaAVCodec {
using namespace std;

void TesterCapi::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    TLOGI(">>");
}

void TesterCapi::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    TLOGI(">>");
}

void TesterCapiOld::OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    TesterCapiOld* tester = static_cast<TesterCapiOld*>(userData);
    if (tester == nullptr) {
        return;
    }
    lock_guard<mutex> lk(tester->inputMtx_);
    tester->inputList_.emplace_back(index, data);
    tester->inputCond_.notify_all();
}

void TesterCapiOld::OnNewOutputData(
    OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    TesterCapiOld* tester = static_cast<TesterCapiOld*>(userData);
    if (tester == nullptr || attr == nullptr) {
        return;
    }
    tester->AfterGotOutput(*attr);
    lock_guard<mutex> lk(tester->outputMtx_);
    tester->outputList_.emplace_back(index, data, *attr);
    tester->outputCond_.notify_all();
}

void TesterCapiNew::OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    TesterCapiNew* tester = static_cast<TesterCapiNew*>(userData);
    if (tester == nullptr) {
        return;
    }
    lock_guard<mutex> lk(tester->inputMtx_);
    tester->inputList_.emplace_back(index, buffer);
    tester->inputCond_.notify_all();
}

void TesterCapiNew::OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    TesterCapiNew* tester = static_cast<TesterCapiNew*>(userData);
    if (tester == nullptr) {
        return;
    }
    OH_AVCodecBufferAttr attr;
    OH_AVBuffer_GetBufferAttr(buffer, &attr);
    tester->AfterGotOutput(attr);
    lock_guard<mutex> lk(tester->outputMtx_);
    tester->outputList_.emplace_back(index, buffer);
    tester->outputCond_.notify_all();
}

bool TesterCapi::Create()
{
    const char* mime = (opt_.protocol == H264) ? OH_AVCODEC_MIMETYPE_VIDEO_AVC : OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
    auto begin = std::chrono::steady_clock::now();
    codec_ = opt_.isEncoder ? OH_VideoEncoder_CreateByMime(mime) : OH_VideoDecoder_CreateByMime(mime);
    if (codec_ == nullptr) {
        TLOGE("Create failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_CreateByMime" : "OH_VideoDecoder_CreateByMime");
    return true;
}

bool TesterCapiOld::SetCallback()
{
    OH_AVCodecAsyncCallback cb {
        &TesterCapi::OnError,
        &TesterCapi::OnStreamChanged,
        &TesterCapiOld::OnNeedInputData,
        &TesterCapiOld::OnNewOutputData,
    };
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = opt_.isEncoder ? OH_VideoEncoder_SetCallback(codec_, cb, this) :
                                        OH_VideoDecoder_SetCallback(codec_, cb, this);
    if (ret != AV_ERR_OK) {
        TLOGE("SetCallback failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_SetCallback" : "OH_VideoDecoder_SetCallback");
    return true;
}

bool TesterCapiNew::SetCallback()
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVCodecCallback cb {
        &TesterCapi::OnError,
        &TesterCapi::OnStreamChanged,
        &TesterCapiNew::OnNeedInputBuffer,
        &TesterCapiNew::OnNewOutputBuffer,
    };
    OH_AVErrCode ret = opt_.isEncoder ? OH_VideoEncoder_RegisterCallback(codec_, cb, this) :
                                        OH_VideoDecoder_RegisterCallback(codec_, cb, this);
    if (ret != AV_ERR_OK) {
        TLOGE("RegisterCallback failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_RegisterCallback" : "OH_VideoDecoder_RegisterCallback");
    return true;
}

bool TesterCapi::Start()
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = opt_.isEncoder ? OH_VideoEncoder_Start(codec_) :
                                        OH_VideoDecoder_Start(codec_);
    if (ret != AV_ERR_OK) {
        TLOGE("Start failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_Start" : "OH_VideoDecoder_Start");
    return true;
}

bool TesterCapi::Stop()
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = opt_.isEncoder ? OH_VideoEncoder_Stop(codec_) :
                                        OH_VideoDecoder_Stop(codec_);
    if (ret != AV_ERR_OK) {
        TLOGE("Stop failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_Stop" : "OH_VideoDecoder_Stop");
    return true;
}

bool TesterCapi::Release()
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = opt_.isEncoder ? OH_VideoEncoder_Destroy(codec_) :
                                        OH_VideoDecoder_Destroy(codec_);
    if (ret != AV_ERR_OK) {
        TLOGE("Destroy failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_Destroy" : "OH_VideoDecoder_Destroy");
    return true;
}

bool TesterCapi::Flush()
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = opt_.isEncoder ? OH_VideoEncoder_Flush(codec_) :
                                        OH_VideoDecoder_Flush(codec_);
    if (ret != AV_ERR_OK) {
        TLOGE("Flush failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_Flush" : "OH_VideoDecoder_Flush");
    return true;
}

void TesterCapiOld::ClearAllBuffer()
{
    {
        lock_guard<mutex> lk(inputMtx_);
        inputList_.clear();
    }
    {
        lock_guard<mutex> lk(outputMtx_);
        outputList_.clear();
    }
}

void TesterCapiNew::ClearAllBuffer()
{
    {
        lock_guard<mutex> lk(inputMtx_);
        inputList_.clear();
    }
    {
        lock_guard<mutex> lk(outputMtx_);
        outputList_.clear();
    }
}

bool TesterCapi::ConfigureEncoder()
{
    auto fmt = shared_ptr<OH_AVFormat>(OH_AVFormat_Create(), OH_AVFormat_Destroy);
    IF_TRUE_RETURN_VAL_WITH_MSG(fmt == nullptr, false, "OH_AVFormat_Create failed");
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_WIDTH, opt_.dispW);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_HEIGHT, opt_.dispH);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(opt_.pixFmt));
    OH_AVFormat_SetDoubleValue(fmt.get(), OH_MD_KEY_FRAME_RATE, opt_.frameRate);
    if (opt_.rangeFlag.has_value()) {
        OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_RANGE_FLAG, opt_.rangeFlag.value());
    }
    if (opt_.primary.has_value()) {
        OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_COLOR_PRIMARIES, opt_.primary.value());
    }
    if (opt_.transfer.has_value()) {
        OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_TRANSFER_CHARACTERISTICS, opt_.transfer.value());
    }
    if (opt_.matrix.has_value()) {
        OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_MATRIX_COEFFICIENTS, opt_.matrix.value());
    }
    if (opt_.iFrameInterval.has_value()) {
        OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_I_FRAME_INTERVAL, opt_.iFrameInterval.value());
    }
    if (opt_.profile.has_value()) {
        OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_PROFILE, opt_.profile.value());
    }
    if (opt_.rateMode.has_value()) {
        OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, opt_.rateMode.value());
    }
    if (opt_.bitRate.has_value()) {
        OH_AVFormat_SetLongValue(fmt.get(), OH_MD_KEY_BITRATE, opt_.bitRate.value());
    }
    if (opt_.quality.has_value()) {
        OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_QUALITY, opt_.quality.value());
    }

    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = OH_VideoEncoder_Configure(codec_, fmt.get());
    if (ret != AV_ERR_OK) {
        TLOGE("ConfigureEncoder failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoEncoder_Configure");
    return true;
}

sptr<Surface> TesterCapi::CreateInputSurface()
{
    OHNativeWindow *window = nullptr;
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = OH_VideoEncoder_GetSurface(codec_, &window);
    if (ret != AV_ERR_OK || window == nullptr) {
        TLOGE("CreateInputSurface failed");
        return nullptr;
    }
    sptr<Surface> surface = window->surface;
    if (surface == nullptr) {
        TLOGE("surface in OHNativeWindow is null");
        return nullptr;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoEncoder_GetSurface");
    // if we dont decrease here, the OHNativeWindow will never be destroyed
    OH_NativeWindow_DestroyNativeWindow(window);
    return surface;
}

bool TesterCapi::NotifyEos()
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = OH_VideoEncoder_NotifyEndOfStream(codec_);
    if (ret != AV_ERR_OK) {
        TLOGE("NotifyEos failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoEncoder_NotifyEndOfStream");
    return true;
}

bool TesterCapi::RequestIDR()
{
    auto fmt = shared_ptr<OH_AVFormat>(OH_AVFormat_Create(), OH_AVFormat_Destroy);
    IF_TRUE_RETURN_VAL_WITH_MSG(fmt == nullptr, false, "OH_AVFormat_Create failed");
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_REQUEST_I_FRAME, true);

    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = OH_VideoEncoder_SetParameter(codec_, fmt.get());
    if (ret != AV_ERR_OK) {
        TLOGE("RequestIDR failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoEncoder_SetParameter");
    return true;
}

bool TesterCapi::GetInputFormat()
{
    if (!opt_.isEncoder) {
        return true;
    }
    auto begin = std::chrono::steady_clock::now();
    OH_AVFormat *fmt = OH_VideoEncoder_GetInputDescription(codec_);
    if (fmt == nullptr) {
        TLOGE("GetInputFormat failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoEncoder_GetInputDescription");
    inputFmt_ = shared_ptr<OH_AVFormat>(fmt, OH_AVFormat_Destroy);
    OH_AVFormat_GetIntValue(inputFmt_.get(), "stride", &inputStride_);
    return true;
}

bool TesterCapi::GetOutputFormat()
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVFormat *fmt = opt_.isEncoder ? OH_VideoEncoder_GetOutputDescription(codec_) :
                                        OH_VideoDecoder_GetOutputDescription(codec_);
    if (fmt == nullptr) {
        TLOGE("GetOutputFormat failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_GetOutputDescription" : "OH_VideoDecoder_GetOutputDescription");
    OH_AVFormat_Destroy(fmt);
    return true;
}

optional<uint32_t> TesterCapi::GetInputStride()
{
    int32_t stride = 0;
    if (OH_AVFormat_GetIntValue(inputFmt_.get(), "stride", &stride)) {
        return stride;
    } else {
        return nullopt;
    }
}

bool TesterCapiOld::WaitForInput(BufInfo& buf)
{
    {
        unique_lock<mutex> lk(inputMtx_);
        if (opt_.timeout == -1) {
            inputCond_.wait(lk, [this] {
                return !inputList_.empty();
            });
        } else {
            bool ret = inputCond_.wait_for(lk, chrono::milliseconds(opt_.timeout), [this] {
                return !inputList_.empty();
            });
            if (!ret) {
                TLOGE("time out");
                return false;
            }
        }
        std::tie(buf.idx, buf.mem) = inputList_.front();
        inputList_.pop_front();
    }
    if (buf.mem == nullptr) {
        TLOGE("null OH_AVMemory");
        return false;
    }
    buf.va = OH_AVMemory_GetAddr(buf.mem);
    buf.capacity = OH_AVMemory_GetSize(buf.mem);
    if (opt_.isEncoder && opt_.isBufferMode) {
        buf.dispW = opt_.dispW;
        buf.dispH = opt_.dispH;
        buf.fmt = displayFmt_;
        if (inputStride_ < static_cast<int32_t>(opt_.dispW)) {
            TLOGE("pixelStride %d < dispW %u", inputStride_, opt_.dispW);
            return false;
        }
        buf.byteStride = inputStride_;
    }
    return true;
}

bool TesterCapiNew::WaitForInput(BufInfo& buf)
{
    {
        unique_lock<mutex> lk(inputMtx_);
        if (opt_.timeout == -1) {
            inputCond_.wait(lk, [this] {
                return !inputList_.empty();
            });
        } else {
            bool ret = inputCond_.wait_for(lk, chrono::milliseconds(opt_.timeout), [this] {
                return !inputList_.empty();
            });
            if (!ret) {
                TLOGE("time out");
                return false;
            }
        }
        std::tie(buf.idx, buf.cavbuf) = inputList_.front();
        inputList_.pop_front();
    }
    if (buf.cavbuf == nullptr) {
        TLOGE("null OH_AVBuffer");
        return false;
    }
    buf.va = OH_AVBuffer_GetAddr(buf.cavbuf);
    buf.capacity = OH_AVBuffer_GetCapacity(buf.cavbuf);
    if (opt_.isEncoder && opt_.isBufferMode) {
        OH_NativeBuffer* nativeBuffer = OH_AVBuffer_GetNativeBuffer(buf.cavbuf);
        if (!NativeBufferToBufferInfo(buf, nativeBuffer)) {
            return false;
        }
    }
    return true;
}

bool TesterCapiOld::WaitForOutput(BufInfo& buf)
{
    {
        unique_lock<mutex> lk(outputMtx_);
        if (opt_.timeout == -1) {
            outputCond_.wait(lk, [this] {
                return !outputList_.empty();
            });
        } else {
            bool waitRes = outputCond_.wait_for(lk, chrono::milliseconds(opt_.timeout), [this] {
                return !outputList_.empty();
            });
            if (!waitRes) {
                TLOGE("time out");
                return false;
            }
        }
        std::tie(buf.idx, buf.mem, buf.attr) = outputList_.front();
        outputList_.pop_front();
    }
    if (buf.attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        TLOGI("output eos, quit loop");
        return false;
    }
    if (buf.mem != nullptr) {
        buf.va = OH_AVMemory_GetAddr(buf.mem);
        buf.capacity = OH_AVMemory_GetSize(buf.mem);
    }
    return true;
}

bool TesterCapiNew::WaitForOutput(BufInfo& buf)
{
    {
        unique_lock<mutex> lk(outputMtx_);
        if (opt_.timeout == -1) {
            outputCond_.wait(lk, [this] {
                return !outputList_.empty();
            });
        } else {
            bool waitRes = outputCond_.wait_for(lk, chrono::milliseconds(opt_.timeout), [this] {
                return !outputList_.empty();
            });
            if (!waitRes) {
                TLOGE("time out");
                return false;
            }
        }
        std::tie(buf.idx, buf.cavbuf) = outputList_.front();
        outputList_.pop_front();
    }
    OH_AVCodecBufferAttr attr;
    OH_AVBuffer_GetBufferAttr(buf.cavbuf, &attr);
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        TLOGI("output eos, quit loop");
        return false;
    }
    buf.attr = attr;
    buf.va = OH_AVBuffer_GetAddr(buf.cavbuf);
    buf.capacity = OH_AVBuffer_GetCapacity(buf.cavbuf);
    return true;
}

bool TesterCapiOld::ReturnInput(const BufInfo& buf)
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode err = opt_.isEncoder ? OH_VideoEncoder_PushInputData(codec_, buf.idx, buf.attr) :
                                        OH_VideoDecoder_PushInputData(codec_, buf.idx, buf.attr);
    if (err != AV_ERR_OK) {
        TLOGE("QueueInputBuffer failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_PushInputData" : "OH_VideoDecoder_PushInputData");
    return true;
}

bool TesterCapiNew::ReturnInput(const BufInfo& buf)
{
    OH_AVBuffer_SetBufferAttr(buf.cavbuf, &buf.attr);
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode err = opt_.isEncoder ? OH_VideoEncoder_PushInputBuffer(codec_, buf.idx) :
                                        OH_VideoDecoder_PushInputBuffer(codec_, buf.idx);
    if (err != AV_ERR_OK) {
        TLOGE("PushInputBuffer failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_PushInputBuffer" : "OH_VideoDecoder_PushInputBuffer");
    return true;
}

bool TesterCapiOld::ReturnOutput(uint32_t idx)
{
    OH_AVErrCode err;
    string apiName;
    auto begin = std::chrono::steady_clock::now();
    if (opt_.isEncoder) {
        err = OH_VideoEncoder_FreeOutputData(codec_, idx);
        apiName = "OH_VideoEncoder_FreeOutputData";
    } else {
        if (opt_.isBufferMode) {
            err = OH_VideoDecoder_FreeOutputData(codec_, idx);
            apiName = "OH_VideoDecoder_FreeOutputData";
        } else {
            err = OH_VideoDecoder_RenderOutputData(codec_, idx);
            apiName = "OH_VideoDecoder_RenderOutputData";
        }
    }
    if (err != AV_ERR_OK) {
        TLOGE("%s failed", apiName.c_str());
        return false;
    }
    CostRecorder::Instance().Update(begin, apiName);
    return true;
}

bool TesterCapiNew::ReturnOutput(uint32_t idx)
{
    OH_AVErrCode err;
    string apiName;
    auto begin = std::chrono::steady_clock::now();
    if (opt_.isEncoder) {
        err = OH_VideoEncoder_FreeOutputBuffer(codec_, idx);
        apiName = "OH_VideoEncoder_FreeOutputBuffer";
    } else {
        if (opt_.isBufferMode) {
            err = OH_VideoDecoder_FreeOutputBuffer(codec_, idx);
            apiName = "OH_VideoDecoder_FreeOutputBuffer";
        } else {
            err = OH_VideoDecoder_RenderOutputBuffer(codec_, idx);
            apiName = "OH_VideoDecoder_RenderOutputBuffer";
        }
    }
    if (err != AV_ERR_OK) {
        TLOGE("%s failed", apiName.c_str());
        return false;
    }
    CostRecorder::Instance().Update(begin, apiName);
    return true;
}

bool TesterCapi::SetOutputSurface(sptr<Surface>& surface)
{
    OHNativeWindow *window = CreateNativeWindowFromSurface(&surface);
    if (window == nullptr) {
        TLOGE("CreateNativeWindowFromSurface failed");
        return false;
    }
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode err = OH_VideoDecoder_SetSurface(codec_, window);
    if (err != AV_ERR_OK) {
        TLOGE("OH_VideoDecoder_SetSurface failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoDecoder_SetSurface");
    return true;
}

bool TesterCapi::ConfigureDecoder()
{
    auto fmt = shared_ptr<OH_AVFormat>(OH_AVFormat_Create(), OH_AVFormat_Destroy);
    IF_TRUE_RETURN_VAL_WITH_MSG(fmt == nullptr, false, "OH_AVFormat_Create failed");
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_WIDTH, opt_.dispW);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_HEIGHT, opt_.dispH);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(opt_.pixFmt));
    OH_AVFormat_SetDoubleValue(fmt.get(), OH_MD_KEY_FRAME_RATE, opt_.frameRate);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_ROTATION, opt_.rotation);

    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = OH_VideoDecoder_Configure(codec_, fmt.get());
    if (ret != AV_ERR_OK) {
        TLOGE("OH_VideoDecoder_Configure failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoDecoder_Configure");
    return true;
}
}