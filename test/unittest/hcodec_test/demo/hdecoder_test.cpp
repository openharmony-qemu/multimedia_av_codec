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

#include "hdecoder_test.h"
#include <thread>
#include "ui/rs_surface_node.h"  // foundation/graphic/graphic_2d/rosen/modules/render_service_client/core/
#include "media_description.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "hcodec_log.h"
#include "command_parser.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace OHOS::Rosen;

HDecoderTest::~HDecoderTest()
{
    if (mWindow != nullptr) {
        mWindow->Destroy();
    }
}

sptr<Surface> HDecoderTest::CreateSurfaceFromWindow()
{
    sptr<WindowOption> option = new WindowOption();
    option->SetWindowRect({0, 0, opt_.dispW, opt_.dispH});
    option->SetWindowType(WindowType::WINDOW_TYPE_FLOAT);
    option->SetWindowMode(WindowMode::WINDOW_MODE_FLOATING);
    sptr<Window> window = Window::Create("DemoWindow", option);
    if (window == nullptr) {
        LOGE("Create Window failed");
        return nullptr;
    }
    shared_ptr<RSSurfaceNode> node = window->GetSurfaceNode();
    if (node == nullptr) {
        LOGE("GetSurfaceNode failed");
        return nullptr;
    }
    sptr<Surface> surface = node->GetSurface();
    if (surface == nullptr) {
        LOGE("GetSurface failed");
        return nullptr;
    }
    window->Show();
    mWindow = window;
    return surface;
}

sptr<Surface> HDecoderTest::CreateSurfaceNormal()
{
    sptr<Surface> consumerSurface = Surface::CreateSurfaceAsConsumer();
    if (consumerSurface == nullptr) {
        LOGE("CreateSurfaceAsConsumer failed");
        return nullptr;
    }
    sptr<IBufferConsumerListener> listener = new Listener(this);
    GSError err = consumerSurface->RegisterConsumerListener(listener);
    if (err != GSERROR_OK) {
        LOGE("RegisterConsumerListener failed");
        return nullptr;
    }
    sptr<IBufferProducer> bufferProducer = consumerSurface->GetProducer();
    sptr<Surface> producerSurface = Surface::CreateSurfaceAsProducer(bufferProducer);
    if (producerSurface == nullptr) {
        LOGE("CreateSurfaceAsProducer failed");
        return nullptr;
    }
    mConsumer = consumerSurface;
    return producerSurface;
}

void HDecoderTest::Listener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t fence;
    int64_t timestamp;
    OHOS::Rect damage;
    GSError err = mTest->mConsumer->AcquireBuffer(buffer, fence, timestamp, damage);
    if (err != GSERROR_OK || buffer == nullptr) {
        LOGW("AcquireBuffer failed");
        return;
    }
    mTest->mConsumer->ReleaseBuffer(buffer, -1);
}

static size_t GenerateRandomNumInRange(size_t rangeStart, size_t rangeEnd)
{
    auto timestamp = chrono::duration_cast<chrono::nanoseconds>(
        chrono::steady_clock::now().time_since_epoch()).count();
    srand(timestamp);
    return rangeStart + rand() % (rangeEnd - rangeStart);
}

void HDecoderTest::Run()
{
    mIfs = ifstream(opt_.inputFile, ios::binary);
    if (!mIfs) {
        LOGE("cannot open %{public}s", opt_.inputFile.c_str());
        return;
    }
    mType = opt_.protocol;
    if (!mDemuxer.LoadNaluListFromPath(opt_.inputFile, mType)) {
        LOGE("no nalu found");
        return;
    }

    string name = (mType == H264) ? "OMX.hisi.video.decoder.avc" : "OMX.hisi.video.decoder.hevc";
    CreateHCodecByName(name, mDecoder);
    if (mDecoder == nullptr) {
        LOGE("create HCodec failed");
        return;
    }
    shared_ptr<CallBack> cb = make_shared<CallBack>(this);
    int32_t err = mDecoder->SetCallback(cb);
    if (err != AVCS_ERR_OK) {
        LOGE("SetCallback failed");
        return;
    }

    if (opt_.bufferType == BufferType::SURFACE) {
        sptr<Surface> surface = CreateSurfaceNormal();
        if (surface == nullptr) {
            return;
        }
        err = mDecoder->SetOutputSurface(surface);
        if (err != AVCS_ERR_OK) {
            LOGE("SetOutputSurface failed");
            return;
        }
    }

    int mockCnt = 0;
    size_t lastSeekTo = 0;
    while (mockCnt++ < opt_.flushCnt) {
        size_t seekAt = GenerateRandomNumInRange(lastSeekTo, mDemuxer.GetTotalNaluCnt());
        size_t seekTo;
        do {
            seekTo = GenerateRandomNumInRange(0, mDemuxer.GetTotalNaluCnt());
        } while (seekTo == seekAt);
        lastSeekTo = seekTo;
        mUserSeekPos.push_back(make_pair(seekAt, seekTo));
        LOGI("Mock flush configure: from (%{public}zu) to (%{public}zu)", seekAt, seekTo);
    }

    Format fmt;
    fmt.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME,
        (mType == H264) ? CodecMimeType::VIDEO_AVC : CodecMimeType::VIDEO_HEVC);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, opt_.dispW);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, opt_.dispH);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, opt_.pixFmt);
    fmt.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, opt_.frameRate);
    fmt.PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, opt_.rotation);
    err = mDecoder->Configure(fmt);
    if (err != AVCS_ERR_OK) {
        LOGE("Configure failed");
        return;
    }
    err = mDecoder->Start();
    if (err != AVCS_ERR_OK) {
        LOGE("Start failed");
        return;
    }
    LOGI("start succ");

    thread flushThread(&HDecoderTest::DealWithUserFlush, this);
    thread outputThread(&HDecoderTest::DealWithOutputLoop, this);
    DealWithInputLoop();
    if (outputThread.joinable()) {
        outputThread.join();
    }
    if (flushThread.joinable()) {
        flushThread.join();
    }

    LOGI("exit thread, begin to stop decoder");
    err = mDecoder->Stop();
    if (err != AVCS_ERR_OK) {
        LOGE("Stop failed");
        return;
    }
    err = mDecoder->Release();
    if (err != AVCS_ERR_OK) {
        LOGE("Release failed");
        return;
    }
}

void HDecoderTest::CallBack::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    LOGI(">>");
}

void HDecoderTest::CallBack::OnOutputFormatChanged(const Format &format)
{
    LOGI(">>");
}

void HDecoderTest::CallBack::OnInputBufferAvailable(uint32_t index)
{
    lock_guard<mutex> lk(mTest->mInputMtx);
    mTest->mInputList.push_back(index);
    mTest->mInputCond.notify_all();
}

void HDecoderTest::CallBack::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    lock_guard<mutex> lk(mTest->mOutputMtx);
    mTest->mOutputList.emplace_back(index, info, flag);
    mTest->mOutputCond.notify_all();
}

optional<std::pair<AVCodecBufferInfo, AVCodecBufferFlag>> HDecoderTest::GetNextSample(
    const shared_ptr<AVSharedMemoryBase>& mem)
{
    if (mem == nullptr) {
        LOGE("AVSharedMemory is null");
        return nullopt;
    }
    uint8_t* dstVa = mem->GetBase();
    int size = mem->GetSize();
    if (dstVa == nullptr || size <= 0) {
        LOGE("invalid va or size");
        return nullopt;
    }
    size_t dstCapacity = static_cast<size_t>(size);

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag;
    NaluUnit nalu;
    {
        lock_guard<mutex> lk(mFlushMtx);
        int offset = -1;
        if (!mNaluSeekPos.empty()) {
            offset = static_cast<int>(mNaluSeekPos.front());
            mNaluSeekPos.pop_front();
        }
        nalu = mDemuxer.GetNext(mType, offset);
        if (nalu.isEos) {
            flag = AVCODEC_BUFFER_FLAG_EOS;
            return std::pair<AVCodecBufferInfo, AVCodecBufferFlag> {info, flag};
        }
        if (nalu.isCsd) {
            flag = AVCODEC_BUFFER_FLAG_CODEC_DATA;
        }
        mFlushCond.notify_all();
    }

    info.size = nalu.endPos - nalu.startPos;
    if ((size_t)info.size > dstCapacity) {
        LOGE("this input buffer has size=%{public}d but dstCapacity=%{public}zu", info.size, dstCapacity);
        return std::nullopt;
    }
    mIfs.seekg(nalu.startPos);
    mIfs.read(reinterpret_cast<char*>(dstVa), info.size);
    info.offset = 0;
    static int64_t pts = 0;
    info.presentationTimeUs = pts;
    pts += 1.0 / opt_.frameRate * 1000000; // 1000000: one second in micro second
    return std::pair<AVCodecBufferInfo, AVCodecBufferFlag> {info, flag};
}

