/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef POST_PROCESSING_DYNAMIC_INTERFACE_TYPES_H
#define POST_PROCESSING_DYNAMIC_INTERFACE_TYPES_H

#include "utils.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PostProcessing {

// processing handler type
using DynamicColorSpaceConverterHandle = void;

// function pointer types
using DynamicIsColorSpaceConversionSupportedFunc = int32_t(*)(const void*, const void*);
using DynamicCreateFunc = DynamicColorSpaceConverterHandle*(*)();
using DynamicDestroyFunc = void(*)(DynamicColorSpaceConverterHandle*);
using DynamicSetCallbackFunc = int32_t(*)(DynamicColorSpaceConverterHandle*, void*, void*);
using DynamicSetOutputSurfaceFunc = int32_t(*)(DynamicColorSpaceConverterHandle*, void*);
using DynamicCreateInputSurfaceFunc = int32_t(*)(DynamicColorSpaceConverterHandle*, void*);
using DynamicSetParameterFunc = int32_t(*)(DynamicColorSpaceConverterHandle*, void*);
using DynamicGetParameterFunc = int32_t(*)(DynamicColorSpaceConverterHandle*, void*);
using DynamicConfigureFunc = int32_t(*)(DynamicColorSpaceConverterHandle*, void*);
using DynamicPrepareFunc = int32_t(*)(DynamicColorSpaceConverterHandle*);
using DynamicStartFunc = int32_t(*)(DynamicColorSpaceConverterHandle*);
using DynamicStopFunc = int32_t(*)(DynamicColorSpaceConverterHandle*);
using DynamicFlushFunc = int32_t(*)(DynamicColorSpaceConverterHandle*);
using DynamicResetFunc = int32_t(*)(DynamicColorSpaceConverterHandle*);
using DynamicReleaseFunc = int32_t(*)(DynamicColorSpaceConverterHandle*);
using DynamicReleaseOutputBufferFunc = int32_t(*)(DynamicColorSpaceConverterHandle*, uint32_t, bool);
using DynamicGetOutputFormatFunc = int32_t(*)(DynamicColorSpaceConverterHandle*, void*);
using DynamicOnProducerBufferReleasedFunc = int32_t(*)(DynamicColorSpaceConverterHandle*);

// function pointer types array
using DynamicInterfaceFuncTypes = TypeArray<
    DynamicIsColorSpaceConversionSupportedFunc,
    DynamicCreateFunc,
    DynamicDestroyFunc,
    DynamicSetCallbackFunc,
    DynamicSetOutputSurfaceFunc,
    DynamicCreateInputSurfaceFunc,
    DynamicSetParameterFunc,
    DynamicGetParameterFunc,
    DynamicConfigureFunc,
    DynamicPrepareFunc,
    DynamicStartFunc,
    DynamicStopFunc,
    DynamicFlushFunc,
    DynamicResetFunc,
    DynamicReleaseFunc,
    DynamicReleaseOutputBufferFunc,
    DynamicGetOutputFormatFunc,
    DynamicOnProducerBufferReleasedFunc
>;

// function symbols
constexpr const char* DYNAMIC_INTERFACE_SYMBOLS[]{
    "ColorSpaceConvertVideoIsColorSpaceConversionSupported",
    "ColorSpaceConvertVideoCreate",
    "ColorSpaceConvertVideoDestroy",
    "ColorSpaceConvertVideoSetCallback",
    "ColorSpaceConvertVideoSetOutputSurface",
    "ColorSpaceConvertVideoCreateInputSurface",
    "ColorSpaceConvertVideoSetParameter",
    "ColorSpaceConvertVideoGetParameter",
    "ColorSpaceConvertVideoConfigure",
    "ColorSpaceConvertVideoPrepare",
    "ColorSpaceConvertVideoStart",
    "ColorSpaceConvertVideoStop",
    "ColorSpaceConvertVideoFlush",
    "ColorSpaceConvertVideoReset",
    "ColorSpaceConvertVideoRelease",
    "ColorSpaceConvertVideoReleaseOutputBuffer",
    "ColorSpaceConvertVideoGetOutputFormat",
    "ColorSpaceConvertVideoOnProducerBufferReleased",
};

// function name enumeration
enum class DynamicInterfaceName : size_t {
    IS_COLORSPACE_CONVERSION_SUPPORTED,
    CREATE,
    DESTROY,
    SET_CALLBACK,
    SET_OUTPUT_SURFACE,
    CREATE_INPUT_SURFACE,
    SET_PARAMETER,
    GET_PARAMETER,
    CONFIGURE,
    PREPARE,
    START,
    STOP,
    FLUSH,
    RESET,
    RELEASE,
    RELEASE_OUPUT_BUFFER,
    GET_OUTPUT_FORMAT,
    ON_PRODUCER_BUFFER_RELEASED,
};

// dynamic interface helper types
template<DynamicInterfaceName E>
using DynamicInterfaceIndex = EnumerationValue<DynamicInterfaceName, E>;

template<DynamicInterfaceName E>
using DynamicInterfaceIndexType = typename DynamicInterfaceIndex<E>::UnderlyingType;

template<DynamicInterfaceName E>
constexpr DynamicInterfaceIndexType<E> DynamicInterfaceIndexValue = DynamicInterfaceIndex<E>::value;

template<DynamicInterfaceName E, typename... Args>
using DynamicInterfaceReturnType =
    std::invoke_result_t<DynamicInterfaceFuncTypes::Get<DynamicInterfaceIndexValue<E>>, Args...>;

constexpr size_t DYNAMIC_INTERFACE_NUM{DynamicInterfaceFuncTypes::size};

} // namespace PostProcessing
} // namespace MediaAVCodec
} // namespace OHOS

#endif // POST_PROCESSING_DYNAMIC_INTERFACE_TYPES_H