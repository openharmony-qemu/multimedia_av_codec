/*
 * Copyright (c) 2026-2026 Huawei Device Co., Ltd.
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

#ifndef THREAD_SAFE_CONTAINER_H
#define THREAD_SAFE_CONTAINER_H

#include <algorithm>
#include <concepts>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <shared_mutex>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace ContainerFeature {
template<typename C>
struct IsAssociative : std::false_type {};

template<typename K, typename V, typename Comp, typename Alloc>
struct IsAssociative<std::map<K, V, Comp, Alloc>> : std::true_type {};

template<typename K, typename V, typename Hash, typename Pred, typename Alloc>
struct IsAssociative<std::unordered_map<K, V, Hash, Pred, Alloc>> : std::true_type {};

template<typename K, typename Comp, typename Alloc>
struct IsAssociative<std::set<K, Comp, Alloc>> : std::true_type {};

template<typename K, typename Hash, typename Pred, typename Alloc>
struct IsAssociative<std::unordered_set<K, Hash, Pred, Alloc>> : std::true_type {};

template<typename C>
struct IsSequence : std::false_type {};

template<typename T, typename Alloc>
struct IsSequence<std::vector<T, Alloc>> : std::true_type {};

template<typename T, typename Alloc>
struct IsSequence<std::deque<T, Alloc>> : std::true_type {};

template<typename T, typename Alloc>
struct IsSequence<std::list<T, Alloc>> : std::true_type {};

template<typename C>
struct HasMappedType : std::false_type {};

template<typename K, typename V, typename Comp, typename Alloc>
struct HasMappedType<std::map<K, V, Comp, Alloc>> : std::true_type {};

template<typename K, typename V, typename Hash, typename Pred, typename Alloc>
struct HasMappedType<std::unordered_map<K, V, Hash, Pred, Alloc>> : std::true_type {};
} 

template<typename Container>
class ThreadSafeContainer {
public:
    ThreadSafeContainer() : container_() {}

    explicit ThreadSafeContainer(Container init) : container_(std::move(init)) {}

    ThreadSafeContainer(const ThreadSafeContainer&) = delete;
    ThreadSafeContainer& operator=(const ThreadSafeContainer&) = delete;

    ThreadSafeContainer(ThreadSafeContainer&&) = delete;
    ThreadSafeContainer& operator=(ThreadSafeContainer&&) = delete;

    template<typename Func>
    void Modify(Func&& f)
    {
        static_assert(std::is_invocable_v<Func, Container&>, "modify Func must be callable with Container&");
        std::unique_lock<std::shared_mutex> lock(mutex_);
        std::forward<Func>(f)(container_);
    }

    template<typename Func>
    auto ModifyWithResult(Func&& f) -> decltype(f(std::declval<Container&>()))
    {
        static_assert(std::is_invocable_v<Func, Container&>, "modify res Func must be callable with Container&");
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return std::forward<Func>(f)(container_);
    }

    void Clear()
    {
        Modify([](Container& c) {
            c.clear();
        });
    }

    size_t Size()
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return container_.size();
    }

    bool Empty()
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return container_.empty();
    }

    template<typename Func, typename C = Container>
    std::enable_if_t<ContainerFeature::HasMappedType<C>::value> ForeachKV(Func&& f) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        for (const auto& [key, value] : container_) {
            f(key, value);
        }
    }

    template<typename Func, typename C = Container>
    std::enable_if_t<!ContainerFeature::HasMappedType<C>::value> Foreach(Func&& f) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::for_each(container_.begin(), container_.end(), std::forward<Func>(f));
    }

    template<typename Func, typename C = Container> void ForeachSnapshot(Func&& f)
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        C snapshot = container_;
        lock.unlock();
        std::for_each(snapshot.begin(), snapshot.end(), std::forward<Func>(f));
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::HasMappedType<C>::value, std::optional<typename C::mapped_type>> Get(
        const typename C::key_type& key) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = container_.find(key);
        if (iter == container_.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::HasMappedType<C>::value, void> Set(const typename C::key_type& key,
        const typename C::mapped_type& value)
    {
        Modify([&](C& c) {
            c[key] = value;
        });
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::HasMappedType<C>::value, void> Set(const typename C::key_type& key,
        typename C::mapped_type&& value)
    {
        Modify([&](C& c) {
            c[key] = std::move(value);
        });
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::IsAssociative<C>::value, bool> Contains(const typename C::key_type& key) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return container_.find(key) != container_.end();
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::HasMappedType<C>::value, void> Insert(const typename C::key_type& key,
        const typename C::mapped_type& value)
    {
        Modify([&](C& c) {
            c.insert(typename C::value_type(key, value));
        });
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::HasMappedType<C>::value, void> Insert(const typename C::key_type& key,
        typename C::mapped_type&& value)
    {
        Modify([&](C& c) {
            c.insert(typename C::value_type(key, std::move(value)));
        });
    }

    template<typename C = Container>
    std::enable_if_t<!ContainerFeature::HasMappedType<C>::value && ContainerFeature::IsAssociative<C>::value, void> Insert(
        const typename C::value_type& value)
    {
        Modify([&](C& c) {
            c.insert(value);
        });
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::HasMappedType<C>::value, void> Remove(const typename C::key_type& key)
    {
        Modify([&](C& c) {
            c.erase(key);
        });
    }

    template<typename C = Container>
    std::enable_if_t<!ContainerFeature::HasMappedType<C>::value && ContainerFeature::IsAssociative<C>::value, void> Remove(
        const typename C::value_type& value)
    {
        Modify([&](C& c) {
            c.erase(value);
        });
    }

    template<typename Func, typename C = Container>
    std::enable_if_t<ContainerFeature::HasMappedType<C>::value, std::optional<typename C::mapped_type>> FindIf(
        Func&& f) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = std::find_if(container_.begin(), container_.end(), std::forward<Func>(f));
        if (iter == container_.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::IsSequence<C>::value, void> PushBack(const typename C::value_type& value)
    {
        Modify([&](C& c) {
            c.push_back(value);
        });
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::IsSequence<C>::value, void> PushBack(typename C::value_type&& value)
    {
        Modify([&](C& c) {
            c.push_back(std::move(value));
        });
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::IsSequence<C>::value, void> PopBack()
    {
        Modify([&](C& c) {
            if (c.empty()) {
                return;
            }
            c.pop_back();
        });
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::IsSequence<C>::value, std::optional<typename C::value_type>> Front() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (container_.empty()) {
            return std::nullopt;
        }
        return container_.front();
    }

    template<typename C = Container>
    std::enable_if_t<ContainerFeature::IsSequence<C>::value, std::optional<typename C::value_type>> Back() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (container_.empty()) {
            return std::nullopt;
        }
        return container_.back();
    }

    template<typename Func, typename C = Container>
    std::enable_if_t<!ContainerFeature::HasMappedType<C>::value && ContainerFeature::IsAssociative<C>::value,
        std::optional<typename C::value_type>> FindIf(Func&& f) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = std::find_if(container_.begin(), container_.end(), std::forward<Func>(f));
        if (iter == container_.end()) {
            return std::nullopt;
        }
        return *iter;
    }

    template<typename Func, typename C = Container>
    std::enable_if_t<ContainerFeature::IsSequence<C>::value, std::optional<typename C::value_type>> FindIf(Func&& f) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = std::find_if(container_.begin(), container_.end(), std::forward<Func>(f));
        if (iter == container_.end()) {
            return std::nullopt;
        }
        return *iter;
    }

private:
    mutable std::shared_mutex mutex_;
    Container container_;
};

}
}
}
}
#endif
