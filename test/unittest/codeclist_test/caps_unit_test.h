/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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
#ifndef CAPS_UNIT_TEST_H
#define CAPS_UNIT_TEST_H

#include "gtest/gtest.h"
#include "codeclist_mock.h"

namespace OHOS {
namespace MediaAVCodec {
class CapsUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    std::shared_ptr<AVCodecList> avCodecList_ = nullptr;
    void CheckAVDecH264(const std::shared_ptr<VideoCaps> &videoCaps) const;
    void CheckAVDecH263(const std::shared_ptr<VideoCaps> &videoCaps) const;
    void CheckAVDecMpeg2Video(const std::shared_ptr<VideoCaps> &videoCaps) const;
    void CheckAVDecMpeg4(const std::shared_ptr<VideoCaps> &videoCaps) const;
    void CheckAVDecAVC(const std::shared_ptr<VideoCaps> &videoCaps) const;
    void CheckAVEncAVC(const std::shared_ptr<VideoCaps> &videoCaps) const;
    void CheckAVEncMpeg4(const std::shared_ptr<VideoCaps> &videoCaps) const;
    void CheckVideoCaps(const std::shared_ptr<VideoCaps> &videoCaps) const;
    void CheckVideoCapsArray(const std::vector<std::shared_ptr<VideoCaps>> &videoCapsArray) const;
    void CheckAVDecMP3(const std::shared_ptr<AudioCaps> &audioCaps) const;
    void CheckAVDecAAC(const std::shared_ptr<AudioCaps> &audioCaps) const;
    void CheckAVDecVorbis(const std::shared_ptr<AudioCaps> &audioCaps) const;
    void CheckAVDecFlac(const std::shared_ptr<AudioCaps> &audioCaps) const;
    void CheckAVDecOpus(const std::shared_ptr<AudioCaps> &audioCaps) const;
    void CheckAVEncAAC(const std::shared_ptr<AudioCaps> &audioCaps) const;
    void CheckAVEncOpus(const std::shared_ptr<AudioCaps> &audioCaps) const;
    void CheckAudioCaps(const std::shared_ptr<AudioCaps> &audioCaps) const;
    void CheckAVDecVivid(const std::shared_ptr<AudioCaps> &audioCaps) const;
    void CheckAudioCapsArray(const std::vector<std::shared_ptr<AudioCaps>> &audioCapsArray) const;

    std::vector<std::shared_ptr<VideoCaps>> GetVideoDecoderCaps();
    std::vector<std::shared_ptr<VideoCaps>> GetVideoEncoderCaps();
    std::vector<std::shared_ptr<AudioCaps>> GetAudioDecoderCaps();
    std::vector<std::shared_ptr<AudioCaps>> GetAudioEncoderCaps();

    std::string codecMimeKey_;
    std::string bitrateKey_;
    std::string widthKey_;
    std::string heightKey_;
    std::string pixelFormatKey_;
    std::string frameRateKey_;
    std::string channelCountKey_;
    std::string sampleRateKey_;
    bool isHardIncluded_ = false;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif
