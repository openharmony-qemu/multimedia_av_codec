/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef VIDEO_POST_PROCESSOR_FACTORY_H
#define VIDEO_POST_PROCESSOR_FACTORY_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "base_video_post_processor.h"

namespace OHOS {
namespace Media {
using VideoPostProcessorInstanceGenerator = std::function<std::shared_ptr<BaseVideoPostProcessor>()>;
using VideoPostProcessorSupportChecker = std::function<bool(const std::shared_ptr<Meta>& meta)>;

class VideoPostProcessorFactory {
public:
    ~VideoPostProcessorFactory() = default;

    VideoPostProcessorFactory(const VideoPostProcessorFactory&) = delete;

    VideoPostProcessorFactory operator=(const VideoPostProcessorFactory&) = delete;

    static VideoPostProcessorFactory& Instance()
    {
        static VideoPostProcessorFactory instance;
        return instance;
    }

    template <typename T>
    std::shared_ptr<T> CreateVideoPostProcessor(const VideoPostProcessorType type)
    {
        auto typedProcessor = std::make_shared<T>();
        return typedProcessor;
    }

    bool IsPostProcessorSupported(const VideoPostProcessorType type, const std::shared_ptr<Meta>& meta)
    {
        return IsPostProcessorSupportedPriv(type, meta);
    }

    template <typename T>
    void RegisterPostProcessor(const VideoPostProcessorType type,
        const VideoPostProcessorInstanceGenerator& generator = nullptr)
    {
        RegisterPostProcessorPriv<T>(type, generator);
    }

    void RegisterChecker(const VideoPostProcessorType type, const VideoPostProcessorSupportChecker& checker = nullptr)
    {
        RegisterCheckerPriv(type, checker);
    }

private:
    VideoPostProcessorFactory() = default;

    std::shared_ptr<BaseVideoPostProcessor> CreateVideoPostProcessorPriv(const VideoPostProcessorType type);
    bool IsPostProcessorSupportedPriv(const VideoPostProcessorType type, const std::shared_ptr<Meta>& meta);

    template <typename T>
    void RegisterPostProcessorPriv(const VideoPostProcessorType type,
        const VideoPostProcessorInstanceGenerator& generator)
    {
        if (generator == nullptr) {
            auto result = generators_.emplace(
                type, []() {
                    return std::make_shared<T>();
                });
            if (!result.second) {
                result.first->second = generator;
            }
        } else {
            auto result = generators_.emplace(type, generator);
            if (!result.second) {
                result.first->second = generator;
            }
        }
    }

    void RegisterCheckerPriv(const VideoPostProcessorType type, const VideoPostProcessorSupportChecker& checker)
    {
        if (checker == nullptr) {
            auto result = checkers_.emplace(
                type, [](const std::shared_ptr<Meta>& meta) {
                    return true;
                });
            if (!result.second) {
                result.first->second = checker;
            }
        } else {
            auto result = checkers_.emplace(type, checker);
            if (!result.second) {
                result.first->second = checker;
            }
        }
    }

    std::unordered_map<VideoPostProcessorType, VideoPostProcessorInstanceGenerator> generators_;
    std::unordered_map<VideoPostProcessorType, VideoPostProcessorSupportChecker> checkers_;
};

template <typename T>
class AutoRegisterPostProcessor {
public:
    explicit AutoRegisterPostProcessor(const VideoPostProcessorType type)
    {
        VideoPostProcessorFactory::Instance().RegisterPostProcessor<T>(type);
        VideoPostProcessorFactory::Instance().RegisterChecker(type);
    }

    AutoRegisterPostProcessor(const VideoPostProcessorType type, const VideoPostProcessorInstanceGenerator& generator)
    {
        VideoPostProcessorFactory::Instance().RegisterPostProcessor<T>(type, generator);
        VideoPostProcessorFactory::Instance().RegisterChecker(type);
    }

    AutoRegisterPostProcessor(const VideoPostProcessorType type, const VideoPostProcessorInstanceGenerator& generator,
                                    const VideoPostProcessorSupportChecker& checker)
    {
        VideoPostProcessorFactory::Instance().RegisterPostProcessor<T>(type, generator);
        VideoPostProcessorFactory::Instance().RegisterChecker(type, checker);
    }

    ~AutoRegisterPostProcessor() = default;
};
} // namespace Media
} // namespace OHOS
#endif // VIDEO_POST_PROCESSOR_FACTORY_H
