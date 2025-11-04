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
#include <cstdint>
#include <memory>
#include <map>

#include "dash_mpd_parser.h"
#include "src/libfuzzer/libfuzzer_macro.h"

#include "mpd.pb.h"

// 命名空间别名
namespace hp = OHOS::Media::Plugins::HttpPlugin; // 业务解析器命名空间
namespace p  = ohos::mpd; // proto命名空间
namespace Core {
    void* GetPluginRegister() {
        return nullptr;
    }
} // OHOS链接桩

// ----------------------XML工具-----------------------
static inline void XMLEscapeAppend(std::string &out, const std::string &in)
{
    for (char c : in) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&apos;"; break;
            default:   out += c;        break;
        }
    }
}

static inline void Attr(std::string &s, const char *k, const std::string &v)
{
    if (!v.empty()) {
        s += " ";
        s += k;
        s += "=\"";
        XMLEscapeAppend(s, v);
        s += "\"";
    }
}

template <class Num>
static inline void AttrNum(std::string &s, const char *k, bool cond, Num v)
{
    if (cond) {
        s += " ";
        s += k;
        s += "=\"";
        s += std::to_string(v);
        s += "\"";
    }
}

static inline void EmitText(std::string &s, const char *tag, const std::string &text)
{
    s += "<";
    s += tag;
    s += ">";
    XMLEscapeAppend(s, text);
    s += "</";
    s += tag;
    s += ">";
}

static inline std::string Iso8601S(uint32_t sec)
{
    return std::string("PT") + std::to_string(sec) + "S";
}

static inline const char* ScanTypeToStr(p::ProtoVideoScanType t)
{
    switch (t) {
        case p::PROTO_SCAN_INTERLACED: return "interlaced";
        case p::PROTO_SCAN_UNKNOW:     return "unknown";
        case p::PROTO_SCAN_PROGRESSIVE:
        default:                       return "progressive";
    }
}

static inline const char* DashTypeToStr(p::ProtoDashType t)
{
    return (t == p::PROTO_DASH_TYPE_DYNAMIC) ? "dynamic" : "static";
}

// ----------------------片段发射器-----------------------
static void EmitUrlType(std::string &s, const char *tag, const p::UrlType &u)
{
    if (!u.source_url().empty() || !u.range().empty()) {
        s += "<";
        s += tag;
        Attr(s, "sourceURL", u.source_url());
        Attr(s, "range", u.range());
        s += "/>";
    }
}

static void EmitDescriptorList(std::string &s, const char *tag,
    const ::google::protobuf::RepeatedPtrField<p::Descriptor> &list, bool with_pssh_child)
{
    for (const auto &d : list) {
        // C++17兼容：不用contains()
        bool has_pssh = false;
        auto it = d.element_map().find("cenc:pssh");
        if (it != d.element_map().end()) {
            has_pssh = true;
        }
        s += "<";
        s += tag;
        Attr(s, "schemeIdUri", d.scheme_id_url());
        Attr(s, "value", d.value());
        if (!d.default_kid().empty()) {
            Attr(s, "cenc:default_KID", d.default_kid());
        }
        if (with_pssh_child && has_pssh) {
            s += ">";
            EmitText(s, "cenc:pssh", it->second);
            s += "</";
            s += tag;
            s += ">";
        } else {
            s += "/>";
        }
    }
}

static void EmitSegTimeline(std::string &s, const ::google::protobuf::RepeatedPtrField<p::SegmentTimelineEntry> &tl)
{
    if (tl.empty()) {
        return;
    }
    s += "<SegmentTimeline>";
    for (const auto &e : tl) {
        s += "<S";

        // proto3标量没有 has_*，用“非默认”判断
        if (e.t() != 0) {
            AttrNum(s, "t", true, e.t());
        }
        if (e.d() != 0) {
            AttrNum(s, "d", true, e.d());
        }
        if (e.r() != 0) {
            AttrNum(s, "r", true, e.r());
        }
        s += "/>";
    }
    s += "</SegmentTimeline>";
}

