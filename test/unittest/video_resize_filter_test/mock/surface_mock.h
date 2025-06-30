/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef SURFACE_MOCK_H
#define SURFACE_MOCK_H

#include <gmock/gmock.h>
#include "video_resize_filter.h"

namespace OHOS {
class MockSurface : public Surface {
public:
    MockSurface() = default;
    ~MockSurface() override = default;

    MOCK_METHOD(GSError, GetProducerInitInfo, (ProducerInitInfo &info), (override));
    MOCK_METHOD(bool, IsConsumer, (), (const, override));
    MOCK_METHOD(sptr<IBufferProducer>, GetProducer, (), (const, override));
    MOCK_METHOD(GSError, RequestBuffer, (sptr<SurfaceBuffer>& buffer, int32_t &fence,
        BufferRequestConfig &config), (override));
    MOCK_METHOD(GSError, RequestBuffers, (std::vector<sptr<SurfaceBuffer>> &buffers,
        std::vector<sptr<SyncFence>> &fences, BufferRequestConfig &config), (override));
    MOCK_METHOD(GSError, CancelBuffer, (sptr<SurfaceBuffer>& buffer), (override));
    MOCK_METHOD(GSError, FlushBuffer, (sptr<SurfaceBuffer>& buffer,
        int32_t fence, BufferFlushConfig &config), (override));
    MOCK_METHOD(GSError, AcquireBuffer, (sptr<SurfaceBuffer>& buffer, int32_t &fence,
        int64_t &timestamp, Rect &damage), (override));
    MOCK_METHOD(GSError, ReleaseBuffer, (sptr<SurfaceBuffer>& buffer, int32_t fence), (override));
    MOCK_METHOD(GSError, RequestBuffer, (sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
        BufferRequestConfig &config), (override));
    MOCK_METHOD(GSError, FlushBuffer, (sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
        BufferFlushConfig &config), (override));
    MOCK_METHOD(GSError, AcquireBuffer, (sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
        int64_t &timestamp, Rect &damage), (override));
    MOCK_METHOD(GSError, ReleaseBuffer, (sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence), (override));
    MOCK_METHOD(GSError, AttachBuffer, (sptr<SurfaceBuffer>& buffer), (override));
    MOCK_METHOD(GSError, DetachBuffer, (sptr<SurfaceBuffer>& buffer), (override));
    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(GSError, SetQueueSize, (uint32_t queueSize), (override));
    MOCK_METHOD(GSError, SetDefaultWidthAndHeight, (int32_t width, int32_t height), (override));
    MOCK_METHOD(int32_t, GetDefaultWidth, (), (override));
    MOCK_METHOD(int32_t, GetDefaultHeight, (), (override));
    MOCK_METHOD(GSError, SetDefaultUsage, (uint64_t usage), (override));
    MOCK_METHOD(uint64_t, GetDefaultUsage, (), (override));
    MOCK_METHOD(GSError, SetUserData, (const std::string &key, const std::string &val), (override));
    MOCK_METHOD(std::string, GetUserData, (const std::string &key), (override));
    MOCK_METHOD(const std::string&, GetName, (), (override));
    MOCK_METHOD(uint64_t, GetUniqueId, (), (const, override));
    MOCK_METHOD(GSError, RegisterConsumerListener, (sptr<IBufferConsumerListener>& listener), (override));
    MOCK_METHOD(GSError, RegisterConsumerListener, (IBufferConsumerListenerClazz *listener), (override));
    MOCK_METHOD(GSError, RegisterReleaseListener, (OnReleaseFunc func), (override));
    MOCK_METHOD(GSError, RegisterDeleteBufferListener, (OnDeleteBufferFunc func, bool isForUniRedraw), (override));
    MOCK_METHOD(GSError, UnregisterConsumerListener, (), (override));
    MOCK_METHOD(GSError, CleanCache, (bool cleanAll), (override));
    MOCK_METHOD(GSError, GoBackground, (), (override));
    MOCK_METHOD(GSError, SetTransform, (GraphicTransformType transform), (override));
    MOCK_METHOD(GraphicTransformType, GetTransform, (), (const, override));
    MOCK_METHOD(GSError, Connect, (), (override));
    MOCK_METHOD(GSError, Disconnect, (), (override));
    MOCK_METHOD(GSError, SetScalingMode, (uint32_t sequence, ScalingMode scalingMode), (override));
    MOCK_METHOD(GSError, GetScalingMode, (uint32_t sequence, ScalingMode &scalingMode), (override));
    MOCK_METHOD(GSError, SetMetaData, (uint32_t sequence,
        const std::vector<GraphicHDRMetaData> &metaData), (override));
    MOCK_METHOD(GSError, SetMetaDataSet, (uint32_t sequence, GraphicHDRMetadataKey key,
        const std::vector<uint8_t> &metaData), (override));
    MOCK_METHOD(GSError, QueryMetaDataType, (uint32_t sequence, HDRMetaDataType &type), (const, override));
    MOCK_METHOD(GSError, GetMetaData, (uint32_t sequence,
        std::vector<GraphicHDRMetaData> &metaData), (const, override));
    MOCK_METHOD(GSError, GetMetaDataSet, (uint32_t sequence, GraphicHDRMetadataKey &key,
        std::vector<uint8_t> &metaData), (const, override));
    MOCK_METHOD(GSError, SetTunnelHandle, (const GraphicExtDataHandle *handle), (override));
    MOCK_METHOD(sptr<SurfaceTunnelHandle>, GetTunnelHandle, (), (const, override));
    MOCK_METHOD(GSError, SetPresentTimestamp, (uint32_t sequence,
        const GraphicPresentTimestamp &timestamp), (override));
    MOCK_METHOD(GSError, GetPresentTimestamp, (uint32_t sequence, GraphicPresentTimestampType type,
        int64_t &time), (const, override));
    MOCK_METHOD(void, Dump, (std::string &result), (const, override));
    MOCK_METHOD(int32_t, GetDefaultFormat, (), (override));
    MOCK_METHOD(GSError, SetDefaultFormat, (int32_t format), (override));
    MOCK_METHOD(int32_t, GetDefaultColorGamut, (), (override));
    MOCK_METHOD(GSError, SetDefaultColorGamut, (int32_t colorGamut), (override));
    MOCK_METHOD(sptr<NativeSurface>, GetNativeSurface, (), (override));
    MOCK_METHOD(bool, QueryIfBufferAvailable, (), (override));
    MOCK_METHOD(GSError, FlushBuffer, (sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
        BufferFlushConfigWithDamages &config, bool needLock), (override));
    MOCK_METHOD(GSError, FlushBuffers, (const std::vector<sptr<SurfaceBuffer>> &buffers,
        const std::vector<sptr<SyncFence>> &fences,
        const std::vector<BufferFlushConfigWithDamages> &configs), (override));
    MOCK_METHOD(GSError, UnRegisterReleaseListener, (), (override));
    MOCK_METHOD(GSError, UnRegisterReleaseListenerBackup, (), (override));
    MOCK_METHOD(GSError, SetWptrNativeWindowToPSurface, (void* nativeWindow), (override));
    MOCK_METHOD(GSError, GetLastFlushedBuffer, (sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
        float matrix[16], bool isUseNewMatrix), (override));
    MOCK_METHOD(GSError, AttachBuffer, (sptr<SurfaceBuffer>& buffer, int32_t timeOut), (override));
    MOCK_METHOD(GSError, RegisterSurfaceDelegator, (sptr<IRemoteObject> client), (override));
    MOCK_METHOD(GSError, RegisterReleaseListener, (OnReleaseFuncWithFence func), (override));
    MOCK_METHOD(GSError, RegisterReleaseListenerBackup, (OnReleaseFuncWithFence func), (override));
    MOCK_METHOD(GSError, RegisterUserDataChangeListener, (const std::string &funcName,
        OnUserDataChangeFunc func), (override));
    MOCK_METHOD(GSError, UnRegisterUserDataChangeListener, (const std::string &funcName), (override));
    MOCK_METHOD(GSError, ClearUserDataChangeListener, (), (override));
    MOCK_METHOD(GSError, AttachBufferToQueue, (sptr<SurfaceBuffer> buffer), (override));
    MOCK_METHOD(GSError, DetachBufferFromQueue, (sptr<SurfaceBuffer> buffer, bool isReserveSlot), (override));
    MOCK_METHOD(GraphicTransformType, GetTransformHint, (), (const, override));
    MOCK_METHOD(GSError, SetTransformHint, (GraphicTransformType transformHint), (override));
    MOCK_METHOD(GSError, SetBufferName, (const std::string &name), (override));
    MOCK_METHOD(void, SetRequestWidthAndHeight, (int32_t width, int32_t height), (override));
    MOCK_METHOD(int32_t, GetRequestWidth, (), (override));
    MOCK_METHOD(int32_t, GetRequestHeight, (), (override));
    MOCK_METHOD(void, SetBufferHold, (bool hold), (override));
    MOCK_METHOD(void, SetWindowConfig, (const BufferRequestConfig& config), (override));
    MOCK_METHOD(void, SetWindowConfigWidthAndHeight, (int32_t width, int32_t height), (override));
    MOCK_METHOD(void, SetWindowConfigStride, (int32_t stride), (override));
    MOCK_METHOD(void, SetWindowConfigFormat, (int32_t format), (override));
    MOCK_METHOD(void, SetWindowConfigUsage, (uint64_t usage), (override));
    MOCK_METHOD(void, SetWindowConfigTimeout, (int32_t timeout), (override));
    MOCK_METHOD(void, SetWindowConfigColorGamut, (GraphicColorGamut colorGamut), (override));
    MOCK_METHOD(void, SetWindowConfigTransform, (GraphicTransformType transform), (override));
    MOCK_METHOD(BufferRequestConfig, GetWindowConfig, (), (override));
    MOCK_METHOD(GSError, SetScalingMode, (ScalingMode scalingMode), (override));
    MOCK_METHOD(GSError, SetSurfaceSourceType, (OHSurfaceSource sourceType), (override));
    MOCK_METHOD(OHSurfaceSource, GetSurfaceSourceType, (), (const, override));
    MOCK_METHOD(GSError, SetSurfaceAppFrameworkType, (std::string appFrameworkType), (override));
    MOCK_METHOD(std::string, GetSurfaceAppFrameworkType, (), (const, override));
    MOCK_METHOD(GSError, SetHdrWhitePointBrightness, (float brightness), (override));
    MOCK_METHOD(GSError, SetSdrWhitePointBrightness, (float brightness), (override));
    MOCK_METHOD(GSError, AcquireLastFlushedBuffer, (sptr<SurfaceBuffer> &buffer, sptr<SyncFence> &fence,
        float matrix[16], uint32_t matrixSize, bool isUseNewMatrix), (override));
    MOCK_METHOD(GSError, ReleaseLastFlushedBuffer, (sptr<SurfaceBuffer> buffer), (override));
    MOCK_METHOD(GSError, SetGlobalAlpha, (int32_t alpha), (override));
    MOCK_METHOD(GSError, SetRequestBufferNoblockMode, (bool noblock), (override));
    MOCK_METHOD(bool, IsInHebcList, (), (override));
    MOCK_METHOD(GSError, RequestAndDetachBuffer, (sptr<SurfaceBuffer>& buffer, sptr<SyncFence>& fence,
        BufferRequestConfig& config), (override));
    MOCK_METHOD(GSError, AttachAndFlushBuffer, (sptr<SurfaceBuffer>& buffer, const sptr<SyncFence>& fence,
        BufferFlushConfig& config, bool needMap), (override));
    MOCK_METHOD(GSError, GetBufferCacheConfig, (const sptr<SurfaceBuffer>& buffer,
        BufferRequestConfig& config), (override));
    MOCK_METHOD(GSError, GetCycleBuffersNumber, (uint32_t& cycleBuffersNumber), (override));
    MOCK_METHOD(GSError, SetCycleBuffersNumber, (uint32_t cycleBuffersNumber), (override));
    MOCK_METHOD(GSError, GetFrameGravity, (int32_t &frameGravity), (override));
    MOCK_METHOD(GSError, SetFrameGravity, (int32_t frameGravity), (override));
    MOCK_METHOD(GSError, GetFixedRotation, (int32_t &fixedRotation), (override));
    MOCK_METHOD(GSError, SetFixedRotation, (int32_t fixedRotation), (override));
    MOCK_METHOD(GSError, ConnectStrictly, (), (override));
    MOCK_METHOD(GSError, DisconnectStrictly, (), (override));
    MOCK_METHOD(GSError, PreAllocBuffers, (const BufferRequestConfig &config, uint32_t allocBufferCount), (override));
    MOCK_METHOD(GSError, ProducerSurfaceLockBuffer, (BufferRequestConfig &config, Region region,
        sptr<SurfaceBuffer>& buffer), (override));
    MOCK_METHOD(GSError, ProducerSurfaceUnlockAndFlushBuffer, (), (override));
    MOCK_METHOD(GSError, ReleaseBuffer, (uint32_t sequence, const sptr<SyncFence>& fence), (override));
};
} // namespace OHOS
#endif // SURFACE_MOCK_H