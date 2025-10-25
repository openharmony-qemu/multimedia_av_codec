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

#include "http_server_fuzz_demo.h"
#include <chrono>
#include <fstream>
#include "unittest_log.h"
#include "http_server_mock.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr int32_t SERVERPORT = 46666;
constexpr int32_t BUFFER_LNE = 4096;
constexpr int32_t DEFAULT_LISTEN = 16;
constexpr int32_t START_INDEX = 1;
constexpr int32_t END_INDEX = 2;
constexpr int32_t THREAD_POOL_MAX_TASKS = 64;
const std::string SERVER_FILE_PATH = "/data/test/media";
} // namespace

HttpServerFuzzDemo::HttpServerFuzzDemo(const uint8_t *data, size_t size)
    : dataPtr_(data), dataBytes_(size) {}

HttpServerFuzzDemo::~HttpServerFuzzDemo()
{
    StopServer();
}

void HttpServerFuzzDemo::StartServer()
{
    StartServer(SERVERPORT);
}

void HttpServerFuzzDemo::StartServer(int32_t port)
{
    threadPool_ = std::make_unique<ThreadPool>("httpServerThreadPool");
    threadPool_->SetMaxTaskNum(THREAD_POOL_MAX_TASKS);
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ == -1) {
        std::cout << "listenFd error" << std::endl;
        return;
    }
    struct sockaddr_in servaddr;
    (void)memset_s(&servaddr, sizeof(servaddr), 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    servaddr.sin_port = htons(port);
    int32_t reuseAddr = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, static_cast<void *>(&reuseAddr), sizeof(int32_t));
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEPORT, static_cast<void *>(&reuseAddr), sizeof(int32_t));
    int32_t flags = fcntl(listenFd_, F_GETFL, 0);
    fcntl(listenFd_, F_SETFL, flags | O_NONBLOCK);

    int32_t ret = bind(listenFd_, reinterpret_cast<struct sockaddr *>(&servaddr), sizeof(servaddr));
    if (ret == -1) {
        std::cout << "bind error" << std::endl;
        return;
    }
    listen(listenFd_, DEFAULT_LISTEN);
    isRunning_.store(true);
    serverLoop_ = std::make_unique<std::thread>(&HttpServerFuzzDemo::ServerLoopFunc, this);
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void HttpServerFuzzDemo::StopServer()
{
    if (!isRunning_.load()) {
        return;
    }
    isRunning_.store(false);
    std::string stopMsg = "Stop Server";
    std::cout << stopMsg << std::endl;
    if (serverLoop_ != nullptr && serverLoop_->joinable()) {
        serverLoop_->join();
        serverLoop_.reset();
    }
    threadPool_->Stop();
    close(listenFd_);
    listenFd_ = 0;
}

void HttpServerFuzzDemo::CloseFd(int32_t &connFd, int32_t &fileFd, bool connCond, bool fileCond)
{
    if (connCond) {
        close(connFd);
    }
    if (fileCond) {
        close(fileFd);
    }
}

void HttpServerFuzzDemo::GetRange(const std::string &recvStr, int32_t &startPos, int32_t &endPos)
{
    std::regex regexRange("Range:\\sbytes=(\\d+)-(\\d+)?");
    std::regex regexDigital("\\d+");
    std::smatch matchVals;
    if (std::regex_search(recvStr, matchVals, regexRange)) {
        std::string startStr = matchVals[START_INDEX].str();
        std::string endStr = matchVals[END_INDEX].str();
        startPos = std::regex_match(startStr, regexDigital) ? std::stoi(startStr) : 0;
        endPos = std::regex_match(endStr, regexDigital) ? std::stoi(endStr) : INT32_MAX;
    } else {
        endPos = 0;
    }
}

void HttpServerFuzzDemo::GetKeepAlive(const std::string &recvStr, int32_t &keep)
{
    std::regex regexRange("Keep-(A|a)live:\\stimeout=(\\d+)");
    std::regex regexDigital("\\d+");
    std::smatch matchVals;
    if (std::regex_search(recvStr, matchVals, regexRange)) {
        std::string keepStr = matchVals[END_INDEX].str();
        keep = std::regex_match(keepStr, regexDigital) ? std::stoi(keepStr) : 0;
    } else {
        std::cout << "Keep-Alive not found" << std::endl;
        keep = 0;
    }
}

void HttpServerFuzzDemo::GetFilePath(const std::string &recvStr, std::string &path)
{
    std::regex regexRange("GET\\s(.+)\\sHTTP");
    std::smatch matchVals;
    if (std::regex_search(recvStr, matchVals, regexRange)) {
        path = matchVals[1].str();
    } else {
        std::cout << "path not found" << std::endl;
        path = "";
    }
    path = SERVER_FILE_PATH + path;
}

