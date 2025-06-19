
// /*
//  * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
//  * Licensed under the Apache License, Version 2.0 (the "License");
//  * you may not use this file except in compliance with the License.
//  * You may obtain a copy of the License at
//  *
//  *     http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing, software
//  * distributed under the License is distributed on an "AS IS" BASIS,
//  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * See the License for the specific language governing permissions and
//  * limitations under the License.
//  */

// #pragma once
// #include <gmock/gmock.h>
// #include <gtest/gtest.h>
// #include "ffmpeg_demuxer_plugin.h"

// class FFmpegMock
// {
// public:
//     MOCK_METHOD(AVPacket *, av_packet_alloc, ());
//     MOCK_METHOD(AVFormatContext *, avformat_alloc_context, ());

//     static ::testing::NiceMock<FFmpegMock> &GetInstance()
//     {
//         static ::testing::NiceMock<FFmpegMock> instance;
//         return instance;
//     };
// };

// AVPacket *av_packet_alloc(void)
// {
//     printf("mock::av_packet_alloc\n");
//     return FFmpegMock::GetInstance().av_packet_alloc();
// }


// AVFormatContext *avformat_alloc_context(void)
// {
//     printf("mock::avformat_alloc_context\n");
//     return FFmpegMock::GetInstance().avformat_alloc_context();
// }