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
#define HST_LOG_TAG "HttpCurlClient"

#include "http_curl_client.h"
#include <algorithm>
#include <regex>
#include <vector>
#include "common/log.h"
#include "osal/task/autolock.h"
#include "securec.h"
#include "net_conn_client.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
const uint32_t MAX_STRING_LENGTH = 512;
const std::string USER_AGENT = "User-Agent";
const std::string REFER = "Refer";

std::string ToString(const std::list<std::string> &lists, char tab = ',')
{
    std::string str;
    for (auto it = lists.begin(); it != lists.end(); ++it) {
        if (it != lists.begin()) {
            str.append(1, tab);
        }
        str.append(*it);
    }
    return str;
}

std::string InsertCharBefore(const std::string input, char from, char preChar, char nextChar)
{
    std::string output = input;
    char arr[] = {preChar, from};
    unsigned long strSize = sizeof(arr) / sizeof(arr[0]);
    std::string str(arr, strSize);
    int64_t pos = static_cast<int64_t>(output.find(from));
    std::size_t length = output.length();
    if (length == 0) {
        return output;
    }
    while (pos > 0 && pos <= length - 1 && pos != std::string::npos) {
        char nextCharTemp = pos >= (length - 1) ? '\0' : output[pos + 1];
        if (nextChar == '\0' || nextCharTemp == '\0' || nextCharTemp != nextChar) {
            output.replace(pos, 1, str);
            length += (strSize - 1);
        }
        pos = output.find(from, pos + strSize);
    }
    return output;
}

std::string Trim(std::string str)
{
    if (str.empty()) {
        return str;
    }
    while ((!str.empty()) && std::isspace(str[0])) {
        str.erase(0, 1);
    }
    if (str.empty()) {
        return str;
    }
    while ((!str.empty()) && std::isspace(str[str.size() - 1])) {
            str.erase(str.size() - 1, 1);
    }
    return str;
}

std::string GetHostnameFromURL(const std::string &url)
{
    std::string delimiter = "://";
    std::string tempUrl = url;
    size_t posStart = tempUrl.find(delimiter);
    if (posStart != std::string::npos) {
        posStart += delimiter.length();
    } else {
        posStart = 0;
    }
    size_t posEnd = std::min({tempUrl.find(":", posStart), tempUrl.find("/", posStart), tempUrl.find("?", posStart)});
    if (posEnd != std::string::npos) {
        return tempUrl.substr(posStart, posEnd - posStart);
    }
    return tempUrl.substr(posStart);
}

bool IsRegexValid(const std::string &regex)
{
    if (Trim(regex).empty()) {
        return false;
    }
    return regex_match(regex, std::regex("^[a-zA-Z0-9\\-_\\.*]+$"));
}

std::string ReplaceCharacters(const std::string &input)
{
    std::string output = InsertCharBefore(input, '*', '.', '\0');
    output = InsertCharBefore(output, '.', '\\', '*');
    return output;
}

bool IsMatch(const std::string &str, const std::string &patternStr)
{
    if (patternStr.empty()) {
        return false;
    }
    if (patternStr == "*") {
        return true;
    }
    if (!IsRegexValid(patternStr)) {
        return patternStr == str;
    }
    std::regex pattern(ReplaceCharacters(patternStr));
    bool isMatch = patternStr != "" && std::regex_match(str, pattern);
    return isMatch;
}

bool IsExcluded(const std::string &str, const std::string &exclusions, const std::string &split)
{
    if (Trim(exclusions).empty()) {
        return false;
    }
    std::size_t start = 0;
    std::size_t end = exclusions.find(split);
    while (end != std::string::npos) {
        if (end - start > 0 && IsMatch(str, Trim(exclusions.substr(start, end - start)))) {
            return true;
        }
        start = end + 1;
        end = exclusions.find(split, start);
    }
    return IsMatch(str, Trim(exclusions.substr(start)));
}

bool IsHostNameExcluded(const std::string &url, const std::string &exclusions, const std::string &split)
{
    std::string hostName = GetHostnameFromURL(url);
    return IsExcluded(hostName, exclusions, split);
}

void GetHttpProxyInfo(std::string &host, int32_t &port, std::string &exclusions)
{
    using namespace NetManagerStandard;
    NetManagerStandard::HttpProxy httpProxy;
    NetConnClient::GetInstance().GetDefaultHttpProxy(httpProxy);
    host = httpProxy.GetHost();
    port = httpProxy.GetPort();
    exclusions = ToString(httpProxy.GetExclusionList());
}

HttpCurlClient::HttpCurlClient(RxHeader headCallback, RxBody bodyCallback, void *userParam)
    : rxHeader_(headCallback), rxBody_(bodyCallback), userParam_(userParam)
{
    MEDIA_LOG_I("HttpCurlClient ctor");
}

HttpCurlClient::~HttpCurlClient()
{
    MEDIA_LOG_I("~HttpCurlClient dtor");
}

