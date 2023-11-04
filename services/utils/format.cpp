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

#include "format.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "meta.h"
#include "securec.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "Format"};
constexpr int32_t MAX_KEY_NUM = 100;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
void CopyFormatDataMap(const Format::FormatDataMap &from, Format::FormatDataMap &to)
{
    for (auto it = to.begin(); it != to.end(); ++it) {
        if (it->second.type == FORMAT_TYPE_ADDR && it->second.addr != nullptr) {
            free(it->second.addr);
            it->second.addr = nullptr;
        }
    }

    to = from;

    for (auto it = to.begin(); it != to.end();) {
        if (it->second.type != FORMAT_TYPE_ADDR || it->second.addr == nullptr) {
            ++it;
            continue;
        }

        it->second.addr = reinterpret_cast<uint8_t *>(malloc(it->second.size));
        if (it->second.addr == nullptr) {
            AVCODEC_LOGE("malloc addr failed. Key: %{public}s", it->first.c_str());
            it = to.erase(it);
            continue;
        }

        errno_t err = memcpy_s(reinterpret_cast<void *>(it->second.addr), it->second.size,
                               reinterpret_cast<const void *>(from.at(it->first).addr), it->second.size);
        if (err != EOK) {
            AVCODEC_LOGE("memcpy addr failed. Key: %{public}s", it->first.c_str());
            free(it->second.addr);
            it->second.addr = nullptr;
            it = to.erase(it);
            continue;
        }
        ++it;
    }
}

Format::~Format()
{
    for (auto it = formatMap_.begin(); it != formatMap_.end(); ++it) {
        if (it->second.type == FORMAT_TYPE_ADDR && it->second.addr != nullptr) {
            free(it->second.addr);
            it->second.addr = nullptr;
        }
    }
}

Format::Format()
{
    isChanged_ = true;
    this->meta_ = std::make_shared<Meta>();
}

Format::Format(const Format &rhs)
{
    if (&rhs == this) {
        return;
    }

    CopyFormatDataMap(rhs.formatMap_, formatMap_);
    if (this->meta_ == nullptr) {
        this->meta_ = std::make_shared<Meta>();
    }
    *(this->meta_) = *(rhs.meta_);
    isChanged_ = rhs.isChanged_;
}

Format::Format(Format &&rhs) noexcept
{
    std::swap(formatMap_, rhs.formatMap_);
    this->meta_ = rhs.meta_;
    isChanged_ = rhs.isChanged_;
}

Format &Format::operator=(const Format &rhs)
{
    if (&rhs == this) {
        return *this;
    }

    CopyFormatDataMap(rhs.formatMap_, this->formatMap_);
    isChanged_ = rhs.isChanged_;
    *(this->meta_) = *(rhs.meta_);
    return *this;
}

Format &Format::operator=(Format &&rhs) noexcept
{
    if (&rhs == this) {
        return *this;
    }

    std::swap(this->formatMap_, rhs.formatMap_);
    isChanged_ = rhs.isChanged_;
    this->meta_ = rhs.meta_;
    return *this;
}

