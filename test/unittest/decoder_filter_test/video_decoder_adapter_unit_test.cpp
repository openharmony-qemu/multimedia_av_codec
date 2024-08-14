/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "video_decoder_adapter_unit_test.h"
#include <malloc.h>
#include <map>
#include <unistd.h>
#include <vector>
#include "avcodec_video_decoder.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "common/log.h"
#include "media_description.h"
#include "surface_type.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "meta/meta_key.h"
#include "meta/meta.h"
#include "video_decoder_adapter.h"
#include "media_core.h"
#include "avcodec_sysevent.h"

namespace OHOS::Media {

using namespace std;
using namespace testing::ext;
void VideoDecoderAdapterUnitTest::SetUpTestCase(void)
{
}

void VideoDecoderAdapterUnitTest::TearDownTestCase(void)
{
}

void VideoDecoderAdapterUnitTest::SetUp()
{
}

void VideoDecoderAdapterUnitTest::TearDown()
{
}

HWTEST_F(VideoDecoderAdapterUnitTest, VideoDecoderAdapter_002, TestSize.Level1)
{
    std::shared_ptr<VideoDecoderAdapter> videoResize = std::make_shared<VideoDecoderAdapter>();
    Status ret = videoResize->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, true, "name");
    EXPECT_EQ(ret, Status::ERROR_INVALID_STATE);
}

}