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

#include "hencoder_test.h"
#include <thread>
#include "sync_fence.h"
#include "av_common.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "type_converter.h"
#include "hcodec_log.h"
#include "utils.h"

namespace OHOS::MediaAVCodec {
using namespace std;

void HEncoderTest::CallBack::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    LOGI(">>");
}

void HEncoderTest::CallBack::OnOutputFormatChanged(const Format &format)
{
    LOGI(">>");
}

void HEncoderTest::CallBack::OnInputBufferAvailable(uint32_t index)
{
    lock_guard<mutex> lk(mTest->mInputMtx);
    mTest->mInputList.push_back(index);
    mTest->mInputCond.notify_all();
}

void HEncoderTest::CallBack::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    lock_guard<mutex> lk(mTest->mOutputMtx);
    mTest->mOutputList.emplace_back(index, info, flag);
    mTest->mOutputCond.notify_one();
}

#define RETURN_ZERO_IF_EOS(expectedSize) \
    do { \
        if (ifs_.gcount() != (expectedSize)) { \
            LOGI("no more data"); \
            return 0; \
        } \
    } while (0)

uint32_t HEncoderTest::ReadOneFrameYUV420P(char* dst)
{
    char* start = dst;
    // copy Y
    for (uint32_t i = 0; i < opt_.dispH; i++) {
        ifs_.read(dst, opt_.dispW);
        RETURN_ZERO_IF_EOS(opt_.dispW);
        dst += stride_;
    }
    // copy U
    for (uint32_t i = 0; i < opt_.dispH / SAMPLE_RATIO; i++) {
        ifs_.read(dst, opt_.dispW / SAMPLE_RATIO);
        RETURN_ZERO_IF_EOS(opt_.dispW / SAMPLE_RATIO);
        dst += stride_ / SAMPLE_RATIO;
    }
    // copy V
    for (uint32_t i = 0; i < opt_.dispH / SAMPLE_RATIO; i++) {
        ifs_.read(dst, opt_.dispW / SAMPLE_RATIO);
        RETURN_ZERO_IF_EOS(opt_.dispW / SAMPLE_RATIO);
        dst += stride_ / SAMPLE_RATIO;
    }
    return dst - start;
}

uint32_t HEncoderTest::ReadOneFrameYUV420SP(char* dst)
{
    char* start = dst;
    // copy Y
    for (uint32_t i = 0; i < opt_.dispH; i++) {
        ifs_.read(dst, opt_.dispW);
        RETURN_ZERO_IF_EOS(opt_.dispW);
        dst += stride_;
    }
    // copy UV
    for (uint32_t i = 0; i < opt_.dispH / SAMPLE_RATIO; i++) {
        ifs_.read(dst, opt_.dispW);
        RETURN_ZERO_IF_EOS(opt_.dispW);
        dst += stride_;
    }
    return dst - start;
}

uint32_t HEncoderTest::ReadOneFrameRGBA(char* dst)
{
    char* start = dst;
    for (uint32_t i = 0; i < opt_.dispH; i++) {
        ifs_.read(dst, opt_.dispW * BYTES_PER_PIXEL_RBGA);
        RETURN_ZERO_IF_EOS(opt_.dispW * BYTES_PER_PIXEL_RBGA);
        dst += stride_;
    }
    return dst - start;
}

uint32_t HEncoderTest::ReadOneFrame(char* dst)
{
    switch (opt_.pixFmt) {
        case YUVI420:
            return ReadOneFrameYUV420P(dst);
        case NV12:
        case NV21:
            return ReadOneFrameYUV420SP(dst);
        case RGBA:
            return ReadOneFrameRGBA(dst);
        default:
            return 0;
    }
}

