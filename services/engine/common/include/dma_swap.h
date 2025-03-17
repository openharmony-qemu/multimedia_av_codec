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

#ifndef DMA_SWAP_H
#define DMA_SWAP_H

#include <sys/types.h>

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
struct DmaBufIoctlSwPara {
    pid_t pid;
    unsigned long ino;
    unsigned int fd;
};

class DmaSwaper {
public:
    int32_t SwapOutDma(pid_t pid, int32_t bufFd);
    int32_t SwapInDma(pid_t pid, int32_t bufFd);
    static DmaSwaper &GetInstance();

private:
    DmaSwaper();
    ~DmaSwaper();
    DmaSwaper(const DmaSwaper &dmaSwaper) = delete;
    const DmaSwaper &operator=(const DmaSwaper &dmaSwaper) = delete;
    int32_t reclaimDriverFd_ = -1;
};
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif