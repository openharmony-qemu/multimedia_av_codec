/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include <climits>
#include "gtest/gtest.h"
#include "http/http_media_downloader.h"

using namespace testing::ext;
namespace OHOS::Media::Plugins::HttpPlugin {
class InitialStreamUnitTest : public testing::Test {};

HWTEST_F(InitialStreamUnitTest, EmptyAndNullCandidatesDoNotInventAStream_001, TestSize.Level2)
{
    auto media = std::make_shared<HttpMediaDownloader>("http://stream.test/no-open", 1, nullptr);
    auto strategy = std::make_shared<PlayStrategy>();
    strategy->width = 1280; // Request 720p so selection is exercised even for an empty candidate list.
    strategy->height = 720;
    media->SetPlayStrategy(strategy);
    media->SetMediaStreams({});
    EXPECT_EQ(media->defaultStream_, nullptr);
    EXPECT_EQ(media->playMediaStreams_.size(), 0u);
    media->SetMediaStreams({nullptr, nullptr});
    EXPECT_EQ(media->defaultStream_, nullptr);
    EXPECT_EQ(media->playMediaStreams_.size(), 2u);
}

HWTEST_F(InitialStreamUnitTest, AllInvalidDimensionsUseFirstNonNullFallback_002, TestSize.Level2)
{
    auto media = std::make_shared<HttpMediaDownloader>("http://stream.test/no-open", 1, nullptr);
    auto first = std::make_shared<PlayMediaStream>();
    first->width = USHRT_MAX;
    first->height = 1;
    auto second = std::make_shared<PlayMediaStream>();
    second->width = 1;
    second->height = USHRT_MAX;
    auto strategy = std::make_shared<PlayStrategy>();
    strategy->width = 640; // Valid 360p request; both candidates must be excluded from distance comparison.
    strategy->height = 360;
    media->SetPlayStrategy(strategy);
    media->SetMediaStreams({nullptr, first, second});
    EXPECT_EQ(media->defaultStream_, first);
    EXPECT_EQ(media->playMediaStreams_.size(), 3u);
}

HWTEST_F(InitialStreamUnitTest, MissingOrInvalidStrategyPreservesInitialStream_003, TestSize.Level2)
{
    auto media = std::make_shared<HttpMediaDownloader>("http://stream.test/no-open", 1, nullptr);
    auto low = std::make_shared<PlayMediaStream>();
    low->width = 640;
    low->height = 360;
    auto high = std::make_shared<PlayMediaStream>();
    high->width = 1280;
    high->height = 720;
    media->SetMediaStreams({low, high});
    media->SetPlayStrategy(nullptr);
    EXPECT_EQ(media->defaultStream_, low);
    auto invalid = std::make_shared<PlayStrategy>();
    invalid->width = USHRT_MAX;
    invalid->height = 720;
    media->SetPlayStrategy(invalid);
    EXPECT_EQ(media->defaultStream_, low);
    EXPECT_EQ(media->initResolution_, 0u);
}
} // namespace OHOS::Media::Plugins::HttpPlugin
