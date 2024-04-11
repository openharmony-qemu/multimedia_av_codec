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
}
namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<AVCodecList> AVCodecListFactory::CreateAVCodecList()
{
    static std::shared_ptr<AVCodecListImpl> impl = std::make_shared<AVCodecListImpl>();
    static bool initialized = false;
    static std::mutex initMutex;
    std::lock_guard lock(initMutex);
    if (!initialized) {
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

CapabilityData *AVCodecListImpl::GetCapability(const std::string &mime, const bool isEncoder,
                                               const AVCodecCategory &category)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool isVendor = (category == AVCodecCategory::AVCODEC_SOFTWARE) ? false : true;
    AVCodecType codecType = AVCODEC_TYPE_NONE;
    bool isVideo = mime.find("video") != std::string::npos;
    if (isVideo) {
        codecType = isEncoder ? AVCODEC_TYPE_VIDEO_ENCODER : AVCODEC_TYPE_VIDEO_DECODER;
    } else {
        codecType = isEncoder ? AVCODEC_TYPE_AUDIO_ENCODER : AVCODEC_TYPE_AUDIO_DECODER;
    }
    if (mimeCapsMap_.find(mime) != mimeCapsMap_.end()) {
        for (uint32_t i = 0; i < mimeCapsMap_[mime].size(); i++) {
            if (mimeCapsMap_[mime][i]->codecType == codecType && mimeCapsMap_[mime][i]->isVendor == isVendor) {
                return mimeCapsMap_[mime][i];
            }
        }
    } else {
        std::vector<CapabilityData *> capsArray;
        mimeCapsMap_.insert(std::make_pair(mime, capsArray));
    }
    CapabilityData capaDataIn;
    int32_t ret = codecListService_->GetCapability(capaDataIn, mime, isEncoder, category);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Get capability failed");
    std::string name = capaDataIn.codecName;
    CHECK_AND_RETURN_RET_LOG(!name.empty(), nullptr, "Get capability failed");
    if (category == AVCodecCategory::AVCODEC_NONE && nameAddrMap_.find(name) != nameAddrMap_.end()) {
        for (uint32_t i = 0; i < mimeCapsMap_[mime].size(); i++) {
            if (mimeCapsMap_[mime][i]->codecType == codecType && mimeCapsMap_[mime][i]->codecName == name) {
                return mimeCapsMap_[mime][i];
            }
        }
    }
    CapabilityData *cap = new CapabilityData(capaDataIn);
    mimeCapsMap_.at(mime).emplace_back(cap);
    uint32_t idx = mimeCapsMap_[mime].size() - 1;
    return mimeCapsMap_[mime][idx];
}

void *AVCodecListImpl::GetBuffer(const std::string &name, uint32_t sizeOfCap)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (nameAddrMap_.find(name) != nameAddrMap_.end()) {
        return nameAddrMap_[name];
    }
    CHECK_AND_RETURN_RET_LOG(sizeOfCap > 0, nullptr, "Get capability buffer failed: invalid size");
    nameAddrMap_[name] = static_cast<void *>(malloc(sizeOfCap));
    return nameAddrMap_[name];
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

void AVCodecListImpl::DeleteBuffer(void *bufAddr)
{
    std::lock_guard<std::mutex> lock(mutex_);
    uint8_t *temp = static_cast<uint8_t *>(bufAddr);
    bufAddrSet_.erase(temp);
    delete[] temp;
}
} // namespace MediaAVCodec
} // namespace OHOS