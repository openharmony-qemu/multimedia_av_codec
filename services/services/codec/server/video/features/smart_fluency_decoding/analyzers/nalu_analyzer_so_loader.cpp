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
    AVCODEC_LOGD("NaluAnalyzerSoLoader ctor, codecType: %{public}d", static_cast<int32_t>(codecType_));
    if (LoadLibrary() && createFunc_) {
        soAnalyzer_ = createFunc_(static_cast<int32_t>(codecType_));
        AVCODEC_LOGI("soAnalyzer_ created: %{public}p", soAnalyzer_);
    }
}

NaluAnalyzerSoLoader::~NaluAnalyzerSoLoader()
{
    AVCODEC_LOGD("NaluAnalyzerSoLoader dtor");
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (soAnalyzer_ && destroyFunc_) {
        destroyFunc_(soAnalyzer_);
        soAnalyzer_ = nullptr;
    }
    UnloadLibrary();
}

bool NaluAnalyzerSoLoader::LoadLibrary()
{
    handle_ = dlopen(LIB_NAME, RTLD_LAZY);
    CHECK_AND_RETURN_RET_LOGW(handle_ != nullptr, false,
        "Failed to load %{public}s: %{public}s", LIB_NAME, dlerror());
    createFunc_ = reinterpret_cast<CreateNaluAnalyzerFunc>(dlsym(handle_, "CreateNaluAnalyzer"));
    destroyFunc_ = reinterpret_cast<DestroyAnalyzerFunc>(dlsym(handle_, "DestroyNaluAnalyzer"));
    setCallbackFunc_ = reinterpret_cast<SetCallbackFunc>(dlsym(handle_, "SetCallback"));
    isNonReferenceFunc_ = reinterpret_cast<IsNonReferenceFunc>(dlsym(handle_, "IsNonReferenceFrame"));
    if (!createFunc_ || !destroyFunc_ || !setCallbackFunc_ || !isNonReferenceFunc_) {
        AVCODEC_LOGE("Failed to find required symbols in %{public}s (create:%{public}d, destroy:%{public}d, "
                     "setCb:%{public}d, isNonRef:%{public}d)", 
                     LIB_NAME, (createFunc_ != nullptr), (destroyFunc_ != nullptr), 
                     (setCallbackFunc_ != nullptr), (isNonReferenceFunc_ != nullptr));
        UnloadLibrary();
        return false;
    }
    AVCODEC_LOGI("Successfully loaded %{public}s and resolved all symbols", LIB_NAME);
    return true;
}

void NaluAnalyzerSoLoader::UnloadLibrary()
{
    if (handle_) {
        dlclose(handle_);
        handle_ = nullptr;
        AVCODEC_LOGI("Library %{public}s closed", LIB_NAME);
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
    if (soAnalyzer_ && setCallbackFunc_) {
        AVCODEC_LOGD("SetAuxInfoCallback executed");
        setCallbackFunc_(soAnalyzer_, StaticAuxInfoCallback, this);
    }
}

bool NaluAnalyzerSoLoader::IsNonReferenceFrame(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    std::lock_guard<std::mutex> lock(apiMutex_);
    if (!soAnalyzer_ || !isNonReferenceFunc_ || !inputBuffer || !inputBuffer->memory_) {
        return false;
    }
    const uint8_t* bufferAddr = inputBuffer->memory_->GetAddr();
    uint32_t bufferSize = static_cast<uint32_t>(inputBuffer->memory_->GetSize());
    bool isNonRef = isNonReferenceFunc_(soAnalyzer_, bufferAddr, bufferSize);
    if (isNonRef) {
        AVCODEC_LOGD("NaluAnalyzer identified non-reference frame (size=%{public}u)", bufferSize);
    }
    return isNonRef;
}

bool NaluAnalyzerSoLoader::IsValid() const
{
    return soAnalyzer_ != nullptr;
}

std::unique_ptr<INaluAnalyzer> NaluAnalyzerFactory::CreateNaluAnalyzer(SFDCodecType codecType)
{
    auto loader = std::make_unique<NaluAnalyzerSoLoader>(codecType);
    if (!loader->IsValid()) {
        AVCODEC_LOGW("CreateNaluAnalyzer failed: NaluAnalyzerSoLoader is invalid for codecType %{public}d", 
                     static_cast<int32_t>(codecType));
        return nullptr;
    }
    return loader;
}

} // namespace SFD
} // namespace MediaAVCodec
} // namespace OHOS
