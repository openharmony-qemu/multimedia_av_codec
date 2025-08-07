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
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_DEMUXER, "AVSourceImpl"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
using namespace Media::Plugins;
std::shared_ptr<AVSource> AVSourceFactory::CreateWithURI(const std::string &uri)
{
    AVCODEC_SYNC_TRACE;

    AVCODEC_LOGD("Create source with uri %{private}s", uri.c_str());

    std::shared_ptr<AVSourceImpl> sourceImpl = std::make_shared<AVSourceImpl>();
    CHECK_AND_RETURN_RET_LOG(sourceImpl != nullptr, nullptr, "New avsource failed");

    int32_t ret = sourceImpl->InitWithURI(uri);

    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init avsource failed");

    return sourceImpl;
}

std::shared_ptr<AVSource> AVSourceFactory::CreateWithFD(int32_t fd, int64_t offset, int64_t size)
{
    AVCODEC_SYNC_TRACE;

    AVCODEC_LOGD("Create source with fd %{private}d, offset=%{public}" PRId64 ", size=%{public}" PRId64,
        fd, offset, size);

    std::shared_ptr<AVSourceImpl> sourceImpl = std::make_shared<AVSourceImpl>();
    CHECK_AND_RETURN_RET_LOG(sourceImpl != nullptr, nullptr, "New avsource failed");

    int32_t ret = sourceImpl->InitWithFD(fd, offset, size);

    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init avsource failed");

    return sourceImpl;
}

std::shared_ptr<AVSource> AVSourceFactory::CreateWithDataSource(
    const std::shared_ptr<Media::IMediaDataSource> &dataSource)
{
    AVCODEC_SYNC_TRACE;

    std::shared_ptr<AVSourceImpl> sourceImpl = std::make_shared<AVSourceImpl>();
    CHECK_AND_RETURN_RET_LOG(sourceImpl != nullptr, nullptr, "New avsource failed");

    int32_t ret = sourceImpl->InitWithDataSource(dataSource);

    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init avsource failed");

    return sourceImpl;
}

int32_t AVSourceImpl::InitWithURI(const std::string &uri)
{
    AVCODEC_SYNC_TRACE;

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer == nullptr, AVCS_ERR_INVALID_OPERATION, "Has been used by demuxer");
    mediaDemuxer = std::make_shared<MediaDemuxer>();
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer != nullptr, AVCS_ERR_INVALID_OPERATION, "Create mediaDemuxer failed");

    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(uri);
    Status ret = mediaDemuxer->SetDataSource(mediaSource);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, StatusToAVCodecServiceErrCode(ret), "Set data source failed");

    sourceUri = uri;
    return AVCS_ERR_OK;
}

int32_t AVSourceImpl::InitWithFD(int32_t fd, int64_t offset, int64_t size)
{
    AVCODEC_SYNC_TRACE;

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer == nullptr, AVCS_ERR_INVALID_OPERATION, "Has been used by demuxer");
    CHECK_AND_RETURN_RET_LOG(fd >= 0, AVCS_ERR_INVALID_VAL, "Input fd is negative");
    if (fd <= STDERR_FILENO) {
        AVCODEC_LOGW("Using special fd: %{public}d, isatty: %{public}d", fd, isatty(fd));
    }
    CHECK_AND_RETURN_RET_LOG(offset >= 0, AVCS_ERR_INVALID_VAL, "Input offset is negative");
    CHECK_AND_RETURN_RET_LOG(size > 0, AVCS_ERR_INVALID_VAL, "Input size must be greater than zero");
    int32_t flag = fcntl(fd, F_GETFL, 0);
    CHECK_AND_RETURN_RET_LOG(flag >= 0, AVCS_ERR_INVALID_VAL, "Get fd status failed");
    CHECK_AND_RETURN_RET_LOG(
        (static_cast<uint32_t>(flag) & static_cast<uint32_t>(O_WRONLY)) != static_cast<uint32_t>(O_WRONLY),
        AVCS_ERR_INVALID_VAL, "Fd is not permitted to read");
    CHECK_AND_RETURN_RET_LOG(lseek(fd, 0, SEEK_CUR) != -1, AVCS_ERR_INVALID_VAL, "Fd unseekable");
    
    std::string uri = "fd://" + std::to_string(fd) + "?offset=" + \
        std::to_string(offset) + "&size=" + std::to_string(size);

    return InitWithURI(uri);
}

int32_t AVSourceImpl::InitWithDataSource(const std::shared_ptr<Media::IMediaDataSource> &dataSource)
{
    AVCODEC_SYNC_TRACE;

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer == nullptr, AVCS_ERR_INVALID_OPERATION, "Has been used by demuxer");
    mediaDemuxer = std::make_shared<MediaDemuxer>();
    CHECK_AND_RETURN_RET_LOG(mediaDemuxer != nullptr, AVCS_ERR_INVALID_OPERATION, "Create mediaDemuxer failed");

    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(dataSource);
    Status ret = mediaDemuxer->SetDataSource(mediaSource);
    CHECK_AND_RETURN_RET_LOG(ret == Status::OK, StatusToAVCodecServiceErrCode(ret), "Set data source failed");

    return AVCS_ERR_OK;
}

AVSourceImpl::AVSourceImpl()
{
    AVCODEC_LOGD("Create instances 0x%{public}06" PRIXPTR, FAKE_POINTER(this));
}

AVSourceImpl::~AVSourceImpl()
{
    if (mediaDemuxer != nullptr) {
        mediaDemuxer = nullptr;
    }
    AVCODEC_LOGD("Destroy instances 0x%{public}06" PRIXPTR, FAKE_POINTER(this));
}

int32_t AVSourceImpl::GetSourceFormat(OHOS::Media::Format &format)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("Get source format");

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    
    std::shared_ptr<OHOS::Media::Meta> mediaInfo = mediaDemuxer->GetGlobalMetaInfo();
    CHECK_AND_RETURN_RET_LOG(mediaInfo != nullptr, AVCS_ERR_INVALID_OPERATION, "Parse media info failed");

    bool set = format.SetMeta(mediaInfo);
    CHECK_AND_RETURN_RET_LOG(set, AVCS_ERR_INVALID_OPERATION, "Convert meta failed");

    return AVCS_ERR_OK;
}

int32_t AVSourceImpl::GetTrackFormat(OHOS::Media::Format &format, uint32_t trackIndex)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("Get track %{public}u format", trackIndex);

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");

    std::vector<std::shared_ptr<Meta>> streamsInfo = mediaDemuxer->GetStreamMetaInfo();
    CHECK_AND_RETURN_RET_LOG(trackIndex < streamsInfo.size(), AVCS_ERR_INVALID_VAL,
        "Just have %{public}zu tracks. index is out of range", streamsInfo.size());

    bool set = format.SetMeta(streamsInfo[trackIndex]);
    CHECK_AND_RETURN_RET_LOG(set, AVCS_ERR_INVALID_OPERATION, "Convert meta failed");

    return AVCS_ERR_OK;
}

int32_t AVSourceImpl::GetUserMeta(OHOS::Media::Format &format)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("get user meta");

    CHECK_AND_RETURN_RET_LOG(mediaDemuxer != nullptr, AVCS_ERR_INVALID_OPERATION, "MediaDemuxer does not exist");
    
    std::shared_ptr<OHOS::Media::Meta> userDataMeta = mediaDemuxer->GetUserMeta();
    CHECK_AND_RETURN_RET_LOG(userDataMeta != nullptr, AVCS_ERR_INVALID_OPERATION, "Parse user info failed");

    bool set = format.SetMeta(userDataMeta);
    CHECK_AND_RETURN_RET_LOG(set, AVCS_ERR_INVALID_OPERATION, "Convert meta failed");

    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS