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

#include "nalu_analyzer_so_loader.h"
#include <dlfcn.h>
#include "avcodec_log.h"
#include "buffer/avbuffer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SFD, "NaluAnalyzerSoLoader"};
constexpr const char* LIB_NAME = "libvideo_nalu_analyzer.z.so";
}

namespace OHOS {
namespace MediaAVCodec {
namespace SFD {
NaluAnalyzerSoLoader::NaluAnalyzerSoLoader(SFDCodecType codecType)
    : codecType_(codecType)
{
    if (LoadLibrary()) {
        if (createFunc_) {
            soParser_ = createFunc_(static_cast<int32_t>(codecType_));
        }
    }
}

NaluAnalyzerSoLoader::~NaluAnalyzerSoLoader()
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (soParser_ && destroyFunc_) {
        destroyFunc_(soParser_);
        soParser_ = nullptr;
    }
    UnloadLibrary();
}

bool NaluAnalyzerSoLoader::LoadLibrary()
{
    handle_ = dlopen(LIB_NAME, RTLD_LAZY);
    if (!handle_) {
        AVCODEC_LOGW("Failed to load %{public}s: %{public}s", LIB_NAME, dlerror());
        return false;
    }
    createFunc_ = reinterpret_cast<CreateNaluAnalyzerFunc>(dlsym(handle_, "CreateNaluAnalyzer"));
    destroyFunc_ = reinterpret_cast<DestroyAnalyzerFunc>(dlsym(handle_, "DestroyNaluAnalyzer"));
    setCallbackFunc_ = reinterpret_cast<SetCallbackFunc>(dlsym(handle_, "SetNaluAnalyzerCallback"));
    isNonReferenceFunc_ = reinterpret_cast<IsNonReferenceFunc>(dlsym(handle_, "IsNonReferenceFrame"));
    if (!createFunc_ || !destroyFunc_ || !setCallbackFunc_ || !isNonReferenceFunc_) {
        AVCODEC_LOGE("Failed to find required symbols in %{public}s", LIB_NAME);
        UnloadLibrary();
        return false;
    }
    AVCODEC_LOGI("Successfully loaded %{public}s", LIB_NAME);
    return true;
}

void NaluAnalyzerSoLoader::UnloadLibrary()
{
    if (handle_) {
        dlclose(handle_);
        handle_ = nullptr;
    }
    createFunc_ = nullptr;
    destroyFunc_ = nullptr;
    setCallbackFunc_ = nullptr;
    isNonReferenceFunc_ = nullptr;
}

void NaluAnalyzerSoLoader::StaticAuxInfoCallback(const StreamAuxInfo* info, void* userData)
{
    if (!info || !userData) {
        return;
    }
    auto* loader = static_cast<NaluAnalyzerSoLoader*>(userData);
    if (loader->callback_) {
        loader->callback_(*info);
    }
}

void NaluAnalyzerSoLoader::SetAuxInfoCallback(OnStreamAuxInfoParsed callback)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    callback_ = callback;
    if (soParser_ && setCallbackFunc_) {
        setCallbackFunc_(soParser_, StaticAuxInfoCallback, this);
    }
}

bool NaluAnalyzerSoLoader::IsNonReferenceFrame(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (!soParser_ || !isNonReferenceFunc_ || !inputBuffer || !inputBuffer->memory_) {
        return false;
    }
    const uint8_t* bufferAddr = inputBuffer->memory_->GetAddr();
    uint32_t bufferSize = static_cast<uint32_t>(inputBuffer->memory_->GetSize());
    return isNonReferenceFunc_(soParser_, bufferAddr, bufferSize);
}

std::unique_ptr<INaluAnalyzer> NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType codecType)
{
    return std::make_unique<NaluAnalyzerSoLoader>(codecType);
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
