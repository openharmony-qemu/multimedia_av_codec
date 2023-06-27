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
 * Description: implementation of param container
 */

#include "param_bundle.h"
#include <cinttypes>
#include "hcodec_log.h"

using namespace std;

ParamSP ParamBundle::Create()
{
    return {new(nothrow)ParamBundle(),
            [](ParamBundle *param) {
                delete param;
            }};
}

bool ParamBundle::HasKey(const std::string &key) const
{
    lock_guard<mutex> lock(m_mtx);
    return m_items.find(key) != m_items.end();
}

void ParamBundle::Clear()
{
    lock_guard<mutex> lock(m_mtx);
    m_items.clear();
}

ParamSP ParamBundle::Copy() const
{
    ParamSP param = ParamBundle::Create();
    if (param == nullptr) {
        return nullptr;
    }
    lock_guard<mutex> lock(m_mtx);
    param->m_items = m_items;
    return param;
}

void ParamBundle::Merge(const ParamSP &other)
{
    if (other == nullptr) {
        return;
    }
    lock_guard<mutex> lock(m_mtx);
    m_items.merge(other->m_items);
}
