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

#ifndef HISTREAMER_DASH_MPD_DOWNLOADER_H
#define HISTREAMER_DASH_MPD_DOWNLOADER_H

#include "dash_common.h"
#include "dash_mpd_def.h"
#include "dash_mpd_parser.h"
#include "dash_mpd_manager.h"
#include "dash_period_manager.h"
#include "dash_adpt_set_manager.h"
#include "dash_representation_manager.h"
#include "download/downloader.h"
#include "media_downloader.h"
#include "meta/media_types.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

using DashMpdEvent = enum DashMpdEvent {
    DASH_MPD_EVENT_OPEN_OK,
    DASH_MPD_EVENT_STREAM_INIT,
    DASH_MPD_EVENT_PARSE_OK
};

enum DashMpdGetRet {
    DASH_MPD_GET_ERROR,
    DASH_MPD_GET_DONE, // get segment normal
    DASH_MPD_GET_UNDONE, // get segment null, as segment list is empty
    DASH_MPD_GET_FINISH // get segment finish
};

using DashMpdSwitchType = enum DashMpdSwitchType {
    DASH_MPD_SWITCH_TYPE_NONE,
    DASH_MPD_SWITCH_TYPE_AUTO,
    DASH_MPD_SWITCH_TYPE_SMOOTH
};

struct DashMpdBitrateParam {
    DashMpdBitrateParam()
    {
        waitSegmentFinish_ = false;
        waitSidxFinish_ = false;
        streamId_ = -1;
        bitrate_ = 0;
        type_ = DASH_MPD_SWITCH_TYPE_NONE;
        position_ = -1;
        nextSegTime_ = 0;
    }

    bool waitSegmentFinish_;
    bool waitSidxFinish_;
    int streamId_;
    unsigned int bitrate_;
    DashMpdSwitchType type_;
    int64_t position_;
    unsigned int nextSegTime_;
};

struct DashMpdTrackParam {
    DashMpdTrackParam()
    {
        waitSidxFinish_ = false;
        isEnd_ = false;
        type_ = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
        streamId_ = -1;
        position_ = -1;
        nextSegTime_ = 0;
    }

    bool waitSidxFinish_;
    bool isEnd_;
    MediaAVCodec::MediaType type_;
    int streamId_;
    int64_t position_;
    unsigned int nextSegTime_;
};

struct MediaSegSampleInfo {
    MediaSegSampleInfo()
    {
        segCount_ = 0;
        segDuration_ = 0;
    }

    int segCount_;
    unsigned int segDuration_;
    std::string mediaUrl_;
};

struct DashMpdCallback {
    virtual ~DashMpdCallback() = default;
    virtual void OnMpdInfoUpdate(DashMpdEvent mpdEvent) = 0;
    virtual void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>>& drmInfos) = 0;
};

class DashMpdDownloader {
public:
    DashMpdDownloader();
    virtual ~DashMpdDownloader();

