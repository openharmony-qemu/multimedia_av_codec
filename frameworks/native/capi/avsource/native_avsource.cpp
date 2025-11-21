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
#include "network_security_config.h"
#include "common/log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "NativeAVSource"};
}

using namespace OHOS::MediaAVCodec;

class NativeAVDataSource : public OHOS::Media::IMediaDataSource {
public:
    explicit NativeAVDataSource(OH_AVDataSource *dataSource)
        : isExt_(false), dataSource_(dataSource), dataSourceExt_(nullptr), userData_(nullptr)
    {
    }

    NativeAVDataSource(OH_AVDataSourceExt* dataSourceExt, void* userData)
        : isExt_(true), dataSource_(nullptr), dataSourceExt_(dataSourceExt), userData_(userData)
    {
    }

    virtual ~NativeAVDataSource() = default;

    int32_t ReadAt(const std::shared_ptr<AVSharedMemory> &mem, uint32_t length, int64_t pos = -1)
    {
        std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(
            mem->GetBase(), mem->GetSize(), mem->GetSize()
        );
        OH_AVBuffer avBuffer(buffer);

        if (isExt_) {
            return dataSourceExt_->readAt(&avBuffer, length, pos, userData_);
        } else {
            return dataSource_->readAt(&avBuffer, length, pos);
        }
    }

    int32_t GetSize(int64_t &size)
    {
        if (isExt_) {
            size = dataSourceExt_->size;
        } else {
            size = dataSource_->size;
        }
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
    bool isExt_ = false;
    OH_AVDataSource* dataSource_;
    OH_AVDataSourceExt* dataSourceExt_ = nullptr;
    void* userData_ = nullptr;
};

std::string static GetProtocolFromURL(const std::string &url)
{
    std::string delimiter = "://";
    size_t pos = url.find(delimiter);
    if (pos != std::string::npos) {
        return url.substr(0, pos);
    }
    return "";
}

std::string static GetHostnameFromURL(const std::string &url)
{
    if (url.empty()) {
        return "";
    }
    std::string delimiter = "://";
    std::string tempUrl = url;
    std::replace(tempUrl.begin(), tempUrl.end(), '\\', '/');
    size_t posStart = tempUrl.find(delimiter);
    if (posStart != std::string::npos) {
        posStart += delimiter.length();
    } else {
        posStart = 0;
    }
    size_t notSlash = tempUrl.find_first_not_of('/', posStart);
    if (notSlash != std::string::npos) {
        posStart = notSlash;
    }
    size_t posEnd = std::min({ tempUrl.find(':', posStart),
                            tempUrl.find('/', posStart), tempUrl.find('?', posStart)});
    if (posEnd != std::string::npos) {
        return tempUrl.substr(posStart, posEnd - posStart);
    }
    return tempUrl.substr(posStart);
}

struct OH_AVSource *OH_AVSource_CreateWithURI(char *uri)
{
    CHECK_AND_RETURN_RET_LOG(uri != nullptr, nullptr, "Uri is nullptr");
    bool isComponentCfg = false;
    std::string protocol = GetProtocolFromURL(uri);
    int32_t ret = OHOS::NetManagerStandard::NetworkSecurityConfig::
        GetInstance().IsCleartextCfgByComponent("Media kit", isComponentCfg);
    MEDIA_LOG_D("Media kit, ret: %{public}d, isComponentCfg: %{public}d, protocol: %{public}s",
                ret, isComponentCfg, protocol.c_str());
    if (isComponentCfg && protocol == "http") {
        bool isCleartextPermitted = true;
        std::string hostName = GetHostnameFromURL(uri);
        OHOS::NetManagerStandard::NetworkSecurityConfig::
            GetInstance().IsCleartextPermitted(hostName, isCleartextPermitted);
        if (!isCleartextPermitted) {
            return nullptr;
        }
    }
    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithURI(uri);
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "New avsource failed");

    struct AVSourceObject *object = new(std::nothrow) AVSourceObject(source);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "New sourceObject failed");
    
    return object;
}

struct OH_AVSource *OH_AVSource_CreateWithFD(int32_t fd, int64_t offset, int64_t size)
{
    CHECK_AND_RETURN_RET_LOG(fd >= 0, nullptr, "Fd is negative");
    CHECK_AND_RETURN_RET_LOG(offset >= 0, nullptr, "Offset is negative");
    CHECK_AND_RETURN_RET_LOG(size > 0, nullptr, "Size must be greater than zero");

    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithFD(fd, offset, size);
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "New avsource failed");
    struct AVSourceObject *object = new(std::nothrow) AVSourceObject(source);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "New sourceObject failed");
    return object;
}

