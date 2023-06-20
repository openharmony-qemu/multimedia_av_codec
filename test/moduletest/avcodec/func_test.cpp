/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"

#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmuxer.h"
#include "native_avmemory.h"
#include "securec.h"
#include "native_avcodec_videodecoder.h"
#include "native_avcodec_videoencoder.h"
#include "native_avcodec_audiodecoder.h"
#include "native_avcodec_audioencoder.h"
#include "avcodec_audio_channel_layout.h"
#include <iostream>
#include <stdio.h>
#include <string>
#include <fcntl.h>
#include <cmath>
#include <mutex>
#include <queue>
#include <atomic>
#include <thread>
namespace OHOS {
namespace Media {
class DemuxerFuncNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
};

static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static int32_t trackCount;

static OH_AVMuxer *muxer = nullptr;

void DemuxerFuncNdkTest::SetUpTestCase() {}
void DemuxerFuncNdkTest::TearDownTestCase() {}
void DemuxerFuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(3840 * 2160);
    trackCount = 0;
}
void DemuxerFuncNdkTest::TearDown()
{
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }

    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }

    if (memory != nullptr) {
        OH_AVMemory_Destroy(memory);
        memory = nullptr;
    }
    if (source != nullptr) {
        OH_AVSource_Destroy(source);
        source = nullptr;
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
    }
    if (muxer != nullptr) {
        OH_AVMuxer_Destroy(muxer);
        muxer = nullptr;
    }
}

class DecSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<uint32_t> outIdxQueue_;
    std::queue<OH_AVCodecBufferAttr> attrQueue_;
    std::queue<OH_AVMemory *> inBufferQueue_;
    std::queue<OH_AVMemory *> outBufferQueue_;
};

class EncSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<uint32_t> outIdxQueue_;
    std::queue<OH_AVCodecBufferAttr> attrQueue_;
    std::queue<OH_AVMemory *> inBufferQueue_;
    std::queue<OH_AVMemory *> outBufferQueue_;
};

} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

/**
 * @tc.number    : DEMUXER_FUNCTION_0800
 * @tc.name      : create source with fd,avcc mp4
 * @tc.desc      : function test
 */

OH_AVCodec *vdec_ = nullptr;
OH_AVCodec *venc_ = nullptr;

OH_AVCodec *adec_ = nullptr;
OH_AVCodec *aenc_ = nullptr;

DecSignal *Vdec_signal = nullptr;
EncSignal *Venc_signal = nullptr;

DecSignal *Adec_signal = nullptr;
EncSignal *Aenc_signal = nullptr;

std::thread *Vdec_thread;
std::thread *Adec_thread;

std::thread *demux_thread;
std::thread *Venc_thread;
std::thread *Aenc_thread;

std::thread *mux_thread;

int32_t audioTrackIndex = 0;
int32_t videoTrackIndex = 0;

std::unique_ptr<std::thread> outputLoop_;
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
std::queue<uint8_t *> yuvList;
std::queue<uint8_t *> audioList;
uint32_t source_width;
uint32_t source_height;
int outputFD = 0;
uint32_t decode_width;
uint32_t decode_height;

uint8_t state = 0;

static int64_t GetFileSize(const char *fileName)
{
    int64_t fileSize = 0;
    if (fileName != nullptr) {
        struct stat fileStatus {};
        if (stat(fileName, &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}
int64_t GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = (int64_t)now.tv_sec * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

void decError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    DecSignal *signal = static_cast<DecSignal *>(userData);
    if (signal == nullptr)
        return;
    cout << "Error errorCode=" << errorCode << endl;
    signal->inCond_.notify_all();
}

void decFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
    int32_t width = 0;
    int32_t height = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_WIDTH, &width);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_HEIGHT, &height);
    cout << width << "*" << height << endl;
    decode_width = width;
    decode_height = height;
}

void decInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    DecSignal *signal = static_cast<DecSignal *>(userData);
    if (signal == nullptr)
        return;
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

void decOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                        void *userData)
{
    DecSignal *signal = static_cast<DecSignal *>(userData);
    if (signal == nullptr) {
        return;
    }
    signal->outIdxQueue_.push(index);
    signal->attrQueue_.push(*attr);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
}

void CreateVDecoder(uint32_t width, uint32_t height)
{
    vdec_ = OH_VideoDecoder_CreateByName("OMX.hisi.video.decoder.avc");
    // OH.Media.Codec.Decoder.Video.AVC
    // OMX.hisi.video.decoder.avc
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return;
    }
    cout << "width:" << width << "     height:" << height << endl;
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, width);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, height);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, 30);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, 0);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, 1);
    OH_VideoDecoder_Configure(vdec_, format);
    OH_AVFormat_Destroy(format);
    Vdec_signal = new DecSignal();
    if (Vdec_signal == nullptr) {
        cout << "Failed to new DecSignal" << endl;
        return;
    }
    OH_AVCodecAsyncCallback cb_;
    cb_.onError = decError;
    cb_.onStreamChanged = decFormatChanged;
    cb_.onNeedInputData = decInputDataReady;
    cb_.onNeedOutputData = decOutputDataReady;
    OH_VideoDecoder_SetCallback(vdec_, cb_, static_cast<void *>(Vdec_signal));
    OH_VideoDecoder_Start(vdec_);
}

void CreateADecoder()
{
    adec_ = OH_AudioDecoder_CreateByName("OH.Media.Codec.Decoder.Audio.AAC");
    struct OH_AVCodecAsyncCallback cb_;
    cb_.onError = decError;
    cb_.onStreamChanged = decFormatChanged;
    cb_.onNeedInputData = decInputDataReady;
    cb_.onNeedOutputData = decOutputDataReady;
    Adec_signal = new DecSignal();
    OH_AudioDecoder_SetCallback(adec_, cb_, Adec_signal);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32P);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32P);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);
    OH_AudioDecoder_Configure(adec_, format);
    OH_AVFormat_Destroy(format);
    OH_AudioDecoder_Prepare(adec_);
    OH_AudioDecoder_Start(adec_);
}

void VDecOutputFunc()
{
    int32_t yuv_size = source_width * source_height * 3 >> 1;
    while (true) {
        OH_AVCodecBufferAttr attr;
        uint32_t index;
        unique_lock<mutex> lock(Vdec_signal->outMutex_);
        Vdec_signal->outCond_.wait(lock, []() { return Vdec_signal->outIdxQueue_.size() > 0; });
        index = Vdec_signal->outIdxQueue_.front();
        attr = Vdec_signal->attrQueue_.front();

        unique_lock<mutex> lock2(Venc_signal->inMutex_);
        Venc_signal->inCond_.wait(lock2, []() { return Venc_signal->inIdxQueue_.size() > 0; });

        OH_AVMemory *dec_buffer = Vdec_signal->outBufferQueue_.front();

        uint32_t enc_index = Venc_signal->inIdxQueue_.front();
        auto buffer = Venc_signal->inBufferQueue_.front();
        OH_AVCodecBufferAttr enc_attr;

        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            Vdec_signal->outIdxQueue_.pop();
            Vdec_signal->attrQueue_.pop();
            Vdec_signal->outBufferQueue_.pop();

            enc_attr.pts = GetSystemTimeUs();
            enc_attr.size = 0;
            enc_attr.offset = 0;
            enc_attr.flags = AVCODEC_BUFFER_FLAGS_EOS;

            OH_VideoEncoder_PushInputData(venc_, enc_index, enc_attr);
            Venc_signal->inIdxQueue_.pop();
            Venc_signal->inBufferQueue_.pop();
            break;
        }

        enc_attr.pts = GetSystemTimeUs();
        enc_attr.size = yuv_size;
        enc_attr.offset = 0;
        enc_attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
        if (attr.size != yuv_size) {
            // copy Y
            memcpy_s(OH_AVMemory_GetAddr(buffer), source_width * source_height, OH_AVMemory_GetAddr(dec_buffer),
                     source_width * source_height);
            // copy UV
            memcpy_s(OH_AVMemory_GetAddr(buffer) + source_width * source_height, (source_width * source_height / 2),
                     OH_AVMemory_GetAddr(dec_buffer) + decode_width * decode_height,
                     (source_width * source_height / 2));
            OH_VideoEncoder_PushInputData(venc_, enc_index, enc_attr);

        } else {
            memcpy_s(OH_AVMemory_GetAddr(buffer), attr.size, OH_AVMemory_GetAddr(dec_buffer), attr.size);
            OH_VideoEncoder_PushInputData(venc_, enc_index, enc_attr);
        }
        cout<<"vdec "<<endl;
        Venc_signal->inIdxQueue_.pop();
        Venc_signal->inBufferQueue_.pop();
        if (OH_VideoDecoder_FreeOutputData(vdec_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
        }
        Vdec_signal->outIdxQueue_.pop();
        Vdec_signal->attrQueue_.pop();
        Vdec_signal->outBufferQueue_.pop();
    }
    cout << "decode thread end" << endl;
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Destroy(vdec_);
}

void ADecOutputFunc()
{
    bool isFirstFrame = true;
    while (true) {
        unique_lock<mutex> lock(Adec_signal->outMutex_);
        Adec_signal->outCond_.wait(lock, []() { return Adec_signal->outIdxQueue_.size() > 0; });

        uint32_t index = Adec_signal->outIdxQueue_.front();
        OH_AVCodecBufferAttr attr = Adec_signal->attrQueue_.front();
        OH_AVMemory *data = Adec_signal->outBufferQueue_.front();
        if (!data)
            return;
        unique_lock<mutex> lock2(Aenc_signal->inMutex_);
        Aenc_signal->inCond_.wait(lock2, []() { return Aenc_signal->inIdxQueue_.size() > 0; });
        uint32_t in = Aenc_signal->inIdxQueue_.front();
        auto buffer = Aenc_signal->inBufferQueue_.front();
        if (!buffer)
            return;
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            OH_AudioEncoder_PushInputData(aenc_, in, attr);
            Adec_signal->outBufferQueue_.pop();
            Adec_signal->attrQueue_.pop();
            Adec_signal->outIdxQueue_.pop();
            break;
        }
        if (isFirstFrame) {
            attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            isFirstFrame = false;
        }
        cout<<"adec "<<endl;
        OH_AudioEncoder_PushInputData(aenc_, in, attr);
        Aenc_signal->inIdxQueue_.pop();
        Aenc_signal->inBufferQueue_.pop();

        Adec_signal->outBufferQueue_.pop();
        Adec_signal->attrQueue_.pop();
        Adec_signal->outIdxQueue_.pop();

        OH_AVErrCode ret = OH_AudioDecoder_FreeOutputData(adec_, index);

        if (ret != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
    }
    cout << "decode audio finish" << endl;
}

void VEncOutputFunc()
{
    while (true) {
        OH_AVCodecBufferAttr attr;
        uint32_t index;
        unique_lock<mutex> lock(Venc_signal->outMutex_);
        Venc_signal->outCond_.wait(lock, []() { return Venc_signal->outIdxQueue_.size() > 0; });

        index = Venc_signal->outIdxQueue_.front();
        attr = Venc_signal->attrQueue_.front();
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            Venc_signal->outIdxQueue_.pop();
            Venc_signal->attrQueue_.pop();
            Venc_signal->outBufferQueue_.pop();
            state++;
            break;
        }
        auto buffer = Venc_signal->outBufferQueue_.front();
        if(muxer)
            OH_AVMuxer_WriteSample(muxer, videoTrackIndex, buffer, attr);
        if (OH_VideoEncoder_FreeOutputData(venc_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
        }
        Venc_signal->outIdxQueue_.pop();
        Venc_signal->attrQueue_.pop();
        Venc_signal->outBufferQueue_.pop();
    }
    cout << "video encode output end" << endl;
    OH_VideoEncoder_Stop(venc_);
    OH_VideoEncoder_Destroy(venc_);
}

