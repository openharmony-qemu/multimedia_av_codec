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
    if (LoadLibrary() && createFunc_) {
        soParser_ = createFunc_(static_cast<int32_t>(codecType_));
    }
}

MvAnalyzerSoLoader::~MvAnalyzerSoLoader()
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (soParser_ && destroyFunc_) {
        destroyFunc_(soParser_);
        soParser_ = nullptr;
    }
    UnloadLibrary();
}

bool MvAnalyzerSoLoader::LoadLibrary()
{
    handle_ = dlopen(LIB_NAME, RTLD_LAZY);
    if (!handle_) {
        AVCODEC_LOGW("Failed to load %{public}s: %{public}s", LIB_NAME, dlerror());
        return false;
    }

    createFunc_ = reinterpret_cast<CreateFunc>(dlsym(handle_, "CreateMvAnalyzerWrapper"));
    destroyFunc_ = reinterpret_cast<DestroyFunc>(dlsym(handle_, "DestroyMvAnalyzerWrapper"));
    setAuxInfoFunc_ = reinterpret_cast<SetAuxInfoFunc>(dlsym(handle_, "SetMvStreamAuxInfo"));
    setVideoInfoFunc_ = reinterpret_cast<SetVideoInfoFunc>(dlsym(handle_, "SetMvVideoInfo"));
    parseFunc_ = reinterpret_cast<ParseFunc>(dlsym(handle_, "ParseMvData"));
    resetFunc_ = reinterpret_cast<ResetFunc>(dlsym(handle_, "ResetMvAnalyzerWrapper"));
    getSizeFunc_ = reinterpret_cast<GetSizeFunc>(dlsym(handle_, "GetTargetMvDataSize"));

    if (!createFunc_ || !destroyFunc_ || !parseFunc_) {
        AVCODEC_LOGE("Failed to find required symbols in %{public}s", LIB_NAME);
        UnloadLibrary();
        return false;
    }
    AVCODEC_LOGI("Successfully loaded %{public}s", LIB_NAME);
    return true;
}

void MvAnalyzerSoLoader::UnloadLibrary()
{
    if (handle_) {
        dlclose(handle_);
        handle_ = nullptr;
    }
    createFunc_ = nullptr;
    destroyFunc_ = nullptr;
    setAuxInfoFunc_ = nullptr;
    setVideoInfoFunc_ = nullptr;
    parseFunc_ = nullptr;
    resetFunc_ = nullptr;
    getSizeFunc_ = nullptr;
}

void MvAnalyzerSoLoader::SetStreamAuxInfo(const StreamAuxInfo& info)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (soParser_ && setAuxInfoFunc_) {
        setAuxInfoFunc_(soParser_, &info);
    }
}

void MvAnalyzerSoLoader::SetVideoInfo(uint32_t width, uint32_t height)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (soParser_ && setVideoInfoFunc_) {
        setVideoInfoFunc_(soParser_, width, height);
    }
}

MVStats MvAnalyzerSoLoader::parse(const uint8_t *buffer, size_t bufferSize)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (!soParser_ || !parseFunc_) {
        return latestStats_;
    }
    latestStats_ = parseFunc_(soParser_, buffer, bufferSize);
    return latestStats_;
}

SFDCodecType MvAnalyzerSoLoader::GetCodecType() const
{
    return codecType_;
}

const MVStats& MvAnalyzerSoLoader::GetStats() const
{
    return latestStats_;
}

void MvAnalyzerSoLoader::Reset()
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (soParser_ && resetFunc_) {
        resetFunc_(soParser_);
    }
    latestStats_ = MVStats();
}

size_t MvAnalyzerSoLoader::GetMvDataSize() const
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (soParser_ && getSizeFunc_) {
        return getSizeFunc_(soParser_);
    }
    return 0;
}

std::unique_ptr<IMvAnalyzer> MvAnalyzerFactory::CreateMvAnalyzer(SFDCodecType codecType)
{
    if (!IsCodecSupported(codecType)) {
        return nullptr;
    }
    return std::make_unique<MvAnalyzerSoLoader>(codecType);
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