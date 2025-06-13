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
#ifndef AUDIO_HDICODEC_INNER_UNIT_TEST_H
#define AUDIO_HDICODEC_INNER_UNIT_TEST_H

#include "hdi_codec.h"
#include <gtest/gtest.h>
#include <atomic>
#include <fstream>
#include <thread>
#include <queue>
#include <string>
#include <condition_variable>
#include <mutex>
#include "buffer/avbuffer.h"
#include "v4_0/codec_ext_types.h"
#include "v4_0/icodec_component_manager.h"
#include "avcodec_info.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Hdi {
using namespace OHOS::HDI::Codec::V4_0;

class HdiCodecInner : public HdiCodec {
public:
    HdiCodecInner() = default;
    ~HdiCodecInner() = default;

    std::shared_ptr<HdiCodecInner> GetSharedPtr()
    {
        return std::static_pointer_cast<HdiCodecInner>(shared_from_this());
    }

    uint32_t& GetCompId()
    {
        return componentId_;
    }

    std::string& GetCompName()
    {
        return componentName_;
    }

    sptr<ICodecCallback>& GetCompCb()
    {
        return compCb_;
    }

    sptr<ICodecComponent>& GetCompNode()
    {
        return compNode_;
    }

    sptr<ICodecComponentManager>& GetCompMgr()
    {
        return compMgr_;
    }

    CodecEventType& GetCompEvent()
    {
        return event_;
    }
};


} // namespace Hdi
} // namespace Plugins
} // namespace Media
} // namespace OHOS


#endif // AUDIO_HDICODEC_INNER_UNIT_TEST_H