    void Open(const std::string &url);
    void Close(bool isAsync);
    void SetStatusCallback(StatusCallbackFunc cb);
    void SetMpdCallback(DashMpdCallback *callback);
    int64_t GetDuration() const;
    Seekable GetSeekable() const;
    std::vector<uint32_t> GetBitRates() const;
    std::vector<uint32_t> GetBitRatesByHdr(bool isHdr) const;
    bool IsBitrateSame(uint32_t bitRate);
    void SeekToTs(int streamId, int64_t seekTime, std::shared_ptr<DashSegment> &seg) const;
    void UpdateDownloadFinished(const std::string &url);
    int GetInUseVideoStreamId() const;
    DashMpdGetRet GetNextSegmentByStreamId(int streamId, std::shared_ptr<DashSegment> &seg);
    DashMpdGetRet GetBreakPointSegment(int streamId, int64_t breakpoint, std::shared_ptr<DashSegment> &seg);
    DashMpdGetRet GetNextVideoStream(DashMpdBitrateParam &param, int &streamId);
    DashMpdGetRet GetNextTrackStream(DashMpdTrackParam &param);
    Status GetStreamInfo(std::vector<StreamInfo> &streams);
    std::shared_ptr<DashStreamDescription> GetStreamByStreamId(int streamId);
    std::shared_ptr<DashStreamDescription> GetUsingStreamByType(MediaAVCodec::MediaType type);
    std::shared_ptr<DashInitSegment> GetInitSegmentByStreamId(int streamId);
    void SetCurrentNumberSeqByStreamId(int streamId, int64_t numberSeq);
    void UpdateCurrentNumberSeqByTime(const std::shared_ptr<DashStreamDescription> &streamDesc, uint32_t nextSegTime);
    void SetHdrStart(bool isHdrStart);
    void SetInitResolution(unsigned int width, unsigned int height);
    void SetDefaultLang(const std::string &lang, MediaAVCodec::MediaType type);
    void SetInterruptState(bool isInterruptNeeded);
    std::string GetUrl() const;

private:
    void ParseManifest();
    void ParseSidx();
    void OpenStream(std::shared_ptr<DashStreamDescription> stream);
    void DoOpen(const std::string &url, int64_t startRange = -1, int64_t endRange = -1);
    bool SaveData(uint8_t *data, uint32_t len);
    void SetOndemandSegBase();
    bool SetOndemandSegBase(std::list<DashAdptSetInfo *> adptSetList);
    bool SetOndemandSegBase(std::list<DashRepresentationInfo *> repList);
    bool CheckToDownloadSidxWithInitSeg(std::shared_ptr<DashStreamDescription> streamDesc);
    bool GetStreamsInfoInMpd();
    void GetStreamsInfoInPeriod(DashPeriodInfo *periodInfo, unsigned int periodIndex, const std::string &mpdBaseUrl);
    void GetStreamsInfoInAdptSet(DashAdptSetInfo *adptSetInfo, const std::string &periodBaseUrl,
                                 DashStreamDescription &streamDesc);
    unsigned int GetResolutionDelta(unsigned int width, unsigned int height);
    bool IsChoosedVideoStream(const std::shared_ptr<DashStreamDescription> &choosedStream,
        const std::shared_ptr<DashStreamDescription> &currentStream);
    bool IsNearToInitResolution(const std::shared_ptr<DashStreamDescription> &choosedStream,
        const std::shared_ptr<DashStreamDescription> &currentStream);
    bool IsLangMatch(const std::string &lang, MediaAVCodec::MediaType type);
    bool ChooseStreamToPlay(MediaAVCodec::MediaType type);
    unsigned int GetSegTimeBySeq(const std::vector<std::shared_ptr<DashSegment>> &segments, int64_t segSeq);
    DashSegmentInitValue GetSegmentsInMpd(std::shared_ptr<DashStreamDescription> streamDesc);
    DashSegmentInitValue GetSegmentsInPeriod(DashPeriodInfo *periodInfo, const std::string &mpdBaseUrl,
                                             std::shared_ptr<DashStreamDescription> streamDesc);
    DashSegmentInitValue GetSegmentsInAdptSet(DashAdptSetInfo *adptSetInfo, const std::string &periodBaseUrl,
                                              std::shared_ptr<DashStreamDescription> streamDesc);
    DashSegmentInitValue GetSegmentsInRepresentation(DashRepresentationInfo *repInfo, const std::string &adptSetBaseUrl,
                                                     std::shared_ptr<DashStreamDescription> streamDesc);
    DashSegmentInitValue GetSegmentsByPeriodInfo(DashPeriodInfo *periodInfo, DashAdptSetInfo *adptSetInfo,
                                                 std::string &periodBaseUrl,
                                                 std::shared_ptr<DashStreamDescription> streamDesc);
    DashSegmentInitValue GetSegmentsByAdptSetInfo(const DashAdptSetInfo *adptSetInfo,
                                                  const DashRepresentationInfo *repInfo,
                                                  std::string &baseUrl,
                                                  std::shared_ptr<DashStreamDescription> streamDesc);
    DashRepresentationInfo *GetRepresemtationFromAdptSet(DashAdptSetInfo *adptSetInfo, unsigned int repIndex);

    DashSegmentInitValue GetSegmentsWithSegTemplate(const DashSegTmpltInfo *segTmpltInfo, std::string id,
                                                    std::shared_ptr<DashStreamDescription> streamDesc);
    DashSegmentInitValue GetSegmentsWithTmpltStatic(const DashSegTmpltInfo *segTmpltInfo, const std::string &mediaUrl,
                                                    std::shared_ptr<DashStreamDescription> streamDesc);
    DashSegmentInitValue GetSegmentsWithTmpltDurationStatic(const DashSegTmpltInfo *segTmpltInfo,
                                                            const std::string &mediaUrl, unsigned int timeScale,
                                                            std::shared_ptr<DashStreamDescription> desc);
    DashSegmentInitValue GetSegmentsWithTmpltTimelineStatic(const DashSegTmpltInfo *segTmpltInfo,
                                                            const std::string &mediaUrl, unsigned int timeScale,
                                                            std::shared_ptr<DashStreamDescription> desc);
    DashSegmentInitValue GetSegmentsInOneTimeline(const DashSegTimeline *timeline, const MediaSegSampleInfo &sampleInfo,
                                                  int64_t &segmentSeq, uint64_t &startTime,
                                                  std::shared_ptr<DashStreamDescription> streamDesc);

