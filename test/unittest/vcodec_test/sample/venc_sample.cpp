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

#include "venc_sample.h"
#include <gtest/gtest.h>

using namespace std;
using namespace OHOS::MediaAVCodec::VCodecTestParam;
namespace OHOS {
namespace MediaAVCodec {
VEncCallbackTest::VEncCallbackTest(std::shared_ptr<VEncSignal> signal) : signal_(signal) {}

VEncCallbackTest::~VEncCallbackTest() {}

void VEncCallbackTest::OnError(int32_t errorCode)
{
    cout << "ADec Error errorCode=" << errorCode;
    if (signal_ == nullptr) {
        return;
    }
    signal_->errorNum_ += 1;
    cout << ", errorNum=" << signal_->errorNum_ << endl;
}

void VEncCallbackTest::OnStreamChanged(std::shared_ptr<FormatMock> format)
{
    (void)format;
    cout << "VEnc Format Changed" << endl;
}

void VEncCallbackTest::OnNeedInputData(uint32_t index, std::shared_ptr<AVMemoryMock> data)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!signal_->isRunning_.load()) {
        return;
    }
    signal_->inIndexQueue_.push(index);
    signal_->inBufferQueue_.push(data);
    signal_->inCond_.notify_all();
}

void VEncCallbackTest::OnNewOutputData(uint32_t index, std::shared_ptr<AVMemoryMock> data, OH_AVCodecBufferAttr attr)
{
    if (signal_ == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal_->outMutex_);
    if (!signal_->isRunning_.load()) {
        return;
    }
    signal_->outIndexQueue_.push(index);
    signal_->outBufferQueue_.push(data);
    signal_->outAttrQueue_.push(attr);
    signal_->outCond_.notify_all();
}

VideoEncSample::VideoEncSample(std::shared_ptr<VEncSignal> signal) : signal_(signal) {}

VideoEncSample::~VideoEncSample()
{
    if (videoEnc_ != nullptr) {
        (void)videoEnc_->Release();
    }
}

bool VideoEncSample::CreateVideoEncMockByMime(const std::string &mime)
{
    videoEnc_ = VCodecMockFactory::CreateVideoEncMockByMime(mime);
    return videoEnc_ != nullptr;
}

bool VideoEncSample::CreateVideoEncMockByName(const std::string &name)
{
    videoEnc_ = VCodecMockFactory::CreateVideoEncMockByName(name);
    return videoEnc_ != nullptr;
}

int32_t VideoEncSample::SetCallback(std::shared_ptr<AVCodecCallbackMock> cb)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->SetCallback(cb);
}

int32_t VideoEncSample::Configure(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->Configure(format);
}

int32_t VideoEncSample::Start()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    int32_t ret = videoEnc_->Start();
    return ret;
}

int32_t VideoEncSample::Stop()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->Stop();
}

int32_t VideoEncSample::Flush()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->Flush();
}

int32_t VideoEncSample::Reset()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->Reset();
}

int32_t VideoEncSample::Release()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->Release();
}

std::shared_ptr<FormatMock> VideoEncSample::GetOutputDescription()
{
    if (videoEnc_ == nullptr) {
        return nullptr;
    }
    return videoEnc_->GetOutputDescription();
}

int32_t VideoEncSample::SetParameter(std::shared_ptr<FormatMock> format)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->SetParameter(format);
}

int32_t VideoEncSample::PushInputData(uint32_t index, OH_AVCodecBufferAttr &attr)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->PushInputData(index, attr);
}

int32_t VideoEncSample::NotifyEos()
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->NotifyEos();
}

int32_t VideoEncSample::FreeOutputData(uint32_t index)
{
    if (videoEnc_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return videoEnc_->FreeOutputData(index);
}

bool VideoEncSample::IsValid()
{
    if (videoEnc_ == nullptr) {
        return false;
    }
    return videoEnc_->IsValid();
}
} // namespace MediaAVCodec
} // namespace OHOS
