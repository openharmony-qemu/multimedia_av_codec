/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <cmath>
#include <iostream>
#include <unistd.h>
#include "string_ex.h"
#include "directory_ex.h"
#include "avcodec_server.h"
#include "i_standard_avcodec_service.h"
#include "avcodecservicestub_fuzzer.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;

namespace OHOS {
namespace MediaAVCodec {
AVCodecServiceStubFuzzer::AVCodecServiceStubFuzzer()
{
}

AVCodecServiceStubFuzzer::~AVCodecServiceStubFuzzer()
{
}

const int32_t SYSTEM_ABILITY_ID = 3011;
const bool RUN_ON_CREATE = false;
bool AVCodecServiceStubFuzzer::FuzzAVCodecOnRemoteRequest(uint8_t *data, size_t size)
{
    if (data == nullptr || size < sizeof(int64_t)) {
        return true;
    }
    std::shared_ptr<AVCodecServer> avcodecServerStub =
        std::make_shared<AVCodecServer>(SYSTEM_ABILITY_ID, RUN_ON_CREATE);
    if (avcodecServerStub == nullptr) {
        return false;
    }

    const int maxIpcNum = 20;
    bool isWirteToken = size > 0 && data[0] % 9 != 0;
    for (uint32_t code = 0; code <= maxIpcNum; code++) {
        MessageParcel msg;
        if (isWirteToken) {
            msg.WriteInterfaceToken(avcodecServerStub->GetDescriptor());
        }
        msg.WriteBuffer(data, size);
        msg.RewindRead(0);
        MessageParcel reply;
        MessageOption option;
        avcodecServerStub->OnRemoteRequest(code, msg, reply, option);
    }

    return true;
}
}

bool FuzzTestAVCodecOnRemoteRequest(uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return true;
    }

    if (size < sizeof(int32_t)) {
        return true;
    }
    AVCodecServiceStubFuzzer testAVCodecServiceStub;
    return testAVCodecServiceStub.FuzzAVCodecOnRemoteRequest(data, size);
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzTestAVCodecOnRemoteRequest(data, size);
    return 0;
}