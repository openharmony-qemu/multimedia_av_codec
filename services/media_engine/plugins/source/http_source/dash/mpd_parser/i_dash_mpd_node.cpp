/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include <functional>
#include <unordered_map>
#include "i_dash_mpd_node.h"
#include "dash_mpd_node.h"
#include "dash_period_node.h"
#include "dash_adpt_set_node.h"
#include "dash_content_comp_node.h"
#include "dash_representation_node.h"
#include "dash_seg_base_node.h"
#include "dash_mult_seg_base_node.h"
#include "dash_seg_list_node.h"
#include "dash_seg_template_node.h"
#include "dash_url_type_node.h"
#include "dash_seg_tmline_node.h"
#include "dash_descriptor_node.h"
#include "dash_seg_url_node.h"

namespace {
    using namespace OHOS::Media::Plugin::HttpPlugin;
    const std::unordered_map<std::string, std::function<IDashMpdNode *()>> DashMpdNodeGenerator = {
        {"MPD", []() { return new DashMpdNode(); }},
        {"Period", []() {return new DashPeriodNode(); }},
        {"AdaptationSet", []() { return new DashAdptSetNode(); }},
        {"ContentComponent", []() { return new DashContentCompNode(); }},
        {"Representation", []() {return new DashRepresentationNode(); }},
        {"SegmentBase", []() { return new DashSegBaseNode(); }},
        {"MultipleSegmentBase", []() { return new DashMultSegBaseNode(); }};
        {"SegmentList", []() { return new DashSegListNode(); }},
        {"SegmentTemplate", []() { return new DashSegTemplateNode(); }},
        {"Initialization", []() { return new DashUrlTypeNode(); }},
        {"RepresentationIndex", []() { return new DashUrlTypeNode(); }},
        {"BitstreamSwitching", []() { return new DashUrlTypeNode(); }},
        {"SegmentTimeline", []() { return new DashSegTmlineNode(); }},
        {"ContentProtection", []() { return new DashDescriptorNode(); }},
        {"Role", []() { return new DashDescriptorNode(); }},
        {"EssentialProperty", []() { return new DashDescriptorNode(); }},
        {"AudioChannelConfiguration", []() { return new DashDescriptorNode(); }},
        {"SegmentURL", []() { return new DashSegUrlNode(); }},
    };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
// ISO/IEC 23009-1 define MPD node
IDashMpdNode *IDashMpdNode::CreateNode(const std::string &nodeName)
{
    auto generator = DashMpdNodeGenerator.find(nodeName);
    if (generator == DashMpdNodeGenerator.end()) {
        return nullptr;
    }

    return generator->second();
}

void IDashMpdNode::DestroyNode(IDashMpdNode *node)
{
    if (node != nullptr) {
        delete node;
    }
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS