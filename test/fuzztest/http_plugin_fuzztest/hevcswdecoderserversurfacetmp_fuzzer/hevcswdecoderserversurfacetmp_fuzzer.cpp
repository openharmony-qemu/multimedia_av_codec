/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "http/http_media_downloader.h"
#include "test_template.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins::HttpPlugin;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "swdecoderserversurfacetmp_fuzzer"

namespace OHOS {
bool SwdecoderServerSurfaceFuzzTest(uint8_t *data, size_t size)
{
    const std::string avsource_url = "http://127.0.0.1:46666/dewu.mp4";
    constexpr int32_t maxBufferSize = 5 * 1024 * 1024;
    std::shared_ptr<HttpMediaDownloader> httpMediaDownloader =
        std::make_shared<HttpMediaDownloader>(avsource_url, 4, nullptr);  // 4
    auto statusCallback = [] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    httpMediaDownloader->SetStatusCallback(statusCallback);
    std::map<std::string, std::string> httpHeader;
    httpMediaDownloader->Open(avsource_url, httpHeader);
    unsigned char buff[maxBufferSize];
    ReadDataInfo readDataInfo;
    g_baseFuzzData = data;
    g_baseFuzzSize = size;
    g_baseFuzzPos = 0;
    readDataInfo.streamId_ = GetData<int32_t>();
    readDataInfo.wantReadLength_ = GetData<unsigned int>();
    readDataInfo.wantReadLength_ %= maxBufferSize;
    readDataInfo.isEos_ = GetData<bool>();
    bool blockingFlag = GetData<bool>();
    httpMediaDownloader->Read(buff, readDataInfo);
    httpMediaDownloader->SetReadBlockingFlag(blockingFlag);
    return true;
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::SwdecoderServerSurfaceFuzzTest(data, size);
    return 0;
}