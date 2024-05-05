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
#include "native_avsource.h"
#include <unistd.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avsource.h"
#include "common/native_mfmagic.h"
#include "native_avformat.h"
#include "native_avmagic.h"
#include "native_object.h"
#include "avbuffer.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER_MUXER, "NativeAVSource"};
}

using namespace OHOS::MediaAVCodec;

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

struct OH_AVSource *OH_AVSource_CreateWithURI(char *uri)
{
    CHECK_AND_RETURN_RET_LOG(uri != nullptr, nullptr, "Create source with uri failed because input uri is nullptr!");
    
    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithURI(uri);
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "New source with uri failed by AVSourceFactory!");

    struct AVSourceObject *object = new(std::nothrow) AVSourceObject(source);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "New AVSourceObject failed when create source with uri!");
    
    return object;
}

struct OH_AVSource *OH_AVSource_CreateWithFD(int32_t fd, int64_t offset, int64_t size)
{
    CHECK_AND_RETURN_RET_LOG(fd > STDERR_FILENO, nullptr,
        "Create source with fd failed because input fd is illegal, fd must be greater than 2!");
    CHECK_AND_RETURN_RET_LOG(offset >= 0, nullptr,
        "Create source with fd failed because input offset is negative");
    CHECK_AND_RETURN_RET_LOG(size > 0, nullptr,
        "Create source with fd failed because input size must be greater than zero");

    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithFD(fd, offset, size);
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "New source with fd failed by AVSourceFactory!");

    struct AVSourceObject *object = new(std::nothrow) AVSourceObject(source);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "New AVSourceObject failed when create source with fd!");

    return object;
}

struct OH_AVSource *OH_AVSource_CreateWithDataSource(OH_AVDataSource *dataSource)
{
    CHECK_AND_RETURN_RET_LOG(dataSource != nullptr, nullptr,
        "Create source with dataSource failed because input dataSource is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataSource->size != 0, nullptr,
        "Create source with dataSource failed because local file input size must be greater than zero");
    std::shared_ptr<NativeAVDataSource> nativeAVDataSource = std::make_shared<NativeAVDataSource>(dataSource);
    CHECK_AND_RETURN_RET_LOG(nativeAVDataSource != nullptr, nullptr,
        "New nativeAVDataSource with dataSource failed!");

    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithDataSource(nativeAVDataSource);
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "New source with dataSource failed by AVSourceFactory!");

    struct AVSourceObject *object = new(std::nothrow) AVSourceObject(source);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr,
        "New AVSourceObject failed when create source with dataSource!");

    return object;
}

OH_AVErrCode OH_AVSource_Destroy(OH_AVSource *source)
{
    CHECK_AND_RETURN_RET_LOG(source != nullptr, AV_ERR_INVALID_VAL,
        "Destroy source failed because input source is nullptr!");
    CHECK_AND_RETURN_RET_LOG(source->magic_ == AVMagic::AVCODEC_MAGIC_AVSOURCE, AV_ERR_INVALID_VAL, "magic error!");

    delete source;
    return AV_ERR_OK;
}

OH_AVFormat *OH_AVSource_GetSourceFormat(OH_AVSource *source)
{
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "Get source format failed because input source is nullptr!");
    CHECK_AND_RETURN_RET_LOG(source->magic_ == AVMagic::AVCODEC_MAGIC_AVSOURCE, nullptr, "magic error!");

    struct AVSourceObject *sourceObj = reinterpret_cast<AVSourceObject *>(source);
    CHECK_AND_RETURN_RET_LOG(sourceObj->source_ != nullptr, nullptr,
        "New AVSourceObject failed when get source format!");

    Format format;
    int32_t ret = sourceObj->source_->GetSourceFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "source_ GetSourceFormat failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Get source format failed, format is nullptr!");
    avFormat->format_ = format;
    
    return avFormat;
}

OH_AVFormat *OH_AVSource_GetTrackFormat(OH_AVSource *source, uint32_t trackIndex)
{
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "Get format failed because input source is nullptr!");
    CHECK_AND_RETURN_RET_LOG(source->magic_ == AVMagic::AVCODEC_MAGIC_AVSOURCE, nullptr, "magic error!");

    struct AVSourceObject *sourceObj = reinterpret_cast<AVSourceObject *>(source);
    CHECK_AND_RETURN_RET_LOG(sourceObj->source_ != nullptr, nullptr,
        "New AVSourceObject failed when get track format!");
    
    Format format;
    int32_t ret = sourceObj->source_->GetTrackFormat(format, trackIndex);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Source GetTrackFormat failed!");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Get format failed, format is nullptr!");
    avFormat->format_ = format;
    
    return avFormat;
}