static void EmitSegBaseInfo(std::string &s, const p::SegBaseInfo &sb)
{
    s += "<SegmentBase";
    AttrNum(s, "timescale", sb.time_scale() != 0, sb.time_scale());
    AttrNum(s, "presentationTimeOffset", sb.presentation_time_offset() != 0, sb.presentation_time_offset());
    if (!sb.index_range().empty()) {
        Attr(s, "indexRange", sb.index_range());
    }
    if (sb.index_range_exact()) {
        Attr(s, "indexRangeExact", "true");
    }
    s += ">";

    // message字段在proto3有presence，可用has_（也可用字符串/默认判断）
    if (sb.has_initialization()) {
        EmitUrlType(s, "Initialization", sb.initialization());
    }
    if (sb.has_representation_index()) {
        EmitUrlType(s, "RepresentationIndex", sb.representation_index());
    }
    s += "</SegmentBase>";
}

static void EmitSegList(std::string &s, const p::SegListInfo & sl)
{
    s += "<SegmentList";
    if (sl.has_mult()) {
        const auto &m = sl.mult();
        AttrNum(s, "duration", m.duration() != 0, m.duration());
        if (!m.start_number().empty()) {
            Attr(s, "startNumber", m.start_number());
        }
        if (m.has_seg_base() && m.seg_base().time_scale() != 0) {
            AttrNum(s, "timescale", true, m.seg_base().time_scale());
        }
        if (m.has_seg_base() && m.seg_base().presentation_time_offset() != 0) {
            AttrNum(s, "presentationTimeOffset", true, m.seg_base().presentation_time_offset());
        }
    }
    s += ">";

    if (sl.has_mult()) {
        const auto &m = sl.mult();
        if (m.timeline_size() > 0) {
            EmitSegTimeline(s, m.timeline());
        }
        if (m.has_bitstream_switching()) {
            EmitUrlType(s, "BitstreamSwitching", m.bitstream_switching());
        }
        if (m.has_seg_base() && m.seg_base().has_initialization()) {
            EmitUrlType(s, "Initialization", m.seg_base().initialization());
        }
        if (m.has_seg_base() && m.seg_base().has_representation_index()) {
            EmitUrlType(s, "RepresentationIndex", m.seg_base().representation_index());
        }
    }
    for (const auto &u : sl.segment_urls()) {
        s += "<SegmentURL";
        if (!u.media().empty()) {
            Attr(s, "media", u.media());
        }
        if (!u.media_range().empty()) {
            Attr(s, "mediaRange", u.media_range());
        }
        if (!u.index().empty()) {
            Attr(s, "index", u.index());
        }
        if (!u.index_range().empty()) {
            Attr(s, "indexRange", u.index_range());
        }
        s += "/>";
    }
    s += "</SegmentList>";
}

static void EmitSegTemplate(std::string &s, const p::SegTemplateInfo &st)
{
    s += "<SegmentTemplate";
    if (st.has_mult()) {
        const auto &m = st.mult();
        if (m.has_seg_base() && m.seg_base().time_scale() != 0) {
            AttrNum(s, "timescale", true, m.seg_base().time_scale());
        }
        if (m.has_seg_base() && m.seg_base().presentation_time_offset() != 0) {
            AttrNum(s, "presentationTimeOffset", true, m.seg_base().presentation_time_offset());
        }
        AttrNum(s, "duration", m.duration() != 0, m.duration());
        if (!m.start_number().empty()) {
            Attr(s, "startNumber", m.start_number());
        }
    }
    if (!st.media().empty()) {
        Attr(s, "media", st.media());
    }
    if (!st.index().empty()) {
        Attr(s, "index", st.index());
    }
    if (!st.initialization().empty()) {
        Attr(s, "initialization", st.initialization());
    }
    if (!st.bitstream_switching_attr().empty()) {
        Attr(s, "bitstreamSwitching", st.bitstream_switching_attr());
    }
    s += ">";
    if (st.has_mult()) {
        const auto &m = st.mult();
        if (m.timeline_size() > 0) {
            EmitSegTimeline(s, m.timeline());
        }
        if (m.has_bitstream_switching()) {
            EmitUrlType(s, "BitstreamSwitching", m.bitstream_switching());
        }
        if (m.has_seg_base() && m.seg_base().has_initialization()) {
            EmitUrlType(s, "Initialization", m.seg_base().initialization());
        }
        if (m.has_seg_base() && m.seg_base().has_representation_index()) {
            EmitUrlType(s, "RepresentationIndex", m.seg_base().representation_index());
        }
    }
    s += "</SegmentTemplate>";
}