    DashSegmentInitValue GetSegmentsWithSegList(const DashSegListInfo *segListInfo, const std::string &baseUrl,
                                                std::shared_ptr<DashStreamDescription> streamDesc);
    void GetSegDurationFromTimeline(unsigned int periodDuration, unsigned int timeScale,
                                    const DashMultSegBaseInfo *multSegBaseInfo, std::list<unsigned int> &durationList);
    int GetSegCountFromTimeline(DashList<DashSegTimeline *>::iterator &it,
                                const DashList<DashSegTimeline *>::iterator &end,
                                unsigned int periodDuration, unsigned int timeScale, uint64_t startTime);

    DashSegmentInitValue GetSegmentsWithBaseUrl(std::list<std::string> baseUrlList,
                                                std::shared_ptr<DashStreamDescription> streamDesc);

    bool GetInitSegFromPeriod(const std::string &periodBaseUrl, const std::string &repId,
                              std::shared_ptr<DashStreamDescription> streamDesc);
    bool GetInitSegFromAdptSet(const std::string &adptSetBaseUrl, const std::string &repId,
                               std::shared_ptr<DashStreamDescription> streamDesc);
    bool GetInitSegFromRepresentation(const std::string &repBaseUrl, const std::string &repId,
                                      std::shared_ptr<DashStreamDescription> streamDesc);
    DashMpdGetRet GetSegmentsInNewStream(std::shared_ptr<DashStreamDescription> destStream);
    void UpdateInitSegUrl(std::shared_ptr<DashStreamDescription> streamDesc, const DashUrlType *urlType,
                          int segTmpltFlag,
                          std::string representationID);

    bool PutStreamToDownload();
    void GetDrmInfos(std::vector<DashDrmInfo>& drmInfos);
    void ProcessDrmInfos();
    void GetDrmInfos(const std::string &drmId, const DashList<DashDescriptor *> &contentProtections,
                     std::vector<DashDrmInfo> &drmInfoList);
    void BuildDashSegment(std::list<std::shared_ptr<SubSegmentIndex>> &subSegIndexList) const;
    void GetStreamDescriptions(const std::string &periodBaseUrl, DashStreamDescription &streamDesc,
                               const std::string &adptSetBaseUrl,
                               std::list<DashRepresentationInfo *> &repList);
    void GetAdpDrmInfos(std::vector<DashDrmInfo> &drmInfos, DashPeriodInfo *const &periodInfo,
                        const std::string &periodDrmId);

private:
    std::string url_ {};
    std::string downloadContent_ {}; // mpd content or sidx content
    std::string defaultAudioLang_ {};
    std::string defaultSubtitleLang_ {};
    DashMpdCallback* callback_ {nullptr};
    std::shared_ptr<Downloader> downloader_ {nullptr};
    std::shared_ptr<DownloadRequest> downloadRequest_ {nullptr};
    std::shared_ptr<DashMpdParser> mpdParser_ {nullptr};
    std::shared_ptr<DashMpdManager> mpdManager_ {nullptr};
    std::shared_ptr<DashPeriodManager> periodManager_ {nullptr};
    std::shared_ptr<DashAdptSetManager> adptSetManager_ {nullptr};
    std::shared_ptr<DashRepresentationManager> representationManager_ {nullptr};
    DashMpdInfo* mpdInfo_ {nullptr};
    unsigned int duration_ {0};
    std::vector<std::shared_ptr<DashStreamDescription>> streamDescriptions_;
    std::shared_ptr<DashStreamDescription> currentDownloadStream_ {nullptr};
    DataSaveFunc dataSave_ {nullptr};
    StatusCallbackFunc statusCallback_ {nullptr};
    bool ondemandSegBase_ {false};
    bool notifyOpenOk_ {false};
    bool isHdrStart_ {false};
    unsigned int initResolution_ {0};
    int selectVideoStreamId_ {-1};
    std::atomic<bool> isInterruptNeeded_{false};
    std::vector<DashDrmInfo> localDrmInfos_;
};
}
}
}
}
#endif