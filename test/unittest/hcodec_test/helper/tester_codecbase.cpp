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
#include "type_converter.h"
#include "hcodec_log.h"
#include "hcodec_utils.h"
#include "hcodec_api.h"

namespace OHOS::MediaAVCodec {
using namespace std;

void TesterCodecBase::CallBack::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    TLOGI(">>");
}

void TesterCodecBase::CallBack::OnOutputFormatChanged(const Format &format)
{
    TLOGI(">>");
}

void TesterCodecBase::CallBack::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    lock_guard<mutex> lk(tester_->inputMtx_);
    tester_->inputList_.emplace_back(index, buffer);
    tester_->inputCond_.notify_all();
}

void TesterCodecBase::CallBack::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (!(buffer->flag_ & AVCODEC_BUFFER_FLAG_EOS)) {
        auto metaStr = StringifyMeta(buffer->meta_);
        TLOGI("%s", metaStr.c_str());
    }
    tester_->AfterGotOutput(OH_AVCodecBufferAttr {
        .pts = buffer->pts_,
        .size = buffer->memory_ ? buffer->memory_->GetSize() : 0,
        .flags = buffer->flag_,
    });
    lock_guard<mutex> lk(tester_->outputMtx_);
    tester_->outputList_.emplace_back(index, buffer);
    tester_->outputCond_.notify_all();
}

bool TesterCodecBase::Create()
{
    string mime = GetCodecMime(opt_.protocol);
    string name = GetCodecName(opt_.isEncoder, mime);
    auto begin = std::chrono::steady_clock::now();
    CreateHCodecByName(name, codec_);
    if (codec_ == nullptr) {
        TLOGE("Create failed");
        return false;
    }
    Media::Meta meta{};
    int32_t err = codec_->Init(meta);
    if (err != AVCS_ERR_OK) {
        TLOGE("Init failed");
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
        TLOGE("SetCallback failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "SetCallback");
    return true;
}

bool TesterCodecBase::Start()
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->Start();
    if (err != AVCS_ERR_OK) {
        TLOGE("Start failed");
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
        TLOGE("Stop failed");
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
        TLOGE("Release failed");
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
        TLOGE("Flush failed");
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

void TesterCodecBase::EnableHighPerf(Format& fmt) const
{
    if (opt_.isHighPerfMode) {
        fmt.PutIntValue("frame_rate_adaptive_mode", 1);
    }
}

bool TesterCodecBase::ConfigureEncoder()
{
    Format fmt;
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, opt_.dispW);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, opt_.dispH);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(opt_.pixFmt));
    fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, opt_.frameRate);
    if (opt_.rangeFlag.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, opt_.rangeFlag.value());
    }
    if (opt_.primary.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, opt_.primary.value());
    }
    if (opt_.transfer.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, opt_.transfer.value());
    }
    if (opt_.matrix.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, opt_.matrix.value());
    }
    if (opt_.iFrameInterval.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, opt_.iFrameInterval.value());
    }
    if (opt_.profile.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, opt_.profile.value());
    }
    if (opt_.rateMode.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, opt_.rateMode.value());
    }
    if (opt_.bitRate.has_value()) {
        fmt.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, opt_.bitRate.value());
    }
    if (opt_.quality.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, opt_.quality.value());
    }
    if (opt_.sqrFactor.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, opt_.sqrFactor.value());
    }
    if (opt_.maxBitrate.has_value()) {
        fmt.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, opt_.maxBitrate.value());
    }
    if (opt_.targetQp.has_value()) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TARGET_QP, opt_.targetQp.value());
    }
    if (opt_.layerCnt.has_value()) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
        int32_t temporalGopSize = 0;
        switch (opt_.layerCnt.value()) {
            case 2: // 2: temporal layerCnt
                temporalGopSize = 2; // 2: temporalGopSize
                break;
            case 3: // 3: temporal layerCnt
                temporalGopSize = 4; // 4: temporalGopSize
                break;
            default:
                break;
        }
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, temporalGopSize);
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, 2); // 2: gop mode
    }
    EnableHighPerf(fmt);
    if (opt_.qpRange.has_value()) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MIN, opt_.qpRange->qpMin);
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MAX, opt_.qpRange->qpMax);
    }
    if (opt_.repeatAfter.has_value()) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, opt_.repeatAfter.value());
    }
    if (opt_.repeatMaxCnt.has_value()) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, opt_.repeatMaxCnt.value());
    }
    if (!opt_.isBufferMode && !opt_.perFrameParamsMap.empty()) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, 1);
        opt_.enableInputCb = true;
    }
    if (opt_.ltrFrameCount > 0) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, opt_.ltrFrameCount);
    }
    if (opt_.paramsFeedback == 1) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_PARAMS_FEEDBACK, opt_.paramsFeedback);
    }
    if (opt_.enableQPMap == 1) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_QP_MAP, opt_.enableQPMap);
    }
    if (opt_.gopBMode.has_value()) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE, opt_.gopBMode.value());
    }
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->Configure(fmt);
    if (err != AVCS_ERR_OK) {
        TLOGE("Configure failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "Configure");

    if (opt_.waterMark.isSet) {
        shared_ptr<AVBuffer> buffer = CreateWaterMarkBuffer();
        err = codec_->SetCustomBuffer(buffer);
        if (err != AVCS_ERR_OK) {
            TLOGE("SetCustomBuffer failed");
            return false;
        }
    }
    return true;
}

bool TesterCodecBase::SetEncoderParameter(const SetParameterParams& param)
{
    Format fmt;
    if (param.bitRate.has_value()) {
        fmt.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, param.bitRate.value());
    }
    if (param.frameRate.has_value()) {
        fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, param.frameRate.value());
    }
    if (param.targetQp.has_value()) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TARGET_QP, param.targetQp.value());
    }
    if (param.operatingRate.has_value()) {
        fmt.PutDoubleValue(OHOS::Media::Tag::VIDEO_OPERATING_RATE, param.operatingRate.value());
    }
    if (param.qpRange.has_value()) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MIN, static_cast<int32_t>(param.qpRange->qpMin));
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MAX, static_cast<int32_t>(param.qpRange->qpMax));
    }
    if (opt_.scaleMode.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, static_cast<int32_t>(opt_.scaleMode.value()));
    }
    if (param.sqrParam.has_value()) {
        if (param.sqrParam->bitrate.has_value()) {
            fmt.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, param.sqrParam->bitrate.value());
        }
        if (param.sqrParam->maxBitrate.has_value()) {
            fmt.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, param.sqrParam->maxBitrate.value());
        }
        if (param.sqrParam->sqrFactor.has_value()) {
            fmt.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, param.sqrParam->sqrFactor.value());
        }
    }
    int32_t err = codec_->SetParameter(fmt);
    if (err != AVCS_ERR_OK) {
        TLOGE("SetParameter failed");
        return false;
    }
    return true;
}

