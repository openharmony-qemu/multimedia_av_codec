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

#include <cstdio>
#include <fcntl.h>
#include <iostream>
#include <malloc.h>
#include <string>
#include <thread>
#include <sys/stat.h>
#include <fstream>

#include "avcodec_common.h"
#include "buffer/avsharedmemorybase.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "native_avmagic.h"
#include "native_avmemory.h"
#include "native_avbuffer.h"

#include "capi_demo/avdemuxer_demo.h"
#include "capi_demo/avsource_demo.h"
#include "demo_log.h"
#include "inner_demo/inner_demuxer_demo.h"
#include "inner_demo/inner_source_demo.h"
#include "server_demo/file_server_demo.h"

#include "avdemuxer_demo_runner.h"
#include "media_data_source.h"

using namespace std;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;

static int64_t g_seekTime = 1000;
static int64_t g_startTime = 0;
static int64_t g_loopTime = 20;
static uint32_t g_maxThreadNum = 16;
static vector<string> g_filelist = {"AAC_44100hz_2c.aac",    "ALAC_44100hz_2c.m4a",
                                    "FLAC_44100hz_2c.flac",  "h264_720x480_aac_44100hz_2c.mp4",
                                    "h264_aac_moovlast.mp4", "h265_720x480_aac_44100hz_2c.mp4",
                                    "MPEG_44100hz_2c.mp3",   "MPEGTS_V1920x1080_A44100hz_2c.ts",
                                    "OGG_44100hz_2c.ogg",    "WAV_44100hz_2c.wav"};
static std::string g_filePath;

static int32_t AVSourceReadAt(OH_AVBuffer *data, int32_t length, int64_t pos)
{
    if (data == nullptr) {
        printf("AVSourceReadAt : data is nullptr!\n");
        return MediaDataSourceError::SOURCE_ERROR_IO;
    }

    std::ifstream infile(g_filePath, std::ofstream::binary);
    if (!infile.is_open()) {
        printf("AVSourceReadAt : open file failed! file:%s\n", g_filePath.c_str());
        return MediaDataSourceError::SOURCE_ERROR_IO;  // 打开文件失败
    }

    infile.seekg(0, std::ios::end);
    int64_t fileSize = infile.tellg();
    if (pos >= fileSize) {
        printf("AVSourceReadAt : pos over or equals file size!\n");
        return MediaDataSourceError::SOURCE_ERROR_EOF;  // pos已经是文件末尾位置，无法读取
    }

    if (pos + length > fileSize) {
        length = fileSize - pos;    // pos+length长度超过文件大小时，读取从pos到文件末尾的数据
    }

    infile.seekg(pos, std::ios::beg);
    if (length <= 0) {
        printf("AVSourceReadAt : raed length less than zero!\n");
        return MediaDataSourceError::SOURCE_ERROR_IO;
    }
    char* buffer = new char[length];
    infile.read(buffer, length);
    infile.close();

    errno_t result = memcpy_s(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(data)),
        OH_AVBuffer_GetCapacity(data), buffer, length);
    delete[] buffer;
    if (result != 0) {
        printf("memcpy_s failed!");
        return MediaDataSourceError::SOURCE_ERROR_IO;
    }

    return length;
}

static void TestNativeSeek(OH_AVMemory *sampleMem, int32_t trackCount, std::shared_ptr<AVDemuxerDemo> avDemuxerDemo)
{
    printf("seek to 1s,mode:SEEK_MODE_NEXT_SYNC\n");
    avDemuxerDemo->SeekToTime(g_seekTime, OH_AVSeekMode::SEEK_MODE_NEXT_SYNC); // 测试seek功能
    avDemuxerDemo->ReadAllSamples(sampleMem, trackCount);
    printf("seek to 1s,mode:SEEK_MODE_PREVIOUS_SYNC\n");
    avDemuxerDemo->SeekToTime(g_seekTime, OH_AVSeekMode::SEEK_MODE_PREVIOUS_SYNC);
    avDemuxerDemo->ReadAllSamples(sampleMem, trackCount);
    printf("seek to 1s,mode:SEEK_MODE_CLOSEST_SYNC\n");
    avDemuxerDemo->SeekToTime(g_seekTime, OH_AVSeekMode::SEEK_MODE_CLOSEST_SYNC);
    avDemuxerDemo->ReadAllSamples(sampleMem, trackCount);
    printf("seek to 0s,mode:SEEK_MODE_CLOSEST_SYNC\n");
    avDemuxerDemo->SeekToTime(g_startTime, OH_AVSeekMode::SEEK_MODE_CLOSEST_SYNC);
    avDemuxerDemo->ReadAllSamples(sampleMem, trackCount);
}

static void ShowSourceDescription(OH_AVFormat *oh_trackformat)
{
    int32_t trackType = -1;
    int64_t duration = -1;
    const char* mimeType = nullptr;
    int64_t bitrate = -1;
    int32_t width = -1;
    int32_t height = -1;
    int32_t audioSampleFormat = -1;
    double keyFrameRate = -1;
    int32_t profile = -1;
    int32_t audioChannelCount = -1;
    int32_t audioSampleRate = -1;
    OH_AVFormat_GetIntValue(oh_trackformat, OH_MD_KEY_TRACK_TYPE, &trackType);
    OH_AVFormat_GetLongValue(oh_trackformat, OH_MD_KEY_DURATION, &duration);
    OH_AVFormat_GetStringValue(oh_trackformat, OH_MD_KEY_CODEC_MIME, &mimeType);
    OH_AVFormat_GetLongValue(oh_trackformat, OH_MD_KEY_BITRATE, &bitrate);
    OH_AVFormat_GetIntValue(oh_trackformat, OH_MD_KEY_WIDTH, &width);
    OH_AVFormat_GetIntValue(oh_trackformat, OH_MD_KEY_HEIGHT, &height);
    OH_AVFormat_GetIntValue(oh_trackformat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &audioSampleFormat);
    OH_AVFormat_GetDoubleValue(oh_trackformat, OH_MD_KEY_FRAME_RATE, &keyFrameRate);
    OH_AVFormat_GetIntValue(oh_trackformat, OH_MD_KEY_PROFILE, &profile);
    OH_AVFormat_GetIntValue(oh_trackformat, OH_MD_KEY_AUD_CHANNEL_COUNT, &audioChannelCount);
    OH_AVFormat_GetIntValue(oh_trackformat, OH_MD_KEY_AUD_SAMPLE_RATE, &audioSampleRate);
    printf("===>tracks:%d duration:%" PRId64 " mimeType:%s bitrate:%" PRId64 " width:%d height:%d audioSampleFormat:%d"
        " keyFrameRate:%.2f profile:%d audioChannelCount:%d audioSampleRate:%d\n", trackType, duration, mimeType,
        bitrate, width, height, audioSampleFormat, keyFrameRate, profile, audioChannelCount, audioSampleRate);
}

