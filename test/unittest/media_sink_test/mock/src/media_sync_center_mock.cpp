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

#include "media_sync_center_mock.h"

namespace OHOS {
namespace Media {
namespace Test {
Status MockMediaSyncCenter::Reset()
{
    auto result = returnStatusQueue_.front();
    returnInt64Queue_.pop();
    return result;
}

void MockMediaSyncCenter::AddSynchronizer(Pipeline::IMediaSynchronizer* syncer)
{
    (void)syncer;
     synchronizerAdded_ = true;
}

void MockMediaSyncCenter::RemoveSynchronizer(Pipeline::IMediaSynchronizer* syncer)
{
    (void)syncer;
}

bool MockMediaSyncCenter::UpdateTimeAnchor(int64_t clockTime, int64_t delayTime, IMediaTime iMediaTime,
    Pipeline::IMediaSynchronizer* supplier)
{
    lastTimeAnchorClock_ = clockTime;
    (void)delayTime;
    (void)iMediaTime;
    (void)supplier;
    updateTimeAnchorTimes_++;
    return returnBool_;
}

int64_t MockMediaSyncCenter::GetMediaTimeNow()
{
    auto result = returnInt64Queue_.front();
    returnInt64Queue_.pop();
    return result;
}

int64_t MockMediaSyncCenter::GetClockTimeNow()
{
    auto result = returnInt64Queue_.front();
    returnInt64Queue_.pop();
    return result;
}

int64_t MockMediaSyncCenter::GetAnchoredClockTime(int64_t mediaTime)
{
    (void)mediaTime;
    auto result = returnInt64Queue_.front();
    returnInt64Queue_.pop();
    return result;
}

void MockMediaSyncCenter::ReportPrerolled(Pipeline::IMediaSynchronizer* supplier)
{
    (void)supplier;
}

void MockMediaSyncCenter::ReportEos(Pipeline::IMediaSynchronizer* supplier)
{
    (void)supplier;
}

void MockMediaSyncCenter::SetMediaTimeRangeStart(int64_t startMediaTime, int32_t trackId,
    Pipeline::IMediaSynchronizer* supplier)
{
    (void)startMediaTime;
    (void)trackId;
    (void)supplier;
    setMediaRangeStartTime_++;
}

void MockMediaSyncCenter::SetMediaTimeRangeEnd(int64_t endMediaTime, int32_t trackId,
    Pipeline::IMediaSynchronizer* supplier)
{
    mediaRangeEndValue_ = endMediaTime;
    (void)trackId;
    (void)supplier;
    setMediaRangeEndTime_++;
}

int64_t MockMediaSyncCenter::GetSeekTime()
{
    auto result = returnInt64Queue_.front();
    returnInt64Queue_.pop();
    return result;
}

Status MockMediaSyncCenter::SetPlaybackRate(float rate)
{
    (void)rate;
    auto result = returnStatusQueue_.front();
    returnStatusQueue_.pop();
    return result;
}

float MockMediaSyncCenter::GetPlaybackRate()
{
    return returnFloat_;
}

void MockMediaSyncCenter::SetMediaStartPts(int64_t startPts)
{
    (void)startPts;
}

int64_t MockMediaSyncCenter::GetMediaStartPts()
{
    startPtsGetTime_++;
    return GetRetInt64Value();
}
 
int64_t MockMediaSyncCenter::GetRetInt64Value()
{
    if (returnInt64Queue_.empty()) {
        return 0;
    }
    auto result = returnInt64Queue_.front();
    returnInt64Queue_.pop();
    return result;
}
 
void MockMediaSyncCenter::SetLastVideoBufferPts(int64_t bufferPts)
{
    (void)bufferPts;
    setLastVideoBufferPtsTimes_++;
}

void MockMediaSyncCenter::SetAudioRenderPts(int64_t audioRenderPts)
{
    (void)audioRenderPts;
}
}  // namespace Test
}  // namespace Media
}  // namespace OHOS