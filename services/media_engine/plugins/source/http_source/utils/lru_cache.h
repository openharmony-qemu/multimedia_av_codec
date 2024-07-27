/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef DEFERRED_PROCCESSING_LRU_CACHE_H
#define DEFERRED_PROCCESSING_LRU_CACHE_H

#include <list>
#include <unordered_map>

namespace OHOS {
namespace Media {

template<class KeyT, class ValueT>
class LruCache {
public:
    explicit LruCache(size_t cacheSize) : cacheSize_(cacheSize)
    {
        if (cacheSize == 0) {
            cacheSize_ = 1;
        }
    }

    void ReCacheSize(size_t cacheSize)
    {
        if (cacheSize == 0) {
            return;
        }
        cacheSize_ = cacheSize;
        Clean();
    }

    void Refer(const KeyT& key, const ValueT& val)
    {
        auto it = itemMap_.find(key);
        if (it != itemMap_.end()) {
            itemList_.erase(it->second);
            itemMap_.erase(it);
        }

        itemList_.push_front(make_pair(key, val));
        itemMap_.insert(make_pair(key, itemList_.begin()));
        Clean();
    }

    bool Exist(const KeyT& key)
    {
        return itemMap_.find(key) != itemMap_.end();
    }

    void Update(const KeyT& keyOld, const KeyT& keyNew, const ValueT& val)
    {
        auto it = itemMap_.find(keyOld);
        if (it == itemMap_.end()) {
            return;
        }
        auto listPos = it->second;
        listPos->first = keyNew;
        listPos->second = val;
        it->second->first = keyNew;
        itemMap_.erase(it);
        itemMap_.insert(make_pair(keyNew, listPos));
    }

    void Delete(const KeyT& key)
    {
        auto it = itemMap_.find(key);
        if (it == itemMap_.end()) {
            return;
        }
        itemList_.erase(it->second);
        itemMap_.erase(it);
    }

    bool GetLruNode(KeyT& key, ValueT& value)
    {
        if (itemList_.empty()) {
            return false;
        }
        auto& back = itemList_.back();
        key = back.first;
        value = back.second;
        return true;
    }

    void Reset()
    {
        itemList_.clear();
        itemMap_.clear();
    }

    void Display(std::function<void(const KeyT& key, const ValueT& value)> showInfo)
    {
        for (auto& item : itemList_) {
            showInfo(item.first, item.second);
        }
    }

    bool Check()
    {
        if (itemList_.size() != itemMap_.size()) {
            return false;
        }
        for (auto& item : itemList_) {
            auto it = itemMap_.find(item.first);
            if (it == itemMap_.end()) {
                return false;
            }
            if (it->second == itemList_.end()) {
                return false;
            }
            if (item != *it->second) {
                return false;
            }
        }
        return true;
    }

private:
    void Clean()
    {
        while (itemMap_.size() > cacheSize_) {
            auto last_it = itemList_.end();
            last_it--;
            itemMap_.erase(last_it->first);
            itemList_.pop_back();
        }
    }

private:
    std::list<std::pair<KeyT, ValueT>> itemList_;
    std::unordered_map<KeyT, decltype(itemList_.begin())> itemMap_;
    size_t cacheSize_;
};
} // namespace Media
} // namespace OHOS
#endif //DEFERRED_PROCCESSING_LRU_CACHE_H