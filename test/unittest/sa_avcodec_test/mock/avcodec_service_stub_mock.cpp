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
#include <mutex>
#include "avcodec_log.h"
#include "avcodec_service_stub.h"
#define PRINT_HILOG
#include "unittest_log.h"
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "AVCodecServiceStubMock"};
std::mutex g_mutex;
std::weak_ptr<OHOS::MediaAVCodec::AVCodecServiceStubMock> g_mockObject;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
void AVCodecServiceStub::RegisterMock(std::shared_ptr<AVCodecServiceStubMock> &mock)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("AVCodecServiceStub:0x%" PRIXPTR, FAKE_POINTER(mock.get()));
    g_mockObject = mock;
}

AVCodecServiceStub::AVCodecServiceStub()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_LOG(mock != nullptr, "mock object is nullptr");
    mock->AVCodecServiceStubCtor();
}

AVCodecServiceStub::~AVCodecServiceStub()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("");
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_LOG(mock != nullptr, "mock object is nullptr");
    mock->AVCodecServiceStubDtor();
}

int32_t AVCodecServiceStub::GetSubSystemAbility(IStandardAVCodecService::AVCodecSystemAbility subSystemId,
                                                const sptr<IRemoteObject> &listener, sptr<IRemoteObject> &stubObject)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("subSystemId:%d, listener refCount:%d", static_cast<int32_t>(subSystemId),
                      listener->GetSptrRefCount());
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->GetSubSystemAbility(subSystemId, listener, stubObject);
}

int32_t AVCodecServiceStub::SetDeathListener(const sptr<IRemoteObject> &object)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    UNITTEST_INFO_LOG("object refCount:%d", object->GetSptrRefCount());
    auto mock = g_mockObject.lock();
    UNITTEST_CHECK_AND_RETURN_RET_LOG(mock != nullptr, AVCS_ERR_UNKNOWN, "mock object is nullptr");
    return mock->SetDeathListener(object);
}
} // namespace MediaAVCodec
} // namespace OHOS
