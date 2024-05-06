/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
 *
 * Description: API of param bundle
 */

#ifndef PARAM_BUNDLE_H
#define PARAM_BUNDLE_H

#include <string>
#include <unordered_map>
#include <any>
#include <memory>
#include <mutex>

class ParamBundle;
using ParamSP = std::shared_ptr<ParamBundle>;

class ParamBundle {
public:
    ParamBundle() = default;
    ~ParamBundle() = default;
    template<typename T>
    void SetValue(const std::string &key, const T &value)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_items[key] = value;
    }

    template<typename T>
    bool GetValue(const std::string &key, T &value) const
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        const auto it = m_items.find(key);
        if (it == m_items.end()) {
            return false;
        }
        try {
            value = std::any_cast<T>(it->second);
            return true;
        } catch (const std::bad_any_cast&) {
            return false;
        }
    }

    ParamBundle(const ParamBundle &) = delete;
    ParamBundle &operator=(const ParamBundle &) = delete;
    ParamBundle(ParamBundle &&) = delete;
    ParamBundle &operator=(ParamBundle &&) = delete;

private:
    mutable std::mutex m_mtx;
    std::unordered_map<std::string, std::any> m_items;
};

#endif