void HDecoderTest::DealWithInputLoop()
{
    while (true) {
        uint32_t inputIdx;
        {
            unique_lock<mutex> lk(mInputMtx);
            if (opt_.timeout == -1) {
                mInputCond.wait(lk, [this] {
                    return !mInputList.empty();
                });
            } else {
                bool waitRes = mInputCond.wait_for(lk, chrono::milliseconds(opt_.timeout), [this] {
                    return !mInputList.empty();
                });
                if (!waitRes) {
                    LOGE("time out");
                    return;
                }
            }
            inputIdx = mInputList.front();
            mInputList.pop_front();
        }
        shared_ptr<AVSharedMemoryBase> buf = mDecoder->GetInputBuffer(inputIdx);
        if (buf == nullptr) {
            LOGE("GetInputBuffer return null");
            continue;
        }
        optional<std::pair<AVCodecBufferInfo, AVCodecBufferFlag>> info = GetNextSample(buf);
        if (!info.has_value()) {
            return;
        }
        int32_t err = mDecoder->QueueInputBuffer(inputIdx, info->first, info->second);
        if (err != AVCS_ERR_OK) {
            LOGE("QueueInputBuffer failed");
            continue;
        }
        if (info->second & AVCODEC_BUFFER_FLAG_EOS) {
            LOGI("got input eos, quit loop");
            break;
        }
    }
}

void HDecoderTest::DealWithUserFlush()
{
    if (opt_.flushCnt <= 0) {
        LOGI("no need flush, quit loop");
        return;
    }
    while (!mUserSeekPos.empty()) {
        size_t seekAt = mUserSeekPos.front().first;
        size_t seekTo = mUserSeekPos.front().second;
        mUserSeekPos.pop_front();
        {
            unique_lock<mutex> lk(mFlushMtx);
            mFlushCond.wait(lk, [this, seekAt] {
                return mNaluSeekPos.empty() && seekAt == mDemuxer.GetNaluCursor();
            });
        }
        LOGI("Mock flush: from (%{public}zu) to (%{public}zu)", seekAt, seekTo);
        int32_t err = mDecoder->Flush();
        if (err != AVCS_ERR_OK) {
            LOGE("Flush failed");
            continue;
        }
        err = mDecoder->Start();
        if (err != AVCS_ERR_OK) {
            LOGE("Start failed after flush");
            continue;
        }
        std::optional<PositionPair> refInfo = mDemuxer.FindReferenceInfo(seekTo);
        if (!refInfo.has_value()) {
            continue;
        }
        {
            lock_guard<mutex> lk(mFlushMtx);
            mNaluSeekPos.push_back(refInfo.value().first);
            mNaluSeekPos.push_back(refInfo.value().second);
        }
    }
    LOGI("flush complete, quit loop");
}

void HDecoderTest::DealWithOutputLoop()
{
    while (true) {
        uint32_t outIdx;
        AVCodecBufferInfo info;
        AVCodecBufferFlag flag;
        {
            unique_lock<mutex> lk(mOutputMtx);
            if (opt_.timeout == -1) {
                mOutputCond.wait(lk, [this] {
                    return !mOutputList.empty();
                });
            } else {
                bool waitRes = mOutputCond.wait_for(lk, chrono::milliseconds(opt_.timeout), [this] {
                    return !mOutputList.empty();
                });
                if (!waitRes) {
                    LOGE("time out");
                    return;
                }
            }
            std::tie(outIdx, info, flag) = mOutputList.front();
            mOutputList.pop_front();
        }
        if (flag & AVCODEC_BUFFER_FLAG_EOS) {
            LOGI("output eos, quit loop");
            break;
        }
        printf("got one output, size=%d, pts=%" PRId64 "\n", info.size, info.presentationTimeUs);
        int32_t ret;
        if (opt_.bufferType == BufferType::SURFACE) {
            ret = mDecoder->RenderOutputBuffer(outIdx);
        } else {
            ret = mDecoder->ReleaseOutputBuffer(outIdx);
        }
        if (ret != AVCS_ERR_OK) {
            LOGE("Render/Release OutputBuffer failed");
        }
    }
}

extern "C" {
int main(int argc, char *argv[])
{
    CommandOpt opt = Parse(argc, argv);
    HDecoderTest test(opt);
    test.Run();
    return 0;
}
}
} // namespace OHOS::MediaAVCodec