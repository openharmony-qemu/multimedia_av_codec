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
#ifndef AVCODEC_SERVICE_STUB_H
#define AVCODEC_SERVICE_STUB_H

#include <gmock/gmock.h>
#include <map>
#include "avcodec_errors.h"
#include "ipc/av_codec_service_ipc_interface_code.h"
#include "iremote_broker.h"
#include "iremote_stub.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class IStandardAVCodecService : public IRemoteBroker {
public:
    /**
     * sub system ability ID
     */
    enum AVCodecSystemAbility : int32_t {
        AVCODEC_CODECLIST,
        AVCODEC_CODEC,
    };

    virtual int32_t GetSubSystemAbility(IStandardAVCodecService::AVCodecSystemAbility subSystemId,
                                        const sptr<IRemoteObject> &listener, sptr<IRemoteObject> &stubObject) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAVCodecServiceInterface");
};

class AVCodecServiceStubMock {
public:
    AVCodecServiceStubMock() = default;
    ~AVCodecServiceStubMock() = default;

    MOCK_METHOD(void, AVCodecServiceStubCtor, ());
    MOCK_METHOD(void, AVCodecServiceStubDtor, ());
    MOCK_METHOD(int32_t, GetSubSystemAbility,
                (IStandardAVCodecService::AVCodecSystemAbility subSystemId, const sptr<IRemoteObject> &listener,
                 sptr<IRemoteObject> &stubObject));
    MOCK_METHOD(int32_t, SetDeathListener, (const sptr<IRemoteObject> &object));
};

class AVCodecServiceStub : public IRemoteStub<IStandardAVCodecService>, public NoCopyable {
public:
    static void RegisterMock(std::shared_ptr<AVCodecServiceStubMock> &mock);

    AVCodecServiceStub();
    ~AVCodecServiceStub();
    int32_t GetSubSystemAbility(IStandardAVCodecService::AVCodecSystemAbility subSystemId,
                                const sptr<IRemoteObject> &listener, sptr<IRemoteObject> &stubObject) override;
    int32_t SetDeathListener(const sptr<IRemoteObject> &object);
};

class IStandardAVCodecListener : public IRemoteBroker {
public:
    virtual ~IStandardAVCodecListener() = default;
    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAVCodecListenerInterface");
};

class AVCodecListenerStub : public IRemoteStub<IStandardAVCodecListener>, public NoCopyable {
public:
    AVCodecListenerStub() = default;
    ~AVCodecListenerStub() = default;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_SERVICE_STUB_H
