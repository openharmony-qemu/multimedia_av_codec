/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#ifndef CODEC_BUFFER_HANDLE_H
#define CODEC_BUFFER_HANDLE_H
#include <shared_mutex>
#include "meta/any.h"
#include "native_avbuffer_info.h"

#define TRANS_HANDLE_FUNC(type)                                                                                        \
    operator type()                                                                                                    \
    {                                                                                                                  \
        return OHOS::Media::AnyCast<type>(any_);                                                                       \
    }

using Any = OHOS::Media::Any;
typedef struct OH_AVBuffer OH_AVBuffer;
typedef struct OH_AVMemory OH_AVMemory;
typedef struct OH_AVFormat OH_AVFormat;
namespace OHOS {
namespace MediaAVCodec {
class AVBufferMock;
class AVMemoryMock;
class FormatMock;
typedef struct ParameterWithAttrMock {
    std::shared_ptr<FormatMock> parameter;
    std::shared_ptr<FormatMock> attribute;
} ParameterWithAttrMock;
} // namespace MediaAVCodec
} // namespace OHOS

namespace OHOS {
namespace MediaAVCodec {
struct CodecBufferHandle {
    explicit CodecBufferHandle(Any handle) : any_(handle) {}
    TRANS_HANDLE_FUNC(std::shared_ptr<AVBufferMock>);
    TRANS_HANDLE_FUNC(std::shared_ptr<AVMemoryMock>);
    TRANS_HANDLE_FUNC(std::shared_ptr<FormatMock>);
    TRANS_HANDLE_FUNC(ParameterWithAttrMock);
    TRANS_HANDLE_FUNC(OH_AVBuffer *);
    TRANS_HANDLE_FUNC(OH_AVMemory *);
    TRANS_HANDLE_FUNC(OH_AVFormat *);
    Any any_;
};

enum CodecBufferType {
    TEST_AVMEMORY_CAPI,
    TEST_AVBUFFER_CAPI,
    TEST_AVFORMAT_CAPI,
    TEST_AVMEMORY_MOCK,
    TEST_AVBUFFER_MOCK,
    TEST_AVFORMAT_MOCK,
    TEST_PARAMETER_WITH_ATTR_MOCK,
    TEST_INVALID_BUFFER_TYPE,
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif