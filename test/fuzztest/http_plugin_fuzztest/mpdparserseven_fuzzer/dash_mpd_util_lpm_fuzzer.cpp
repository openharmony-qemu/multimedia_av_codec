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
#include <string>
#include <vector>
#include <list>
#include "mpd_util_input.pb.h"
#include "dash/include/mpd_parser/dash_mpd_def.h"
#include "dash/include/mpd_parser/dash_mpd_util.h"
#include "src/libfuzzer/libfuzzer_macro.h"

using namespace OHOS::Media::Plugins::HttpPlugin;
namespace p = ohos::mpdutil;

DEFINE_PROTO_FUZZER(const p::UtilInput& in)
{
    // 1) URL相关
    if (in.has_url()) {
        std::string src = in.url().has_src() ? in.url().src() : std::string();
        DashList<std::string> baseList;
        for (const auto& b : in.url().base_list()) {
            baseList.push_back(b);
        }
        std::string baseOne = in.url().has_base_one() ? in.url().base_one() : std::string();

        (void)DashUrlIsAbsolute(src);
        {
            std::string tmp = src;
            DashAppendBaseUrl(tmp, baseList);
        }
        {
            std::string tmp = src;
            DashAppendBaseUrl(tmp, baseOne);
        }
        {
            std::string tmpSrc = src;
            std::string tmpBase = baseOne;
            BuildSrcUrl(tmpSrc, tmpBase);
        }
    }

    // 2) Duration
    if (in.has_dur()) {
        std::string d = in.dur().duration();
        if (d.find('P') == std::string::npos) {
            d.insert(d.begin(), 'P');
        }
        uint32_t out = 0;
        (void)DashStrToDuration(d, out);
    }

    // 3) Attr索引
    if (in.has_attrs()) {
        std::vector<const char*> dict;
        dict.reserve(in.attrs().dict_size());
        for (const auto& s : in.attrs().dict()) {
            dict.push_back(s.c_str());
        }
        uint32_t dictN = static_cast<uint32_t>(dict.size());
        for (const auto& q : in.attrs().query()) {
            (void)DashGetAttrIndex(q, dict.empty() ? nullptr : &dict[0], dictN);
        }
    }

    // 4) Range
    if (in.has_range()) {
        int64_t beg = -1;
        int64_t end = -1;
        DashParseRange(in.range().range(), beg, end);
    }

    // 5) HDR判定
    if (in.desc_size() > 0) {
        DashList<DashDescriptor*> props;
        for (const auto& d : in.desc()) {
            auto* node = new (std::nothrow) DashDescriptor;
            if (!node) {
                continue;
            }
            node->schemeIdUrl_ = d.has_scheme() ? d.scheme() : "";
            node->value_       = d.has_value() ? d.value() : "";
            props.push_back(node);
        }
        (void)DashStreamIsHdr(props);
        while (!props.empty()) {
            auto* p = props.front();
            props.pop_front();
            delete p;
        }
    }
}