int32_t HttpServerFuzzDemo::SendRequestSize(int32_t &connFd, int32_t &fileFd, const std::string &recvStr)
{
    int32_t startPos = 0;
    int32_t endPos = 0;
    int32_t ret = 0;
    int32_t fileSize = lseek(fileFd, 0, SEEK_END);
    GetRange(recvStr, startPos, endPos);
    if (endPos <= 0) {
        endPos = fileSize - 1;
    }
    int32_t size = std::min(endPos, fileSize) - std::max(startPos, 0) + 1;
    if (endPos < startPos) {
        size = 0;
    }
    if (startPos > 0) {
        ret = lseek(fileFd, startPos, SEEK_SET);
    } else {
        ret = lseek(fileFd, 0, SEEK_SET);
    }
    if (ret < 0) {
        std::cout << "lseek is failed, ret=" << ret << std::endl;
        CloseFd(connFd, fileFd, true, true);
        return -1;
    }
    startPos = std::max(startPos, 0);
    endPos = std::min(endPos, fileSize);
    std::stringstream sstr;
    sstr << "HTTP/2 206 Partial Content\r\n";
    sstr << "Server:demohttp\r\n";
    sstr << "Content-Length: " << size << "\r\n";
    sstr << "Content-Range: bytes " << startPos << "-" << endPos << "/" << fileSize << "\r\n\r\n";
    std::string httpContext = sstr.str();
    ret = send(connFd, httpContext.c_str(), httpContext.size(), MSG_NOSIGNAL);
    if (ret <= 0) {
        std::cout << "send httpContext failed, ret=" << ret << std::endl;
        CloseFd(connFd, fileFd, true, true);
        return -1;
    }
    return size;
}

int32_t HttpServerFuzzDemo::SendRequestFuzzSize(int32_t connFd, const std::string &recvStr, int32_t &startPos,
    int32_t &endPos)
{
    int32_t ret = 0;
    int32_t fileSize = dataBytes_;
    int32_t fileFd = -1;
    GetRange(recvStr, startPos, endPos);
    if (endPos <= 0) {
        endPos = fileSize - 1;
    }
    int32_t size = std::min(endPos, fileSize) - std::max(startPos, 0) + 1;
    if (endPos < startPos) {
        size = 0;
    }
    if (size <= 0) {
        std::cout << "startpos " << startPos << ", error size " << size << std::endl;
        CloseFd(connFd, fileFd, true, true);
        return -1;
    }
    startPos = std::max(startPos, 0);
    endPos = std::min(endPos, fileSize);
    std::stringstream sstr;
    sstr << "HTTP/2 206 Partial Content\r\n";
    sstr << "Server:demohttp\r\n";
    sstr << "Content-Length: " << size << "\r\n";
    sstr << "Content-Range: bytes " << startPos << "-" << endPos << "/" << fileSize << "\r\n\r\n";
    std::string httpContext = sstr.str();
    ret = send(connFd, httpContext.c_str(), httpContext.size(), MSG_NOSIGNAL);
    if (ret <= 0) {
        std::cout << "send httpContext failed, ret=" << ret << std::endl;
        CloseFd(connFd, fileFd, true, true);
        return -1;
    }
    return size;
}

int32_t HttpServerFuzzDemo::SetKeepAlive(int32_t &connFd, int32_t &keepAlive, int32_t &keepIdle)
{
    int ret = 0;
    if (keepIdle <= 0) {
        return ret;
    }
    int32_t keepInterval = 1;
    int32_t keepCount = 1;
    ret = setsockopt(connFd, SOL_SOCKET, SO_KEEPALIVE, static_cast<void *>(&keepAlive), sizeof(keepAlive));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "set SO_KEEPALIVE failed, ret=%d", ret);
    ret = setsockopt(connFd, SOL_TCP, TCP_KEEPIDLE, static_cast<void *>(&keepIdle), sizeof(keepIdle));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "set TCP_KEEPIDLE failed, ret=%d", ret);
    ret = setsockopt(connFd, SOL_TCP, TCP_KEEPINTVL, static_cast<void *>(&keepInterval), sizeof(keepInterval));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "set TCP_KEEPINTVL failed, ret=%d", ret);
    ret = setsockopt(connFd, SOL_TCP, TCP_KEEPCNT, static_cast<void *>(&keepCount), sizeof(keepCount));
    UNITTEST_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "set TCP_KEEPCNT failed, ret=%d", ret);
    return ret;
}

