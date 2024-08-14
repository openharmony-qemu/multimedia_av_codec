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

#ifndef AUDIO_DECODER_FILTER_UNIT_TEST_H
#define AUDIO_DECODER_FILTER_UNIT_TEST_H

#include <cstring>
#include "gtest/gtest.h"

#include "media_codec/media_codec.h"
#include "filter/filter.h"
#include "plugin/plugin_time.h"
#include "foundation/multimedia/drm_framework/services/drm_service/ipc/i_keysession_service.h"
#include "buffer/avbuffer_queue.h"

namespace OHOS {
namespace Media {

class AudioDecoderFilterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
};

class TestEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TestEventReceiver()
    {
        std::cout << "event receiver constructor" << std::endl;
    }

    void OnEvent(const Event &event)
    {
        std::cout << event.srcFilter << std::endl;
    }
};

class TestFilterCallback : public Pipeline::FilterCallback {
public:
    explicit TestFilterCallback()
    {
        std::cout << "filter back constructor" << std::endl;
    }

    Status OnCallback(const std::shared_ptr<Pipeline::Filter>& filter, Pipeline::FilterCallBackCommand cmd, Pipeline::StreamType outType)
    {
        return Status::OK;
    }
};

class TestFilterLinkCallback : public Pipeline::FilterLinkCallback {
public:
    explicit TestFilterLinkCallback()
    {
        std::cout << "filter back constructor" << std::endl;
    }
    void OnLinkedResult(const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnLinkedResult" << std::endl;
    }
    void OnUnlinkedResult(std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnUnlinkedResult" << std::endl;
    }
    void OnUpdatedResult(std::shared_ptr<Meta>& meta)
    {
        std::cout << "call OnUpdatedResult" << std::endl;
    }
};

}
}

#endif