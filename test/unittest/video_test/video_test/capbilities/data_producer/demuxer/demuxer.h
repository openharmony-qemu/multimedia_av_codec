/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_SAMPLE_DATA_PRODUCER_DEMUXER_H
#define AVCODEC_SAMPLE_DATA_PRODUCER_DEMUXER_H

#include <cstdio>
#include "data_producer_base.h"
#include "native_avdemuxer.h"
#include "native_avsource.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class Demuxer : public DataProducerBase {
public:
    int32_t Init(const std::shared_ptr<SampleInfo> &info) override;
    int32_t Seek(int64_t position)override;

private:
    int32_t FillBuffer(CodecBufferInfo &bufferInfo) override;
    int64_t GetFileSize(int32_t fd);
    int32_t GetVideoTrackInfo(std::shared_ptr<OH_AVFormat> sourceFormat);
    bool IsEOS() override;

    std::shared_ptr<FILE> file_ = nullptr;
    std::shared_ptr<OH_AVSource> source_ = nullptr;
    std::shared_ptr<OH_AVDemuxer> demuxer_ = nullptr;
    int32_t videoTrackId_ = -1;
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_DATA_PRODUCER_DEMUXER_H