void AEncOutputFunc()
{
    while (true) {
        OH_AVCodecBufferAttr attr;
        uint32_t index;
        unique_lock<mutex> lock(Aenc_signal->outMutex_);
        Aenc_signal->outCond_.wait(lock, []() { return Aenc_signal->outIdxQueue_.size() > 0; });

        index = Aenc_signal->outIdxQueue_.front();
        attr = Aenc_signal->attrQueue_.front();
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            Aenc_signal->outIdxQueue_.pop();
            Aenc_signal->attrQueue_.pop();
            Aenc_signal->outBufferQueue_.pop();
            state++;
            break;
        }
        auto buffer = Aenc_signal->outBufferQueue_.front();
        OH_AVMuxer_WriteSample(muxer, audioTrackIndex, buffer, attr);

        if (OH_AudioEncoder_FreeOutputData(aenc_, index) != AV_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
        }
        Aenc_signal->outIdxQueue_.pop();
        Aenc_signal->attrQueue_.pop();
        Aenc_signal->outBufferQueue_.pop();
    }
    cout << "audio encode output end" << endl;
    OH_AudioEncoder_Stop(aenc_);
    OH_AudioEncoder_Destroy(aenc_);
}

void encError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode=" << errorCode << endl;
}

void encFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
}

void encInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    EncSignal *signal = static_cast<EncSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

void encOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                        void *userData)
{
    EncSignal *signal = static_cast<EncSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outIdxQueue_.push(index);
    signal->attrQueue_.push(*attr);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
}

void CreateVEncoder()
{
    venc_ = OH_VideoEncoder_CreateByName("OMX.hisi.video.encoder.avc");

    Venc_signal = new EncSignal();
    OH_AVCodecAsyncCallback cb_;
    cb_.onError = encError;
    cb_.onStreamChanged = encFormatChanged;
    cb_.onNeedInputData = encInputDataReady;
    cb_.onNeedOutputData = encOutputDataReady;
    OH_VideoEncoder_SetCallback(venc_, cb_, static_cast<void *>(Venc_signal));
    OH_AVFormat *format = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, source_width);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, source_height);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, 30);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 1000000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR);
    cout << OH_AVFormat_DumpInfo(format) << "format" << endl;
    int ret = OH_VideoEncoder_Configure(venc_, format);
    cout << "encode configure ret = " << ret << endl;
    OH_AVFormat_Destroy(format);
    OH_VideoEncoder_Start(venc_);
}

void CreateAEncoder()
{
    venc_ = OH_AudioEncoder_CreateByName("OH.Media.Codec.Encoder.Audio.AAC");
    Aenc_signal = new EncSignal();
    OH_AVCodecAsyncCallback cb_;
    cb_.onError = encError;
    cb_.onStreamChanged = encFormatChanged;
    cb_.onNeedInputData = encInputDataReady;
    cb_.onNeedOutputData = encOutputDataReady;
    OH_AudioEncoder_SetCallback(venc_, cb_, static_cast<void *>(Aenc_signal));
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_F32P);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_F32P);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, 169000);
    int ret = OH_AudioEncoder_Configure(venc_, format);
    cout << "encode configure ret = " << ret << endl;
    OH_AVFormat_Destroy(format);
    OH_AudioEncoder_Start(venc_);
}

