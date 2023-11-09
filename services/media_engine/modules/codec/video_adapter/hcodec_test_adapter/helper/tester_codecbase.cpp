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

#include "tester_codecbase.h"
#include "avcodec_errors.h"
#include "hcodec_log.h"
#include "hcodec_api.h"

using namespace std;
using namespace OHOS::MediaAVCodec;

void TesterCodecBase::CallBack::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    LOGI(">>");
}

void TesterCodecBase::CallBack::OnOutputFormatChanged(const Format &format)
{
    LOGI(">>");
    int outBufCnt = 0;
    if (format.GetIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, outBufCnt)) {
        LOGI("outBufCnt = %{public}d", outBufCnt);
    }
}

void TesterCodecBase::CallBack::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    lock_guard<mutex> lk(tester_->inputMtx_);
    tester_->inputList_.emplace_back(index, buffer);
    tester_->inputCond_.notify_all();
}

void TesterCodecBase::CallBack::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    lock_guard<mutex> lk(tester_->outputMtx_);
    tester_->outputList_.emplace_back(index, buffer);
    tester_->outputCond_.notify_all();
}

bool TesterCodecBase::Create()
{
    string name = GetCodecName(opt_.isEncoder, (opt_.protocol == H264) ? "video/avc" : "video/hevc");
    auto begin = std::chrono::steady_clock::now();
    CreateHCodecByName(name, codec_);
    if (codec_ == nullptr) {
        LOGE("Create failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "Create");
    return true;
}

bool TesterCodecBase::SetCallback()
{
    shared_ptr<CallBack> cb = make_shared<CallBack>(this);
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->SetCallback(cb);
    if (err != AVCS_ERR_OK) {
        LOGE("SetCallback failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "SetCallback");
    return true;
}

bool TesterCodecBase::Prepare()
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->Prepare();
    if (err != AVCS_ERR_OK) {
        LOGE("Prepare failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "Prepare");
    return true;
}

bool TesterCodecBase::Start()
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->Start();
    if (err != AVCS_ERR_OK) {
        LOGE("Start failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "Start");
    return true;
}

bool TesterCodecBase::Stop()
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->Stop();
    if (err != AVCS_ERR_OK) {
        LOGE("Stop failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "Stop");
    return true;
}

bool TesterCodecBase::Release()
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->Release();
    if (err != AVCS_ERR_OK) {
        LOGE("Release failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "Release");
    return true;
}

bool TesterCodecBase::Flush()
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->Flush();
    if (err != AVCS_ERR_OK) {
        LOGE("Flush failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "Flush");
    return true;
}

void TesterCodecBase::ClearAllBuffer()
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

void TesterCodecBase::EnableHighPerf(Format& fmt)
{
    if (opt_.isHighPerfMode) {
        fmt.PutIntValue("working_in_max_frequency", 1);
        fmt.PutStringValue("process_name", "cast_engine_service");
    }
}

bool TesterCodecBase::ConfigureEncoder()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, opt_.dispW);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, opt_.dispH);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, opt_.pixFmt);
    fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, opt_.frameRate);

    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, opt_.rangeFlag);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, opt_.primary);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, opt_.transfer);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, opt_.matrix);

    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, opt_.iFrameInterval);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, opt_.profile);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, opt_.rateMode);
    fmt.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, opt_.bitRate);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, opt_.quality);
    EnableHighPerf(fmt);

    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->Configure(fmt);
    if (err != AVCS_ERR_OK) {
        LOGE("Configure failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "Configure");
    return true;
}

bool TesterCodecBase::CreateInputSurface()
{
    auto begin = std::chrono::steady_clock::now();
    surface_ = codec_->CreateInputSurface();
    if (surface_ == nullptr) {
        LOGE("CreateInputSurface failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "CreateInputSurface");
    return true;
}

bool TesterCodecBase::NotifyEos()
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->NotifyEos();
    if (err != AVCS_ERR_OK) {
        LOGE("NotifyEos failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "NotifyEos");
    return true;
}

bool TesterCodecBase::RequestIDR()
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->SignalRequestIDRFrame();
    if (err != AVCS_ERR_OK) {
        LOGE("RequestIDR failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "SignalRequestIDRFrame");
    return true;
}

bool TesterCodecBase::GetInputFormat()
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->GetInputFormat(inputFmt_);
    if (err != AVCS_ERR_OK) {
        LOGE("GetInputFormat failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "GetInputFormat");
    return true;
}

bool TesterCodecBase::GetOutputFormat()
{
    Format fmt;
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->GetOutputFormat(fmt);
    if (err != AVCS_ERR_OK) {
        LOGE("GetOutputFormat failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "GetOutputFormat");
    return true;
}

optional<uint32_t> TesterCodecBase::GetInputStride()
{
    int32_t stride = 0;
    if (inputFmt_.GetIntValue("stride", stride)) {
        return stride;
    } else {
        return nullopt;
    }
}

std::optional<uint32_t> TesterCodecBase::GetInputIndexForAvBuffer(std::shared_ptr<AVBuffer>& avBuffer)
{
    uint32_t inputIdx;
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
                return nullopt;
            }
        }
        std::tie(inputIdx, avBuffer) = inputList_.front();
        inputList_.pop_front();
    }
    if (avBuffer == nullptr) {
        LOGE("null AVBuffer");
        return nullopt;
    }
    return inputIdx;
}

bool TesterCodecBase::QueueInputForAvBuffer(uint32_t idx, std::shared_ptr<AVBuffer>& avBuffer)
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->QueueInputBuffer(idx, avBuffer);
    if (err != AVCS_ERR_OK) {
        LOGE("QueueInputBuffer failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "QueueInputBuffer");
    return true;
}

std::optional<uint32_t> TesterCodecBase::GetOutputIndex()
{
    uint32_t outIdx;
    std::shared_ptr<AVBuffer> buffer;
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
                return nullopt;
            }
        }
        std::tie(outIdx, buffer) = outputList_.front();
        outputList_.pop_front();
    }
    if (buffer->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
        LOGI("output eos, quit loop");
        return nullopt;
    }
    return outIdx;
}

bool TesterCodecBase::ReturnOutput(uint32_t idx)
{
    int32_t ret;
    string apiName;
    auto begin = std::chrono::steady_clock::now();
    if (opt_.isEncoder || opt_.isBufferMode) {
        ret = codec_->ReleaseOutputBuffer(idx);
        apiName = "ReleaseOutputBuffer";
    } else {
        ret = codec_->RenderOutputBuffer(idx);
        apiName = "RenderOutputBuffer";
    }
    if (ret != AVCS_ERR_OK) {
        LOGE("%{public}s failed", apiName.c_str());
        return false;
    }
    CostRecorder::Instance().Update(begin, apiName);
    return true;
}

bool TesterCodecBase::SetOutputSurface(sptr<Surface>& surface)
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->SetOutputSurface(surface);
    if (err != AVCS_ERR_OK) {
        LOGE("SetOutputSurface failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "SetOutputSurface");
    return true;
}

bool TesterCodecBase::ConfigureDecoder()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, opt_.dispW);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, opt_.dispH);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, opt_.pixFmt);
    fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, opt_.frameRate);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, opt_.rotation);
    EnableHighPerf(fmt);

    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->Configure(fmt);
    if (err != AVCS_ERR_OK) {
        LOGE("Configure failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "Configure");
    return true;
}