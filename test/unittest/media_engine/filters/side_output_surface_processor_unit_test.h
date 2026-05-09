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
#ifndef SIDE_OUTPUT_SURFACE_PROCESSOR_UNIT_TEST_H
#define SIDE_OUTPUT_SURFACE_PROCESSOR_UNIT_TEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "side_output_surface_processor.h"
#include "base_video_post_processor.h"

namespace OHOS {
namespace Media {
namespace Pipeline {

class MockPostProcessorCallback : public PostProcessorCallback {
public:
    MockPostProcessorCallback() = default;
    ~MockPostProcessorCallback() override = default;

    MOCK_METHOD(void, OnError, (int32_t errorCode), (override));
    MOCK_METHOD(void, OnOutputFormatChanged, (const Format &format), (override));
    MOCK_METHOD(void, OnInputBufferAvailable, (uint32_t index, std::shared_ptr<AVBuffer> buffer), (override));
    MOCK_METHOD(void, OnOutputBufferAvailable, (uint32_t index, std::shared_ptr<AVBuffer> buffer, bool isInSeekContinous), (override));
};

class MockEventReceiver : public EventReceiver {
public:
    MockEventReceiver() = default;
    MOCK_METHOD(void, OnEvent, (const Event &event), (override));
};

class MockSurfaceBuffer : public SurfaceBuffer {
public:
    MockSurfaceBuffer() = default;
    virtual ~MockSurfaceBuffer() = default;

    MOCK_METHOD(BufferHandle*, GetBufferHandle, (), (const, override));
    MOCK_METHOD(int32_t, GetWidth, (), (const, override));
    MOCK_METHOD(int32_t, GetHeight, (), (const, override));
    MOCK_METHOD(int32_t, GetStride, (), (const, override));
    MOCK_METHOD(int32_t, GetFormat, (), (const, override));
    MOCK_METHOD(uint64_t, GetUsage, (), (const, override));
    MOCK_METHOD(uint64_t, GetPhyAddr, (), (const, override));
    MOCK_METHOD(int32_t, GetFileDescriptor, (), (const, override));
    MOCK_METHOD(uint32_t, GetSize, (), (const, override));
    MOCK_METHOD(void*, GetVirAddr, (), (override));
    MOCK_METHOD(GraphicColorGamut, GetSurfaceBufferColorGamut, (), (const, override));
    MOCK_METHOD(GraphicTransformType, GetSurfaceBufferTransform, (), (const, override));
    MOCK_METHOD(void, SetSurfaceBufferColorGamut, (const GraphicColorGamut& colorGamut), (override));
    MOCK_METHOD(void, SetSurfaceBufferTransform, (const GraphicTransformType& transform), (override));
    MOCK_METHOD(int32_t, GetSurfaceBufferWidth, (), (const, override));
    MOCK_METHOD(int32_t, GetSurfaceBufferHeight, (), (const, override));
    MOCK_METHOD(void, SetSurfaceBufferWidth, (int32_t width), (override));
    MOCK_METHOD(void, SetSurfaceBufferHeight, (int32_t height), (override));
    MOCK_METHOD(uint32_t, GetSeqNum, (), (const, override));
    MOCK_METHOD(void, SetExtraData, (sptr<BufferExtraData> bedata), (override));
    MOCK_METHOD(sptr<BufferExtraData>, GetExtraData, (), (const, override));
    MOCK_METHOD(GSError, WriteToMessageParcel, (MessageParcel &parcel), (override));
    MOCK_METHOD(GSError, ReadFromMessageParcel, (MessageParcel &parcel, std::function<int32_t(MessageParcel&, std::function<int32_t(Parcel&)>)> buffer), (override));
    MOCK_METHOD(void, SetBufferHandle, (BufferHandle *handle), (override));
    MOCK_METHOD(GSError, Alloc, (const BufferRequestConfig &config), (override));
    MOCK_METHOD(GSError, Map, (), (override));
    MOCK_METHOD(GSError, Unmap, (), (override));
    MOCK_METHOD(GSError, FlushCache, (), (override));
    MOCK_METHOD(GSError, InvalidateCache, (), (override));
    MOCK_METHOD(GSError, SetMetadata, (uint32_t key, const std::vector<uint8_t>& value, bool enableCache), (override));
    MOCK_METHOD(GSError, GetMetadata, (uint32_t key, std::vector<uint8_t>& value), (override));
    MOCK_METHOD(GSError, ListMetadataKeys, (std::vector<uint32_t>& keys), (override));
    MOCK_METHOD(GSError, EraseMetadataKey, (uint32_t key), (override));
    MOCK_METHOD(OH_NativeBuffer*, SurfaceBufferToNativeBuffer, (), (override));
    MOCK_METHOD(void, SetAndMergeSyncFence, (const sptr<SyncFence>& syncFence), (override));
    MOCK_METHOD(sptr<SyncFence>, GetSyncFence, (), (const, override));
    MOCK_METHOD(GSError, Alloc, (const BufferRequestConfig &config, const sptr<SurfaceBuffer>& previousBuffer), (override));
};

class MockSurface : public Surface {
public:
    MockSurface() = default;
    virtual ~MockSurface() = default;

    MOCK_METHOD(bool, IsConsumer, (), (const, override));
    MOCK_METHOD(sptr<IBufferProducer>, GetProducer, (), (const, override));
    MOCK_METHOD(GSError, RequestBuffer, (sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence, BufferRequestConfig& config), (override));
    MOCK_METHOD(GSError, FlushBuffer, (sptr<SurfaceBuffer>& buffer, int32_t fence, BufferFlushConfig& config), (override));
    MOCK_METHOD(GSError, CancelBuffer, (sptr<SurfaceBuffer>& buffer), (override));
    MOCK_METHOD(GSError, ReleaseBuffer, (sptr<SurfaceBuffer>& buffer, int32_t fence), (override));
    MOCK_METHOD(GSError, AcquireBuffer, (sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence, int64_t& timestamp, Rect& damage), (override));
    MOCK_METHOD(GSError, AttachBuffer, (sptr<SurfaceBuffer>& buffer), (override));
    MOCK_METHOD(GSError, DetachBuffer, (sptr<SurfaceBuffer>& buffer), (override));
    
    MOCK_METHOD(GSError, SetQueueSize, (uint32_t queueSize), (override));
    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(int32_t, GetDefaultWidth, (), (override));
    MOCK_METHOD(int32_t, GetDefaultHeight, (), (override));
    MOCK_METHOD(GSError, SetDefaultWidthAndHeight, (int32_t width, int32_t height), (override));
    MOCK_METHOD(GSError, SetDefaultUsage, (uint64_t usage), (override));
    MOCK_METHOD(uint64_t, GetDefaultUsage, (), (override));
    
    MOCK_METHOD(GSError, SetUserData, (const std::string &key, const std::string &val), (override));
    MOCK_METHOD(std::string, GetUserData, (const std::string &key), (override));
    MOCK_METHOD(const std::string&, GetName, (), (override));
    MOCK_METHOD(uint64_t, GetUniqueId, (), (const, override));
    
    MOCK_METHOD(GSError, Connect, (), (override));
    MOCK_METHOD(GSError, Disconnect, (), (override));
    MOCK_METHOD(GSError, CleanCache, (bool cleanAll), (override));
    MOCK_METHOD(GSError, GoBackground, (), (override));
    
    MOCK_METHOD(GSError, SetTransform, (GraphicTransformType transform), (override));
    MOCK_METHOD(GraphicTransformType, GetTransform, (), (const, override));
    MOCK_METHOD(GSError, SetScalingMode, (uint32_t sequence, ScalingMode scalingMode), (override));
    MOCK_METHOD(GSError, SetScalingMode, (ScalingMode scalingMode), (override));
    MOCK_METHOD(GSError, SetMetaData, (uint32_t sequence, const std::vector<GraphicHDRMetaData> &metaData), (override));
    MOCK_METHOD(GSError, SetMetaDataSet, (uint32_t sequence, GraphicHDRMetadataKey key, const std::vector<uint8_t> &metaData), (override));
    MOCK_METHOD(GSError, SetTunnelHandle, (const GraphicExtDataHandle *handle), (override));
    MOCK_METHOD(void, Dump, (std::string &result), (const, override));
    
    MOCK_METHOD(GSError, AttachBuffer, (sptr<SurfaceBuffer>& buffer, int32_t timeOut), (override));
    MOCK_METHOD(GSError, RegisterSurfaceDelegator, (sptr<IRemoteObject> client), (override));
    MOCK_METHOD(GSError, RegisterReleaseListener, (OnReleaseFuncWithFence func), (override));
    MOCK_METHOD(GSError, UnRegisterReleaseListener, (), (override));
    MOCK_METHOD(GSError, RegisterUserDataChangeListener, (const std::string &funcName, OnUserDataChangeFunc func), (override));
    MOCK_METHOD(GSError, UnRegisterUserDataChangeListener, (const std::string &funcName), (override));
    MOCK_METHOD(GSError, ClearUserDataChangeListener, (), (override));
    MOCK_METHOD(GSError, AttachBufferToQueue, (sptr<SurfaceBuffer> buffer), (override));
    MOCK_METHOD(GSError, DetachBufferFromQueue, (sptr<SurfaceBuffer> buffer, bool isReserveSlot), (override));
    MOCK_METHOD(GraphicTransformType, GetTransformHint, (), (const, override));
    MOCK_METHOD(GSError, SetTransformHint, (GraphicTransformType transformHint), (override));
    MOCK_METHOD(void, SetBufferHold, (bool hold), (override));
    MOCK_METHOD(GSError, SetSurfaceSourceType, (OHSurfaceSource sourceType), (override));
    MOCK_METHOD(OHSurfaceSource, GetSurfaceSourceType, (), (const, override));
    MOCK_METHOD(GSError, SetSurfaceAppFrameworkType, (std::string appFrameworkType), (override));
    MOCK_METHOD(std::string, GetSurfaceAppFrameworkType, (), (const, override));
    MOCK_METHOD(GSError, UnregisterConsumerListener, (), (override));
};

class TestableSideOutputSurfaceProcessor : public SideOutputSurfaceProcessor {
public:
    using SideOutputSurfaceProcessor::SurfaceBufferInfo;
    using SideOutputSurfaceProcessor::ProcessorState;
    
    using SideOutputSurfaceProcessor::consumerSurface_;
    using SideOutputSurfaceProcessor::producerSurface_;
    using SideOutputSurfaceProcessor::sideSurface_;
    using SideOutputSurfaceProcessor::consumerBufferQueue_;
    using SideOutputSurfaceProcessor::producerBufferQueue_;
    using SideOutputSurfaceProcessor::renderBufferQueue_;
    using SideOutputSurfaceProcessor::pendingSideSrcBuffer_;
    using SideOutputSurfaceProcessor::pendingSideTimestamp_;
    using SideOutputSurfaceProcessor::isRunning_;
    using SideOutputSurfaceProcessor::isInitialized_;
    using SideOutputSurfaceProcessor::isProcessing_;
    using SideOutputSurfaceProcessor::isBufferQueueReady_;
    using SideOutputSurfaceProcessor::requestCfg_;
    using SideOutputSurfaceProcessor::callback_;
    using SideOutputSurfaceProcessor::eventReceiver_;
    using SideOutputSurfaceProcessor::state_;
    using SideOutputSurfaceProcessor::sampleTask_;
    using SideOutputSurfaceProcessor::sampleProcessing_;
    using SideOutputSurfaceProcessor::sampleDone_;
    using SideOutputSurfaceProcessor::sampleResult_;
    using SideOutputSurfaceProcessor::isPaused_;
    using SideOutputSurfaceProcessor::worker_;
    
    using SideOutputSurfaceProcessor::OnConsumerBufferAvailable;
    using SideOutputSurfaceProcessor::OnProducerBufferReleased;
    using SideOutputSurfaceProcessor::GetConsumerAndProducerBuffer;
    using SideOutputSurfaceProcessor::ProcessBuffer;
    using SideOutputSurfaceProcessor::OutputBuffer;
    using SideOutputSurfaceProcessor::RenderOutputBufferInner;
    using SideOutputSurfaceProcessor::FindBufferFromRenderQueue;
    using SideOutputSurfaceProcessor::FlushBufferToProducer;
    using SideOutputSurfaceProcessor::ProcessBuffers;
    using SideOutputSurfaceProcessor::WaitTrigger;
    using SideOutputSurfaceProcessor::ClearBufferQueues;
    using SideOutputSurfaceProcessor::PushBufferToConsumer;
    using SideOutputSurfaceProcessor::DealNotRenderBuffer;
    using SideOutputSurfaceProcessor::CopyBufferToSideSurface;
    using SideOutputSurfaceProcessor::RequestSideBuffer;
    using SideOutputSurfaceProcessor::CopyBufferData;
    using SideOutputSurfaceProcessor::FlushSideBuffer;
    using SideOutputSurfaceProcessor::HandleSampleTask;
    using SideOutputSurfaceProcessor::DealNoImage;
    using SideOutputSurfaceProcessor::GetVideoSample;
};

class SideOutputSurfaceProcessorUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    sptr<MockSurfaceBuffer> CreateMockBufferWithMemory(uint32_t size);
    
    std::shared_ptr<TestableSideOutputSurfaceProcessor> processor_ { nullptr };
    std::shared_ptr<MockPostProcessorCallback> callback_ {nullptr};
    std::shared_ptr<MockEventReceiver> receiver_ {nullptr};
    sptr<MockSurface> mockSurface_ {nullptr};
    sptr<MockSurfaceBuffer> mockBuffer_ {nullptr};
    std::vector<void*> allocatedBuffers_;
};

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // SIDE_OUTPUT_SURFACE_PROCESSOR_UNIT_TEST_H