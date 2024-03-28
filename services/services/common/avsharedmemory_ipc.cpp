/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "avsharedmemory_ipc.h"
#include <unistd.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "buffer/avsharedmemorybase.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVSharedMemoryIPC"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
int32_t WriteAVSharedMemoryToParcel(const std::shared_ptr<AVSharedMemory> &memory, MessageParcel &parcel)
{
    std::shared_ptr<Media::AVSharedMemoryBase> baseMem = std::static_pointer_cast<Media::AVSharedMemoryBase>(memory);
    if (baseMem == nullptr) {
        AVCODEC_LOGE("invalid pointer");
        return AVCS_ERR_INVALID_VAL;
    }

    int32_t fd = baseMem->GetFd();
    int32_t size = baseMem->GetSize();

    (void)parcel.WriteFileDescriptor(fd);
    parcel.WriteInt32(size);
    parcel.WriteUint32(baseMem->GetFlags());
    parcel.WriteString(baseMem->GetName());

    return AVCS_ERR_OK;
}

std::shared_ptr<AVSharedMemory> ReadAVSharedMemoryFromParcel(MessageParcel &parcel)
{
    int32_t fd = parcel.ReadFileDescriptor();
    int32_t size = parcel.ReadInt32();
    uint32_t flags = parcel.ReadUint32();
    std::string name = parcel.ReadString();

    std::shared_ptr<AVSharedMemory> memory = Media::AVSharedMemoryBase::CreateFromRemote(fd, size, flags, name);
    if (memory == nullptr || memory->GetBase() == nullptr) {
        AVCODEC_LOGE("create remote Media::AVSharedMemoryBase failed");
        memory = nullptr;
    }

    (void)::close(fd);
    return memory;
}
} // namespace MediaAVCodec
} // namespace OHOS