void demuxFunc()
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    bool isFirstFrame_ = true;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            if ((audioIsEnd && (index == 1)) || (videoIsEnd && (index == 0))) {
                continue;
            }
            OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr);

            if (index == 1) {
                // TODO:Audio
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    audioIsEnd = true;
                }
                unique_lock<mutex> lock(Adec_signal->inMutex_);
                Adec_signal->inCond_.wait(lock, []() { return Adec_signal->inIdxQueue_.size() > 0; });
                uint32_t in = Adec_signal->inIdxQueue_.front();
                auto buffer = Adec_signal->inBufferQueue_.front();
                uint8_t *fileBuffer = OH_AVMemory_GetAddr(buffer);
                uint8_t *memBuffer = OH_AVMemory_GetAddr(memory);
                memcpy_s(fileBuffer, attr.size, memBuffer, attr.size);
                if (isFirstFrame_) {
                    attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
                    isFirstFrame_ = false;
                } else {
                    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
                }
                OH_AudioDecoder_PushInputData(adec_, in, attr);
                Adec_signal->inIdxQueue_.pop();
                Adec_signal->inBufferQueue_.pop();

            } else if (index == 0) {
                attr.pts = GetSystemTimeUs();
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    videoIsEnd = true;
                    printf("eos\n");
                }
                unique_lock<mutex> lock(Vdec_signal->inMutex_);
                Vdec_signal->inCond_.wait(lock, []() { return Vdec_signal->inIdxQueue_.size() > 0; });
                uint32_t in = Vdec_signal->inIdxQueue_.front();
                auto buffer = Vdec_signal->inBufferQueue_.front();

                uint8_t *fileBuffer = OH_AVMemory_GetAddr(buffer);
                uint8_t *memBuffer = OH_AVMemory_GetAddr(memory);
                memcpy_s(fileBuffer, attr.size, memBuffer, attr.size);
                OH_VideoDecoder_PushInputData(vdec_, in, attr);
                Vdec_signal->inIdxQueue_.pop();
                Vdec_signal->inBufferQueue_.pop();
            }
        }
    }
    cout << "demux thread end" << endl;
    OH_AVDemuxer_Destroy(demuxer);
    OH_AVSource_Destroy(source);
}

void CreateMuxer()
{
    outputFD = open("/data/test/media/output.mp4", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    muxer = OH_AVMuxer_Create(outputFD, AV_OUTPUT_FORMAT_MPEG_4);

    OH_AVFormat *VFormat = OH_VideoEncoder_GetOutputDescription(venc_);
    OH_AVFormat *AFormat = OH_AudioEncoder_GetOutputDescription(aenc_);

    OH_AVFormat_SetIntValue(AFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    OH_AVFormat_SetIntValue(AFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetStringValue(AFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);

    OH_AVMuxer_AddTrack(muxer, &videoTrackIndex, VFormat);
    OH_AVMuxer_AddTrack(muxer, &audioTrackIndex, AFormat);
    OH_AVFormat_Destroy(VFormat);
    OH_AVFormat_Destroy(AFormat);

    OH_AVMuxer_Start(muxer);
}

HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_0800, TestSize.Level0)
{
    const char *file = "/data/test/media/avcc_10sec.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);

    demuxer = OH_AVDemuxer_CreateWithSource(source);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));

    for (int32_t index = 0; index < trackCount; index++) {
        OH_AVDemuxer_SelectTrackByID(demuxer, index);
    }
    int32_t width = 0;
    int32_t height = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    int32_t track_type = 0;
    OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &track_type);
    OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height);
    OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width);
    source_width = width;
    source_height = height;
    CreateVDecoder(width, height);
    //CreateADecoder();
    CreateVEncoder();
    //CreateAEncoder();
    //CreateMuxer();
    //  Demux
    demux_thread = new thread(&demuxFunc);
    demux_thread->detach();
    Vdec_thread = new thread(&VDecOutputFunc);
    Vdec_thread->detach();
    //Adec_thread = new thread(&ADecOutputFunc);
    //Adec_thread->detach();
   
    Venc_thread = new thread(&VEncOutputFunc);
    Venc_thread->detach();
    //Aenc_thread = new thread(&AEncOutputFunc);
    //Aenc_thread->detach();
    
    while (true) {
        if (state == 2)
            break;
        usleep(100000);
    }
    OH_AVMuxer_Stop(muxer);
    close(fd);
    close(outputFD);
    cout << "all module finish" << endl;
}