static void EmitCommonAE_Attrs(std::string &s, const p::CommonAE &c)
{
    AttrNum(s, "width", c.width() != 0, c.width());
    AttrNum(s, "height", c.height() != 0, c.height());
    AttrNum(s, "startWithSAP", c.start_with_sap() != 0, c.start_with_sap());
    if (!c.sar().empty()) {
        Attr(s, "sar", c.sar());
    }
    if (!c.mime_type().empty()) {
        Attr(s, "mimeType", c.mime_type());
    }
    if (!c.codecs().empty()) {
        Attr(s, "codecs", c.codecs());
    }
    if (!c.audio_sampling_rate().empty()) {
        Attr(s, "audioSamplingRate", c.audio_sampling_rate());
    }
    if (!c.frame_rate().empty()) {
        Attr(s, "frameRate", c.frame_rate());
    }
    if (!c.profiles().empty()) {
        Attr(s, "profiles", c.profiles());
    }
    if (!c.cuvv_version().empty()) {
        Attr(s, "cuvvVersion", c.cuvv_version());
    }
    if (c.max_playout_rate() != 0.0) {
        Attr(s, "maxPlayoutRate", std::to_string(c.max_playout_rate()));
    }
    if (c.coding_dependency()) {
        Attr(s, "codingDependency", "true");
    }
    if (c.scan_type() != p::PROTO_SCAN_PROGRESSIVE) {
        Attr(s, "scanType", ScanTypeToStr(c.scan_type()));
    }
}

static void EmitRepresentation(std::string &s, const p::Representation &r)
{
    s += "<Representation";
    if (!r.id().empty()) {
        Attr(s, "id", r.id());
    }
    AttrNum(s, "bandwidth", r.bandwidth() != 0, r.bandwidth());
    AttrNum(s, "qualityRanking", r.quality_ranking() != 0, r.quality_ranking());
    if (!r.volume_adjust().empty()) {
        Attr(s, "volumeAdjust_", r.volume_adjust());
    }
    if (r.has_common()) {
        EmitCommonAE_Attrs(s, r.common());
    }
    s += ">";

    for (const auto &bu : r.base_url()) {
        EmitText(s, "BaseURL", bu);
    }
    if (r.has_common()) {
        EmitDescriptorList(s, "ContentProtection", r.common().content_protection(), /*pssh*/true);
        EmitDescriptorList(s, "EssentialProperty", r.common().essential_property(), false);
        EmitDescriptorList(s, "AudioChannelConfiguration", r.common().audio_channel_configuration(), false);
    }
    if (r.has_seg_base()) {
        EmitSegBaseInfo(s, r.seg_base());
    }
    if (r.has_seg_list()) {
        EmitSegList(s, r.seg_list());
    }
    if (r.has_seg_template()) {
        EmitSegTemplate(s, r.seg_template());
    }
    s += "</Representation>";
}