Status HttpCurlClient::Init()
{
    FALSE_LOG(curl_global_init(CURL_GLOBAL_ALL) == CURLE_OK);
    return Status::OK;
}

std::string HttpCurlClient::ClearHeadTailSpace(std::string& str)
{
    if (str.empty()) {
        return str;
    }
    str.erase(0, str.find_first_not_of(" "));
    str.erase(str.find_last_not_of(" ") + 1);
    return str;
}

void HttpCurlClient::CheckHeaderKey(const std::string setKey, const std::string setValue)
{
    if (setKey != USER_AGENT && setKey != REFER) {
        MEDIA_LOG_E("Setted invalid key " PUBLIC_LOG_S " .", setKey.c_str());
        return;
    }
    if (setKey == USER_AGENT) {
        if (!setValue.empty()) {
            userAgent_ = setValue;
            MEDIA_LOG_I("Setted User-Agent: " PUBLIC_LOG_S " success. ", setValue.c_str());
        } else {
            MEDIA_LOG_I("Setted User-Agent failed, value is empty. ");
        }
    } else if (setKey == REFER) {
        if (!setValue.empty()) {
            referer_ = setValue;
            MEDIA_LOG_I("Setted Refer: " PUBLIC_LOG_S " success. ", setValue.c_str());
        } else {
            MEDIA_LOG_I("Setted Refer failed, value is empty. ");
        }
    }
}

void HttpCurlClient::HttpHeaderParse(std::map<std::string, std::string> httpHeader)
{
    if (httpHeader.empty()) {
        MEDIA_LOG_D("Set http header fail, http header is empty.");
        return;
    }
    for (std::map<std::string, std::string>::iterator iter = httpHeader.begin(); iter != httpHeader.end(); iter++) {
        std::string setKey = iter->first;
        std::string setValue = iter->second;
        if (setKey.length() <= MAX_STRING_LENGTH && setValue.length() <= MAX_STRING_LENGTH) {
            ClearHeadTailSpace(setKey);
            CheckHeaderKey(setKey, setValue);
        } else {
            MEDIA_LOG_E("Set httpHeader fail, the length of key or value is too long, more than 512.");
            MEDIA_LOG_E("key: " PUBLIC_LOG_S " value: " PUBLIC_LOG_S, setKey.c_str(), setValue.c_str());
        }
    }
    MEDIA_LOG_D("User-Agent: " PUBLIC_LOG_S " Referer: " PUBLIC_LOG_S, httpHeader["User-Agent"].c_str(),
        httpHeader["Referer"].c_str());
}

Status HttpCurlClient::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader)
{
    easyHandle_ = curl_easy_init();
    FALSE_RETURN_V(easyHandle_ != nullptr, Status::ERROR_NULL_POINTER);
    std::map<std::string, std::string> header = httpHeader;
    HttpHeaderParse(header);
    InitCurlEnvironment(url);
    return Status::OK;
}

Status HttpCurlClient::Close()
{
    AutoLock lock(mutex_);
    MEDIA_LOG_I("Close client");
    curl_easy_setopt(easyHandle_, CURLOPT_TIMEOUT, 1);
    if (easyHandle_) {
        curl_easy_cleanup(easyHandle_);
        easyHandle_ = nullptr;
    }
    return Status::OK;
}

Status HttpCurlClient::Deinit()
{
    if (easyHandle_) {
        curl_easy_cleanup(easyHandle_);
        easyHandle_ = nullptr;
    }
    curl_global_cleanup();
    return Status::OK;
}

