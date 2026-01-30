/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef MEDIA_DEMUXER_PTS_FUNCTIONS_UNITTEST_H
#define MEDIA_DEMUXER_PTS_FUNCTIONS_UNITTEST_H

#include "gtest/gtest.h"

#include "mock/avbuffer.h"
#include "mock/source.h"
#include "mock/demuxer_plugin_manager.h"
#include "media_demuxer.h"
#include "stream_demuxer.h"
#include "common/media_source.h"
#include "buffer/avbuffer_queue.h"

namespace OHOS {
namespace Media {
class MediaDemuxerPtsUnitTest : public testing::Test {
public:

    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);

    std::shared_ptr<MediaDemuxer> demuxerPtr_;
};
} // OHOS
} // Media
#endif // MEDIA_DEMUXER_PTS_FUNCTIONS_UNITTEST_H
