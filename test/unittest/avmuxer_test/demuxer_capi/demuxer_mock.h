/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#ifndef DEMUXER_MOCK_H
#define DEMUXER_MOCK_H
#include <string>
#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "native_avformat.h"
namespace OHOS {
namespace MediaAVCodec {
class DemuxerMock {
public:
    ~DemuxerMock();
    int32_t Start(std::string fileName);
    int32_t GetVideoFrame(OH_AVBuffer *buffer, OH_AVCodecBufferAttr *info);
private:
    int32_t UpdateTrackInfo();
    int fd_ = 0;
    OH_AVSource *source_ = nullptr;
    OH_AVDemuxer *demuxer_ = nullptr;
    OH_AVFormat *sourceFormat_ = nullptr;
    uint32_t videoTrackId_ = 0xffffffff;
    uint32_t audioTrackId_ = 0xffffffff;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif