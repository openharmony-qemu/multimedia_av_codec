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

#ifndef METADATA_FILTER_UNIT_TEST_H
#define METADATA_FILTER_UNIT_TEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mock/avbuffer.h"
#include "metadata_filter.h"
#include "metadata_filter.cpp"


namespace OHOS {
namespace Media {
class MockSurfaceBuffer;
class MetaDataFilterUnittest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);

protected:
    std::shared_ptr<Pipeline::MetaDataFilter> metaData_{ nullptr };
};
class MockSurface : public Surface {
public:
    MOCK_METHOD(bool, IsConsumer, (), (const, override));
    MOCK_METHOD(sptr<IBufferProducer>, GetProducer, (), (const, override));
    MOCK_METHOD(GSError, AttachBuffer, (sptr<SurfaceBuffer>& buffer), (override));
    MOCK_METHOD(GSError, DetachBuffer, (sptr<SurfaceBuffer>& buffer), (override));
    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(GSError, SetQueueSize, (uint32_t queueSize), (override));
    MOCK_METHOD(int32_t, GetDefaultWidth, (), (override));
    MOCK_METHOD(int32_t, GetDefaultHeight, (), (override));
    MOCK_METHOD(GSError, SetDefaultUsage, (uint64_t usage), (override));
    MOCK_METHOD(uint64_t, GetDefaultUsage, (), (override));
    MOCK_METHOD(GSError, SetUserData, (const std::string &key, const std::string &val), (override));
    MOCK_METHOD(std::string, GetUserData, (const std::string &key), (override));
    MOCK_METHOD(const std::string&, GetName, (), (override));
    MOCK_METHOD(uint64_t, GetUniqueId, (), (const, override));
    MOCK_METHOD(GSError, SetMetaData,
        (uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData), (override));
    MOCK_METHOD(GSError, SetMetaDataSet,
        (uint32_t sequence, GraphicHDRMetadataKey key, const std::vector<uint8_t> &metaData), (override));
    MOCK_METHOD(GSError, SetTunnelHandle, (const GraphicExtDataHandle *handle), (override));
    MOCK_METHOD(void, Dump, (std::string &result), (const, override));
    MOCK_METHOD(GSError, AttachBuffer, (sptr<SurfaceBuffer>& buffer, int32_t timeOut), (override));
    MOCK_METHOD(GSError, RegisterSurfaceDelegator, (sptr<IRemoteObject> client), (override));
    MOCK_METHOD(GSError, AttachBufferToQueue, (sptr<SurfaceBuffer> buffer), (override));
    MOCK_METHOD(GSError, DetachBufferFromQueue, (sptr<SurfaceBuffer> buffer, bool isReserveSlot), (override));
    MOCK_METHOD(GSError, SetScalingMode, (ScalingMode scalingMode), (override));
    MOCK_METHOD(GSError, AcquireBuffer, (sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
        int64_t &timestamp, Rect &damage), (override));
};
} // OHOS
} // Media

#endif