/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <mutex>
#include "avcodec_log.h"
#include "codecbase.h"
#include "fcodec_loader.h"
#include "hcodec_loader.h"
#define PRINT_HILOG
#include "unittest_log.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "CodecBaseMock"};
std::mutex g_mutex;
std::weak_ptr<OHOS::MediaAVCodec::CodecBaseMock> g_mockObject;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<CodecBase> HCodecLoader::CreateByName(const std::string &name)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("name: %s", name.c_str());
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, nullptr, "mock object is nullptr");
    return mock->CreateHCodecByName(name);
}

std::shared_ptr<CodecBase> FCodecLoader::CreateByName(const std::string &name)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("name: %s", name.c_str());
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, nullptr, "mock object is nullptr");
    return mock->CreateFCodecByName(name);
}

int32_t HCodecLoader::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("HCodec");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    auto item = mock->GetHCapabilityList();
    caps = item.second;
    return item.first;
}

int32_t FCodecLoader::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("FCodec");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    auto item = mock->GetFCapabilityList();
    caps = item.second;
    return item.first;
}

void CodecBase::RegisterMock(std::shared_ptr<CodecBaseMock> &mock)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("CodecBase:0x%" PRIXPTR, FAKE_POINTER(mock.get()));
    g_mockObject = mock;
}

CodecBase::CodecBase()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_LOG(mock != nullptr, "mock object is nullptr");
    mock->CodecBaseCtor();
}

CodecBase::~CodecBase()
{
    UNITTEST_INFO_LOG("");
}

int32_t CodecBase::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("callback:0x%" PRIXPTR, FAKE_POINTER(callback.get()));
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    mock->codecCb_ = callback;
    return mock->SetCallback(std::shared_ptr<AVCodecCallback>(nullptr));
}

int32_t CodecBase::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("callback:0x%" PRIXPTR, FAKE_POINTER(callback.get()));
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    mock->videoCb_ = callback;
    return mock->SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr));
}

int32_t CodecBase::Configure(const Format &format)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("format:%s", format.Stringify().c_str());
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->Configure();
}

int32_t CodecBase::Start()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->Start();
}

int32_t CodecBase::Stop()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->Stop();
}

int32_t CodecBase::Flush()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->Flush();
}

int32_t CodecBase::Reset()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->Reset();
}

int32_t CodecBase::Release()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->Release();
}

int32_t CodecBase::SetParameter(const Format &format)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("format:%s", format.Stringify().c_str());
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->SetParameter();
}

int32_t CodecBase::GetOutputFormat(Format &format)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("format:%s", format.Stringify().c_str());
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->GetOutputFormat();
}

int32_t CodecBase::QueueInputBuffer(uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("index:%u, size:%d, flag:%d", index, info.size, static_cast<int32_t>(flag));
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->QueueInputBuffer(index, info, flag);
}

int32_t CodecBase::QueueInputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("index:%u", index);
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->QueueInputBuffer(index);
}

int32_t CodecBase::QueueInputParameter(uint32_t index)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("index:%u", index);
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->QueueInputParameter(index);
}

int32_t CodecBase::ReleaseOutputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("index:%u", index);
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->ReleaseOutputBuffer(index);
}

int32_t CodecBase::NotifyEos()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->NotifyEos();
}

sptr<Surface> CodecBase::CreateInputSurface()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, nullptr, "mock object is nullptr");
    return mock->CreateInputSurface();
}

int32_t CodecBase::SetInputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("surface:0x%" PRIXPTR, FAKE_POINTER(surface.GetRefPtr()));
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->SetInputSurface(surface);
}

int32_t CodecBase::SetOutputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->SetOutputSurface(surface);
}

int32_t CodecBase::RenderOutputBuffer(uint32_t index)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("index:%u", index);
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->RenderOutputBuffer(index);
}

int32_t CodecBase::SignalRequestIDRFrame()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->SignalRequestIDRFrame();
}

int32_t CodecBase::GetInputFormat(Format &format)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("format:%s", format.Stringify().c_str());
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->GetInputFormat(format);
}

std::string CodecBase::GetHidumperInfo()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, "", "mock object is nullptr");
    return mock->GetHidumperInfo();
}

int32_t CodecBase::Init(Media::Meta &callerInfo)
{
    (void) callerInfo;
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("Init");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->Init();
}

int32_t CodecBase::CreateCodecByName(const std::string &name)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("name: %s", name.c_str());
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->CreateCodecByName(name);
}

int32_t CodecBase::Configure(const std::shared_ptr<Media::Meta> &meta)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("meta:0x%" PRIXPTR, FAKE_POINTER(meta.get()));
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->Configure(meta);
}

int32_t CodecBase::SetParameter(const std::shared_ptr<Media::Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("parameter:0x%" PRIXPTR, FAKE_POINTER(parameter.get()));
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->SetParameter(parameter);
}

int32_t CodecBase::GetOutputFormat(std::shared_ptr<Media::Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("parameter:0x%" PRIXPTR, FAKE_POINTER(parameter.get()));
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->GetOutputFormat(parameter);
}

int32_t CodecBase::SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("bufferQueueProducer:0x%" PRIXPTR, FAKE_POINTER(bufferQueueProducer.GetRefPtr()));
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->SetOutputBufferQueue(bufferQueueProducer);
}

int32_t CodecBase::Prepare()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->Prepare();
}

sptr<Media::AVBufferQueueProducer> CodecBase::GetInputBufferQueue()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, nullptr, "mock object is nullptr");
    return mock->GetInputBufferQueue();
}

void CodecBase::ProcessInputBuffer()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_LOG(mock != nullptr, "mock object is nullptr");
    mock->ProcessInputBuffer();
}

int32_t CodecBase::SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
                                            const bool svpFlag)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->SetAudioDecryptionConfig(keySession, svpFlag);
}
} // namespace MediaAVCodec
} // namespace OHOS
