/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "dynamic_interface.h"
#include <dlfcn.h>
#include "utils.h"

namespace {
static constexpr const char* LIBRARY_PATH{"libvideoprocessingengine.z.so"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace PostProcessing {

DynamicInterface::~DynamicInterface()
{
    Unload();
}

bool DynamicInterface::Load()
{
    CHECK_AND_RETURN_RET_LOG(OpenLibrary(), false, "Load VPE library failed.");
    AVCODEC_LOGD("Library opened.");
    CHECK_AND_RETURN_RET_LOG(ReadSymbols(), false, "Read VPE Symbols failed.");
    AVCODEC_LOGD("Symbols loaded.");
    return true;
}

void DynamicInterface::Unload()
{
    ClearSymbols();
    AVCODEC_LOGD("Symbols cleared.");
    CloseLibrary();
    AVCODEC_LOGD("Library closed.");
}

bool DynamicInterface::OpenLibrary()
{
    if (lib_ != nullptr) {
        AVCODEC_LOGI("VPE lib is already loaded.");
        return true;
    }
    lib_ = dlopen(LIBRARY_PATH, RTLD_LAZY);
    CHECK_AND_RETURN_RET_LOG(lib_ != nullptr, false, "Load VPE lib failed.");
    return true;
}

void DynamicInterface::CloseLibrary()
{
    if (lib_ != nullptr) {
        dlclose(lib_);
        lib_ = nullptr;
    }
}

bool DynamicInterface::ReadSymbols()
{
    for (size_t i = 0; i < DYNAMIC_INTERFACE_NUM; ++i) {
        auto fp = dlsym(lib_, DYNAMIC_INTERFACE_SYMBOLS[i]);
        CHECK_AND_RETURN_RET_LOG(fp != nullptr, false, "Symbol not found");
        interfaces_[i] = static_cast<void*>(fp);
    }

    return true;
}

void DynamicInterface::ClearSymbols()
{
    interfaces_.fill(nullptr);
}

} // namespace PostProcessing
} // namespace MediaAVCodec
} // namespace OHOS