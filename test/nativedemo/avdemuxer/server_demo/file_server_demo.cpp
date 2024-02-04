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

#include "file_server_demo.h"
#include "demo_log.h"

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

FileServerDemo::FileServerDemo() {}

FileServerDemo::~FileServerDemo()
{
    StopServer();
}

void FileServerDemo::StartServer()
{
    threadPool_ = std::make_unique<ThreadPool>("fileServerThreadPool");
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
    servaddr.sin_port = htons(SERVERPORT);
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
    serverLoop_ = std::make_unique<std::thread>(&FileServerDemo::ServerLoopFunc, this);
}

void FileServerDemo::StopServer()
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

void FileServerDemo::CloseFd(int32_t &connFd, int32_t &fileFd, bool connCond, bool fileCond)
{
    if (connCond) {
        close(connFd);
    }
    if (fileCond) {
        close(fileFd);
    }
}

void FileServerDemo::GetRange(const std::string &recvStr, int32_t &startPos, int32_t &endPos)
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
        std::cout << "Range not found" << std::endl;
        endPos = 0;
    }
}

void FileServerDemo::GetKeepAlive(const std::string &recvStr, int32_t &keep)
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

void FileServerDemo::GetFilePath(const std::string &recvStr, std::string &path)
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

int32_t FileServerDemo::SendRequestSize(int32_t &connFd, int32_t &fileFd, const std::string &recvStr)
{
    int32_t startPos = 0;
    int32_t endPos = 0;
    int32_t ret = 0;
    int32_t fileSize = lseek(fileFd, 0, SEEK_END);
    GetRange(recvStr, startPos, endPos);
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

int32_t FileServerDemo::SetKeepAlive(int32_t &connFd, int32_t &keepAlive, int32_t &keepIdle)
{
    int ret = 0;
    if (keepIdle <= 0) {
        return ret;
    }
    int32_t keepInterval = 1;
    int32_t keepCount = 1;
    ret = setsockopt(connFd, SOL_SOCKET, SO_KEEPALIVE, static_cast<void *>(&keepAlive), sizeof(keepAlive));
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "set SO_KEEPALIVE failed, ret=%d", ret);
    ret = setsockopt(connFd, SOL_TCP, TCP_KEEPIDLE, static_cast<void *>(&keepIdle), sizeof(keepIdle));
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "set TCP_KEEPIDLE failed, ret=%d", ret);
    ret = setsockopt(connFd, SOL_TCP, TCP_KEEPINTVL, static_cast<void *>(&keepInterval), sizeof(keepInterval));
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "set TCP_KEEPINTVL failed, ret=%d", ret);
    ret = setsockopt(connFd, SOL_TCP, TCP_KEEPCNT, static_cast<void *>(&keepCount), sizeof(keepCount));
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == 0, ret, "set TCP_KEEPCNT failed, ret=%d", ret);
    return ret;
}

void FileServerDemo::FileReadFunc(int32_t connFd)
{
    char recvBuff[BUFFER_LNE] = {0};
    int32_t ret = recv(connFd, recvBuff, BUFFER_LNE - 1, 0);
    int32_t fileFd = -1;
    int32_t keepAlive = 1;
    int32_t keepIdle = 10;
    std::string recvStr = std::string(recvBuff);
    std::string path = "";
    if (ret <= 0) {
        CloseFd(connFd, fileFd, true, false);
        return;
    }
    GetKeepAlive(recvStr, keepIdle);
    (void)SetKeepAlive(connFd, keepAlive, keepIdle);
    GetFilePath(recvStr, path);
    if (path == "") {
        std::cout << "Path error, path:" << path << std::endl;
        CloseFd(connFd, fileFd, true, false);
        return;
    }
    fileFd = open(path.c_str(), O_RDONLY);
    if (fileFd == -1) {
        std::cout << "File does not exist, path:" << path << std::endl;
        CloseFd(connFd, fileFd, true, true);
        return;
    }
    int32_t size = SendRequestSize(connFd, fileFd, recvStr);
    while (size > 0) {
        int32_t sendSize = std::min(BUFFER_LNE, size);
        std::vector<uint8_t> fileBuff(sendSize);
        ret = read(fileFd, fileBuff.data(), sendSize);
        DEMO_CHECK_AND_BREAK_LOG(ret > 0, "read file failed, ret=%d", ret);
        size -= ret;
        ret = send(connFd, fileBuff.data(), std::min(ret, sendSize), MSG_NOSIGNAL);
        if (ret <= 0) { // send file buffer failed
            break;
        }
    }
    if (ret > 0) {
        std::string httpContext = "HTTP/2 200 OK\r\nServer:demohttp\r\n";
        send(connFd, httpContext.c_str(), httpContext.size(), MSG_NOSIGNAL);
    }
    CloseFd(connFd, fileFd, true, true);
}

void FileServerDemo::ServerLoopFunc()
{
    while (isRunning_.load()) {
        struct sockaddr_in caddr;
        int32_t len = sizeof(caddr);
        int32_t connFd =
            accept(listenFd_, reinterpret_cast<struct sockaddr *>(&caddr), reinterpret_cast<socklen_t *>(&len));
        if (connFd < 0) {
            continue;
        }
        threadPool_->AddTask([connFd]() { FileReadFunc(connFd); });
    }
}
} // namespace MediaAVCodec
} // namespace OHOS