static void EmitAdaptationSet(std::string &s, const p::AdaptationSet &a)
{
    s += "<AdaptationSet";
    if (!a.mime_type().empty()) {
        Attr(s, "mimeType", a.mime_type());
    }
    if (!a.lang().empty()) {
        Attr(s, "lang", a.lang());
    }
    if (!a.content_type().empty()) {
        Attr(s, "contentType", a.content_type());
    }
    if (!a.par().empty()) {
        Attr(s, "par", a.par());
    }
    if (!a.min_frame_rate().empty()) {
        Attr(s, "minFrameRate", a.min_frame_rate());
    }
    if (!a.max_frame_rate().empty()) {
        Attr(s, "maxFrameRate", a.max_frame_rate());
    }
    if (!a.video_type().empty()) {
        Attr(s, "videoType", a.video_type());
    }
    AttrNum(s, "id", a.id() != 0, a.id());
    AttrNum(s, "group", a.group() != 0, a.group());
    AttrNum(s, "minBandwidth", a.min_bandwidth() != 0, a.min_bandwidth());
    AttrNum(s, "maxBandwidth", a.max_bandwidth() != 0, a.max_bandwidth());
    AttrNum(s, "minWidth", a.min_width() != 0, a.min_width());
    AttrNum(s, "maxWidth", a.max_width() != 0, a.max_width());
    AttrNum(s, "minHeight", a.min_height() != 0, a.min_height());
    AttrNum(s, "maxHeight", a.max_height() != 0, a.max_height());
    AttrNum(s, "cameraIndex", a.camera_index() != 0, a.camera_index());
    if (a.segment_alignment()) {
        Attr(s, "segmentAlignment", "true");
    }
    if (a.subsegment_alignment()) {
        Attr(s, "subsegmentAlignment", "true");
    }
    if (a.bitstream_switching()) {
        Attr(s, "bitstreamSwitching", "true");
    }
    if (a.has_common()) {
        EmitCommonAE_Attrs(s, a.common());
    }
    s += ">";

    for (const auto &u : a.base_url()) {
        EmitText(s, "BaseURL", u);
    }
    for (const auto &cc : a.content_components()) {
        s += "<ContentComponent";
        AttrNum(s, "id", cc.id() != 0, cc.id());
        if (!cc.par().empty()) {
            Attr(s, "par", cc.par());
        }
        if (!cc.lang().empty()) {
            Attr(s, "lang", cc.lang());
        }
        if (!cc.content_type().empty()) {
            Attr(s, "contentType", cc.content_type());
        }
        s += "/>";
    }

    if (a.has_common()) {
        EmitDescriptorList(s, "ContentProtection", a.common().content_protection(), /*pssh*/true);
        EmitDescriptorList(s, "EssentialProperty", a.common().essential_property(), false);
        EmitDescriptorList(s, "AudioChannelConfiguration", a.common().audio_channel_configuration(), false);
    }

    if (a.has_seg_base()) {
        EmitSegBaseInfo(s, a.seg_base());
    }
    if (a.has_seg_list()) {
        EmitSegList(s, a.seg_list());
    }
    if (a.has_seg_template()) {
        EmitSegTemplate(s, a.seg_template());
    }

    for (const auto &r : a.representations()) {
        EmitRepresentation(s, r);
    }
    s += "</AdaptationSet>";
}

static void EmitPeriod(std::string &s, const p::Period &pobj)
{
    s += "<Period";
    if (pobj.start_sec() != 0) {
        Attr(s, "start", Iso8601S(pobj.start_sec()));
    }
    if (pobj.duration_sec() != 0) {
        Attr(s, "duration", Iso8601S(pobj.duration_sec()));
    }
    if (pobj.bitstream_switching()) {
        Attr(s, "bitstreamSwitching", "true");
    }
    if (!pobj.id().empty()) {
        Attr(s, "id", pobj.id());
    }
    s += ">";

    for (const auto &bu : pobj.base_url()) {
        EmitText(s, "BaseURL", bu);
    }
    if (pobj.has_seg_base()) {
        EmitSegBaseInfo(s, pobj.seg_base());
    }
    if (pobj.has_seg_list()) {
        EmitSegList(s, pobj.seg_list());
    }
    if (pobj.has_seg_template()) {
        EmitSegTemplate(s, pobj.seg_template());
    }
    for (const auto &a : pobj.adaptation_sets()) {
        EmitAdaptationSet(s, a);
    }
    s += "</Period>";
}

