/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include <list>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "network/network_client.h"
#include "network/network_typs.h"
#include "curl/curl.h"
#include "osal/task/mutex.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

std::string ToString(const std::list<std::string> &lists, char tab = ',');
std::string InsertCharBefore(std::string input, char from, char preChar, char nextChar);
std::string Trim(std::string str);
bool IsRegexValid(const std::string &regex);
std::string ReplaceCharacters(const std::string &input);
bool IsMatch(const std::string &str, const std::string &patternStr);
bool IsExcluded(const std::string &str, const std::string &exclusions, const std::string &split);

class HttpCurlClient : public NetworkClient {
public:
    HttpCurlClient(RxHeader headCallback, RxBody bodyCallback, void* userParam);

    ~HttpCurlClient() override;

    Status Init() override;

    Status Open(const std::string& url, const std::map<std::string, std::string>& httpHeader,
                int32_t timeoutMs) override;

    Status RequestData(long startPos, int len, const RequestInfo& requestInfo,
        HandleResponseCbFunc completedCb) override;

    Status Close(bool isAsync) override;

    Status Deinit() override;
    Status GetIp(std::string &ip) override;
    void SetAppUid(int32_t appUid) override;

    struct SocketOwner {
        uid_t uid;
        gid_t gid;
    };
private:
    void InitCurlEnvironment(const std::string& url, int32_t timeoutMs);
    void InitCurProxy(const std::string& url);
    std::string UrlParse(const std::string& url) const;
    void HttpHeaderParse(const std::map<std::string, std::string>& httpHeader);
    static std::string ClearHeadTailSpace(std::string& str);
    void CheckRequestRange(long startPos, int len);
    Status SetIp();

private:
    RxHeader rxHeader_;
    RxBody rxBody_;
    void *userParam_;
    CURL* easyHandle_ {nullptr};
    mutable Mutex mutex_;
    struct curl_slist* headerList_ {nullptr};
    std::string ip_ {};
    bool ipFlag_ {false};
    bool isFirstRequest_ {true};
    bool isFirstOpen_ {true};
    volatile int32_t appUid_ {-1};
};
}
}
}
}
#endif