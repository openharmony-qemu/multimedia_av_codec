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

#include "avcodeclist_impl.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "i_avcodec_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecListImpl"};
const std::vector<std::string> VIDEO_MIME_VEC = {
    std::string(OHOS::MediaAVCodec::CodecMimeType::VIDEO_AVC),
    std::string(OHOS::MediaAVCodec::CodecMimeType::VIDEO_HEVC),
    std::string(OHOS::MediaAVCodec::CodecMimeType::VIDEO_VVC),
    std::string(OHOS::MediaAVCodec::CodecMimeType::VIDEO_MPEG2),
    std::string(OHOS::MediaAVCodec::CodecMimeType::VIDEO_H263),
    std::string(OHOS::MediaAVCodec::CodecMimeType::VIDEO_MPEG4),
    std::string(OHOS::MediaAVCodec::CodecMimeType::VIDEO_RV30),
    std::string(OHOS::MediaAVCodec::CodecMimeType::VIDEO_RV40),
    std::string(OHOS::MediaAVCodec::CodecMimeType::VIDEO_VP8),
    std::string(OHOS::MediaAVCodec::CodecMimeType::VIDEO_VP9)};
const std::vector<std::string> AUDIO_MIME_VEC = {
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_AMR_NB),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_AMR_WB),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_MPEG),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_AAC),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_VORBIS),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_OPUS),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_FLAC),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_RAW),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_G711MU),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_G711A),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_COOK),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_AC3),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_AVS3DA),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_LBVC),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_APE),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_MIMETYPE_L2HC),
    std::string(OHOS::MediaAVCodec::CodecMimeType::AUDIO_VIVID)};
}
namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVCodecList> AVCodecListFactory::CreateAVCodecList()
{
    static std::shared_ptr<AVCodecListImpl> impl = std::make_shared<AVCodecListImpl>();
    static bool initialized = false;
    static std::mutex initMutex;
    std::lock_guard lock(initMutex);
    if (!initialized || impl->IsServiceDied()) {
        int32_t ret = impl->Init();
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init AVCodecListImpl failed");
        initialized = true;
    }
    return impl;
}

int32_t AVCodecListImpl::Init()
{
    codecListService_ = AVCodecServiceFactory::GetInstance().CreateCodecListService();
    CHECK_AND_RETURN_RET_LOG(codecListService_ != nullptr, AVCS_ERR_UNKNOWN, "Create AVCodecList service failed");
    return AVCS_ERR_OK;
}

bool AVCodecListImpl::IsServiceDied()
{
    return codecListService_ == nullptr || codecListService_->IsServiceDied();
}

AVCodecListImpl::AVCodecListImpl()
{
    AVCODEC_LOGD("Create AVCodecList instances successful");
}

AVCodecListImpl::~AVCodecListImpl()
{
    if (codecListService_ != nullptr) {
        (void)AVCodecServiceFactory::GetInstance().DestroyCodecListService(codecListService_);
        codecListService_ = nullptr;
    }
    for (auto iter = nameAddrMap_.begin(); iter != nameAddrMap_.end(); iter++) {
        if (iter->second != nullptr) {
            free(iter->second);
            iter->second = nullptr;
        }
    }
    nameAddrMap_.clear();
    for (auto addr : bufAddrSet_) {
        if (addr != nullptr) {
            delete[] addr;
        }
    }
    bufAddrSet_.clear();
    for (auto iter = mimeCapsMap_.begin(); iter != mimeCapsMap_.end(); iter++) {
        std::string mime = iter->first;
        for (uint32_t i = 0; i < mimeCapsMap_[mime].size(); i++) {
            delete mimeCapsMap_[mime][i];
            mimeCapsMap_[mime][i] = nullptr;
        }
        mimeCapsMap_[mime].clear();
    }
    mimeCapsMap_.clear();
    AVCODEC_LOGD("Destroy AVCodecList instances successful");
}

