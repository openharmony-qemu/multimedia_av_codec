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
#include <vector>
#include <memory>

#include "mpd_parser_def.h"
#include "sidx_box_parser.h"
#include "sidx.pb.h"  // protoc 生成

#include "src/libfuzzer/libfuzzer_macro.h"

using namespace OHOS::Media::Plugins::HttpPlugin;

// 简单大端写入工具
static inline void W32BE(uint32_t v, std::vector<char>& o) {
    o.push_back(static_cast<char>((v >> 24) & 0xFF));
    o.push_back(static_cast<char>((v >> 16) & 0xFF));
    o.push_back(static_cast<char>((v >> 8) & 0xFF));
    o.push_back(static_cast<char>(v & 0xFF));
}

static inline void W64BE(uint64_t v, std::vector<char>& o) {
    W32BE(static_cast<uint32_t>(v >> 32), o);
    W32BE(static_cast<uint32_t>(v & 0xFFFFFFFFULL), o);
}

static void BuildSidxBytes(const sidx::SidxBox& m, std::vector<char>& out) {
    out.clear();

    // 预留 header
    const size_t hdr = out.size();
    W32BE(0 /*size placeholder*/, out);
    out.insert(out.end(), {'s', 'i', 'd', 'x'});

    // vflag: 高 8bit 是 version；低 24bit 当作 flags
    uint32_t vflag = ((m.version() & 0xFF) << 24) | (m.flags() & 0x00FFFFFFu);
    W32BE(vflag, out);
    
    // reference_ID（占位）
    W32BE(1u, out);

    // timescale
    W32BE(m.timescale(), out);

    // earliestPresentationTime + firstOffset 取决于 version
    if ((m.version() & 1u) == 0) {
        W32BE(static_cast<uint32_t>(m.earliest_presentation_time()), out);
        W32BE(static_cast<uint32_t>(m.first_offset()), out);
    } else {
        W64BE(m.earliest_presentation_time(), out);
        W64BE(m.first_offset(), out);
    }

    // reserved(2) + referenceCount(2)
    out.push_back(0);
    out.push_back(0);
    uint16_t refcnt = static_cast<uint16_t>(m.refs_size());
    out.push_back(static_cast<char>((refcnt >> 8) & 0xFF));
    out.push_back(static_cast<char>(refcnt & 0xFF));

    // 每个 reference 12B
    for (int i = 0; i < m.refs_size(); ++i) {
        const auto& r = m.refs(i);
        uint32_t typeAndSize = (r.reference_type() ? 0x80000000u : 0) | (r.referenced_size() & 0x7FFFFFFFu);
        W32BE(typeAndSize, out);
        W32BE(r.duration(), out);
        W32BE(r.sap_info(), out);
    }

    // 回填 box size（虽然被测函数不使用 size，但尽量构造合法 BMFF）
    uint32_t box_size = static_cast<uint32_t>(out.size());
    out[0] = static_cast<char>((box_size >> 24) & 0xFF);
    out[1] = static_cast<char>((box_size >> 16) & 0xFF);
    out[2] = static_cast<char>((box_size >> 8) & 0xFF);
    out[3] = static_cast<char>(box_size & 0xFF);

    // 可选：尾部截断，命中短包/OOB-read 路径
    if (m.truncate_tail() && out.size() > 8) {
        // 保证至少保留 header；LPM 会自己探索更细粒度的截断
        out.resize(8 + (out.size() - 8) / 2);
    }
}

DEFINE_PROTO_FUZZER(const sidx::SidxBox& msg) {
    std::vector<char> bytes;
    BuildSidxBytes(msg, bytes);

    DashList<std::shared_ptr<SubSegmentIndex>> table;
    int64_t endOff = static_cast<int64_t>(msg.sidx_end_offset());

    (void)SidxBoxParser::ParseSidxBox(bytes.data(),
        static_cast<uint32_t>(bytes.size()),
        endOff,
        table);
    table.clear();
}