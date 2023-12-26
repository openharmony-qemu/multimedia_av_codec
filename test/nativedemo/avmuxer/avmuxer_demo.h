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

#ifndef AVMUXER_DEMO_H
#define AVMUXER_DEMO_H

#include "avmuxer_demo_base.h"

namespace OHOS {
namespace MediaAVCodec {
class AVMuxerDemo : public AVMuxerDemoBase {
public:
    AVMuxerDemo() = default;
    ~AVMuxerDemo() override = default;
private:
    void DoRunMuxer() override;
    int DoWriteSample(uint32_t trackIndex, std::shared_ptr<AVBuffer> sample) override;
    int DoAddTrack(int32_t &trackIndex, std::shared_ptr<Meta> trackDesc) override;
    sptr<AVBufferQueueProducer> DoGetInputBufferQueue(uint32_t trackIndex) override;

    void DoRunMultiThreadCase() override;
    void DoRunMuxer(const std::string &runMode);
    void SetParameter();
    std::shared_ptr<AVMuxer> avmuxer_;
};
}  // namespace MediaAVCodec
}  // namespace OHOS
#endif  // AVMUXER_DEMO_H