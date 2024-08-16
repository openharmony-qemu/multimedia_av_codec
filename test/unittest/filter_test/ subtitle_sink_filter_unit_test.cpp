/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include "subtitle_sink_filter_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace {
static const std::string MEDIA_ROOT = "file:///data/test/media/";
static const std::string VIDEO_FILE1 = MEDIA_ROOT + "camera_info_parser.mp4";
static const std::string MIME_IMAGE = "image";
}  // namespace

namespace OHOS {
namespace Media {
namespace Pipeline {
class FilterLinkCallbackMock : public FilterLinkCallback {
public:
    void OnLinkedResult(const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta)
    {
        (void)queue;
        (void)meta;
    }

    void OnUnlinkedResult(std::shared_ptr<Meta>& meta)
    {
        (void)meta;
    }

    void OnUpdatedResult(std::shared_ptr<Meta>& meta)
    {
        (void)meta;
    }
}
void SubtitleSinkFilterUnitTest::SetUpTestCase(void) {}

void SubtitleSinkFilterUnitTest::TearDownTestCase(void) {}

void SubtitleSinkFilterUnitTest::SetUp(void)
{
    subtitleSinkFilter_ = std::make_shared<SubtitleSinkFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_DEMUXER);
    auto receiver = std::make_shared<FilterEventReceiverMock>();
    subtitleSinkFilter_->receiver_ = receiver;
    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    subtitleSinkFilter_->SetDataSource(mediaSource);
}

void SubtitleSinkFilterUnitTest::TearDown(void)
{
    subtitleSinkFilter_ = nullptr;
}

/**
 * @tc.name: DoPrepare
 * @tc.desc: DoPrepare
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoPrepare, TestSize.Level1)
{ 
    auto callback = std::make_shared<FilterLinkCallbackMock>();
    subtitleSinkFilter_->onLinkedResultCallback_ = callback;
    res = subtitleSinkFilter_->DoPrepare();
}

/**
 * @tc.name: DoStart
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoStart, TestSize.Level1)
{
    subtitleSinkFilter_->state_ = FilterState::RUNNING;
    res = subtitleSinkFilter_->DoStart();

    subtitleSinkFilter_->state_ = FilterState::READY;
    res = subtitleSinkFilter_->DoStart();

    subtitleSinkFilter_->state_ = FilterState::PAUSED;
    res = subtitleSinkFilter_->DoStart();

    subtitleSinkFilter_->state_ = FilterState::ERROR;
    res = subtitleSinkFilter_->DoStart();
    
    auto callback = std::make_shared<FilterLinkCallbackMock>();
    subtitleSinkFilter_->onLinkedResultCallback_ = callback;
    res = subtitleSinkFilter_->DoStart();
}

/**
 * @tc.name: DoPause
 * @tc.desc: DoPause
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoPause, TestSize.Level1)
{
    subtitleSinkFilter_->state_ = FilterState::PAUSED;
    res = subtitleSinkFilter_->DoPause();

    subtitleSinkFilter_->state_ = FilterState::STOPPED;
    res = subtitleSinkFilter_->DoPause();

    subtitleSinkFilter_->state_ = FilterState::READY;
    res = subtitleSinkFilter_->DoPause();

    subtitleSinkFilter_->state_ = FilterState::RUNNING;
    res = subtitleSinkFilter_->DoPause();

    subtitleSinkFilter_->state_ = FilterState::ERROR;
    res = subtitleSinkFilter_->DoPause();
    
    auto callback = std::make_shared<FilterLinkCallbackMock>();
    subtitleSinkFilter_->onLinkedResultCallback_ = callback;
    res = subtitleSinkFilter_->DoPause();
}

/**
 * @tc.name: DoResume
 * @tc.desc: DoResume
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoResume, TestSize.Level1)
{
    subtitleSinkFilter_->state_ = FilterState::PAUSED;
    res = subtitleSinkFilter_->DoResume();

    subtitleSinkFilter_->frameCnt_ = 1;
    res = subtitleSinkFilter_->DoResume();

    subtitleSinkFilter_->state_ = FilterState::STOPPED;
    res = subtitleSinkFilter_->DoResume();
}

/**
 * @tc.name: DoFlush
 * @tc.desc: DoFlush
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoFlush, TestSize.Level1)
{
    auto res = subtitleSinkFilter_->DoFlush();

    subtitleSinkFilter_subtitleSink_ = nullptr;
    res = subtitleSinkFilter_->DoFlush();
}

/**
 * @tc.name: DoStop
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoStop, TestSize.Level1)
{
    auto res = subtitleSinkFilter_->DoStop();

    subtitleSinkFilter_subtitleSink_ = nullptr;
    res = subtitleSinkFilter_->DoStop();
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS