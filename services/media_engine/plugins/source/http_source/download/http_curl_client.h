/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_HTTP_CURL_CLIENT_H
#define HISTREAMER_HTTP_CURL_CLIENT_H

#include <string>
#include "network_client.h"
#include "curl/curl.h"
#include "osal/task/mutex.h"
#include <list>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class HttpCurlClient : public NetworkClient {
public:
    HttpCurlClient(RxHeader headCallback, RxBody bodyCallback, void* userParam);

    ~HttpCurlClient() override;

    Status Init() override;

    Status Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override;

    Status RequestData(long startPos, int len, NetworkServerErrorCode& serverCode,
                       NetworkClientErrorCode& clientCode) override;

    Status Close() override;

    Status Deinit() override;

private:
    void InitCurlEnvironment(const std::string& url);
    std::string UrlParse(const std::string& url) const;
    void HttpHeaderParse(std::map<std::string, std::string> httpHeader);
    static std::string ClearHeadTailSpace(std::string& str);
    void CheckHeaderKey(std::string setKey, std::string setValue);

private:
    RxHeader rxHeader_;
    RxBody rxBody_;
    void *userParam_;
    CURL* easyHandle_ {nullptr};
    mutable Mutex mutex_;
    std::string userAgent_ {"Harmony OS UA"};
    std::string referer_ {};
};
}
}
}
}
#endif