bool TesterCodecBase::SetEncoderPerFrameParam(BufInfo& buf, const PerFrameParams& param)
{
    if (buf.avbuf == nullptr) {
        return false;
    }
    shared_ptr<Media::Meta> meta = buf.avbuf->meta_;
    if (meta == nullptr) {
        return false;
    }
    if (param.requestIdr.has_value()) {
        meta->SetData(OHOS::Media::Tag::VIDEO_REQUEST_I_FRAME, param.requestIdr.value());
    }
    if (param.qpRange.has_value()) {
        meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_QP_MIN, static_cast<int32_t>(param.qpRange->qpMin));
        meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_QP_MAX, static_cast<int32_t>(param.qpRange->qpMax));
    }
    if (param.ltrParam.has_value()) {
        meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_MARK_LTR,
            static_cast<bool>(param.ltrParam->markAsLTR));
        if (param.ltrParam->useLTR > 0) {
            meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_USE_LTR,
                static_cast<int32_t>(param.ltrParam->useLTRPoc));
        }
    }
    if (param.discard.has_value()) {
        meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, param.discard.value());
    }
    if (param.ebrParam.has_value()) {
        meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_QP_MIN, static_cast<int32_t>(param.ebrParam->minQp));
        meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_QP_MAX, static_cast<int32_t>(param.ebrParam->maxQp));
        meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_QP_START, static_cast<int32_t>(param.ebrParam->startQp));
        meta->SetData(OHOS::Media::Tag::VIDEO_PER_FRAME_IS_SKIP, static_cast<bool>(param.ebrParam->isSkip));
    }
    if (param.roiParams.has_value()) {
        meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_ROI_PARAMS, param.roiParams.value());
    }
    if (param.absQpMap.has_value() && param.qpMapValue.has_value()) {
        meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_ABS_QP_MAP, param.absQpMap.value());

        size_t widthInBlock = (opt_.dispW - 1) / 16 + 1; // divide by 16x16 block
        size_t heightInBlock = (opt_.dispH - 1) / 16 + 1; // divide by 16x16 block
        vector<uint8_t> qpMap (widthInBlock * heightInBlock,
                               static_cast<uint8_t>(param.qpMapValue.value()));
        meta->SetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_QP_MAP, qpMap);
    }
    return true;
}