bool Format::PutIntValue(const std::string_view &key, int32_t value)
{
    if (formatMap_.size() >= MAX_KEY_NUM) {
        AVCODEC_LOGE("put int error, formatmap out of size");
        return false;
    }
    isChanged_ = true;
    FormatData data;
    data.type = FORMAT_TYPE_INT32;
    data.val.int32Val = value;
    RemoveKey(key);
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutLongValue(const std::string_view &key, int64_t value)
{
    if (formatMap_.size() >= MAX_KEY_NUM) {
        AVCODEC_LOGE("put long error, formatmap out of size");
        return false;
    }
    isChanged_ = true;
    FormatData data;
    data.type = FORMAT_TYPE_INT64;
    data.val.int64Val = value;
    RemoveKey(key);
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutFloatValue(const std::string_view &key, float value)
{
    if (formatMap_.size() >= MAX_KEY_NUM) {
        AVCODEC_LOGE("put float error, formatmap out of size");
        return false;
    }
    isChanged_ = true;
    FormatData data;
    data.type = FORMAT_TYPE_FLOAT;
    data.val.floatVal = value;
    RemoveKey(key);
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutDoubleValue(const std::string_view &key, double value)
{
    if (formatMap_.size() >= MAX_KEY_NUM) {
        AVCODEC_LOGE("put double error, formatmap out of size");
        return false;
    }
    isChanged_ = true;
    FormatData data;
    data.type = FORMAT_TYPE_DOUBLE;
    data.val.doubleVal = value;
    RemoveKey(key);
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::PutStringValue(const std::string_view &key, const std::string_view &value)
{
    if (formatMap_.size() >= MAX_KEY_NUM) {
        AVCODEC_LOGE("put string error, formatmap out of size");
        return false;
    }
    isChanged_ = true;
    FormatData data;
    data.type = FORMAT_TYPE_STRING;
    data.stringVal = value;
    RemoveKey(key);
    auto ret = formatMap_.insert(std::make_pair(key, data));
    return ret.second;
}

bool Format::GetStringValue(const std::string_view &key, std::string &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_STRING) {
        AVCODEC_LOGE("Format::GetFormat failed. Key: %{public}s", key.data());
        return false;
    }
    value = iter->second.stringVal;
    return true;
}

bool Format::GetIntValue(const std::string_view &key, int32_t &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_INT32) {
        AVCODEC_LOGE("Format::GetFormat failed. Key: %{public}s", key.data());
        return false;
    }
    value = iter->second.val.int32Val;
    return true;
}

bool Format::GetLongValue(const std::string_view &key, int64_t &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_INT64) {
        AVCODEC_LOGE("Format::GetFormat failed. Key: %{public}s", key.data());
        return false;
    }
    value = iter->second.val.int64Val;
    return true;
}

bool Format::GetFloatValue(const std::string_view &key, float &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_FLOAT) {
        AVCODEC_LOGE("Format::GetFormat failed. Key: %{public}s", key.data());
        return false;
    }
    value = iter->second.val.floatVal;
    return true;
}

bool Format::GetDoubleValue(const std::string_view &key, double &value) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_DOUBLE) {
        AVCODEC_LOGE("Format::GetFormat failed. Key: %{public}s", key.data());
        return false;
    }
    value = iter->second.val.doubleVal;
    return true;
}

bool Format::PutBuffer(const std::string_view &key, const uint8_t *addr, size_t size)
{
    if (formatMap_.size() >= MAX_KEY_NUM) {
        AVCODEC_LOGE("put buffer error, formatmap out of size");
        return false;
    }
    if (addr == nullptr) {
        AVCODEC_LOGE("put buffer error, addr is nullptr");
        return false;
    }

    constexpr size_t sizeMax = 1 * 1024 * 1024;
    if (size > sizeMax) {
        AVCODEC_LOGE("PutBuffer input size failed. Key: %{public}s", key.data());
        return false;
    }

    FormatData data;
    data.type = FORMAT_TYPE_ADDR;
    data.addr = reinterpret_cast<uint8_t *>(malloc(size));
    if (data.addr == nullptr) {
        AVCODEC_LOGE("malloc addr failed. Key: %{public}s", key.data());
        return false;
    }

    errno_t err = memcpy_s(reinterpret_cast<void *>(data.addr), size, reinterpret_cast<const void *>(addr), size);
    if (err != EOK) {
        AVCODEC_LOGE("PutBuffer memcpy addr failed. Key: %{public}s", key.data());
        free(data.addr);
        return false;
    }

    RemoveKey(key);

    data.size = size;
    auto ret = formatMap_.insert(std::make_pair(key, data));
    isChanged_ = true;
    return ret.second;
}

bool Format::GetBuffer(const std::string_view &key, uint8_t **addr, size_t &size) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end() || iter->second.type != FORMAT_TYPE_ADDR) {
        AVCODEC_LOGE("Format::GetBuffer failed. Key: %{public}s", key.data());
        return false;
    }
    *addr = iter->second.addr;
    size = iter->second.size;
    return true;
}

bool Format::ContainKey(const std::string_view &key) const
{
    auto iter = formatMap_.find(key);
    if (iter != formatMap_.end()) {
        return true;
    }

    return false;
}

FormatDataType Format::GetValueType(const std::string_view &key) const
{
    auto iter = formatMap_.find(key);
    if (iter == formatMap_.end()) {
        return FORMAT_TYPE_NONE;
    }

    return iter->second.type;
}

void Format::RemoveKey(const std::string_view &key)
{
    auto iter = formatMap_.find(key);
    if (iter != formatMap_.end()) {
        if (iter->second.type == FORMAT_TYPE_ADDR && iter->second.addr != nullptr) {
            free(iter->second.addr);
            iter->second.addr = nullptr;
        }
        formatMap_.erase(iter);
    }
    isChanged_ = true;
}

const Format::FormatDataMap &Format::GetFormatMap() const
{
    return formatMap_;
}

