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

#ifndef CODEC_SERVICE_STUB_MOCK_H
#define CODEC_SERVICE_STUB_MOCK_H

#include <gmock/gmock.h>
#include <map>
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "ipc/av_codec_service_ipc_interface_code.h"
#include "iremote_broker.h"
#include "iremote_stub.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class IStandardCodecService : public IRemoteBroker {
public:
    virtual ~IStandardCodecService() = default;
    virtual int32_t Init(AVCodecType type, bool isMimeType, const std::string &name) = 0;
    virtual int32_t DestroyStub() = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardCodecService");
};

class CodecServiceStub;
class CodecServiceStubMock {
public:
    CodecServiceStubMock() = default;
    ~CodecServiceStubMock() = default;

    MOCK_METHOD(void, CodecServiceStubDtor, ());
    MOCK_METHOD(CodecServiceStub *, Create, ());
    MOCK_METHOD(int32_t, Init, (AVCodecType type, bool isMimeType, const std::string &name));
    MOCK_METHOD(int32_t, DestroyStub, ());
    MOCK_METHOD(int32_t, DumpInfo, (int32_t fd));
};

class CodecServiceStub : public IRemoteStub<IStandardCodecService>, public NoCopyable {
public:
    static void RegisterMock(std::shared_ptr<CodecServiceStubMock> &mock);

    static sptr<CodecServiceStub> Create();
    CodecServiceStub();
    ~CodecServiceStub();
    int32_t Init(AVCodecType type, bool isMimeType, const std::string &name) override;
    int32_t DestroyStub() override;
    int32_t DumpInfo(int32_t fd);
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_SERVICE_STUB_MOCK_H