struct OH_AVSource *OH_AVSource_CreateWithDataSource(OH_AVDataSource *dataSource)
{
    CHECK_AND_RETURN_RET_LOG(dataSource != nullptr, nullptr, "Input dataSource is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataSource->size != 0, nullptr, "Datasource size must be greater than zero");
    CHECK_AND_RETURN_RET_LOG(dataSource->readAt != nullptr, nullptr, "Datasource readAt is nullptr");
    std::shared_ptr<NativeAVDataSource> nativeAVDataSource = std::make_shared<NativeAVDataSource>(dataSource);
    CHECK_AND_RETURN_RET_LOG(nativeAVDataSource != nullptr, nullptr, "New nativeAVDataSource failed");

    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithDataSource(nativeAVDataSource);
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "New avsource failed");

    struct AVSourceObject *object = new(std::nothrow) AVSourceObject(source);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "New sourceObject failed");

    return object;
}

OH_AVErrCode OH_AVSource_Destroy(OH_AVSource *source)
{
    CHECK_AND_RETURN_RET_LOG(source != nullptr, AV_ERR_INVALID_VAL, "Input source is nullptr");
    CHECK_AND_RETURN_RET_LOG(source->magic_ == AVMagic::AVCODEC_MAGIC_AVSOURCE, AV_ERR_INVALID_VAL, "Magic error");

    delete source;
    return AV_ERR_OK;
}

OH_AVFormat *OH_AVSource_GetSourceFormat(OH_AVSource *source)
{
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "Input source is nullptr");
    CHECK_AND_RETURN_RET_LOG(source->magic_ == AVMagic::AVCODEC_MAGIC_AVSOURCE, nullptr, "Magic error");

    struct AVSourceObject *sourceObj = reinterpret_cast<AVSourceObject *>(source);
    CHECK_AND_RETURN_RET_LOG(sourceObj != nullptr, nullptr, "Get sourceObject failed");
    CHECK_AND_RETURN_RET_LOG(sourceObj->source_ != nullptr, nullptr, "Get sourceObject failed");

    Format format;
    int32_t ret = sourceObj->source_->GetSourceFormat(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Source GetSourceFormat failed");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Format is nullptr");
    avFormat->format_ = format;
    
    return avFormat;
}

OH_AVFormat *OH_AVSource_GetTrackFormat(OH_AVSource *source, uint32_t trackIndex)
{
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "Input source is nullptr");
    CHECK_AND_RETURN_RET_LOG(source->magic_ == AVMagic::AVCODEC_MAGIC_AVSOURCE, nullptr, "Magic error");

    struct AVSourceObject *sourceObj = reinterpret_cast<AVSourceObject *>(source);
    CHECK_AND_RETURN_RET_LOG(sourceObj != nullptr, nullptr, "Get sourceObject failed");
    CHECK_AND_RETURN_RET_LOG(sourceObj->source_ != nullptr, nullptr, "Get sourceObject failed");
    
    Format format;
    int32_t ret = sourceObj->source_->GetTrackFormat(format, trackIndex);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Source GetTrackFormat failed");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Format is nullptr");
    avFormat->format_ = format;
    
    return avFormat;
}

OH_AVFormat *OH_AVSource_GetCustomMetadataFormat(OH_AVSource *source)
{
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "Input source is nullptr");
    CHECK_AND_RETURN_RET_LOG(source->magic_ == AVMagic::AVCODEC_MAGIC_AVSOURCE, nullptr, "Magic error");

    struct AVSourceObject *sourceObj = reinterpret_cast<AVSourceObject *>(source);
    CHECK_AND_RETURN_RET_LOG(sourceObj != nullptr, nullptr, "Get sourceObject failed");
    CHECK_AND_RETURN_RET_LOG(sourceObj->source_ != nullptr, nullptr, "Get sourceObject failed");

    Format format;
    int32_t ret = sourceObj->source_->GetUserMeta(format);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Source GetMetaData failed");

    OH_AVFormat *avFormat = OH_AVFormat_Create();
    CHECK_AND_RETURN_RET_LOG(avFormat != nullptr, nullptr, "Format is nullptr");
    avFormat->format_ = format;
    
    return avFormat;
}

struct OH_AVSource *OH_AVSource_CreateWithDataSourceExt(OH_AVDataSourceExt *dataSource, void *userData)
{
    CHECK_AND_RETURN_RET_LOG(dataSource != nullptr, nullptr, "Input dataSourceExt is nullptr");
    CHECK_AND_RETURN_RET_LOG(dataSource->size > 0, nullptr, "DatasourceExt size must be greater than zero");
    CHECK_AND_RETURN_RET_LOG(dataSource->readAt != nullptr, nullptr, "DatasourceExt readAt is nullptr");

    std::shared_ptr<NativeAVDataSource> nativeAVDataSource = std::make_shared<NativeAVDataSource>(dataSource, userData);
    CHECK_AND_RETURN_RET_LOG(nativeAVDataSource != nullptr, nullptr, "New nativeAVDataSourceExt failed");

    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithDataSource(nativeAVDataSource);
    CHECK_AND_RETURN_RET_LOG(source != nullptr, nullptr, "New avsource failed");

    struct AVSourceObject *object = new(std::nothrow) AVSourceObject(source);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, nullptr, "New sourceObject failed");

    return object;
}
