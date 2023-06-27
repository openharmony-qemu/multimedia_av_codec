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
    send(listenFd_, stopMsg.c_str(), stopMsg.size(), 0);
    if (serverLoop_ != nullptr && serverLoop_->joinable()) {
        serverLoop_->join();
        serverLoop_.reset();
    }
    threadPool_->Stop();
    close(listenFd_);
    listenFd_ = 0;
}

void FileServerDemo::GetRange(std::string &fileName, int32_t &startPos, int32_t &endPos)
{
    std::regex regexRange("Range:\\sbytes=(\\d+)-(\\d+)?");
    std::smatch matchRange;
    if (std::regex_search(fileName, matchRange, regexRange)) {
        startPos = std::stoi(matchRange[START_INDEX].str());
        endPos = std::stoi(matchRange[END_INDEX].str());
    } else {
        std::cout << "range not found" << std::endl;
        endPos = 0;
    }
}

void FileServerDemo::FileReadFunc(int32_t connFd)
{
    int32_t startPos = 0, endPos = 0;
    char pathBuff[BUFFER_LNE] = {0};
    int32_t ret = recv(connFd, pathBuff, BUFFER_LNE - 1, 0);
    if (ret <= 0) {
        close(connFd);
        return;
    }
    std::string fileName = std::string(pathBuff);
    GetRange(fileName, startPos, endPos);
    int32_t findIndex = fileName.find_first_of("/");
    if (findIndex < 0 || findIndex >= 10) { // 10: expect less than 10
        close(connFd);
        return;
    }
    fileName.erase(fileName.begin(), fileName.begin() + findIndex);
    fileName.erase(fileName.begin() + fileName.find_first_of(" "), fileName.end());
    if (fileName == "") {
        close(connFd);
        return;
    }
    std::string path = SERVER_FILE_PATH;
    path += fileName;
    int32_t fileFd = open(path.c_str(), O_RDONLY);
    if (fileFd == -1) {
        printf("File does not exist");
        close(connFd);
        close(fileFd);
        return;
    }
    int32_t fileSize = lseek(fileFd, 0, SEEK_END);
    int32_t requestDataSize = std::min(endPos, fileSize) - std::max(startPos, 0) + 1;
    int32_t size = requestDataSize;
    if (startPos == 0 && endPos == 0) {
        size = fileSize;
    } else if (endPos < startPos) {
        size = 0;
    }
    if (startPos) {
        ret = lseek(fileFd, startPos, SEEK_SET);
    } else {
        ret = lseek(fileFd, 0, SEEK_SET);
    }
    if (ret < 0) {
        printf("lseek is failed, ret=%d", ret);
        close(connFd);
        close(fileFd);
        return;
    }
    std::string httpContext = "HTTP/1.1 200 OK\r\nServer:testhttp\r\n";
    httpContext += "Content-Length: " + std::to_string(size) + "\r\n\r\n";
    ret = send(connFd, httpContext.c_str(), httpContext.size(), MSG_NOSIGNAL);
    if (ret < 0) {
        printf("send httpContext failed, ret=%d", ret);
        close(connFd);
        close(fileFd);
        return;
    }
    char fileBuff[BUFFER_LNE] = {0};
    while (requestDataSize > 0) {
        ret = read(fileFd, fileBuff, BUFFER_LNE);
        DEMO_CHECK_AND_BREAK_LOG(ret > 0, "read file failed, ret=%d", ret);
        requestDataSize -= ret;
        ret = send(connFd, fileBuff, ret, MSG_NOSIGNAL);
        DEMO_CHECK_AND_BREAK_LOG(ret >= 0, "send file buffer failed, ret=%d", ret);
    }
    close(connFd);
    close(fileFd);
}

void FileServerDemo::ServerLoopFunc()
{
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
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