int32_t HttpServerFuzzDemo::SendContinue(int32_t connFd, std::string &path, int32_t &fileFd,
    const std::string &recvStr)
{
    int32_t ret = 0;
    fileFd = open(path.c_str(), O_RDONLY);
    if (fileFd == -1) {
        std::cout << "file does not exist, path:" << path << " errno:" << errno << std::endl;
        CloseFd(connFd, fileFd, true, true);
        return -1;
    }
    int32_t size = SendRequestSize(connFd, fileFd, recvStr);
    while (size > 0) {
        int32_t sendSize = std::min(BUFFER_LNE, size);
        std::vector<uint8_t> fileBuff(sendSize);
        ret = read(fileFd, fileBuff.data(), sendSize);
        UNITTEST_CHECK_AND_BREAK_LOG(ret > 0, "read file failed, ret=%d", ret);
        size -= ret;
        ret = send(connFd, fileBuff.data(), std::min(ret, sendSize), MSG_NOSIGNAL);
        if (ret <= 0) {
            std::cout << "send file buffer failed, ret=" << ret << " errno" << errno << std::endl;
            break;
        }
    }
    return ret;
}

bool HttpServerFuzzDemo::SaveFuzzDataToFile(const std::string &path)
{
    std::string pathFile = "/data/local/tmp/";
    pathFile += path.substr(path.rfind("/") + 1);
    std::ofstream handleFile(pathFile.c_str(), std::ios::out | std::ios::binary);
    if (!handleFile.is_open()) {
        std::cout << "save file " << pathFile << " failed, ret = " << errno << std::endl;
        return false;
    }
    handleFile.write(reinterpret_cast<const char*>(dataPtr_), dataBytes_);
    handleFile.close();
    return true;
}

int32_t HttpServerFuzzDemo::SendFuzzData(int32_t connFd, const std::string &path, const std::string &recvStr)
{
    if (dataPtr_ == nullptr || dataBytes_ <= sizeof(int32_t)) {
        std::cout << "No fuzz data, size: " << dataBytes_ << std::endl;
        return -1;
    }
    SaveFuzzDataToFile(path);

    int32_t ret = 0;
    int32_t startPos = 0;
    int32_t endPos = 0;
    int32_t size = SendRequestFuzzSize(connFd, recvStr, startPos, endPos);
    while (size > 0) {
        int32_t remainByte = dataBytes_ - startPos;
        int32_t sendSize = std::min(BUFFER_LNE, remainByte);
        const uint8_t *fileBuff = dataPtr_ + startPos;
        ret = send(connFd, fileBuff, sendSize, MSG_NOSIGNAL);
        if (ret <= 0) {
            std::cout << "send file buffer failed, ret=" << ret << std::endl;
            break;
        }
        size -= ret;
        startPos += ret;
    }
    return ret;
}

void HttpServerFuzzDemo::FileReadFunc(int32_t connFd, HttpServerFuzzDemo* server)
{
    char recvBuff[BUFFER_LNE] = {0};
    int32_t ret = recv(connFd, recvBuff, BUFFER_LNE - 1, 0);
    int32_t fileFd = -1;
    int32_t keepAlive = 1;
    int32_t keepIdle = 10;
    std::string recvStr = std::string(recvBuff);
    std::string path = "";
    if (ret <= 0) {
        std::cout << "recv error, ret=" << ret << std::endl;
        CloseFd(connFd, fileFd, true, false);
        return;
    }
    GetKeepAlive(recvStr, keepIdle);
    (void)server->SetKeepAlive(connFd, keepAlive, keepIdle);
    GetFilePath(recvStr, path);
    if (path == "") {
        std::cout << "Path error, path:" << path << std::endl;
        CloseFd(connFd, fileFd, true, false);
        return;
    } else if (path.rfind(FAKE_FUZZ_M3U8) != std::string::npos) {
        std::cout << "M3u8 path: " << path << std::endl;
        ret = server->SendFuzzData(connFd, path, recvStr);
    } else {
        ret = server->SendContinue(connFd, path, fileFd, recvStr);
    }

    if (ret > 0) {
        std::string httpContext = "HTTP/2 200 OK\r\nServer:demohttp\r\n";
        send(connFd, httpContext.c_str(), httpContext.size(), MSG_NOSIGNAL);
    } else {
        std::string httpContext = "HTTP/2 500 Internal Server Error\r\nServer:demohttp\r\n";
        send(connFd, httpContext.c_str(), httpContext.size(), MSG_NOSIGNAL);
    }
    CloseFd(connFd, fileFd, true, true);
}

void HttpServerFuzzDemo::ServerLoopFunc()
{
    while (isRunning_.load()) {
        struct sockaddr_in caddr;
        int32_t len = sizeof(caddr);
        int32_t connFd =
            accept(listenFd_, reinterpret_cast<struct sockaddr *>(&caddr), reinterpret_cast<socklen_t *>(&len));
        if (connFd < 0) {
            continue;
        }
        threadPool_->AddTask([connFd, this]() { FileReadFunc(connFd, this); });
    }
}
} // namespace MediaAVCodec
} // namespace OHOS