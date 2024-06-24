/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LAYER_INFO_IPBBB_H
#define LAYER_INFO_IPBBB_H

#include <iostream>

auto GopInfoIPBBB = R"([
    { "gopId" : 0, "gopSize" : 60, "startFrameId" : 0 },
    { "gopId" : 1, "gopSize" : 60, "startFrameId" : 60}
])"_json;

auto FrameLayerInfoIPBBB = R"([
    { "frameId" :   0, "dts" :       0, "layer" : 3, "discardable" : false },
    { "frameId" :   1, "dts" :   16667, "layer" : 2, "discardable" : false },
    { "frameId" :   2, "dts" :   33335, "layer" : 1, "discardable" : true  },
    { "frameId" :   3, "dts" :   50001, "layer" : 0, "discardable" : true  },
    { "frameId" : 119, "dts" : 1983373, "layer" : 0, "discardable" : true  }
])"_json;

#endif // LAYER_INFO_IPBBB_H