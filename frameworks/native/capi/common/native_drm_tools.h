/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef NATIVE_DRM_TOOLS_H
#define NATIVE_DRM_TOOLS_H

#include <stdint.h>
#include <vector>
#include <map>
#include <mutex>
#include <shared_mutex>
#include "securec.h"
#include "native_avdemuxer.h"

namespace OHOS {
namespace MediaAVCodec {
class NativeDrmTools {
public:
    static constexpr uint32_t MAX_PSSH_COUNT = 200;
    static constexpr uint32_t MAX_PSSH_ENTRIES_SIZE = 5000;

    explicit NativeDrmTools() = default;
    virtual ~NativeDrmTools() = default;
    static int32_t MallocAndCopyDrmInfo(OH_DrmInfo *info,
        const std::multimap<std::string, std::vector<uint8_t>> &drmInfoMap)
    {
        size_t count = drmInfoMap.size();
        info->psshCount = count;
        size_t entriesSize = sizeof(struct OH_PsshEntry) * count;
        if (count <= 0 || count > MAX_PSSH_COUNT || entriesSize <= 0 || entriesSize > MAX_PSSH_ENTRIES_SIZE) {
            return AV_ERR_INVALID_VAL;
        }
        info->entries = (struct OH_PsshEntry *)malloc(entriesSize);
        if (info->entries == NULL) {
            return AV_ERR_NO_MEMORY;
        }
        memset_s(info->entries, entriesSize, 0x00, entriesSize);

        size_t index = 0;
        for (auto &item : drmInfoMap) {
            const uint32_t uuidSize = 16;
            info->entries[index].uuid = (unsigned char *)malloc(uuidSize);
            if (info->entries[index].uuid == NULL) {
                FreeDrmInfo(info, count);
                return AV_ERR_NO_MEMORY;
            }
            uint32_t step = 2;
            for (uint32_t i = 0; i < item.first.size(); i += step) {
                std::string byteString = item.first.substr(i, step);
                unsigned char byte = (unsigned char)strtol(byteString.c_str(), NULL, uuidSize);
                info->entries[index].uuid[i / step] = byte;
            }

            info->entries[index].dataLen = item.second.size();
            info->entries[index].data = (void *)malloc(item.second.size());
            if (info->entries[index].data == NULL) {
                FreeDrmInfo(info, count);
                return AV_ERR_NO_MEMORY;
            }
            memcpy_s(info->entries[index].data, item.second.size(), static_cast<const void *>(item.second.data()),
                item.second.size());
            index++;
        }
        return AV_ERR_OK;
    }

    static int32_t FreeDrmInfo(OH_DrmInfo *info, size_t count)
    {
        if (info != NULL && info->entries != NULL) {
            for (size_t index = 0; index < count; index++) {
                if (info->entries[index].uuid != NULL) {
                    free(info->entries[index].uuid);
                    info->entries[index].uuid = NULL;
                }
                if (info->entries[index].data != NULL) {
                    free(info->entries[index].data);
                    info->entries[index].data = NULL;
                }
            }
            free(info->entries);
            info->entries = NULL;
        }
        return AV_ERR_OK;
    }

private:
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // NATIVE_DRM_TOOLS_H