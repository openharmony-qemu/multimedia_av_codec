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

#include "dma_swap.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CODEC_UTILS"};
#define DMA_DEVICE_FILE "/dev/dma_reclaim"
#define DMA_BUF_RECLAIM_IOC_MAGIC 'd'
#define DMA_BUF_RECLAIM_FD _IOWR(DMA_BUF_RECLAIM_IOC_MAGIC, 0x07, int)
#define DMA_BUF_RESUME_FD _IOWR(DMA_BUF_RECLAIM_IOC_MAGIC, 0x08, int)
} // namespace

DmaSwaper::DmaSwaper()
{
    CHECK_AND_RETURN_LOG(reclaimDriverFd_ <= 0, "Already initialized!");
    reclaimDriverFd_ = open(DMA_DEVICE_FILE, O_RDWR | O_CLOEXEC | O_NONBLOCK);
    CHECK_AND_RETURN_LOG(reclaimDriverFd_ > 0, "Fail to open device!");
}

DmaSwaper::~DmaSwaper()
{
    CHECK_AND_RETURN_LOG(reclaimDriverFd_ > 0, "Invalid fd!");
    close(reclaimDriverFd_);
    reclaimDriverFd_ = -1;
}

int32_t DmaSwaper::SwapOutDma(pid_t pid, int32_t bufFd)
{
    CHECK_AND_RETURN_RET_LOG(reclaimDriverFd_ > 0, AVCS_ERR_UNKNOWN, "reclaimDriverFd_ <= 0!");
    CHECK_AND_RETURN_RET_LOG(pid > 0, AVCS_ERR_UNKNOWN, "pid <= 0!");
    CHECK_AND_RETURN_RET_LOG(bufFd > 0, AVCS_ERR_UNKNOWN, "bufFd <= 0!");
    DmaBufIoctlSwPara param {
        .pid = pid,
        .ino = 0,
        .fd = bufFd
    };
    AVCODEC_LOGI("SwapOutDma do ioctl!");
    return ioctl(reclaimDriverFd_, DMA_BUF_RECLAIM_FD, &param);
}

int32_t DmaSwaper::SwapInDma(pid_t pid, int32_t bufFd)
{
    CHECK_AND_RETURN_RET_LOG(reclaimDriverFd_ > 0, AVCS_ERR_UNKNOWN, "reclaimDriverFd_ <= 0!");
    CHECK_AND_RETURN_RET_LOG(pid > 0, AVCS_ERR_UNKNOWN, "pid <= 0!");
    CHECK_AND_RETURN_RET_LOG(bufFd > 0, AVCS_ERR_UNKNOWN, "bufFd <= 0!");
    DmaBufIoctlSwPara param {
        .pid = pid,
        .ino = 0,
        .fd = bufFd
    };
    AVCODEC_LOGI("SwapInDma do ioctl!");
    return ioctl(reclaimDriverFd_, DMA_BUF_RESUME_FD, &param);
}

DmaSwaper &DmaSwaper::GetInstance()
{
    static DmaSwaper swaper;
    return swaper;
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS