/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AVSOURCE_MOCK_H
#define AVSOURCE_MOCK_H

#include <string>
#include <memory>
#include "nocopyable.h"
#include "avformat_mock.h"
#include "av_common.h"
#include "avcodec_common.h"
#include "avsource.h"
#include "native_avsource.h"

namespace OHOS {
namespace MediaAVCodec {
class AVSourceMock : public NoCopyable {
public:
    virtual ~AVSourceMock() = default;
    virtual int32_t Destroy() =0;
    virtual std::shared_ptr<FormatMock> GetSourceFormat() = 0;
    virtual std::shared_ptr<FormatMock> GetTrackFormat(uint32_t trackIndex) = 0;
};

class __attribute__((visibility("default"))) AVSourceMockFactory {
public:
    static std::shared_ptr<AVSourceMock> CreateSourceWithURI(char *uri);
    static std::shared_ptr<AVSourceMock> CreateSourceWithFD(int32_t fd, int64_t offset, int64_t size);
private:
    AVSourceMockFactory() = delete;
    ~AVSourceMockFactory() = delete;
};
}  // namespace MediaAVCodec
}  // namespace OHOS
#endif // AVSOURCE_MOCK_H