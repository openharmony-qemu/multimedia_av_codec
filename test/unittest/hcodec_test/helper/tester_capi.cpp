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
#include "native_avcodec_videoencoder.h"
#include "native_avcodec_videodecoder.h"
#include "native_window.h"
#include "surface.h"
#include "hcodec_log.h"

using namespace std;
using namespace OHOS::MediaAVCodec;

void TesterCapi::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    LOGI(">>");
}

void TesterCapi::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    LOGI(">>");
}

void TesterCapi::OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    TesterCapi* tester = static_cast<TesterCapi*>(userData);
    if (tester == nullptr) {
        return;
    }
    lock_guard<mutex> lk(tester->inputMtx_);
    tester->inputList_.emplace_back(index, data);
    tester->inputCond_.notify_all();
}

void TesterCapi::OnNewOutputData(
    OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    TesterCapi* tester = static_cast<TesterCapi*>(userData);
    if (tester == nullptr || attr == nullptr) {
        return;
    }
    lock_guard<mutex> lk(tester->outputMtx_);
    tester->outputList_.emplace_back(index, data, *attr);
    tester->outputCond_.notify_all();
}

bool TesterCapi::Create()
{
    const char* mime = (opt_.protocol == H264) ? OH_AVCODEC_MIMETYPE_VIDEO_AVC : OH_AVCODEC_MIMETYPE_VIDEO_HEVC;
    auto begin = std::chrono::steady_clock::now();
    codec_ = opt_.isEncoder ? OH_VideoEncoder_CreateByMime(mime) : OH_VideoDecoder_CreateByMime(mime);
    if (codec_ == nullptr) {
        LOGE("Create failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_CreateByMime" : "OH_VideoDecoder_CreateByMime");
    return true;
}

bool TesterCapi::SetCallback()
{
    OH_AVCodecAsyncCallback cb {
        &TesterCapi::OnError,
        &TesterCapi::OnStreamChanged,
        &TesterCapi::OnNeedInputData,
        &TesterCapi::OnNewOutputData,
    };
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = opt_.isEncoder ? OH_VideoEncoder_SetCallback(codec_, cb, this) :
                                        OH_VideoDecoder_SetCallback(codec_, cb, this);
    if (ret != AV_ERR_OK) {
        LOGE("SetCallback failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_SetCallback" : "OH_VideoDecoder_SetCallback");
    return true;
}

bool TesterCapi::Start()
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = opt_.isEncoder ? OH_VideoEncoder_Start(codec_) :
                                        OH_VideoDecoder_Start(codec_);
    if (ret != AV_ERR_OK) {
        LOGE("Start failed");
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
        LOGE("Stop failed");
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
        LOGE("Destroy failed");
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
        LOGE("Flush failed");
        return false;
    }
    CostRecorder::Instance().Update(begin,
        opt_.isEncoder ? "OH_VideoEncoder_Flush" : "OH_VideoDecoder_Flush");
    return true;
}

bool TesterCapi::ConfigureEncoder()
{
    auto fmt = shared_ptr<OH_AVFormat>(OH_AVFormat_Create(), OH_AVFormat_Destroy);
    IF_TRUE_RETURN_VAL_WITH_MSG(fmt == nullptr, false, "OH_AVFormat_Create failed");
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_WIDTH, opt_.dispW);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_HEIGHT, opt_.dispH);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_PIXEL_FORMAT, opt_.pixFmt);
    OH_AVFormat_SetDoubleValue(fmt.get(), OH_MD_KEY_FRAME_RATE, opt_.frameRate);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_RANGE_FLAG, opt_.rangeFlag);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_COLOR_PRIMARIES, opt_.primary);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_TRANSFER_CHARACTERISTICS, opt_.transfer);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_MATRIX_COEFFICIENTS, opt_.matrix);

    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_I_FRAME_INTERVAL, opt_.iFrameInterval);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_PROFILE, opt_.profile);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, opt_.rateMode);
    OH_AVFormat_SetLongValue(fmt.get(), OH_MD_KEY_BITRATE, opt_.bitRate);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_QUALITY, opt_.quality);

    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = OH_VideoEncoder_Configure(codec_, fmt.get());
    if (ret != AV_ERR_OK) {
        LOGE("ConfigureEncoder failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoEncoder_Configure");
    return true;
}

