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
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <list>
#include <memory>
#include <algorithm>
#include "dash/include/mpd_parser/dash_mpd_def.h"
#include "dash/include/mpd_parser/dash_mpd_util.h"

using namespace OHOS::Media::Plugins::HttpPlugin;
const size_t FUZZ_SIZE_MAX = 1 << 20;
static std::string TakeStr(const uint8_t*& p, size_t& n, size_t max_len) {
    if (n == 0) {
        return {};
    }
    size_t len = std::min(n, max_len);
    std::string s(reinterpret_cast<const char*>(p), len);
    p += len;
    n -= len;
    return s;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (!data || size == 0) {
        return 0;
    }

    size = std::min(size, FUZZ_SIZE_MAX);

    const uint8_t* p = data;
    size_t n = size;

    // 1) URL相关
    std::string srcUrl = TakeStr(p, n, 128);
    std::string baseOne = TakeStr(p, n, 128);

    // base列表
    DashList<std::string> baseList;
    for (int i = 0; i < 3 && n > 0; ++i) {
        baseList.push_back(TakeStr(p, n, 64));
    }

    // 调用URL相关API
    (void)DashUrlIsAbsolute(srcUrl);
    {
        std::string tmp = srcUrl;
        DashAppendBaseUrl(tmp, baseList);
        (void)tmp;
    }
    {
        std::string tmp = srcUrl;
        DashAppendBaseUrl(tmp, baseOne);
        (void)tmp;
    }
    {
        std::string tmpSrc = srcUrl;
        std::string tmpBase = baseOne;
        BuildSrcUrl(tmpSrc, tmpBase);
        (void)tmpSrc;
    }

    // 2) IOS-8601 duration
    std::string durStr = TakeStr(p, n, 64);

    // 强行注入一种合法前缀，提升成功解析概率
    if (durStr.find('P') == std::string::npos) {
        durStr.insert(durStr.begin(), 'P');
    }
    uint32_t duration = 0;
    (void)DashStrToDuration(durStr, duration);

    // 3) 属性索引
    static const char* kAttrs[] = {
        "id", "bandwidth", "width", "height", "frameRate", "codecs", "mimeType",
        "duration", "startNumber", "timescale", "presentationTimeOffset",
        "indexRange", "indexRangeExact", "schemeIdUri", "value", "media", "lang"
    };
    std::string q = TakeStr(p, n, 32);
    (void)DashGetAttrIndex(q, kAttrs, static_cast<uint32_t>(sizeof(kAttrs) / sizeof(kAttrs[0])));

    // 4) Range解析
    std::string r = TakeStr(p, n, 32);
    if (r.find('-') == std::string::npos && r.size() >= 2) {
        r.insert(r.begin() + r.size() / 2, '-');
    }
    int64_t beg = -1;
    int64_t end = -1;
    DashParseRange(r, beg, end);

    // 5) HDR判定（构造若干Descriptor）
    DashList<DashDescriptor*> props;
    auto pushDesc = [&](const std::string& scheme, const std::string& val) {
        auto* d = new (std::nothrow) DashDescriptor;
        if (!d) {
            return;
        }
        d->schemeIdUrl_ = scheme;
        d->value_ = val;
        props.push_back(d);
    };
    pushDesc("cicp:MatrixCoefficients", "9");    // BT.2020
    pushDesc("cicp:ColourPrimaries", "9");
    pushDesc("cicp:TransferCharacteristics", "14");

    // 再放入一些随机噪音
    if (n > 0) {
        pushDesc(TakeStr(p, n, 16), TakeStr(p, n, 8));
    }
    
    (void)DashStreamIsHdr(props);

    // 清理
    while (!props.empty()) {
        auto* d = props.front();
        props.pop_front();
        delete d;
    }

    return 0;
}