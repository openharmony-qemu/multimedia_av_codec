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
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
#include <sstream>
#include <cctype>
#include "iconsumer_surface.h"
#include "native_buffer_inner.h"
#include "videoenc_api11_sample.h"
#include "native_avcapability.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

bool VEncAPI11Sample::ReadCmdOutput(const std::string& cmd, std::string& output)
{
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe == nullptr) {
        cout << "CheckDmabufNameSuffix: popen failed for [" << cmd << "]" << endl;
        return false;
    }
    char buffer[512];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }
    pclose(pipe);
    cout << "CheckDmabufNameSuffix: output=[" << output << "]" << endl;
    return true;
}

bool VEncAPI11Sample::FindBufNameColumnIndex(const std::string& header, size_t& colIndex)
{
    std::istringstream hs(header);
    std::string col;
    size_t idx = 0;
    while (hs >> col) {
        if (col == "buf_name") {
            colIndex = idx;
            return true;
        }
        idx++;
    }
    cout << "CheckDmabufNameSuffix: buf_name column not found in header" << endl;
    return false;
}

std::string VEncAPI11Sample::ExtractColumnByIndex(const std::string& line, size_t colIndex)
{
    std::istringstream ls(line);
    std::string token;
    for (size_t i = 0; i <= colIndex; i++) {
        if (!(ls >> token)) {
            return "";
        }
    }
    return token;
}

std::string VEncAPI11Sample::ExtractSuffixFromBufName(const std::string& bufName)
{
    size_t colonPos = bufName.find(':');
    if (colonPos == std::string::npos) {
        return "";
    }
    return bufName.substr(colonPos + 1);
}

bool VEncAPI11Sample::IsNumericString(const std::string& str)
{
    if (str.empty()) {
        return false;
    }
    for (char c : str) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

void VEncAPI11Sample::CountMatchingRows(std::istringstream& ss, size_t colIndex, const std::string& expectedSuffix,
    int& totalCount, int& matchCount)
{
    totalCount = 0;
    matchCount = 0;
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) {
            continue;
        }
        std::string bufName = ExtractColumnByIndex(line, colIndex);
        if (bufName.empty() || IsNumericString(bufName)) {
            continue;
        }
        std::string suffix = ExtractSuffixFromBufName(bufName);
        totalCount++;
        bool matched = (suffix == expectedSuffix);
        matchCount += matched ? 1 : 0;
        cout << "CheckDmabufNameSuffix: buf_name=[" << bufName << "] suffix=[" << suffix << "] expected=[" <<
        expectedSuffix << "] " << (matched ? "matched" : "mismatched") << endl;
    }
}

bool VEncAPI11Sample::CheckDmabufNameSuffix(const std::string& expectedSuffix)
{
    if (expectedSuffix.empty()) {
        cout << "CheckDmabufNameSuffix: expected suffix is empty" << endl;
        return false;
    }
    pid_t pid = getpid();
    std::string cmd = "cat /proc/" + std::to_string(pid) + "/mm_dmabuf_info";
    cout << "CheckDmabufNameSuffix: running [" << cmd << "] expecting [" << expectedSuffix << "]" << endl;
    std::string output;
    if (!ReadCmdOutput(cmd, output)) {
        return false;
    }
    std::istringstream ss(output);
    std::string header;
    if (!std::getline(ss, header) || header.empty()) {
        cout << "CheckDmabufNameSuffix: empty or invalid output" << endl;
        return false;
    }
    size_t colIndex = 0;
    if (!FindBufNameColumnIndex(header, colIndex)) {
        return false;
    }
    int totalCount = 0;
    int matchCount = 0;
    CountMatchingRows(ss, colIndex, expectedSuffix, totalCount, matchCount);
    cout << "CheckDmabufNameSuffix: total=" << totalCount << " matched=" << matchCount << endl;
    if (matchCount == 0) {
        cout << "CheckDmabufNameSuffix: not all matched for [" << expectedSuffix << "]" << endl;
        return false;
    }
    return true;
}

void VEncAPI11Sample::CheckDmabufSuffixOnce()
{
    if (!needCheckDmabufNameSuffix || dmabufSuffixChecked) {
        return;
    }
    dmabufSuffixChecked = true;
    if (!CheckDmabufNameSuffix(expectedDmabufSuffix)) {
        errCount++;
    }
}