bool TesterCapi::CreateInputSurface()
{
    OHNativeWindow *window = nullptr;
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = OH_VideoEncoder_GetSurface(codec_, &window);
    if (ret != AV_ERR_OK || window == nullptr) {
        LOGE("CreateInputSurface failed");
        return false;
    }
    surface_ = window->surface;
    if (surface_ == nullptr) {
        LOGE("surface in OHNativeWindow is null");
        return false;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoEncoder_GetSurface");
    // if we dont decrease here, the OHNativeWindow will never be destroyed
    OH_NativeWindow_DestroyNativeWindow(window);
    return true;
}

bool TesterCapi::NotifyEos()
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = OH_VideoEncoder_NotifyEndOfStream(codec_);
    if (ret != AV_ERR_OK) {
        LOGE("NotifyEos failed");
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
        LOGE("RequestIDR failed");
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
        LOGE("GetInputFormat failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoEncoder_GetInputDescription");
    inputFmt_ = shared_ptr<OH_AVFormat>(fmt, OH_AVFormat_Destroy);
    return true;
}

bool TesterCapi::GetOutputFormat()
{
    auto begin = std::chrono::steady_clock::now();
    OH_AVFormat *fmt = opt_.isEncoder ? OH_VideoEncoder_GetOutputDescription(codec_) :
                                        OH_VideoDecoder_GetOutputDescription(codec_);
    if (fmt == nullptr) {
        LOGE("GetOutputFormat failed");
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

void TesterCapi::InputLoop()
{
    while (true)  {
        uint32_t inputIdx;
        OH_AVMemory* mem;
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
                    LOGE("Input wait time out");
                    return;
                }
            }
            std::tie(inputIdx, mem) = inputList_.front();
            inputList_.pop_front();
        }
        if (mem == nullptr) {
            continue;
        }
        char *dstVa = reinterpret_cast<char*>(OH_AVMemory_GetAddr(mem));
        int size = OH_AVMemory_GetSize(mem);
        if (dstVa == nullptr || size <= 0) {
            LOGE("invalid va or size");
            continue;
        }

        OH_AVCodecBufferAttr info;
        info.offset = 0;
        info.flags = 0;
        if (opt_.isEncoder) {
            info.size = ReadOneFrame(dstVa);
        } else {
            pair<uint32_t, uint32_t> p = GetNextSample(dstVa, size);
            info.size = static_cast<int32_t>(p.first);
            info.flags = p.second;
        }
        if (info.size == 0 || (opt_.inputCnt > 0 && currInputCnt_ > opt_.inputCnt)) {
            info.flags = AVCODEC_BUFFER_FLAGS_EOS;
            OH_AVErrCode err = opt_.isEncoder ? OH_VideoEncoder_PushInputData(codec_, inputIdx, info) :
                                                OH_VideoDecoder_PushInputData(codec_, inputIdx, info);
            if (err != AV_ERR_OK) {
                LOGE("queue eos failed");
                continue;
            } else {
                LOGI("queue eos succ, quit loop");
                return;
            }
        }
        info.pts = GetNowUs();
        auto begin = std::chrono::steady_clock::now();
        OH_AVErrCode err = opt_.isEncoder ? OH_VideoEncoder_PushInputData(codec_, inputIdx, info) :
                                            OH_VideoDecoder_PushInputData(codec_, inputIdx, info);
        if (err != AV_ERR_OK) {
            LOGE("QueueInputBuffer failed");
            continue;
        }
        CostRecorder::Instance().Update(begin,
            opt_.isEncoder ? "OH_VideoEncoder_PushInputData" : "OH_VideoDecoder_PushInputData");
        currInputCnt_++;
        if (opt_.isEncoder && currInputCnt_ == opt_.numIdrFrame) {
            RequestIDR();
        }
    }
}

void TesterCapi::OutputLoop()
{
    while (true) {
        uint32_t outIdx;
        OH_AVMemory* mem;
        OH_AVCodecBufferAttr attr;
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
                    LOGE("time out");
                    return;
                }
            }
            std::tie(outIdx, mem, attr) = outputList_.front();
            outputList_.pop_front();
        }
        if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            LOGI("output eos, quit loop");
            break;
        }
        OH_AVErrCode err;
        string apiName;
        auto begin = std::chrono::steady_clock::now();
        if (opt_.isEncoder) {
            err = OH_VideoEncoder_FreeOutputData(codec_, outIdx);
            apiName = "OH_VideoEncoder_FreeOutputData";
        } else {
            if (opt_.isBufferMode) {
                err = OH_VideoDecoder_FreeOutputData(codec_, outIdx);
                apiName = "OH_VideoDecoder_FreeOutputData";
            } else {
                err = OH_VideoDecoder_RenderOutputData(codec_, outIdx);
                apiName = "OH_VideoDecoder_RenderOutputData";
            }
        }
        if (err != AV_ERR_OK) {
            LOGE("%{public}s failed", apiName.c_str());
            continue;
        }
        CostRecorder::Instance().Update(begin, apiName);
    }
}

bool TesterCapi::SetOutputSurface(sptr<Surface>& surface)
{
    OHNativeWindow *window = CreateNativeWindowFromSurface(&surface);
    if (window == nullptr) {
        LOGE("CreateNativeWindowFromSurface failed");
        return false;
    }
    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode err = OH_VideoDecoder_SetSurface(codec_, window);
    if (err != AV_ERR_OK) {
        LOGE("OH_VideoDecoder_SetSurface failed");
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
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_PIXEL_FORMAT, opt_.pixFmt);
    OH_AVFormat_SetDoubleValue(fmt.get(), OH_MD_KEY_FRAME_RATE, opt_.frameRate);
    OH_AVFormat_SetIntValue(fmt.get(), OH_MD_KEY_ROTATION, opt_.rotation);

    auto begin = std::chrono::steady_clock::now();
    OH_AVErrCode ret = OH_VideoDecoder_Configure(codec_, fmt.get());
    if (ret != AV_ERR_OK) {
        LOGE("OH_VideoDecoder_Configure failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "OH_VideoDecoder_Configure");
    return true;
}