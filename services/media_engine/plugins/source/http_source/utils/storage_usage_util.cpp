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
#include "storage_usage_util.h"
#include "common/media_core.h"
#include "common/log.h"
 
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_PLAYER, "HiStreamerStorageUsageUtil" };
}
 
namespace OHOS {
namespace Media {
constexpr int64_t STORAGE_THRESHOLD_500G = 500000000000;
constexpr int64_t STORAGE_EXTRA_2G = 2000000000;
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
    int64_t freeSize =
        static_cast<long long>(diskInfo.f_bsize) * static_cast<long long>(diskInfo.f_bfree);
    return freeSize;
}
 
int64_t StorageUsageUtil::GetTotalSize()
{
    struct statvfs diskInfo;
    int ret = statvfs("/data", &diskInfo);
    if (ret != 0) {
        MEDIA_LOG_I("get totalsize failed, errno:%{public}d", errno);
        return -1;
    }
    int64_t totalSize =
        static_cast<long long>(diskInfo.f_bsize) * static_cast<long long>(diskInfo.f_blocks);
    return totalSize;
}
 
bool StorageUsageUtil::HasEnoughStorage()
{
    int64_t freesize = GetFreeSize();
    int64_t totalSize = GetTotalSize();
    if (freesize == -1 || totalSize == -1) {
        return false;
    }
    MEDIA_LOG_I("StorageUsageUtil freesize:" PUBLIC_LOG_D64 "; totalsize:" PUBLIC_LOG_D64, freesize, totalSize);
    if (totalSize > STORAGE_THRESHOLD_500G) {
        return freesize > (static_cast<long long>(totalSize * STORAGE_PERCENT_THRESHOLD_OVER_500G) + STORAGE_EXTRA_2G);
    }
    return freesize > (static_cast<long long>(totalSize * STORAGE_PERCENT_THRESHOLD) + STORAGE_EXTRA_2G);
}
} // namespace Media
} // namespace OHOS