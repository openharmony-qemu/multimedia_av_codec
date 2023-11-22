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

#ifndef AV_CODEC_ENGIN_BASE_FACTORY_H
#define AV_CODEC_ENGIN_BASE_FACTORY_H

#include <functional>
#include <unordered_map>

namespace OHOS {
namespace MediaAVCodec {
template <typename I, typename Identity, typename... Args>
class AVCodecBaseFactory {
public:
    using self = AVCodecBaseFactory<I, Identity, Args...>;

    template <typename... TS>
    static std::shared_ptr<I> make_sharePtr(const Identity &k, TS &&...args)
    {
        auto it = builders().find(k);
        if (it == builders().end())
            return nullptr;
        return it->second(std::forward<TS>(args)...);
    }

    template <typename T>
    struct CodecRegister : public I {
        friend T;
        static bool avRegister()
        {
            const auto r = T::Identify();
            AVCodecBaseFactory::builders()[r] = [](Args &&...args) -> std::shared_ptr<I> {
                return std::make_shared<T>(std::forward<Args>(args)...);
            };
            return true;
        }
        static bool registered;

    private:
        CodecRegister() : I()
        {
            (void)registered;
        }
    };

private:
    friend I;
    AVCodecBaseFactory() = default;
    using builder = std::function<std::shared_ptr<I>(Args...)>;

    static auto &builders()
    {
        static std::unordered_map<Identity, builder> box;
        return box;
    }
};

template <typename I, typename Identify, typename... Args>
template <typename T>
bool AVCodecBaseFactory<I, Identify, Args...>::CodecRegister<T>::registered =
    AVCodecBaseFactory<I, Identify, Args...>::CodecRegister<T>::avRegister();
} // namespace MediaAVCodec
} // namespace OHOS

#endif