void HttpCurlClient::InitCurlEnvironment(const std::string& url)
{
    curl_easy_setopt(easyHandle_, CURLOPT_URL, UrlParse(url).c_str());
    curl_easy_setopt(easyHandle_, CURLOPT_CONNECTTIMEOUT, 2); // 2
    curl_easy_setopt(easyHandle_, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(easyHandle_, CURLOPT_SSL_VERIFYHOST, 0L);
#ifndef CA_DIR
    curl_easy_setopt(easyHandle_, CURLOPT_CAINFO, "/etc/ssl/certs/" "cacert.pem");
#else
    curl_easy_setopt(easyHandle_, CURLOPT_CAINFO, CA_DIR "cacert.pem");
#endif
    curl_easy_setopt(easyHandle_, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(easyHandle_, CURLOPT_FORBID_REUSE, 0L);
    curl_easy_setopt(easyHandle_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easyHandle_, CURLOPT_WRITEFUNCTION, rxBody_);
    curl_easy_setopt(easyHandle_, CURLOPT_WRITEDATA, userParam_);
    curl_easy_setopt(easyHandle_, CURLOPT_HEADERFUNCTION, rxHeader_);
    curl_easy_setopt(easyHandle_, CURLOPT_HEADERDATA, userParam_);
    curl_easy_setopt(easyHandle_, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(easyHandle_, CURLOPT_TCP_KEEPINTVL, 5L); // 5 心跳
    if (url.find(".ts") == std::string::npos) {
        MEDIA_LOG_I("InitCurlEnvironment url: " PUBLIC_LOG_S " .", url.c_str());
        curl_easy_setopt(easyHandle_, CURLOPT_TIMEOUT_MS, 5000L);
    }
    std::string host;
    std::string exclusions;
    int32_t port = 0;
    GetHttpProxyInfo(host, port, exclusions);
    if (!host.empty() && !IsHostNameExcluded(url, exclusions, ",")) {
        MEDIA_LOG_I("InitCurlEnvironment host: " PUBLIC_LOG_S ", port " PUBLIC_LOG_U32 ", exclusions " PUBLIC_LOG_S,
            host.c_str(), port, exclusions.c_str());
        curl_easy_setopt(easyHandle_, CURLOPT_PROXY, host.c_str());
        curl_easy_setopt(easyHandle_, CURLOPT_PROXYPORT, port);
        auto curlTunnelValue = (url.find("https://") != std::string::npos) ? 1L : 0L;
        curl_easy_setopt(easyHandle_, CURLOPT_HTTPPROXYTUNNEL, curlTunnelValue);
        auto proxyType = (host.find("https://") != std::string::npos) ? CURLPROXY_HTTPS : CURLPROXY_HTTP;
        curl_easy_setopt(easyHandle_, CURLOPT_PROXYTYPE, proxyType);
    } else {
        if (host.empty()) {
            MEDIA_LOG_I("InitCurlEnvironment host is empty.");
        }
        if (IsHostNameExcluded(url, exclusions, ",")) {
            MEDIA_LOG_I("InitCurlEnvironment host name is excluded.");
        }
    }
    curl_easy_setopt(easyHandle_, CURLOPT_USERAGENT, userAgent_.c_str());
    if (!referer_.empty()) {
        curl_easy_setopt(easyHandle_, CURLOPT_REFERER, referer_.c_str());
    }
}

std::string HttpCurlClient::UrlParse(const std::string& url) const
{
    std::string s;
    std::regex_replace(std::back_inserter(s), url.begin(), url.end(), std::regex(" "), "%20");
    return s;
}

// RequestData run in HttpDownload thread,
// Open, Close, Deinit run in other thread.
// Should call Open before start HttpDownload thread.
// Should Pause HttpDownload thread then Close, Deinit.
Status HttpCurlClient::RequestData(long startPos, int len, NetworkServerErrorCode& serverCode,
                                   NetworkClientErrorCode& clientCode)
{
    FALSE_RETURN_V(easyHandle_ != nullptr, Status::ERROR_NULL_POINTER);
    if (startPos >= 0) {
        char requestRange[128] = {0};
        if (len > 0) {
            snprintf_s(requestRange, sizeof(requestRange), sizeof(requestRange) - 1, "%ld-%ld",
                       startPos, startPos + len - 1);
        } else {
            snprintf_s(requestRange, sizeof(requestRange), sizeof(requestRange) - 1, "%ld-", startPos);
        }
        MEDIA_LOG_DD("RequestData: requestRange " PUBLIC_LOG_S, requestRange);
        curl_easy_setopt(easyHandle_, CURLOPT_RANGE, requestRange);
    }
    curl_slist *headers {nullptr};
    headers = curl_slist_append(headers, "Connection: Keep-alive");
    headers = curl_slist_append(headers, "Keep-Alive: timeout=120");
    curl_easy_setopt(easyHandle_, CURLOPT_HTTPHEADER, headers);
    
    MEDIA_LOG_D("RequestData: startPos " PUBLIC_LOG_D32 ", len " PUBLIC_LOG_D32, static_cast<int>(startPos), len);
    AutoLock lock(mutex_);
    FALSE_RETURN_V(easyHandle_ != nullptr, Status::ERROR_NULL_POINTER);
    CURLcode returnCode = curl_easy_perform(easyHandle_);
    if (headers != nullptr) {
        curl_slist_free_all(headers);
    }
    clientCode = NetworkClientErrorCode::ERROR_OK;
    serverCode = 0;
    if (returnCode != CURLE_OK) {
        MEDIA_LOG_E("Curl error " PUBLIC_LOG_D32, returnCode);
        if (returnCode == CURLE_WRITE_ERROR) {
            clientCode = NetworkClientErrorCode::ERROR_NOT_RETRY;
        } else if (returnCode == CURLE_COULDNT_CONNECT || returnCode == CURLE_OPERATION_TIMEDOUT) {
            clientCode = NetworkClientErrorCode::ERROR_TIME_OUT;
        } else {
            clientCode = NetworkClientErrorCode::ERROR_UNKNOWN;
        }
        return Status::ERROR_CLIENT;
    } else {
        int64_t httpCode = 0;
        curl_easy_getinfo(easyHandle_, CURLINFO_RESPONSE_CODE, &httpCode);
        if (httpCode >= 400) { // 400
            MEDIA_LOG_E("Http error " PUBLIC_LOG_D64, httpCode);
            serverCode = httpCode;
            return Status::ERROR_SERVER;
        }
    }
    return Status::OK;
}
}
}
}
}