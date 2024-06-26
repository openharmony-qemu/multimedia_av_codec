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

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
// ISO/IEC 23009-1 define MPD node
IDashMpdNode *IDashMpdNode::CreateNode(const std::string &nodeName)
{
    if (nodeName == "MPD") {
        return new DashMpdNode();
    }

    if (nodeName == "Period") {
        return new DashPeriodNode();
    }

    if (nodeName == "AdaptationSet") {
        return new DashAdptSetNode();
    }

    if (nodeName == "ContentComponent") {
        return new DashContentCompNode();
    }

    if (nodeName == "Representation") {
        return new DashRepresentationNode();
    }

    if (nodeName == "SegmentBase") {
        return new DashSegBaseNode();
    }

    if (nodeName == "MultipleSegmentBase") {
        return new DashMultSegBaseNode();
    }

    if (nodeName == "SegmentList") {
        return new DashSegListNode();
    }

    if (nodeName == "SegmentTemplate") {
        return new DashSegTemplateNode();
    }

    if (nodeName == "Initialization" || nodeName == "RepresentationIndex" || nodeName == "BitstreamSwitching") {
        return new DashUrlTypeNode();
    }

    if (nodeName == "SegmentTimeline") {
        return new DashSegTmlineNode();
    }

    if (nodeName == "ContentProtection" || nodeName == "Role" || nodeName == "EssentialProperty" ||
        nodeName == "AudioChannelConfiguration") {
        return new DashDescriptorNode();
    }

    if (nodeName == "SegmentURL") {
        return new DashSegUrlNode();
    }

    return nullptr;
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