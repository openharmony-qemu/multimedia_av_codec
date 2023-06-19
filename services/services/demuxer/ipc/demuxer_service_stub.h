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
#ifndef DEMUXER_SERVICE_STUB_H
#define DEMUXER_SERVICE_STUB_H

#include <map>
#include <string>
#include "i_standard_demuxer_service.h"
#include "demuxer_server.h"
#include "iremote_stub.h"
#include "avcodec_common.h"

namespace OHOS {
namespace MediaAVCodec {
class DemuxerServiceStub : public IRemoteStub<IStandardDemuxerService>, public NoCopyable {
public:
    static sptr<DemuxerServiceStub> Create();
    virtual ~DemuxerServiceStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) override;
    using DemuxerStubFunc = int32_t(DemuxerServiceStub::*)(MessageParcel &data, MessageParcel &reply);

    // 业务
    int32_t Init(uintptr_t sourceAddr) override;
    int32_t SelectTrackByID(uint32_t trackIndex) override;
    int32_t UnselectTrackByID(uint32_t trackIndex) override;
    int32_t ReadSample(uint32_t trackIndex, std::shared_ptr<AVSharedMemory> sample,
        AVCodecBufferInfo &info, AVCodecBufferFlag &flag) override;
    int32_t SeekToTime(int64_t millisecond, const AVSeekMode mode) override;

    int32_t DestroyStub() override;
private:
    DemuxerServiceStub();
    int32_t InitStub();

    int32_t Init(MessageParcel &data, MessageParcel &reply);
    int32_t SelectTrackByID(MessageParcel &data, MessageParcel &reply);
    int32_t UnselectTrackByID(MessageParcel &data, MessageParcel &reply);
    int32_t ReadSample(MessageParcel &data, MessageParcel &reply);
    int32_t SeekToTime(MessageParcel &data, MessageParcel &reply);
    int32_t DestroyStub(MessageParcel &data, MessageParcel &reply);

    std::shared_ptr<IDemuxerService> demuxerServer_ = nullptr;
    std::map<uint32_t, DemuxerStubFunc> demuxerFuncs_;
    std::mutex mutex_;
};
}  // namespace MediaAVCodec
}  // namespace OHOS
#endif