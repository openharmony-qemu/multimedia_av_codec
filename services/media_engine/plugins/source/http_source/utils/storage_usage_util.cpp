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
 
#include <sys/statvfs.h>
#include <cstdint>
#include <limits>
#include "storage_usage_util.h"
#include "common/media_core.h"
#include "common/log.h"
#include "safe_arithmetic.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_PLAYER, "HiStreamerStorageUsageUtil" };
}

namespace OHOS {
namespace Media {
constexpr double DOUBLE_TO_INT64_SAFE_MAX = 9.0e15;
constexpr int64_t STORAGE_THRESHOLD_500G = 500LL * 1000 * 1000 * 1000;
constexpr int64_t STORAGE_EXTRA_2G = 2LL * 1000 * 1000 * 1000;
constexpr double STORAGE_PERCENT_THRESHOLD_OVER_500G = 0.05;
constexpr double STORAGE_PERCENT_THRESHOLD = 0.1;
 
int64_t StorageUsageUtil::GetFreeSize()
{
    struct statvfs diskInfo;
    int ret = statvfs("/data", &diskInfo);
    if (ret != 0) {
        MEDIA_LOG_I("get freesize failed, errno:%{public}d", errno);
        return -1;
    }
    uint64_t result = static_cast<uint64_t>(diskInfo.f_bsize) * diskInfo.f_bfree;
    if (result > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        return std::numeric_limits<int64_t>::max();
    }
    return static_cast<int64_t>(result);
}

int64_t StorageUsageUtil::GetTotalSize()
{
    struct statvfs diskInfo;
    int ret = statvfs("/data", &diskInfo);
    if (ret != 0) {
        MEDIA_LOG_I("get totalsize failed, errno:%{public}d", errno);
        return -1;
    }
    uint64_t result = static_cast<uint64_t>(diskInfo.f_bsize) * diskInfo.f_blocks;
    if (result > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        return std::numeric_limits<int64_t>::max();
    }
    return static_cast<int64_t>(result);
}
 
bool StorageUsageUtil::HasEnoughStorage()
{
    int64_t freesize = GetFreeSize();
    int64_t totalSize = GetTotalSize();
    if (freesize == -1 || totalSize == -1) {
        return false;
    }
    MEDIA_LOG_I("StorageUsageUtil freesize:" PUBLIC_LOG_D64 "; totalsize:" PUBLIC_LOG_D64, freesize, totalSize);
    auto safeThreshold = [](int64_t total, double percent) -> int64_t {
        double r = static_cast<double>(total) * percent;
        if (r >= DOUBLE_TO_INT64_SAFE_MAX) {
            return std::numeric_limits<int64_t>::max() - STORAGE_EXTRA_2G;
        }
        return static_cast<int64_t>(r);
    };
    if (totalSize > STORAGE_THRESHOLD_500G) {
        return freesize > (safeThreshold(totalSize, STORAGE_PERCENT_THRESHOLD_OVER_500G) + STORAGE_EXTRA_2G);
    }
    return freesize > (safeThreshold(totalSize, STORAGE_PERCENT_THRESHOLD) + STORAGE_EXTRA_2G);
}
} // namespace Media
} // namespace OHOS