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

#ifndef NATIVE_AVSOURCE_H
#define NATIVE_AVSOURCE_H

#include <stdint.h>
#include "native_avcodec_base.h"
#include "native_averrors.h"
#include "native_avformat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct OH_AVSource OH_AVSource;

/**
 * @brief Creates an OH_AVSource instance that models the media with data source.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param dataSource User customized media resource.
 * @return Returns a pointer to an OH_AVSource instance
 * @since 12
*/
OH_AVSource *OH_AVSource_CreateWithDataSource(OH_AVDataSource *dataSource);

/**
 * @brief Creates an OH_AVSource instance that models the media at the URI.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param uri An URI for a remote media resource.
 * @return Returns a pointer to an OH_AVSource instance
 * @since 10
*/
OH_AVSource *OH_AVSource_CreateWithURI(char *uri);

/**
 * @brief Creates an OH_AVSource instance that models the media at the FileDescriptor.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param fd The fileDescriptor of data source.
 * @param offset The offset into the file to start reading.
 * @param size The file size in bytes.
 * @return Returns a pointer to an OH_AVSource instance
 * @since 10
*/
OH_AVSource *OH_AVSource_CreateWithFD(int32_t fd, int64_t offset, int64_t size);

/**
 * @brief Destroy the OH_AVSource instance and free the internal resources.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param source Pointer to an OH_AVSource instance.
 * @return Returns AV_ERR_OK if the execution is successful,
 * otherwise returns a specific error code, refer to {@link OH_AVErrCode}
 * @since 10
*/
OH_AVErrCode OH_AVSource_Destroy(OH_AVSource *source);

/**
 * @brief Get the format info of source.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param source Pointer to an OH_AVSource instance.
 * @return Returns the source's format info.
 * @since 10
*/
OH_AVFormat *OH_AVSource_GetSourceFormat(OH_AVSource *source);

/**
 * @brief Get the format info of track.
 * @syscap SystemCapability.Multimedia.Media.Spliter
 * @param source Pointer to an OH_AVSource instance.
 * @param trackIndex The track index to get format.
 * @return Returns the track's format info.
 * @since 10
*/
OH_AVFormat *OH_AVSource_GetTrackFormat(OH_AVSource *source, uint32_t trackIndex);

#ifdef __cplusplus
}
#endif

#endif // NATIVE_AVSOURCE_H