void HEncoderTest::DealWithInputByteBufferLoop()
{
    Format inputFormat;
    int32_t err = encoder_->GetInputFormat(inputFormat);
    int32_t stride = 0;
    if (err == AVCS_ERR_OK && inputFormat.GetIntValue("stride", stride) && stride >= opt_.dispW) {
        stride_ = stride;
    } else {
        LOGW("GetInputFormat failed");
        stride_ = opt_.dispW;
    }
    LOGI("stride=%{public}d", stride_);
    while (true)  {
        uint32_t inputIdx;
        {
            unique_lock<mutex> lk(mInputMtx);
            if (opt_.timeout == -1) {
                mInputCond.wait(lk, [this] {
                    return !mInputList.empty();
                });
            } else {
                bool ret = mInputCond.wait_for(lk, chrono::milliseconds(opt_.timeout), [this] {
                    return !mInputList.empty();
                });
                if (!ret) {
                    LOGE("Input wait time out");
                    return;
                }
            }

            inputIdx = mInputList.front();
            mInputList.pop_front();
        }

        shared_ptr<AVSharedMemoryBase> frame = encoder_->GetInputBuffer(inputIdx);
        if (frame == nullptr) {
            LOGE("GetInputBuffer return null");
            return;
        }
        char *dstVa = reinterpret_cast<char *>(frame->GetBase());
        int size = frame->GetSize();
        if (dstVa == nullptr || size <= 0) {
            LOGE("invalid va or size");
            return;
        }
        uint32_t filledLen = ReadOneFrame(dstVa);
        AVCodecBufferInfo info;
        AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
        info.size = filledLen;
        info.presentationTimeUs = 0;
        info.offset = 0;
        if (filledLen == 0) {
            flag = AVCODEC_BUFFER_FLAG_EOS;
        }

        curFrameNum++;
        if (opt_.numIdrFrame > 0 && (curFrameNum == opt_.numIdrFrame)) {
            err = encoder_->SignalRequestIDRFrame();
            if (err != AVCS_ERR_OK) {
                LOGE("SignalRequestIDRFrame failed");
                opt_.numIdrFrame = 0;
            }
        }

        err = encoder_->QueueInputBuffer(inputIdx, info, flag);
        if (err != AVCS_ERR_OK) {
            LOGE("QueueInputBuffer failed");
            return;
        }
        
        if (flag & AVCODEC_BUFFER_FLAG_EOS) {
            LOGI("got input eos, quit loop");
            break;
        }
    }
}


void HEncoderTest::DealWithInputSurfaceLoop()
{
    BufferRequestConfig cfg = {opt_.dispW, opt_.dispH, 32, displayFmt_,
                               BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA, 0, };

    while (true)  {
        sptr<SurfaceBuffer> surfaceBuffer;
        int32_t fence;
        GSError err = surface_->RequestBuffer(surfaceBuffer, fence, cfg);
        if (err != GSERROR_OK || surfaceBuffer == nullptr) {
            LOGW("RequestBuffer failed, GSError=%{public}d", err);
            this_thread::sleep_for(30ms);
            continue;
        }
        char* dst = reinterpret_cast<char *>(surfaceBuffer->GetVirAddr());
        int stride = surfaceBuffer->GetStride();
        if (dst == nullptr || stride < opt_.dispW) {
            LOGE("invalid va or stride %{public}d", stride);
            surface_->CancelBuffer(surfaceBuffer);
            continue;
        }
        stride_ = stride;
        bool readSuccFlag = ReadOneFrame(dst);
        if (!readSuccFlag) {
            LOGI("input eos, quit loop");
            encoder_->NotifyEos();
            break;
        }

        curFrameNum++;
        if (opt_.numIdrFrame > 0 && (curFrameNum == opt_.numIdrFrame)) {
            int32_t ret = encoder_->SignalRequestIDRFrame();
            if (ret != AVCS_ERR_OK) {
                LOGE("SignalRequestIDRFrame failed");
                opt_.numIdrFrame = 0;
            }
        }

        BufferFlushConfig flushConfig = {
            .damage = {
                .w = opt_.dispW,
                .h = opt_.dispH,
            },
            .timestamp = 0,
        };
        err = surface_->FlushBuffer(surfaceBuffer, -1, flushConfig);
        if (err != GSERROR_OK) {
            LOGE("FlushBuffer failed");
            break;
        }
    }
}

void HEncoderTest::DealWithOutputLoop()
{
    while (true) {
        uint32_t outIdx;
        AVCodecBufferInfo info;
        AVCodecBufferFlag flag;
        {
            unique_lock<mutex> lk(mOutputMtx);
            if (opt_.timeout == -1) {
                mOutputCond.wait(lk, [this] {
                    return !mOutputList.empty();
                });
            } else {
                bool waitRes = mOutputCond.wait_for(lk, chrono::milliseconds(opt_.timeout), [this] {
                    return !mOutputList.empty();
                });
                if (!waitRes) {
                    LOGE("time out");
                    return;
                }
            }
            std::tie(outIdx, info, flag) = mOutputList.front();
            mOutputList.pop_front();
        }
        if (flag & AVCODEC_BUFFER_FLAG_EOS) {
            LOGI("output eos, quit loop");
            break;
        }
        printf("got one output, size=%d, pts=%" PRId64 "\n", info.size, info.presentationTimeUs);
        shared_ptr<AVSharedMemoryBase> frame = encoder_->GetOutputBuffer(outIdx);
        if (frame == nullptr) {
            LOGE("GetOutputBuffer return null");
            return;
        }
        if (frame->GetBase() == nullptr) {
            LOGE("AVSharedMemory has null va");
            return;
        }
        int32_t ret = encoder_->ReleaseOutputBuffer(outIdx);
        if (ret != AVCS_ERR_OK) {
            LOGE("ReleaseOutputBuffer failed");
            return;
        }
    }
}

void HEncoderTest::Run()
{
    ifs_ = ifstream(opt_.inputFile, ios::binary);
    if (!ifs_) {
        LOGE("Failed to open file %{public}s", opt_.inputFile.c_str());
        return;
    }
    mType = opt_.protocol;
    optional<GraphicPixelFormat> displayFmt = TypeConverter::InnerFmtToDisplayFmt(opt_.pixFmt);
    if (!displayFmt.has_value()) {
        LOGE("invalid color format=%{public}d", opt_.pixFmt);
        return;
    }
    displayFmt_ = displayFmt.value();

    string name = (mType == H264) ? "OMX.hisi.video.encoder.avc" : "OMX.hisi.video.encoder.hevc";
    CreateHCodecByName(name, encoder_);
    if (encoder_ == nullptr) {
        LOGE("create encoder failed");
        return;
    }

    shared_ptr<CallBack> cb = make_shared<CallBack>(this);
    int32_t err = encoder_->SetCallback(cb);
    if (err != AVCS_ERR_OK) {
        LOGE("SetCallback failed");
        return;
    }

    // configure encoder
    Format fmt;
    fmt.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME,
        (mType == H264) ? CodecMimeType::VIDEO_AVC : CodecMimeType::VIDEO_HEVC);
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
    err = encoder_->Configure(fmt);
    if (err != AVCS_ERR_OK) {
        LOGE("Configure failed");
        return;
    }

    if (opt_.bufferType == BufferType::SURFACE) {
        surface_ = encoder_->CreateInputSurface();
        if (surface_ == nullptr) {
            LOGE("CreateInputSurface failed");
            return;
        }
    }

    err = encoder_->Start();
    if (err != AVCS_ERR_OK) {
        LOGE("Start failed");
        return;
    }
    LOGI("start succ");

    thread th(&HEncoderTest::DealWithOutputLoop, this);
    if (opt_.bufferType == BufferType::SURFACE) {
        DealWithInputSurfaceLoop();
    } else {
        DealWithInputByteBufferLoop();
    }
    if (th.joinable()) {
        th.join();
    }
    LOGI("exit");

    err = encoder_->Stop();
    if (err != AVCS_ERR_OK) {
        LOGE("Stop failed");
        return;
    }
    err = encoder_->Release();
    if (err != AVCS_ERR_OK) {
        LOGE("Release failed");
        return;
    }
}


extern "C" {
int main(int argc, char *argv[])
{
    CommandOpt opt = Parse(argc, argv);
    HEncoderTest test(opt);
    test.Run();
    return 0;
}
}
}