std::string Format::Stringify() const
{
    std::string outString;
    for (auto iter = formatMap_.begin(); iter != formatMap_.end(); iter++) {
        switch (GetValueType(iter->first)) {
            case FORMAT_TYPE_INT32:
                outString += iter->first + " = " + std::to_string(iter->second.val.int32Val) + " | ";
                break;
            case FORMAT_TYPE_INT64:
                outString += iter->first + " = " + std::to_string(iter->second.val.int64Val) + " | ";
                break;
            case FORMAT_TYPE_FLOAT:
                outString += iter->first + " = " + std::to_string(iter->second.val.floatVal) + " | ";
                break;
            case FORMAT_TYPE_DOUBLE:
                outString += iter->first + " = " + std::to_string(iter->second.val.doubleVal) + " | ";
                break;
            case FORMAT_TYPE_STRING:
                outString += iter->first + " = " + iter->second.stringVal + " | ";
                break;
            case FORMAT_TYPE_ADDR:
                break;
            default:
                AVCODEC_LOGE("Format::Stringify failed. Key: %{public}s", iter->first.c_str());
        }
    }
    return outString;
}

std::shared_ptr<Meta> &Format::GetMeta()
{
    if (isChanged_) {
        meta_ = std::make_shared<Meta>();
        for (auto iter = formatMap_.begin(); iter != formatMap_.end(); iter++) {
            switch (GetValueType(iter->first)) {
                case FORMAT_TYPE_INT32:
                    meta_->SetData(iter->first, iter->second.val.int32Val);
                    break;
                case FORMAT_TYPE_INT64:
                    meta_->SetData(iter->first, iter->second.val.int64Val);
                    break;
                case FORMAT_TYPE_FLOAT:
                    meta_->SetData(iter->first, iter->second.val.floatVal);
                    break;
                case FORMAT_TYPE_DOUBLE:
                    meta_->SetData(iter->first, iter->second.val.doubleVal);
                    break;
                case FORMAT_TYPE_STRING:
                    meta_->SetData(iter->first, iter->second.stringVal);
                    break;
                case FORMAT_TYPE_ADDR:
                    meta_->SetData(iter->first, std::vector(iter->second.addr, iter->second.addr + iter->second.size));
                    break;
                default:
                    AVCODEC_LOGE("Format::get Meta failed. Key: %{public}s", iter->first.c_str());
            }
        }
    }
    isChanged_ = false;
    return meta_;
}

bool Format::SetMeta(const Meta &meta)
{
    bool ret = true;
    for (auto iter = meta.begin(); iter != meta.end(); ++iter) {
        std::string key = iter->first;
        // Meta::ValueType type = meta.GetValueType<key.c_str()>();
        // if (type == Meta::ValueType::UINT32_T) {
        //     ret &= PutIntValue(key, AnyCast<uint32_t>(iter->second));
        // } else if (type == Meta::ValueType::INT32_T) {
        //     ret &= PutIntValue(key, AnyCast<int32_t>(iter->second));
        // }
        if (Any::IsSameTypeWith<int32_t>(iter->second)) {
            ret &= PutIntValue(key, AnyCast<int32_t>(iter->second));
        } else if (Any::IsSameTypeWith<uint32_t>(iter->second)) {
            ret &= PutIntValue(key, AnyCast<uint32_t>(iter->second));
        } else if (Any::IsSameTypeWith<int64_t>(iter->second)) {
            ret &= PutLongValue(key, AnyCast<int64_t>(iter->second));
        } else if (Any::IsSameTypeWith<float>(iter->second)) {
            ret &= PutFloatValue(key, AnyCast<float>(iter->second));
        } else if (Any::IsSameTypeWith<double>(iter->second)) {
            ret &= PutDoubleValue(key, AnyCast<double>(iter->second));
        } else if (Any::IsSameTypeWith<std::string>(iter->second)) {
            ret &= PutStringValue(key, AnyCast<std::string>(iter->second));
        } else if (Any::IsSameTypeWith<std::vector<uint8_t>>(iter->second)) {
            ret &= PutBuffer(key, static_cast<const uint8_t *>(AnyCast<std::vector<uint8_t>>(iter->second).data()),
                             static_cast<size_t>(AnyCast<std::vector<uint8_t>>(iter->second).size()));
        } else {
            AVCODEC_LOGE("fail to set Meta Key: %{public}s", iter->first.c_str());
            return false;
        }
    }
    isChanged_ = false;
    *meta_ = meta;
    return ret;
}
} // namespace MediaAVCodec
} // namespace OHOS