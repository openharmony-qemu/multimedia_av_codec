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
// check is associative container -> map/set
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

// check is sequence container -> vector/deque/list
template<typename C>
struct IsSequence : std::false_type {};

template<typename T, typename Alloc>
struct IsSequence<std::vector<T, Alloc>> : std::true_type {};

template<typename T, typename Alloc>
struct IsSequence<std::deque<T, Alloc>> : std::true_type {};

template<typename T, typename Alloc>
struct IsSequence<std::list<T, Alloc>> : std::true_type {};
} // namespace ContainerFeature

// for containers in which element is copyable type, provide add, del, query, modify with thread safety
template<typename T, template<typename...> class Container>
class ThreadSafeContainer {
public:
    ThreadSafeContainer() : container_() {}

    explicit ThreadSafeContainer(Container<T> init) : container_(std::move(init)) {}

    ThreadSafeContainer(const ThreadSafeContainer&) = delete;
    ThreadSafeContainer& operator=(const ThreadSafeContainer&) = delete;

    ThreadSafeContainer(ThreadSafeContainer&&) noexcept = default;
    ThreadSafeContainer& operator=(ThreadSafeContainer&&) noexcept = default;

    template<typename Func>
    void Modify(Func&& f)
    {
        static_assert(std::is_invocable_v<Func, Container<T>&>, "modify Func must be callable with Container<T>&");
        std::unique_lock<std::shared_mutex> lock(mutex_);
        std::forward<Func>(f)(container_);
    }

    template<typename Func>
    auto ModifyWithResult(Func&& f) -> decltype(f(std::declval<Container<T>&>()))
    {
        static_assert(std::is_invocable_v<Func, Container<T>&>, "modify res Func must be callable with Container<T>&");
        std::unique_lock<std::shared_mutex> lock(mutex_);
        return std::forward<Func>(f)(container_);
    }

    void Clear()
    {
        Modify([](Container<T>& c) {
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

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsAssociative<U>::value, bool> Contains(const typename U::key_type& key) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return container_.find(key) != container_.end();
    }

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsAssociative<U>::value, std::optional<typename U::mapped_type>> Get(
        const typename U::key_type& key) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = container_.find(key);
        if (iter == container_.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsAssociative<U>::value, void> Set(const typename U::key_type& key,
        const typename U::mapped_type& value)
    {
        Modify([&](U& c) {
            c[key] = value;
        });
    }

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsAssociative<U>::value, void> Set(const typename U::key_type& key,
        typename U::mapped_type&& value)
    {
        Modify([&](U& c) {
            c[key] = std::move(value);
        });
    }

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsAssociative<U>::value, void> Remove(const typename U::key_type& key)
    {
        Modify([&](U& c) {
            c.erase(key);
        });
    }

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsSequence<U>::value, void> PushBack(const T& value)
    {
        Modify([&](U& c) {
            c.push_back(value);
        });
    }

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsSequence<U>::value, void> PushBack(T&& value)
    {
        Modify([&](U& c) {
            c.push_back(std::move(value));
        });
    }

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsSequence<U>::value, void> PopBack()
    {
        Modify([&](U& c) {
            if (c.empty()) {
                return;
            }
            c.pop_back();
        });
    }

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsSequence<U>::value, std::optional<T>> Front() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (container_.empty()) {
            return std::nullopt;
        }
        return container_.front();
    }

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsSequence<U>::value, std::optional<T>> Back() const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        if (container_.empty()) {
            return std::nullopt;
        }
        return container_.back();
    }

    template<typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsAssociative<U>::value, void> Insert(const typename U::key_type& value)
    {
        Modify([&](U& c) {
            c.insert(value);
        });
    }

    template<typename Func, typename U = Container<T>> void Foreach(Func&& f) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::for_each(container_.begin(), container_.end(), std::forward<Func>(f));
    }

    template<typename Func, typename U = Container<T>> void ForeachSnapshot(Func&& f)
    {
        // lock shared for copy
        std::shared_lock<std::shared_mutex> lock(mutex_);

        // copy container to snapshot
        U snapshot = container_;

        // unlock after copy
        lock.unlock();

        // traverse each element in snapshot
        std::for_each(snapshot.begin(), snapshot.end(), std::forward<Func>(f));
    }

    template<typename Func, typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsAssociative<U>::value, std::optional<typename U::mapped_type>> FindIf(
        Func&& f) const
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        auto iter = std::find_if(container_.begin(), container_.end(), std::forward<Func>(f));
        if (iter == container_.end()) {
            return std::nullopt;
        }
        return iter->second;
    }

    template<typename Func, typename U = Container<T>>
    std::enable_if_t<ContainerFeature::IsSequence<U>::value, std::optional<T>> FindIf(Func&& f) const
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
    Container<T> container_;
};

}
}
}
}
#endif