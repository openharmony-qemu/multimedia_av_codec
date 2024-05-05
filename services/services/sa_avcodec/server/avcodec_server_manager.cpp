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

#include "avcodec_server_manager.h"
#include <thread>
#include <unistd.h>
#include <codecvt>
#include <locale>
#include "avcodec_trace.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_xcollie.h"
#include "avcodec_dump_utils.h"
#ifdef SUPPORT_CODEC
#include "codec_service_stub.h"
#endif
#ifdef SUPPORT_CODECLIST
#include "codeclist_service_stub.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecServerManager"};
constexpr uint32_t DUMP_MENU_INDEX = 0x01000000;
constexpr uint32_t DUMP_INSTANCE_INDEX = 0x01010000;
constexpr uint32_t DUMP_PID_INDEX = 0x01010100;
constexpr uint32_t DUMP_UID_INDEX = 0x01010200;
constexpr uint32_t DUMP_OFFSET_16 = 16;

const std::vector<const std::string> SA_DUMP_MENU_DUMP_TABLE = {
    "All", "Codec"
};

const std::map<OHOS::MediaAVCodec::AVCodecServerManager::StubType, const std::string> STUB_TYPE_STRING_MAP = {
    { OHOS::MediaAVCodec::AVCodecServerManager::StubType::CODEC,  "Codec"}
};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
AVCodecServerManager& AVCodecServerManager::GetInstance()
{
    static AVCodecServerManager instance;
    return instance;
}

void WriteFd(int32_t fd, std::string& str)
{
    if (fd != -1) {
        write(fd, str.c_str(), str.size());
    }
    str.clear();
}

int32_t WriteInfo(int32_t fd, std::string& dumpString, std::vector<Dumper> dumpers, bool needDetail)
{
    WriteFd(fd, dumpString);

    if (needDetail) {
        int32_t instanceIndex = 0;
        for (auto iter : dumpers) {
            AVCodecDumpControler dumpControler;
            dumpControler.AddInfo(DUMP_INSTANCE_INDEX, std::string("Instance_") +
                std::to_string(instanceIndex++) + "_Info");
            dumpControler.AddInfo(DUMP_PID_INDEX, "PID", std::to_string(iter.pid_));
            dumpControler.AddInfo(DUMP_UID_INDEX, "UID", std::to_string(iter.uid_));
            dumpControler.GetDumpString(dumpString);
            WriteFd(fd, dumpString);

            int32_t ret = iter.entry_(fd);
            std::string lineBreak = "\n";
            WriteFd(fd, lineBreak);
            CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, OHOS::INVALID_OPERATION, "Call dumper callback failed");
        }
    }

    return OHOS::NO_ERROR;
}

void AVCodecServerManager::DumpServer(int32_t fd, StubType stubType, std::unordered_set<std::u16string> &argSets)
{
    auto itServeName = STUB_TYPE_STRING_MAP.find(stubType);
    CHECK_AND_RETURN_LOG(itServeName != STUB_TYPE_STRING_MAP.end(), "Get a unknow stub type!");

    std::string ServeName = itServeName->second;
    std::string dumpString = "[" + ServeName + "_Server]\n";
    bool dumpFlag = argSets.find(u"All") != argSets.end();
    dumpFlag = dumpFlag || (argSets.find(std::wstring_convert<
        std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(ServeName)) != argSets.end());
    int32_t ret = WriteInfo(fd, dumpString, dumperTbl_[stubType], dumpFlag);
    CHECK_AND_RETURN_LOG(ret == OHOS::NO_ERROR,
        "Failed to write %{public}s server information", ServeName.c_str());
}

int32_t AVCodecServerManager::Dump(int32_t fd, const std::vector<std::u16string>& args)
{
    std::string dumpString;
    std::unordered_set<std::u16string> argSets;
    for (decltype(args.size()) index = 0; index < args.size(); ++index) {
        argSets.insert(args[index]);
    }

#ifdef SUPPORT_CODEC
    DumpServer(fd, StubType::CODEC, argSets);
#endif
    int32_t ret = AVCodecXCollie::GetInstance().Dump(fd);
    CHECK_AND_RETURN_RET_LOG(ret == OHOS::NO_ERROR, OHOS::INVALID_OPERATION,
                             "Failed to write xcollie dump information");

    if (argSets.empty() || argSets.find(u"h") != argSets.end() || argSets.find(u"help") != argSets.end()) {
        PrintDumpMenu(fd);
    }

    return OHOS::NO_ERROR;
}

AVCodecServerManager::AVCodecServerManager()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecServerManager::~AVCodecServerManager()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecServerManager::CreateStubObject(StubType type, sptr<IRemoteObject> &object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    switch (type) {
#ifdef SUPPORT_CODECLIST
        case CODECLIST: {
            return CreateCodecListStubObject(object);
        }
#endif
#ifdef SUPPORT_CODEC
        case CODEC: {
            return CreateCodecStubObject(object);
        }
#endif
        default: {
            AVCODEC_LOGE("default case, av_codec server manager failed");
            return AVCS_ERR_UNSUPPORT;
        }
    }
}

#ifdef SUPPORT_CODECLIST
int32_t AVCodecServerManager::CreateCodecListStubObject(sptr<IRemoteObject> &object)
{
    sptr<CodecListServiceStub> stub = CodecListServiceStub::Create();
    CHECK_AND_RETURN_RET_LOG(stub != nullptr, AVCS_ERR_CREATE_CODECLIST_STUB_FAILED,
        "Failed to create AVCodecListServiceStub");
    object = stub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_CREATE_CODECLIST_STUB_FAILED,
        "Failed to create AVCodecListServiceStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    codecListStubMap_[object] = pid;
    AVCODEC_LOGD("The number of codeclist services(%{public}zu).", codecListStubMap_.size());
    return AVCS_ERR_OK;
}
#endif
#ifdef SUPPORT_CODEC
int32_t AVCodecServerManager::CreateCodecStubObject(sptr<IRemoteObject> &object)
{
    sptr<CodecServiceStub> stub = CodecServiceStub::Create();
    CHECK_AND_RETURN_RET_LOG(stub != nullptr, AVCS_ERR_CREATE_AVCODEC_STUB_FAILED, "Failed to create CodecServiceStub");

    object = stub->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_CREATE_AVCODEC_STUB_FAILED,
        "Failed to create CodecServiceStub");

    pid_t pid = IPCSkeleton::GetCallingPid();
    codecStubMap_[object] = pid;

    Dumper dumper;
    dumper.entry_ = [stub](int32_t fd) -> int32_t { return stub->DumpInfo(fd); };
    dumper.pid_ = pid;
    dumper.uid_ = IPCSkeleton::GetCallingUid();
    dumper.remoteObject_ = object;
    dumperTbl_[StubType::CODEC].emplace_back(dumper);

    AVCODEC_LOGD("The number of codec services(%{public}zu).", codecStubMap_.size());
    return AVCS_ERR_OK;
}
#endif

void AVCodecServerManager::EraseObject(std::map<sptr<IRemoteObject>, pid_t>::iterator& iter,
                                       std::map<sptr<IRemoteObject>, pid_t>& stubMap,
                                       pid_t pid,
                                       const std::string& stubName)
{
    if (iter != stubMap.end()) {
        AVCODEC_LOGD("destroy %{public}s stub services(%{public}zu) pid(%{public}d).", stubName.c_str(),
                     stubMap.size(), pid);
        (void)stubMap.erase(iter);
        return;
    }
    AVCODEC_LOGE("find %{public}s object failed, pid(%{public}d).", stubName.c_str(), pid);
    return;
}

void AVCodecServerManager::DestroyStubObject(StubType type, sptr<IRemoteObject> object)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pid_t pid = IPCSkeleton::GetCallingPid();
    DestroyDumper(type, object);
    auto compare_func = [object](std::pair<sptr<IRemoteObject>, pid_t> objectPair) -> bool {
        return objectPair.first == object;
    };
    switch (type) {
        case CODEC: {
            auto it = find_if(codecStubMap_.begin(), codecStubMap_.end(), compare_func);
            EraseObject(it, codecStubMap_, pid, "codec");
            return;
        }
        case CODECLIST: {
            auto it = find_if(codecListStubMap_.begin(), codecListStubMap_.end(), compare_func);
            EraseObject(it, codecListStubMap_, pid, "codeclist");
            return;
        }
        default: {
            AVCODEC_LOGE("default case, av_codec server manager failed, pid(%{public}d).", pid);
            break;
        }
    }
}

void AVCodecServerManager::EraseObject(std::map<sptr<IRemoteObject>, pid_t>& stubMap, pid_t pid)
{
    for (auto it = stubMap.begin(); it != stubMap.end();) {
        if (it->second == pid) {
            executor_.Commit(it->first);
            it = stubMap.erase(it);
        } else {
            it++;
        }
    }
    return;
}

void AVCodecServerManager::DestroyStubObjectForPid(pid_t pid)
{
    std::lock_guard<std::mutex> lock(mutex_);
    DestroyDumperForPid(pid);
    AVCODEC_LOGD("codec stub services(%{public}zu) pid(%{public}d).", codecStubMap_.size(), pid);
    EraseObject(codecStubMap_, pid);
    AVCODEC_LOGD("codec stub services(%{public}zu).", codecStubMap_.size());
    AVCODEC_LOGD("codeclist stub services(%{public}zu) pid(%{public}d).", codecListStubMap_.size(), pid);
    EraseObject(codecListStubMap_, pid);
    AVCODEC_LOGD("codeclist stub services(%{public}zu).", codecListStubMap_.size());
    executor_.Clear();
}

void AVCodecServerManager::DestroyDumper(StubType type, sptr<IRemoteObject> object)
{
    for (auto it = dumperTbl_[type].begin(); it != dumperTbl_[type].end(); it++) {
        if (it->remoteObject_ == object) {
            (void)dumperTbl_[type].erase(it);
            AVCODEC_LOGD("AVCodecServerManager::DestroyDumper");
            if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
                AVCODEC_LOGW("failed to call InstanceDump");
            }
            return;
        }
    }
}

void AVCodecServerManager::DestroyDumperForPid(pid_t pid)
{
    for (auto& dumpers : dumperTbl_) {
        for (auto it = dumpers.second.begin(); it != dumpers.second.end();) {
            if (it->pid_ == pid) {
                it = dumpers.second.erase(it);
                AVCODEC_LOGD("AVCodecServerManager::DestroyDumperForPid");
            } else {
                it++;
            }
        }
    }
    if (Dump(-1, std::vector<std::u16string>()) != OHOS::NO_ERROR) {
        AVCODEC_LOGW("failed to call InstanceDump");
    }
}

void AVCodecServerManager::AsyncExecutor::Commit(sptr<IRemoteObject> obj)
{
    std::lock_guard<std::mutex> lock(listMutex_);
    freeList_.push_back(obj);
}

void AVCodecServerManager::AsyncExecutor::Clear()
{
    std::thread(&AVCodecServerManager::AsyncExecutor::HandleAsyncExecution, this).detach();
}

void AVCodecServerManager::AsyncExecutor::HandleAsyncExecution()
{
    std::list<sptr<IRemoteObject>> tempList;
    {
        std::lock_guard<std::mutex> lock(listMutex_);
        freeList_.swap(tempList);
    }
    tempList.clear();
}

void AVCodecServerManager::PrintDumpMenu(int32_t fd)
{
    AVCodecDumpControler dumpControler;
    dumpControler.AddInfo(DUMP_MENU_INDEX, "[AVCodec Dump Menu]");
    uint32_t index = 1;
    for (auto iter : SA_DUMP_MENU_DUMP_TABLE) {
        dumpControler.AddInfo(DUMP_MENU_INDEX + (index << DUMP_OFFSET_16), iter);
        index++;
    }

    std::string dumpString;
    dumpControler.GetDumpString(dumpString);
    dumpString += "\n";
    dumpString += "Add args to get more infomation about avcodec components.\n";
    dumpString += "Example: hidumper -s AVCodecService -a \"All\"\n";

    if (fd != -1) {
        write(fd, dumpString.c_str(), dumpString.size());
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
