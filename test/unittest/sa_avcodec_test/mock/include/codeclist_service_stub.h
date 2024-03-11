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

#ifndef CODECLIST_SERVICE_STUB_MOCK_H
#define CODECLIST_SERVICE_STUB_MOCK_H

#include <gmock/gmock.h>
#include "avcodec_errors.h"
#include "ipc/av_codec_service_ipc_interface_code.h"
#include "iremote_broker.h"
#include "iremote_stub.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class IStandardCodecListService : public IRemoteBroker {
public:
    virtual ~IStandardCodecListService() = default;

    virtual int32_t DestroyStub() = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardCodecListService");
};

class CodecListServiceStub;
class CodecListServiceStubMock {
public:
    CodecListServiceStubMock() = default;
    ~CodecListServiceStubMock() = default;

    MOCK_METHOD(void, CodecListServiceStubDtor, ());
    MOCK_METHOD(CodecListServiceStub *, Create, ());
    MOCK_METHOD(int32_t, DestroyStub, ());
};

class CodecListServiceStub : public IRemoteStub<IStandardCodecListService>, public NoCopyable {
public:
    static void RegisterMock(std::shared_ptr<CodecListServiceStubMock> &mock);

    static sptr<CodecListServiceStub> Create();
    CodecListServiceStub();
    ~CodecListServiceStub();
    int32_t DestroyStub() override;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECLIST_SERVICE_STUB_MOCK_H
