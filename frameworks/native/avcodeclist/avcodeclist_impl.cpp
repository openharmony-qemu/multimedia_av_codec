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
    mimeCapsMap_.clear();
    capabilityListCache_.clear();
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
    CHECK_AND_RETURN_RET_LOG(category >= AVCodecCategory::AVCODEC_NONE &&
        category <= AVCodecCategory::AVCODEC_SOFTWARE && !mime.empty(),
        nullptr, "Unsupported param, mime: %{public}s, category %{public}d", mime.c_str(), category);

    // Search capbility from cache
    bool isHardward = category != AVCodecCategory::AVCODEC_SOFTWARE;
    auto mimeCapsRange = mimeCapsMap_.equal_range(mime);
    for (auto mimeCapIter = mimeCapsRange.first; mimeCapIter != mimeCapsRange.second; mimeCapIter++) {
        const auto &codecType = mimeCapIter->second->codecType;
        auto nextIter = mimeCapIter;
        nextIter++;
        if ((codecType == AVCODEC_TYPE_VIDEO_ENCODER || codecType == AVCODEC_TYPE_AUDIO_ENCODER) == isEncoder &&
            (isHardward == mimeCapIter->second->isVendor)) {
            return mimeCapIter->second.get();
        }
    }

    // Get capbility from service
    auto capData = std::make_shared<CapabilityData>();
    CHECK_AND_RETURN_RET_LOG(capData != nullptr, nullptr, "Create cap data failed");
    int32_t ret = codecListService_->GetCapability(*capData, mime, isEncoder, category);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK && !capData->codecName.empty(), nullptr,
        "Get capability failed from service, mime: %{public}s, isEnc: %{public}d, category: %{public}d",
        mime.c_str(), isEncoder, category);

    auto capInCache = std::find_if(mimeCapsRange.first, mimeCapsRange.second,
        [&capData](const auto &item) { return item.second->codecName == capData->codecName; });
    if (capInCache != mimeCapsRange.second) {
        return capInCache->second.get();
    }
    mimeCapsMap_.emplace(mime, capData);
    AVCODEC_LOGD("Get capabilityData successfully, mime: %{public}s, isEnc: %{public}d, category: %{public}d",
        mime.c_str(), isEncoder, category);
    return capData.get();
}

std::vector<std::shared_ptr<CapabilityData>> AVCodecListImpl::GetCapabilityList(int32_t codecType)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // Search capbility List from cache
    auto iter = capabilityListCache_.find(codecType);
    if (iter != capabilityListCache_.end()) {
        return iter->second;
    }
       
    // Get capbility List from service
    std::vector<std::shared_ptr<CapabilityData>> remoteCapList;
    int32_t ret = codecListService_->GetCapabilityList(remoteCapList);
    if (ret != AVCS_ERR_OK || remoteCapList.empty()) {
        AVCODEC_LOGE("GetCapabilityList failed from service, ret: %{public}d", ret);
        return std::vector<std::shared_ptr<CapabilityData>>();
    }
    std::unordered_multimap<int32_t, std::vector<shared_ptr<CapabilityData>>> newCache;
     for (const auto& cap : remoteCapList) {
        if (cap == nullptr) {
            continue;
        }
        newCache[cap->codecType].push_back(cap);
    }
    capabilityListCache_ = std::move(newCache);
    auto resultIter = capabilityListCache_.find(codecType);
    if (resultIter != capabilityListCache_.end()) {
        AVCODEC_LOGD("Get CapabilityList Successfully from service, codecType: %{public}d, count: %{public}zu",
                     codecType, resultIter->second.size());
        return resultIter->second;
    }
    return std::vector<std::shared_ptr<CapabilityData>>();
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