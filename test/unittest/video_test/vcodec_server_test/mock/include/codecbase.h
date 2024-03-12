/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef CODECBASE_MOCK_H
#define CODECBASE_MOCK_H

#include <gmock/gmock.h>
#include <map>
#include <string>
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"
#include "buffer/avbuffer_queue_producer.h"
#include "buffer/avsharedmemorybase.h"
#include "surface.h"

namespace OHOS {
namespace MediaAVCodec {
enum class CallbackFlag : uint8_t {
    MEMORY_CALLBACK = 1,
    BUFFER_CALLBACK,
    INVALID_CALLBACK,
};

class CodecBase;
class CodecBaseMock {
public:
    CodecBaseMock() = default;
    ~CodecBaseMock() = default;

    MOCK_METHOD(void, CodecBaseCtor, ());
    MOCK_METHOD(void, CodecBaseDtor, ());

    MOCK_METHOD(void, CreateFCodecByName, (const std::string &name));
    MOCK_METHOD(std::shared_ptr<CodecBase>, CreateHCodecByName, (const std::string &name));

    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<AVCodecCallback> &callback));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaCodecCallback> &callback));
    MOCK_METHOD(int32_t, Configure, (const Format &format));
    MOCK_METHOD(int32_t, Start, ());
    MOCK_METHOD(int32_t, Stop, ());
    MOCK_METHOD(int32_t, Flush, ());
    MOCK_METHOD(int32_t, Reset, ());
    MOCK_METHOD(int32_t, Release, ());
    MOCK_METHOD(int32_t, SetParameter, (const Format &format));
    MOCK_METHOD(int32_t, GetOutputFormat, (Format & format));
    MOCK_METHOD(int32_t, QueueInputBuffer, (uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag));
    MOCK_METHOD(int32_t, QueueInputBuffer, (uint32_t index));
    MOCK_METHOD(int32_t, ReleaseOutputBuffer, (uint32_t index));

    MOCK_METHOD(int32_t, NotifyEos, ());
    MOCK_METHOD(sptr<Surface>, CreateInputSurface, ());
    MOCK_METHOD(int32_t, SetInputSurface, (sptr<Surface> surface));
    MOCK_METHOD(int32_t, SetOutputSurface, (sptr<Surface> surface));
    MOCK_METHOD(int32_t, RenderOutputBuffer, (uint32_t index));
    MOCK_METHOD(int32_t, SignalRequestIDRFrame, ());
    MOCK_METHOD(int32_t, GetInputFormat, (Format & format));

    /* API11 audio codec interface */
    MOCK_METHOD(int32_t, CreateCodecByName, (const std::string &name));
    MOCK_METHOD(int32_t, Configure, (const std::shared_ptr<Media::Meta> &meta));
    MOCK_METHOD(int32_t, SetParameter, (const std::shared_ptr<Media::Meta> &parameter));
    MOCK_METHOD(int32_t, GetOutputFormat, (std::shared_ptr<Media::Meta> & parameter));
    MOCK_METHOD(int32_t, SetOutputBufferQueue, (const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer));
    MOCK_METHOD(int32_t, Prepare, ());
    MOCK_METHOD(sptr<Media::AVBufferQueueProducer>, GetInputBufferQueue, ());
    MOCK_METHOD(void, ProcessInputBuffer, ());

    std::weak_ptr<AVCodecCallback> codecCb_;
    std::weak_ptr<MediaCodecCallback> videoCb_;
};

class CodecBase {
public:
    static void RegisterMock(std::shared_ptr<CodecBaseMock> &mock);

    CodecBase();
    virtual ~CodecBase();
    virtual int32_t SetCallback(const std::shared_ptr<AVCodecCallback> &callback);
    virtual int32_t SetCallback(const std::shared_ptr<MediaCodecCallback> &callback);
    virtual int32_t Configure(const Format &format);
    virtual int32_t Start();
    virtual int32_t Stop();
    virtual int32_t Flush();
    virtual int32_t Reset();
    virtual int32_t Release();
    virtual int32_t SetParameter(const Format &format);
    virtual int32_t GetOutputFormat(Format &format);
    virtual int32_t QueueInputBuffer(uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag);
    virtual int32_t QueueInputBuffer(uint32_t index);
    virtual int32_t ReleaseOutputBuffer(uint32_t index);

    virtual int32_t NotifyEos();
    virtual sptr<Surface> CreateInputSurface();
    virtual int32_t SetInputSurface(sptr<Surface> surface);
    virtual int32_t SetOutputSurface(sptr<Surface> surface);
    virtual int32_t RenderOutputBuffer(uint32_t index);
    virtual int32_t SignalRequestIDRFrame();
    virtual int32_t GetInputFormat(Format &format);

    /* API11 audio codec interface */
    virtual int32_t CreateCodecByName(const std::string &name);
    virtual int32_t Configure(const std::shared_ptr<Media::Meta> &meta);
    virtual int32_t SetParameter(const std::shared_ptr<Media::Meta> &parameter);
    virtual int32_t GetOutputFormat(std::shared_ptr<Media::Meta> &parameter);
    virtual int32_t SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer);
    virtual int32_t Prepare();
    virtual sptr<Media::AVBufferQueueProducer> GetInputBufferQueue();
    virtual void ProcessInputBuffer();
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECBASE_MOCK_H