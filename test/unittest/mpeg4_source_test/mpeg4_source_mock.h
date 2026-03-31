/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef MPEG4_SOURCE_MOCK_H
#define MPEG4_SOURCE_MOCK_H

#include <string>
#include <memory>
#include "nocopyable.h"
#include "avformat_mock.h"
#include "av_common.h"
#include "avcodec_common.h"
#include "avsource.h"
#include "native_avsource.h"
#include "common/native_mfmagic.h"

namespace OHOS {
namespace MediaAVCodec {
class Mpeg4SourceMock : public NoCopyable {
public:
    virtual ~Mpeg4SourceMock() = default;
    virtual int32_t Destroy() =0;
    virtual std::shared_ptr<FormatMock> GetSourceFormat() = 0;
    virtual std::shared_ptr<FormatMock> GetTrackFormat(uint32_t trackIndex) = 0;
    virtual std::shared_ptr<FormatMock> GetUserData() = 0;
};

class __attribute__((visibility("default"))) Mpeg4SourceMockFactory {
public:
    static std::shared_ptr<Mpeg4SourceMock> CreateSourceWithURI(char *uri);
    static std::shared_ptr<Mpeg4SourceMock> CreateSourceWithFD(int32_t fd, int64_t offset, int64_t size);
    static std::shared_ptr<Mpeg4SourceMock> CreateWithDataSource(
        const std::shared_ptr<Media::IMediaDataSource> &dataSource);
    static std::shared_ptr<Mpeg4SourceMock> CreateWithDataSource(OH_AVDataSource *dataSource);
private:
    Mpeg4SourceMockFactory() = delete;
    ~Mpeg4SourceMockFactory() = delete;
};

class NativeAVDataSource : public OHOS::Media::IMediaDataSource {
public:
    explicit NativeAVDataSource(OH_AVDataSource *dataSource)
        : dataSource_(dataSource)
    {
    }
    virtual ~NativeAVDataSource() = default;

    int32_t ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos = -1)
    {
        std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(
            mem->GetBase(), mem->GetSize(), mem->GetSize()
        );
        OH_AVBuffer* avBuffer = new OH_AVBuffer(buffer);
        return dataSource_->readAt(avBuffer, length, pos);
    }

    int32_t GetSize(int64_t &size)
    {
        size = dataSource_->size;
        return 0;
    }

    int32_t ReadAt(int64_t pos, uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
    {
        return ReadAt(mem, length, pos);
    }

    int32_t ReadAt(uint32_t length, const std::shared_ptr<AVSharedMemory> &mem)
    {
        return ReadAt(mem, length);
    }

private:
    OH_AVDataSource* dataSource_;
};

}  // namespace MediaAVCodec
}  // namespace OHOS
#endif // MPEG4_SOURCE_MOCK_H