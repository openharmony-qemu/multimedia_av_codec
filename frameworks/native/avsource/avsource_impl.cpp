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
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include "i_avcodec_service.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_trace.h"
#include "avsource_impl.h"
#include "common/media_source.h"
#include "common/status.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER_MUXER, "AVSourceImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
using namespace Media::Plugins;
std::shared_ptr<AVSource> AVSourceFactory::CreateWithURI(const std::string &uri)
{
    AVCODEC_SYNC_TRACE;

    AVCODEC_LOGD("create source with uri: uri=%{private}s", uri.c_str());

    std::shared_ptr<AVSourceImpl> sourceImpl = std::make_shared<AVSourceImpl>();
    CHECK_AND_RETURN_RET_LOG(sourceImpl != nullptr, nullptr, "New AVSourceImpl failed");

    int32_t ret = sourceImpl->InitWithURI(uri);

    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init AVSourceImpl failed");

    return sourceImpl;
}

std::shared_ptr<AVSource> AVSourceFactory::CreateWithFD(int32_t fd, int64_t offset, int64_t size)
{
    AVCODEC_SYNC_TRACE;

    AVCODEC_LOGD("create source with fd: fd=%{private}d, offset=%{public}" PRId64 ", size=%{public}" PRId64,
        fd, offset, size);

    std::shared_ptr<AVSourceImpl> sourceImpl = std::make_shared<AVSourceImpl>();
    CHECK_AND_RETURN_RET_LOG(sourceImpl != nullptr, nullptr, "New AVSourceImpl failed");

    int32_t ret = sourceImpl->InitWithFD(fd, offset, size);

    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init AVSourceImpl failed");

    return sourceImpl;
}

std::shared_ptr<AVSource> AVSourceFactory::CreateWithDataSource(
    const std::shared_ptr<Media::IMediaDataSource> &dataSource)
{
    AVCODEC_SYNC_TRACE;

    std::shared_ptr<AVSourceImpl> sourceImpl = std::make_shared<AVSourceImpl>();
    CHECK_AND_RETURN_RET_LOG(sourceImpl != nullptr, nullptr, "New AVSourceImpl failed");

    int32_t ret = sourceImpl->InitWithDataSource(dataSource);

    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init AVSourceImpl failed");

    return sourceImpl;
}

int32_t AVSourceImpl::InitWithURI(const std::string &uri)
{
    AVCODEC_SYNC_TRACE;

    CHECK_AND_RETURN_RET_LOG(demuxerEngine == nullptr, AVCS_ERR_INVALID_OPERATION,
        "Create source failed due to has been used by demuxer.");
    demuxerEngine = std::make_shared<MediaDemuxer>();
    demuxerEngine->SetPlayerId("AVSource_URI");
    CHECK_AND_RETURN_RET_LOG(demuxerEngine != nullptr, AVCS_ERR_INVALID_OPERATION,
        "Init AVSource with uri failed due to create demuxer engine failed.");

    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(uri);
    Status ret = demuxerEngine->SetDataSource(mediaSource);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, StatusToAVCodecServiceErrCode(ret),
        "Init AVSource with uri failed due to set data source for demuxer engine failed.");

    sourceUri = uri;
    return AVCS_ERR_OK;
}

int32_t AVSourceImpl::InitWithFD(int32_t fd, int64_t offset, int64_t size)
{
    AVCODEC_SYNC_TRACE;

    CHECK_AND_RETURN_RET_LOG(demuxerEngine == nullptr, AVCS_ERR_INVALID_OPERATION,
        "Create source failed due to has been used by demuxer.");
    CHECK_AND_RETURN_RET_LOG(fd > STDERR_FILENO, AVCS_ERR_INVALID_VAL,
        "Create source with uri failed because input fd is illegal, fd must be greater than 2!");
    CHECK_AND_RETURN_RET_LOG(offset >= 0, AVCS_ERR_INVALID_VAL,
        "Create source with fd failed because input offset is negative");
    CHECK_AND_RETURN_RET_LOG(size > 0, AVCS_ERR_INVALID_VAL,
        "Create source with fd failed because input size must be greater than zero");
    int32_t flag = fcntl(fd, F_GETFL, 0);
    CHECK_AND_RETURN_RET_LOG(flag >= 0, AVCS_ERR_INVALID_VAL, "get fd status failed");
    CHECK_AND_RETURN_RET_LOG(
        (static_cast<uint32_t>(flag) & static_cast<uint32_t>(O_WRONLY)) != static_cast<uint32_t>(O_WRONLY),
        AVCS_ERR_INVALID_VAL, "Fd not be permitted to read ");
    CHECK_AND_RETURN_RET_LOG(lseek(fd, 0, SEEK_CUR) != -1, AVCS_ERR_INVALID_VAL, "Fd is not seekable");
    
    std::string uri = "fd://" + std::to_string(fd) + "?offset=" + \
        std::to_string(offset) + "&size=" + std::to_string(size);

    return InitWithURI(uri);
}

