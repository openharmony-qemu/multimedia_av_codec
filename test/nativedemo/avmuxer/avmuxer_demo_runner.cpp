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

#include "avmuxer_demo_runner.h"
#include <iostream>
#include <thread>
#include "avmuxer_demo.h"
#include "avmuxer_ffmpeg_demo.h"
#include "avmuxer_engine_demo.h"
#include "native_avmuxer_demo.h"

using namespace std;
using namespace OHOS::MediaAVCodec;

constexpr int RUN_TIME = 60000;
constexpr int DEMO_THREAD_COUNT = 10;

static int RunLoopNativeMuxer(const string &out)
{
    time_t startTime = 0;
    time_t curTime = 0;
    (void)time(&startTime);
    (void)time(&curTime);
    while (difftime(curTime, startTime) < RUN_TIME) {
        RunNativeMuxer(out.c_str());
        (void)time(&curTime);
    }
    return 0;
}

static int RunAVMuxer()
{
    auto avmuxer = std::make_unique<AVMuxerDemo>();
    avmuxer->RunCase();
    cout << "demo avmuxer end" << endl;
    return 0;
}

static int RunAVMuxerWithMultithread()
{
    auto avmuxer = std::make_unique<AVMuxerDemo>();
    avmuxer->RunMultiThreadCase();
    cout << "demo multi thread avmuxer end" << endl;
    return 0;
}

static int RunFfmpegMuxer()
{
    std::unique_ptr<AVMuxerDemoBase> ffmpegMuxer = std::make_unique<AVMuxerFFmpegDemo>();
    ffmpegMuxer->RunCase();
    cout << "demo ffmpegMuxer end" << endl;
    return 0;
}

static int RunEngineMuxer()
{
    std::unique_ptr<AVMuxerDemoBase> muxer = std::make_unique<AVMuxerEngineDemo>();
    muxer->RunCase();
    cout << "demo engine demo end" << endl;
    return 0;
}

static int RunLoopEngineMuxer()
{
    time_t startTime = 0;
    time_t curTime = 0;
    (void)time(&startTime);
    (void)time(&curTime);
    while (difftime(curTime, startTime) < RUN_TIME) {
        RunEngineMuxer();
        (void)time(&curTime);
    }
    return 0;
}

void AVMuxerDemoCase(void)
{
    cout << "Please select a muxer demo(default native muxer demo): " << endl;
    cout << "0:native_muxer" << endl;
    cout << "1:native_muxer loop" << endl;
    cout << "2:native_muxer multithread" << endl;
    cout << "3:inner_muxer" << endl;
    cout << "4:inner_muxer with multithread write" << endl;
    cout << "5:ffmpeg_muxer" << endl;
    cout << "6:engine_muxer" << endl;
    cout << "7:engine_muxer loop" << endl;

    string mode;
    (void)getline(cin, mode);

    if (mode == "0" || mode == "") {
        (void)NativeSelectMode();
        (void)RunNativeMuxer("native_mux");
    } else if (mode == "1") {
        (void)NativeSelectMode();
        (void)RunLoopNativeMuxer("loop_native_mux");
    } else if (mode == "2") {
        (void)NativeSelectMode();
        vector<thread> vecThread;
        for (int i = 0; i < DEMO_THREAD_COUNT; ++i) {
            string out = to_string(i + 1);
            out += "_native_mux";
            vecThread.push_back(thread(RunLoopNativeMuxer, out));
        }
        for (int i = 0; i < DEMO_THREAD_COUNT; ++i) {
            vecThread[i].join();
        }
    } else if (mode == "3") {
        (void)RunAVMuxer();
    } else if (mode == "4") {
        (void)RunAVMuxerWithMultithread();
    } else if (mode == "5") {
        (void)RunFfmpegMuxer();
    } else if (mode == "6") {
        (void)RunEngineMuxer();
    } else if (mode == "7") {
        (void)RunLoopEngineMuxer();
    }
}