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

#ifndef FILE_SERVER_DEMO_H
#define FILE_SERVER_DEMO_H

#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <regex>
#include <securec.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <condition_variable>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread_pool.h>
#include "nocopyable.h"


namespace OHOS {
namespace MediaAVCodec {
class FileServerDemo {
public:
    explicit FileServerDemo();
    virtual ~FileServerDemo();
    void StartServer();
    void StopServer();

private:
    static void FileReadFunc(int32_t connFd);
    static void GetRange(std::string &fileName, int32_t &startPos, int32_t &endPos);
    void ServerLoopFunc();
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> serverLoop_ = nullptr;
    std::unique_ptr<ThreadPool> threadPool_ = nullptr;
    int32_t listenFd_ = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif