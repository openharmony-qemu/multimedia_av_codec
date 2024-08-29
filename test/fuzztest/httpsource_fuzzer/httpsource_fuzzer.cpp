/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include "hls/hls_media_downloader.h"
#include "http/http_media_downloader.h"
#include "dash_segment_downloader.h"

#define FUZZ_PROJECT_NAME "httpsource_fuzzer"
namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

const std::string MP4_SEGMENT_BASE = "http://127.0.0.1:46666/dewu.mp4";

bool HlsMediaDownloaderFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<HlsMediaDownloader> downloader = std::make_shared<HlsMediaDownloader>();
    uint8_t data_[size];
    errno_t res = memcpy_s(data_, size, data, size);
    if (res != 0) {
        return false;
    }
    bool result = downloader->Save(data_, static_cast<uint32_t>(size));
    return result;
}

bool HttpMediaDownloaderFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<HttpMediaDownloader> downloader = std::make_shared<HttpMediaDownloader>("test");
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                              std::shared_ptr<DownloadRequest>& request) {};
    downloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    downloader->Open(MP4_SEGMENT_BASE, httpHeader)
    uint8_t data_[size];
    errno_t res = memcpy_s(data_, size, data, size);
    if (res != 0) {
        return false;
    }
    bool result = downloader->Save(data_, static_cast<uint32_t>(size));
    return result;
}

bool DashSegmentDownloaderFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    std::shared_ptr<DashSegmentDownloader> downloader = std::make_shared<DashSegmentDownloader>(nullptr,
            1, MediaAVCodec::MediaType::MEDIA_TYPE_VID, 10);
    uint8_t data_[size];
    errno_t res = memcpy_s(data_, size, data, size);
    if (res != 0) {
        return false;
    }
    bool result = downloader->Save(data_, static_cast<uint32_t>(size));
    return result;
}

}
}
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::Media::Plugins::HttpPlugin::HlsMediaDownloaderFuzzTest(data, size);
    OHOS::Media::Plugins::HttpPlugin::HttpMediaDownloaderFuzzTest(data, size);
    OHOS::Media::Plugins::HttpPlugin::DashSegmentDownloaderFuzzTest(data, size);
    return 0;
}
