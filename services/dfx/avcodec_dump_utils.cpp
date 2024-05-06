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

#include "avcodec_dump_utils.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecDumpUtils"};
    constexpr uint32_t DUMP_LEVEL_4 = 4;
    constexpr uint32_t DUMP_LEVEL_3 = 3;
    constexpr uint32_t DUMP_LEVEL_2 = 2;
    constexpr uint32_t DUMP_SPACE_LENGTH = 4;
    constexpr uint32_t DUMP_OFFSET_24 = 24;
    constexpr uint32_t DUMP_OFFSET_16 = 16;
    constexpr uint32_t DUMP_OFFSET_8 = 8;
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
int32_t AVCodecDumpControler::AddInfo(const uint32_t dumpIdx, const std::string &name, const std::string &value)
{
    CHECK_AND_RETURN_RET_LOG((dumpIdx >> DUMP_OFFSET_24) > 0, AVCS_ERR_INVALID_VAL,
                             "Add dump info failed, get a invalid dump index.");
    CHECK_AND_RETURN_RET_LOG(!name.empty(), AVCS_ERR_INVALID_VAL,
                             "Add dump info failed, get a empty name.");
    if (dumpInfoMap_.find(dumpIdx) != dumpInfoMap_.end()) {
        AVCODEC_LOGW("Dump info index already exist, index: %{public}d, name: %{public}s.", dumpIdx, name.c_str());
        return AVCS_ERR_OK;
    }

    auto level = GetLevel(dumpIdx);
    length_[level - 1] = length_[level - 1] > name.length() ? length_[level - 1] : name.length();
    dumpInfoMap_.emplace(dumpIdx, make_pair(name, value));
    return AVCS_ERR_OK;
}

int32_t AVCodecDumpControler::AddInfoFromFormat(const uint32_t dumpIdx, const Format &format,
                                                const std::string_view &key, const std::string &name)
{
    CHECK_AND_RETURN_RET_LOG(!key.empty(), AVCS_ERR_INVALID_VAL, "Add dump info failed, get a empty key.");

    std::string value;
    bool ret = false;
    switch (format.GetValueType(key)) {
        case FORMAT_TYPE_INT32: {
            int32_t valueTemp = 0;
            ret = format.GetIntValue(key, valueTemp);
            value = std::to_string(valueTemp);
            break;
        }
        case FORMAT_TYPE_INT64: {
            int64_t valueTemp = 0;
            ret = format.GetLongValue(key, valueTemp);
            value = std::to_string(valueTemp);
            break;
        }
        case FORMAT_TYPE_FLOAT: {
            float valueTemp = 0;
            ret = format.GetFloatValue(key, valueTemp);
            value = std::to_string(valueTemp);
            break;
        }
        case FORMAT_TYPE_DOUBLE: {
            double valueTemp = 0;
            ret = format.GetDoubleValue(key, valueTemp);
            value = std::to_string(valueTemp);
            break;
        }
        case FORMAT_TYPE_STRING: {
            ret = format.GetStringValue(key, value);
            break;
        }
        case FORMAT_TYPE_ADDR:
            break;
        default:
            AVCODEC_LOGE("Add info from format failed. Key: %{public}s", key.data());
    }
    if (ret != true) {
        return AVCS_ERR_INVALID_VAL;
    }

    this->AddInfo(dumpIdx, name, value);
    return AVCS_ERR_OK;
}

int32_t AVCodecDumpControler::AddInfoFromFormatWithMapping(const uint32_t dumpIdx,
                                                           const Format &format, const std::string_view &key,
                                                           const std::string &name,
                                                           std::map<int32_t, const std::string> mapping)
{
    int32_t val;
    if (format.GetIntValue(key, val) == true) {
        auto itMappingString = mapping.find(val);
        const std::string mappingString = itMappingString != mapping.end() ? itMappingString->second : "";
        AddInfo(dumpIdx, name, mappingString);
    } else {
        return AVCS_ERR_INVALID_VAL;
    }
    return AVCS_ERR_OK;
}

int32_t AVCodecDumpControler::GetDumpString(std::string &dumpString)
{
    for (auto iter : dumpInfoMap_) {
        auto level = GetLevel(iter.first);
        std::string name = iter.second.first;
        std::string value = iter.second.second;
        dumpString += std::string((level - 1) * DUMP_SPACE_LENGTH, ' ')
            + name + std::string(length_[level - 1] - name.length(), ' ');
        if (!value.empty()) {
            dumpString +=  " - " + value;
        }
        dumpString += std::string("\n");
    }
    return AVCS_ERR_OK;
}

uint32_t AVCodecDumpControler::GetLevel(const uint32_t dumpIdx)
{
    uint32_t level = 1;
    if (dumpIdx & UINT8_MAX) {
        level = DUMP_LEVEL_4;
    } else if ((dumpIdx >> DUMP_OFFSET_8) & UINT8_MAX) {
        level = DUMP_LEVEL_3;
    } else if ((dumpIdx >> DUMP_OFFSET_16) & UINT8_MAX) {
        level = DUMP_LEVEL_2;
    }
    return level;
}
} // namespace OHOS
} // namespace MediaAVCodec