sptr<Surface> TesterCodecBase::CreateInputSurface()
{
    auto begin = std::chrono::steady_clock::now();
    sptr<Surface> ret = codec_->CreateInputSurface();
    if (ret == nullptr) {
        TLOGE("CreateInputSurface failed");
        return nullptr;
    }
    CostRecorder::Instance().Update(begin, "CreateInputSurface");
    return ret;
}

bool TesterCodecBase::NotifyEos()
{
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->NotifyEos();
    if (err != AVCS_ERR_OK) {
        TLOGE("NotifyEos failed");
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
        TLOGE("RequestIDR failed");
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
        TLOGE("GetInputFormat failed");
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
        TLOGE("GetOutputFormat failed");
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

bool TesterCodecBase::WaitForInput(BufInfo& buf)
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
        std::tie(buf.idx, buf.avbuf) = inputList_.front();
        inputList_.pop_front();
    }
    if (buf.avbuf == nullptr) {
        TLOGE("null avbuffer");
        return false;
    }
    if (opt_.enableInputCb) {
        return true;
    }
    if (buf.avbuf->memory_ == nullptr) {
        TLOGE("null memory in avbuffer");
        return false;
    }
    buf.va = buf.avbuf->memory_->GetAddr();
    buf.capacity = buf.avbuf->memory_->GetCapacity();
    if (opt_.isEncoder && opt_.isBufferMode) {
        sptr<SurfaceBuffer> surfaceBuffer = buf.avbuf->memory_->GetSurfaceBuffer();
        if (!SurfaceBufferToBufferInfo(buf, surfaceBuffer)) {
            return false;
        }
    }
    return true;
}

bool TesterCodecBase::ReturnInput(const BufInfo& buf)
{
    if (!opt_.enableInputCb) {
        buf.avbuf->pts_ = buf.attr.pts;
        buf.avbuf->flag_ = buf.attr.flags;
        buf.avbuf->memory_->SetOffset(buf.attr.offset);
        buf.avbuf->memory_->SetSize(buf.attr.size);
    }

    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->QueueInputBuffer(buf.idx);
    if (err != AVCS_ERR_OK) {
        TLOGE("QueueInputBuffer failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "QueueInputBuffer");
    return true;
}

bool TesterCodecBase::WaitForOutput(BufInfo& buf)
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
        std::tie(buf.idx, buf.avbuf) = outputList_.front();
        outputList_.pop_front();
    }
    if (buf.avbuf == nullptr) {
        TLOGE("null avbuffer");
        return false;
    }
    if (buf.avbuf->flag_ & AVCODEC_BUFFER_FLAG_EOS) {
        TLOGI("output eos, quit loop");
        return false;
    }
    buf.attr.pts = buf.avbuf->pts_;
    if (buf.avbuf->memory_) {
        buf.va = buf.avbuf->memory_->GetAddr();
        buf.capacity = static_cast<size_t>(buf.avbuf->memory_->GetCapacity());
        buf.attr.size = buf.avbuf->memory_->GetSize();
    }
    return true;
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
        TLOGE("%s failed", apiName.c_str());
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
        TLOGE("SetOutputSurface failed");
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
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(opt_.pixFmt));
    fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, opt_.frameRate);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, opt_.rotation);
    EnableHighPerf(fmt);
    if (opt_.scaleMode.has_value()) {
        fmt.PutIntValue(MediaDescriptionKey::MD_KEY_SCALE_TYPE, static_cast<int32_t>(opt_.scaleMode.value()));
    }
    if (opt_.isVrrEnable.has_value()) {
        fmt.PutIntValue(OHOS::Media::Tag::VIDEO_DECODER_OUTPUT_ENABLE_VRR, opt_.isVrrEnable.value());
    }
    auto begin = std::chrono::steady_clock::now();
    int32_t err = codec_->Configure(fmt);
    if (err != AVCS_ERR_OK) {
        TLOGE("Configure failed");
        return false;
    }
    CostRecorder::Instance().Update(begin, "Configure");
    return true;
}
}