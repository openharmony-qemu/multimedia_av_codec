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
#ifndef AVMUXER_IMPL_H
#define AVMUXER_IMPL_H

#include "avmuxer.h"
#include "media_muxer.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class AVMuxerImpl : public AVMuxer, public NoCopyable {
public:
    AVMuxerImpl();
    ~AVMuxerImpl() override;
    int32_t Init(int32_t fd, Plugins::OutputFormat format);
    int32_t SetParameter(const std::shared_ptr<Meta> &param) override;
    int32_t SetUserMeta(const std::shared_ptr<Meta> &userMeta) override;
    int32_t AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc) override;
    sptr<AVBufferQueueProducer> GetInputBufferQueue(uint32_t trackIndex) override;
    int32_t Start() override;
    int32_t WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample) override;
    int32_t Stop() override;

private:
    int32_t StatusConvert(Status status);

private:
    std::shared_ptr<Media::MediaMuxer> muxerEngine_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVMUXER_IMPL_H