static void RunNativeDemuxer(const std::string &filePath, const std::string &fileMode)
{
    auto avSourceDemo = std::make_shared<AVSourceDemo>();
    int32_t fd = -1;
    if (fileMode == "1") {
        avSourceDemo->CreateWithURI((char *)(filePath.c_str()));
    } else if (fileMode == "0" || fileMode == "2") {
        if ((fd = open(filePath.c_str(), O_RDONLY)) < 0) {
            printf("open file failed\n");
            return;
        }
        int64_t fileSize = avSourceDemo->GetFileSize(filePath);
        if (fileMode == "0") {
            avSourceDemo->CreateWithFD(fd, 0, fileSize);
        } else if (fileMode == "2") {
            g_filePath = filePath;
            OH_AVDataSource dataSource = {fileSize, AVSourceReadAt};
            avSourceDemo->CreateWithDataSource(&dataSource);
        }
    }

    auto avDemuxerDemo = std::make_shared<AVDemuxerDemo>();
    OH_AVSource *av_source = avSourceDemo->GetAVSource();
    avDemuxerDemo->CreateWithSource(av_source);
    int32_t trackCount = 0;
    int64_t duration = 0;
    OH_AVFormat *oh_avformat = avSourceDemo->GetSourceFormat();
    OH_AVFormat_GetIntValue(oh_avformat, OH_MD_KEY_TRACK_COUNT, &trackCount); // 北向获取sourceformat
    OH_AVFormat_GetLongValue(oh_avformat, OH_MD_KEY_DURATION, &duration);
    printf("====>total tracks:%d duration:%" PRId64 "\n", trackCount, duration);
    for (int32_t i = 0; i < trackCount; i++) {
        OH_AVFormat *oh_trackformat = avSourceDemo->GetTrackFormat(i);
        ShowSourceDescription(oh_trackformat);
        avDemuxerDemo->SelectTrackByID(i); // 添加轨道
    }
    uint32_t buffersize = 10 * 1024 * 1024;
    OH_AVMemory *sampleMem = OH_AVMemory_Create(buffersize); // 创建memory
    avDemuxerDemo->ReadAllSamples(sampleMem, trackCount);
    TestNativeSeek(sampleMem, trackCount, avDemuxerDemo);
    OH_AVMemory_Destroy(sampleMem);
    OH_AVFormat_Destroy(oh_avformat);
    avDemuxerDemo->Destroy();
    avSourceDemo->Destroy();
    if (fileMode == "0" && fd > 0) {
        close(fd);
    }
}

static void RunDrmNativeDemuxer(const std::string &filePath, const std::string &fileMode)
{
    auto avSourceDemo = std::make_shared<AVSourceDemo>();
    int32_t fd = -1;
    if (fileMode == "0") {
        fd = open(filePath.c_str(), O_RDONLY);
        if (fd < 0) {
            printf("open file failed\n");
            return;
        }
        size_t filesize = avSourceDemo->GetFileSize(filePath);
        avSourceDemo->CreateWithFD(fd, 0, filesize);
    } else if (fileMode == "1") {
        avSourceDemo->CreateWithURI((char *)(filePath.c_str()));
    }
    auto avDemuxerDemo = std::make_shared<AVDemuxerDemo>();
    OH_AVSource *av_source = avSourceDemo->GetAVSource();
    avDemuxerDemo->CreateWithSource(av_source);

    // test drm event callback
    avDemuxerDemo->SetDrmAppCallback();

    int32_t trackCount = 0;
    int64_t duration = 0;
    OH_AVFormat *oh_avformat = avSourceDemo->GetSourceFormat();
    OH_AVFormat_GetIntValue(oh_avformat, OH_MD_KEY_TRACK_COUNT, &trackCount); // 北向获取sourceformat
    OH_AVFormat_GetLongValue(oh_avformat, OH_MD_KEY_DURATION, &duration);
    printf("====>total tracks:%d duration:%" PRId64 "\n", trackCount, duration);
    for (int32_t i = 0; i < trackCount; i++) {
        avDemuxerDemo->SelectTrackByID(i); // 添加轨道
    }
    uint32_t buffersize = 10 * 1024 * 1024;
    OH_AVMemory *sampleMem = OH_AVMemory_Create(buffersize); // 创建memory
    avDemuxerDemo->ReadAllSamples(sampleMem, trackCount);
    printf("seek to 1s,mode:SEEK_MODE_NEXT_SYNC\n");
    avDemuxerDemo->SeekToTime(g_seekTime, OH_AVSeekMode::SEEK_MODE_NEXT_SYNC); // 测试seek功能
    avDemuxerDemo->ReadAllSamples(sampleMem, trackCount);

    // test drm GetMediaKeySystemInfos
    avDemuxerDemo->GetMediaKeySystemInfo();

    OH_AVMemory_Destroy(sampleMem);
    OH_AVFormat_Destroy(oh_avformat);
    avDemuxerDemo->Destroy();
    avSourceDemo->Destroy();
    if (fileMode == "0" && fd > 0) {
        close(fd);
    }
}

static void RunInnerSourceDemuxer(const std::string &filePath, const std::string &fileMode)
{
    auto innerSourceDemo = std::make_shared<InnerSourceDemo>();
    int32_t fd = -1;
    if (fileMode == "0") {
        fd = open(filePath.c_str(), O_RDONLY);
        if (fd < 0) {
            printf("open file failed\n");
            return;
        }
        size_t filesize = innerSourceDemo->GetFileSize(filePath);
        innerSourceDemo->CreateWithFD(fd, 0, filesize);
    } else if (fileMode == "1") {
        innerSourceDemo->CreateWithURI(filePath);
    }
    auto innerDemuxerDemo = std::make_shared<InnerDemuxerDemo>();
    innerDemuxerDemo->CreateWithSource(innerSourceDemo->avsource_);
    int32_t trackCount = 0;
    int64_t duration = 0;
    Format source_format = innerSourceDemo->GetSourceFormat();
    source_format.GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, trackCount);
    source_format.GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, duration);
    printf("====>duration:%" PRId64 " total tracks:%d\n", duration, trackCount);
    for (int32_t i = 0; i < trackCount; i++) {
        innerDemuxerDemo->SelectTrackByID(i); // 添加轨道
    }
    innerDemuxerDemo->UnselectTrackByID(0); // 去掉轨道
    innerDemuxerDemo->SelectTrackByID(0);
    uint32_t buffersize = 1024 * 1024;
    std::shared_ptr<AVSharedMemoryBase> sharedMemory =
        std::make_shared<AVSharedMemoryBase>(buffersize, AVSharedMemory::FLAGS_READ_WRITE, "userBuffer");
    sharedMemory->Init();
    innerDemuxerDemo->ReadAllSamples(sharedMemory, trackCount); // demuxer run
    printf("seek to 1s,mode:SEEK_NEXT_SYNC\n");
    innerDemuxerDemo->SeekToTime(g_seekTime, SeekMode::SEEK_NEXT_SYNC); // 测试seek功能
    innerDemuxerDemo->ReadAllSamples(sharedMemory, trackCount);
    printf("seek to 1s,mode:SEEK_PREVIOUS_SYNC\n");
    innerDemuxerDemo->SeekToTime(g_seekTime, SeekMode::SEEK_PREVIOUS_SYNC);
    innerDemuxerDemo->ReadAllSamples(sharedMemory, trackCount);
    printf("seek to 1s,mode:SEEK_CLOSEST_SYNC\n");
    innerDemuxerDemo->SeekToTime(g_seekTime, SeekMode::SEEK_CLOSEST_SYNC);
    innerDemuxerDemo->ReadAllSamples(sharedMemory, trackCount);
    printf("seek to 0s,mode:SEEK_CLOSEST_SYNC\n");
    innerDemuxerDemo->SeekToTime(g_startTime, SeekMode::SEEK_CLOSEST_SYNC);
    innerDemuxerDemo->ReadAllSamples(sharedMemory, trackCount);
    innerDemuxerDemo->Destroy();
    if (fileMode == "0" && fd > 0) {
        close(fd);
    }
}

static void RunNativeDemuxerLoop(const std::string &filePath, const std::string &fileMode)
{
    time_t startTime = 0;
    time_t curTime = 0;
    (void)time(&startTime);
    (void)time(&curTime);
    while (difftime(curTime, startTime) < g_loopTime) {
        RunNativeDemuxer(filePath, fileMode);
        (void)time(&curTime);
    }
    return;
}

static void RunInnerSourceDemuxerLoop(const std::string &filePath, const std::string &fileMode)
{
    time_t startTime = 0;
    time_t curTime = 0;
    (void)time(&startTime);
    (void)time(&curTime);
    while (difftime(curTime, startTime) < g_loopTime) {
        RunInnerSourceDemuxer(filePath, fileMode);
        (void)time(&curTime);
    }
    return;
}

static void RunNativeDemuxerMulti(const std::string &filePath, const std::string &fileMode)
{
    vector<thread> vecThread;
    for (uint32_t i = 0; i < g_maxThreadNum; ++i) {
        vecThread.emplace_back(RunNativeDemuxerLoop, filePath, fileMode);
    }
    for (thread &val : vecThread) {
        val.join();
    }
    return;
}

static void RunInnerSourceDemuxerMulti(const std::string &filePath, const std::string &fileMode)
{
    vector<thread> vecThread;
    for (uint32_t i = 0; i < g_maxThreadNum; ++i) {
        vecThread.emplace_back(RunInnerSourceDemuxerLoop, filePath, fileMode);
    }
    for (thread &val : vecThread) {
        val.join();
    }
    return;
}

static void RunNativeDemuxerAllFormat(const std::string &fileMode)
{
    string pathRoot;
    if (fileMode == "0") {
        pathRoot = "/data/test/media/";
    } else if (fileMode == "1") {
        pathRoot = "http://127.0.0.1:46666/";
    }
    int64_t groupNum = g_filelist.size() / g_maxThreadNum;
    groupNum = (g_loopTime % g_maxThreadNum) == 0 ? groupNum : (groupNum + 1);
    int64_t looptime = g_loopTime / groupNum;
    std::mutex mutexPrint;
    auto loopfunc = [pathRoot, looptime, fileMode, &mutexPrint](uint32_t i) {
        const string filePath = pathRoot + g_filelist[i];
        time_t startTime = 0;
        time_t curTime = 0;
        (void)time(&startTime);
        (void)time(&curTime);
        while (difftime(curTime, startTime) < looptime) {
            RunNativeDemuxer(filePath, fileMode);
            (void)time(&curTime);
        }
        unique_lock<mutex> lock(mutexPrint);
        cout << filePath << " loop done" << endl;
    };
    for (uint32_t index = 0; index < g_filelist.size(); index += g_maxThreadNum) {
        vector<thread> vecThread;
        for (uint32_t i = 0; (i < g_maxThreadNum) && ((index + i) < g_filelist.size()); ++i) {
            vecThread.emplace_back(loopfunc, index + i);
        }
        for (thread &val : vecThread) {
            val.join();
        }
    }
    return;
}

void AVSourceDemuxerDemoCase(void)
{
    cout << "Please select a demuxer demo(default native demuxer demo): " << endl;
    cout << "0:native_demuxer" << endl;
    cout << "1:ffmpeg_demuxer" << endl;
    cout << "2:native_demuxer loop" << endl;
    cout << "3:ffmpeg_demuxer loop" << endl;
    cout << "4:native_demuxer multithread" << endl;
    cout << "5:ffmpeg_demuxer multithread" << endl;
    cout << "6:native_demuxer all format" << endl;
    cout << "7:native_demuxer drm test" << endl;
    string mode;
    string fileMode;
    string filePath;
    std::unique_ptr<FileServerDemo> server = nullptr;
    (void)getline(cin, mode);
    cout << "Please select file path (0) or uri (1) or dataSource (2)" << endl;
    (void)getline(cin, fileMode);
    if (fileMode == "1") {
        server = std::make_unique<FileServerDemo>();
        server->StartServer();
    }
    if (mode != "6") {
        cout << "Please input file path or uri:" << endl;
        (void)getline(cin, filePath);
    }
    if (mode >= "2" && mode <= "6") {
        cout << "Please set the time spent:" << endl;
        cin >> g_loopTime;
    }
    if (mode == "0" || mode == "") {
        RunNativeDemuxer(filePath, fileMode);
    } else if (mode == "1") {
        RunInnerSourceDemuxer(filePath, fileMode);
    } else if (mode == "2") {
        RunNativeDemuxerLoop(filePath, fileMode);
    } else if (mode == "3") {
        RunInnerSourceDemuxerLoop(filePath, fileMode);
    } else if (mode == "4") {
        RunNativeDemuxerMulti(filePath, fileMode);
    } else if (mode == "5") {
        RunInnerSourceDemuxerMulti(filePath, fileMode);
    } else if (mode == "6") {
        RunNativeDemuxerAllFormat(fileMode);
    } else if (mode == "7") {
        RunDrmNativeDemuxer(filePath, fileMode);
    } else {
        printf("select 0 or 1\n");
    }
    if (fileMode == "1") {
        server->StopServer();
    }
}