static std::string BuildMPDXML(const p::MpdDoc &doc)
{
    std::string s;
    s.reserve(64 * 1024);
    s += "<MPD";
    s += " type=\"";
    s += DashTypeToStr(doc.type());
    s += "\"";
    if (!doc.profile().empty()) {
        Attr(s, "profiles", doc.profile());
    }
    if (!doc.media_type().empty()) {
        Attr(s, "mediaType", doc.media_type());
    }
    if (doc.media_presentation_duration_sec() != 0) {
        Attr(s, "mediaPresentationDuration", Iso8601S(doc.media_presentation_duration_sec()));
    }
    if (doc.minimum_update_period_sec() != 0) {
        Attr(s, "minimumUpdatePeriod", Iso8601S(doc.minimum_update_period_sec()));
    }
    if (doc.min_buffer_time_sec() != 0) {
        Attr(s, "minBufferTime", Iso8601S(doc.min_buffer_time_sec()));
    }
    if (doc.time_shift_buffer_depth_sec() != 0) {
        Attr(s, "timeShiftBufferDepth", Iso8601S(doc.time_shift_buffer_depth_sec()));
    }
    if (doc.suggested_presentation_delay_sec() != 0) {
        Attr(s, "suggestedPresentationDelay", Iso8601S(doc.suggested_presentation_delay_sec()));
    }
    if (doc.max_segment_duration_sec() != 0) {
        Attr(s, "maxSegmentDuration", Iso8601S(doc.max_segment_duration_sec()));
    }
    if (doc.max_subsegment_duration_sec() != 0) {
        Attr(s, "maxSubsegmentDuration", Iso8601S(doc.max_subsegment_duration_sec()));
    }
    if (doc.hw_total_view_number() != 0) {
        AttrNum(s, "hwTotalViewNumber", true, doc.hw_total_view_number());
    }
    if (doc.hw_default_view_index() != 0) {
        AttrNum(s, "hwDefaultViewIndex", true, doc.hw_default_view_index());
    }

    // availability_start_time_ms暂不转字符串，留空即不输出
    s += ">";

    for (const auto &bu : doc.base_url()) {
        EmitText(s, "BaseURL", bu);
    }
    for (const auto &pobj : doc.periods()) {
        EmitPeriod(s, pobj);
    }
    s += "</MPD>";
    return s;
}

// ----------------------- LPM PostProcessor -----------------------
// 注意：使用全限定名，避免与 DEFINE_PROTO_FUZZER 内部的别名冲突
static protobuf_mutator::libfuzzer::PostProcessorRegistration<p::MpdDoc>
    reg_mpd([](p::MpdDoc* d, unsigned) {
        if (d->periods_size() == 0) {
            auto *pobj = d->add_periods();
            pobj->set_start_sec(0);
            pobj->set_duration_sec(10);
        }
        if (d->media_presentation_duration_sec() == 0) {
            d->set_media_presentation_duration_sec(d->periods_size() ? 60 : 10);
        }

        for (auto &pobj : *d->mutable_periods()) {
            if (pobj.duration_sec() == 0) {
                pobj.set_duration_sec(30);
            }
            if (pobj.has_seg_list() && pobj.mutable_seg_list()->has_mult()) {
                auto *m = pobj.mutable_seg_list()->mutable_mult();
                if (m->start_number().empty()) {
                    m->set_start_number("1");
                }
            }
            if (pobj.has_seg_template() && pobj.mutable_seg_template()->has_mult()) {
                auto *m = pobj.mutable_seg_template()->mutable_mult();
                if (m->start_number().empty()) {
                    m->set_start_number("1");
                }
            }
        }
    });

// ------------------- LPM 入口 ---------------------
DEFINE_PROTO_FUZZER(const p::MpdDoc& doc)
{
    std::string xml = BuildMPDXML(doc);
    if (xml.size() > (1u << 20)) {
        return; // 1 MiB 上限
    }
    hp::DashMpdParser parser;
    parser.ParseMPD(xml.c_str(), static_cast<uint32_t>(xml.size()));
    parser.Clear();
}