std::string AVCodecListImpl::FindDecoder(const Format &format)
{
    return codecListService_->FindDecoder(format);
}

std::string AVCodecListImpl::FindEncoder(const Format &format)
{
    return codecListService_->FindEncoder(format);
}

bool AVCodecListImpl::IsStrMatch(const std::string &str, const std::vector<std::string>& strVec)
{
    for (const auto& s : strVec) {
        if (s.compare(str) == 0) {
            return true;
        }
    }
    return false;
}

CapabilityData *AVCodecListImpl::GetCapability(const std::string &mime, const bool isEncoder,
                                               const AVCodecCategory &category)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(category >= AVCodecCategory::AVCODEC_NONE &&
        category <= AVCodecCategory::AVCODEC_SOFTWARE, nullptr,
        "Unsupported category %{public}d", category);
    bool isHardward = (category == AVCodecCategory::AVCODEC_SOFTWARE) ? false : true;
    AVCodecType codecType = AVCODEC_TYPE_NONE;
    if (IsStrMatch(mime, VIDEO_MIME_VEC)) {
        codecType = isEncoder ? AVCODEC_TYPE_VIDEO_ENCODER : AVCODEC_TYPE_VIDEO_DECODER;
    } else if (IsStrMatch(mime, AUDIO_MIME_VEC)) {
        codecType = isEncoder ? AVCODEC_TYPE_AUDIO_ENCODER : AVCODEC_TYPE_AUDIO_DECODER;
    } else {
        AVCODEC_LOGE("GetCapability failed, mime is %{public}s", mime.c_str());
        return nullptr;
    }
    if (mimeCapsMap_.find(mime) == mimeCapsMap_.end()) {
        std::vector<CapabilityData *> capsArray;
        mimeCapsMap_.emplace(mime, capsArray);
    }
    for (auto cap : mimeCapsMap_[mime]) {
        if (cap->codecType == codecType && cap->isVendor == isHardward) {
            return cap;
        }
    }
    CapabilityData capaDataIn;
    int32_t ret = codecListService_->GetCapability(capaDataIn, mime, isEncoder, category);
    std::string name = capaDataIn.codecName;
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK && !name.empty(), nullptr, "Get capability failed from service,"
        "mime: %{public}s, isEnc: %{public}d, category: %{public}d", mime.c_str(), isEncoder, category);
    if (category == AVCodecCategory::AVCODEC_NONE) {
        for (auto cap : mimeCapsMap_[mime]) {
            if (cap->codecType == codecType && cap->codecName == name) {
                return cap;
            }
        }
    }
    CapabilityData *cap = new CapabilityData(capaDataIn);
    CHECK_AND_RETURN_RET_LOG(cap != nullptr, nullptr, "New capabilityData failed");
    mimeCapsMap_[mime].emplace_back(cap);
    AVCODEC_LOGD("Get capabilityData successfully, mime: %{public}s, isEnc: %{public}d, category: %{public}d",
        mime.c_str(), isEncoder, category);
    return cap;
}

void *AVCodecListImpl::GetBuffer(const std::string &name, uint32_t sizeOfCap)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (nameAddrMap_.find(name) != nameAddrMap_.end()) {
        return nameAddrMap_[name];
    }
    CHECK_AND_RETURN_RET_LOG(sizeOfCap > 0, nullptr, "Get capability buffer failed: invalid size");
    void *buf = static_cast<void *>(malloc(sizeOfCap));
    if (buf != nullptr) {
        nameAddrMap_[name] = buf;
        return buf;
    } else {
        return nullptr;
    }
}

void *AVCodecListImpl::NewBuffer(size_t bufSize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(bufSize > 0, nullptr, "new buffer failed: invalid size");
    uint8_t *temp = new uint8_t[bufSize];
    CHECK_AND_RETURN_RET_LOG(temp != nullptr, nullptr, "new buffer failed: no memory");

    bufAddrSet_.insert(temp);
    return static_cast<void *>(temp);
}
} // namespace MediaAVCodec
} // namespace OHOS