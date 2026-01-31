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
#ifndef AVCODEC_CLIENT_H
#define AVCODEC_CLIENT_H

#include "avcodec_death_recipient.h"
#include "avcodec_listener_stub.h"
#include "i_avcodec_service.h"
#include "i_standard_avcodec_service.h"

#ifdef SUPPORT_CODEC
#include "codec_client.h"
#endif

#ifdef SUPPORT_CODECLIST
#include "codeclist_client.h"
#endif

#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class AVCodecClient : public IAVCodecService, public NoCopyable {
public:
    AVCodecClient() noexcept;
    ~AVCodecClient();
    int32_t SuspendFreeze(const std::vector<pid_t> &pidList) override;
    int32_t SuspendActive(const std::vector<pid_t> &pidList) override;
    int32_t SuspendActiveAll() override;
    int32_t GetActiveSecureDecoderPids(std::vector<pid_t> &pidList) override;

#ifdef SUPPORT_CODEC
    int32_t CreateCodecService(std::shared_ptr<ICodecService> &codecClient) override;
    int32_t DestroyCodecService(std::shared_ptr<ICodecService> codecClient) override;
#endif

#ifdef SUPPORT_CODECLIST
    std::shared_ptr<ICodecListService> CreateCodecListService() override;
    int32_t DestroyCodecListService(std::shared_ptr<ICodecListService> codecListClient) override;
    sptr<IStandardCodecListService> GetCodecListServiceProxy() override;
#endif

private:
    int32_t CreateInstanceAndTryInTimes(IStandardAVCodecService::AVCodecSystemAbility subSystemId,
                                        sptr<IRemoteObject> &object, uint32_t tryTimes = 3);
    sptr<IStandardAVCodecService> GetAVCodecProxy();
    sptr<IStandardAVCodecService> GetTemporaryAVCodecProxy();
    bool IsAlived();
    static void AVCodecServerDied(pid_t pid);
    void DoAVCodecServerDied();
    void CancelTimer();
    void TryReleaseResources();
    void ScheduleReleaseResources();

    sptr<IStandardAVCodecService> avCodecProxy_ = nullptr;
    sptr<AVCodecListenerStub> listenerStub_ = nullptr;
    sptr<AVCodecDeathRecipient> deathRecipient_ = nullptr;

#ifdef SUPPORT_CODEC
    std::list<std::shared_ptr<ICodecService>> codecClientList_;
#endif
#ifdef SUPPORT_CODECLIST
    std::list<std::shared_ptr<ICodecListService>> codecListClientList_;
#endif
#ifdef SUPPORT_START_STOP_ON_DEMAND
    static void ReleaseTimerCallback(void *data);
    int32_t releaseTimerId_ = 0;
    static constexpr uint32_t RELEASE_DELAY_SECONDS = 180; // Delay before releasing resources (3 minutes)
#endif
    std::mutex mutex_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_CLIENT_H