int32_t AVSourceImpl::InitWithDataSource(const std::shared_ptr<Media::IMediaDataSource> &dataSource)
{
    AVCODEC_SYNC_TRACE;

    CHECK_AND_RETURN_RET_LOG(demuxerEngine == nullptr, AVCS_ERR_INVALID_OPERATION,
        "Create source failed due to has been used by demuxer.");
    demuxerEngine = std::make_shared<MediaDemuxer>();
    demuxerEngine->SetPlayerId("AVSource_Data");
    CHECK_AND_RETURN_RET_LOG(demuxerEngine != nullptr, AVCS_ERR_INVALID_OPERATION,
        "Init AVSource with dataSource failed due to create demuxer engine failed.");

    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(dataSource);
    Status ret = demuxerEngine->SetDataSource(mediaSource);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, StatusToAVCodecServiceErrCode(ret),
        "Init AVSource with dataSource failed due to set data source for demuxer engine failed.");

    return AVCS_ERR_OK;
}

AVSourceImpl::AVSourceImpl()
{
    AVCODEC_LOGD("AVSourceImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVSourceImpl::~AVSourceImpl()
{
    AVCODEC_LOGD("Destroy AVSourceImpl for source %{private}s", sourceUri.c_str());
    if (demuxerEngine != nullptr) {
        demuxerEngine = nullptr;
    }
    AVCODEC_LOGD("AVSourceImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVSourceImpl::GetSourceFormat(OHOS::Media::Format &format)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("get source format.");

    CHECK_AND_RETURN_RET_LOG(demuxerEngine != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist.");
    
    std::shared_ptr<OHOS::Media::Meta> mediaInfo = demuxerEngine->GetGlobalMetaInfo();
    CHECK_AND_RETURN_RET_LOG(mediaInfo != nullptr, AVCS_ERR_INVALID_OPERATION,
        "Get source format failed due to parse media info failed.");

    bool set = format.SetMeta(mediaInfo);
    CHECK_AND_RETURN_RET_LOG(set, AVCS_ERR_INVALID_OPERATION, "Get source format failed due to convert meta failed.");

    return AVCS_ERR_OK;
}

int32_t AVSourceImpl::GetTrackFormat(OHOS::Media::Format &format, uint32_t trackIndex)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("get track format: trackIndex=%{public}u", trackIndex);

    CHECK_AND_RETURN_RET_LOG(demuxerEngine != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist.");

    std::vector<std::shared_ptr<Meta>> streamsInfo = demuxerEngine->GetStreamMetaInfo();
    CHECK_AND_RETURN_RET_LOG(trackIndex < streamsInfo.size(), AVCS_ERR_INVALID_VAL,
        "Just have %{public}zu tracks. index is out of range.", streamsInfo.size());

    bool set = format.SetMeta(streamsInfo[trackIndex]);
    CHECK_AND_RETURN_RET_LOG(set, AVCS_ERR_INVALID_OPERATION, "Get track format failed due to convert meta failed.");

    return AVCS_ERR_OK;
}

int32_t AVSourceImpl::GetUserMeta(OHOS::Media::Format &format)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("get user meta.");

    CHECK_AND_RETURN_RET_LOG(demuxerEngine != nullptr, AVCS_ERR_INVALID_OPERATION, "Demuxer engine does not exist.");
    
    std::shared_ptr<OHOS::Media::Meta> userDataMeta = demuxerEngine->GetUserMeta();
    CHECK_AND_RETURN_RET_LOG(userDataMeta != nullptr, AVCS_ERR_INVALID_OPERATION,
        "Get user meta failed due to parse media info failed.");

    bool set = format.SetMeta(userDataMeta);
    CHECK_AND_RETURN_RET_LOG(set, AVCS_ERR_INVALID_OPERATION, "Get user meta failed due to convert meta failed.");

    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS