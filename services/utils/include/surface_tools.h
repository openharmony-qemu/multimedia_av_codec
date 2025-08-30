/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_SURFACE_TOOLS_H
#define AVCODEC_SURFACE_TOOLS_H

#include <string>
#include <mutex>
#include "surface.h"
#include "refbase.h"

namespace OHOS {
namespace MediaAVCodec {
class SurfaceTools {
public:
    static SurfaceTools &GetInstance();
    bool RegisterReleaseListener(int32_t instanceId, sptr<Surface> surface, OnReleaseFunc callback,
        OHSurfaceSource type = OH_SURFACE_SOURCE_VIDEO);
    void CleanCache(int32_t instanceId, sptr<Surface> surface, bool cleanAll);
    void ReleaseSurface(int32_t instanceId, sptr<Surface> surface, bool cleanAll, bool abadon = false);

private:
    std::mutex mutex_;
    std::unordered_map<uint64_t, int32_t> surfaceProducerMap_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_SURFACE_TOOLS_H