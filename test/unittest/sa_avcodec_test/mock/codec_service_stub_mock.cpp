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
#include "codec_service_stub.h"
#define PRINT_HILOG
#include "unittest_log.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "CodecServiceStubMock"};
std::mutex g_mutex;
std::weak_ptr<OHOS::MediaAVCodec::CodecServiceStubMock> g_mockObject;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
void CodecServiceStub::RegisterMock(std::shared_ptr<CodecServiceStubMock> &mock)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("CodecServiceStub:0x%" PRIXPTR, FAKE_POINTER(mock.get()));
    g_mockObject = mock;
}

sptr<CodecServiceStub> CodecServiceStub::Create()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, nullptr, "mock object is nullptr");
    return mock->Create();
}

CodecServiceStub::CodecServiceStub()
{
    UNITTEST_INFO_LOG("");
}

CodecServiceStub::~CodecServiceStub()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_LOG(mock != nullptr, "mock object is nullptr");
    mock->CodecServiceStubDtor();
}

int32_t CodecServiceStub::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("type:%d, isMimeType:%d, name:%s", static_cast<int32_t>(type), static_cast<int32_t>(isMimeType),
                      name.c_str());
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->Init(type, isMimeType, name);
}

int32_t CodecServiceStub::DestroyStub()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->DestroyStub();
}

int32_t CodecServiceStub::DumpInfo(int32_t fd)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("fd:%d", fd);
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->DumpInfo(fd);
}

} // namespace MediaAVCodec
} // namespace OHOS
