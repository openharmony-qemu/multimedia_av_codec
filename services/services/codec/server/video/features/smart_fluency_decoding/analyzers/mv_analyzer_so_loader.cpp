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

#include "mv_analyzer_so_loader.h"
#include <dlfcn.h>
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "MvAnalyzerSoLoader"};
constexpr const char* LIB_NAME = "libvideo_mv_analyzer.z.so";
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
MvAnalyzerSoLoader::MvAnalyzerSoLoader(SFDCodecType codecType)
    : codecType_(codecType)
{
    AVCODEC_LOGD("MvAnalyzerSoLoader ctor, codecType: %{public}d", static_cast<int32_t>(codecType_));
    if (LoadLibrary() && createFunc_) {
        soAnalyzer_ = createFunc_(static_cast<int32_t>(codecType_));
        AVCODEC_LOGI("soAnalyzer_ created: %{public}p", soAnalyzer_);
    }
}

MvAnalyzerSoLoader::~MvAnalyzerSoLoader()
{
    AVCODEC_LOGD("MvAnalyzerSoLoader dtor");
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (soAnalyzer_ && destroyFunc_) {
        destroyFunc_(soAnalyzer_);
        soAnalyzer_ = nullptr;
    }
    UnloadLibrary();
}

bool MvAnalyzerSoLoader::LoadLibrary()
{
    handle_ = dlopen(LIB_NAME, RTLD_LAZY);
    CHECK_AND_RETURN_RET_LOGW(handle_ != nullptr, false,
        "Failed to load %{public}s: %{public}s", LIB_NAME, dlerror());

    createFunc_ = reinterpret_cast<CreateFunc>(dlsym(handle_, "CreateMvAnalyzer"));
    destroyFunc_ = reinterpret_cast<DestroyFunc>(dlsym(handle_, "DestroyMvAnalyzer"));
    setAuxInfoFunc_ = reinterpret_cast<SetAuxInfoFunc>(dlsym(handle_, "SetStreamAuxInfo"));
    setVideoInfoFunc_ = reinterpret_cast<SetVideoInfoFunc>(dlsym(handle_, "SetVideoInfo"));
    parseFunc_ = reinterpret_cast<ParseFunc>(dlsym(handle_, "ParseMvData"));
    if (!createFunc_ || !destroyFunc_ || !setAuxInfoFunc_ || !setVideoInfoFunc_ || !parseFunc_) {
        AVCODEC_LOGE("Failed to find required symbols in %{public}s (create:%{public}d, destroy:%{public}d, "
                     "setAux:%{public}d, setVideo:%{public}d, parse:%{public}d)",
                     LIB_NAME, (createFunc_ != nullptr), (destroyFunc_ != nullptr),
                     (setAuxInfoFunc_ != nullptr), (setVideoInfoFunc_ != nullptr), (parseFunc_ != nullptr));
        UnloadLibrary();
        return false;
    }
    AVCODEC_LOGI("Successfully loaded %{public}s and resolved all symbols", LIB_NAME);
    return true;
}

void MvAnalyzerSoLoader::UnloadLibrary()
{
    if (handle_) {
        dlclose(handle_);
        handle_ = nullptr;
        AVCODEC_LOGI("Library %{public}s closed", LIB_NAME);
    }
    createFunc_ = nullptr;
    destroyFunc_ = nullptr;
    setAuxInfoFunc_ = nullptr;
    setVideoInfoFunc_ = nullptr;
    parseFunc_ = nullptr;
}

void MvAnalyzerSoLoader::SetStreamAuxInfo(const StreamAuxInfo& info)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (soAnalyzer_ && setAuxInfoFunc_) {
        AVCODEC_LOGD("SetStreamAuxInfo executed: ctuSize %{public}u, directSpatial %{public}u",
                     info.ctuSize, info.directSpatialMvPredFlag);
        setAuxInfoFunc_(soAnalyzer_, &info);
    }
}

void MvAnalyzerSoLoader::SetVideoInfo(uint32_t width, uint32_t height)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (soAnalyzer_ && setVideoInfoFunc_) {
        AVCODEC_LOGD("SetVideoInfo executed: %{public}ux%{public}u", width, height);
        setVideoInfoFunc_(soAnalyzer_, width, height);
    }
}

MVStats MvAnalyzerSoLoader::Analyze(const uint8_t *buffer)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    MVStats mvStats = MVStats();
    if (soAnalyzer_ && parseFunc_) {
        mvStats = parseFunc_(soAnalyzer_, buffer);
    }
    return mvStats;
}

SFDCodecType MvAnalyzerSoLoader::GetCodecType() const
{
    return codecType_;
}

bool MvAnalyzerSoLoader::IsValid() const
{
    return soAnalyzer_ != nullptr;
}

std::unique_ptr<IMvAnalyzer> MvAnalyzerFactory::CreateMvAnalyzer(SFDCodecType codecType)
{
    if (!IsCodecSupported(codecType)) {
        AVCODEC_LOGW("CreateMvAnalyzer failed: codecType %{public}d not supported", static_cast<int32_t>(codecType));
        return nullptr;
    }
    auto loader = std::make_unique<MvAnalyzerSoLoader>(codecType);
    if (!loader->IsValid()) {
        AVCODEC_LOGW("CreateMvAnalyzer failed: MvAnalyzerSoLoader is invalid for codecType %{public}d",
                     static_cast<int32_t>(codecType));
        return nullptr;
    }
    return loader;
}

bool MvAnalyzerFactory::IsCodecSupported(SFDCodecType codecType)
{
    return codecType == SFDCodecType::AVC ||
           codecType == SFDCodecType::HEVC ||
           codecType == SFDCodecType::VVC;
}
} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS