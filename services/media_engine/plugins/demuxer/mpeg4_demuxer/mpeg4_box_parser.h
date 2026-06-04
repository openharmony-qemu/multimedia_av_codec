/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef MPEG4_BOX_PARSER_H
#define MPEG4_BOX_PARSER_H

#include <memory>
#include <set>
#include <map>
#include <string>
#include "converter.h"
#include "mpeg4_audio_parser.h"
#include "mpeg4_sample_helper.h"
#include "plugin/plugin_definition.h"
#include "plugin/plugin_info.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {

class MPEG4DemuxerPlugin;

class MPEG4AtomParser {
public:
    explicit MPEG4AtomParser();
    ~MPEG4AtomParser();
    struct CodecParams {
        std::unique_ptr<uint8_t[]> data = nullptr;
        int64_t extradataSize = 0;
        TrackType trackType = INVALID_TYPE;
        HdrBoxInfo hdrBoxInfo {false, false};
        CodecParams() = default;
    };

    struct Track {
        std::shared_ptr<Track> next = nullptr;
        uint32_t trackIndex = 0;
        int32_t trackId = 0;
        std::shared_ptr<MPEG4SampleHelper> sampleHelper = nullptr;
        std::unique_ptr<int32_t[]> displayMatrix = nullptr;  // 3x3 display matrix (9个int32_t), 按行存储
        bool hasDisplayMatrix = false;  // 是否有有效的display matrix
        uint32_t currentSampleIndex = 0;
        int64_t duration = 0; // track的duration，mdhd box中定义的
        int64_t totalElstDuration = 0; // elst box中所有entry的segment duration之和
        int64_t sidxDuration = 0; // sidx box的dts
        int64_t sidxReferencedSize = 0; // sum of referenced_size from sidx entries
        int64_t elstInitEmptyEdit = 0;
        int64_t elstShiftStartTime = 0;
        std::vector<MPEG4SampleHelper::EditListEntry> editListEntries;
        uint32_t tkhdWidth = 0;
        uint32_t tkhdHeight = 0;
        uint32_t stsdWidth = 0;
        uint32_t stsdHeight = 0;
        uint32_t hSpacing = 0;
        uint32_t vSpacing = 0;
        bool hasPasp = false;
        bool inferredTrackType = false;
        CodecParams codecParms{};

        Track() = default;
    };
    
    struct FragmentEntry {
        int64_t firstDts = -1;
        int64_t moofOffset;
        int64_t nextAtomOffset = -1;
        int64_t duration = -1;
        bool hasRead = false;
    };
    
    MediaInfo GetMediaInfo() const;
    std::shared_ptr<Track> GetFirstTrack() const;
    std::shared_ptr<Meta> GetUserFormat() const;
    void GetFragmentEntry(std::vector<FragmentEntry>& entry);
    void SetDataReader(const std::shared_ptr<DemuxerDataReader> dataReader);
    void SetSeekable(const Seekable seekable);
    bool GetHasSidx();
    bool GetHasMoof();
    Status MPEG4ParseHeader(const std::shared_ptr<DataSource>& source, Seekable seekable, int64_t readTimeoutMs = -1);
    Status ParseMoof(const std::shared_ptr<Track>& track, int64_t offset);
    
private:
    bool formatDebug_ = false;
    std::shared_ptr<Track> firstTrack_;
    std::shared_ptr<Track> lastTrack_;
    std::shared_ptr<Track> currentTrack_;

    enum ColorRange : uint32_t {
        UNSPECIFIED = 0,
        MPEG = 1,
        JPEG = 2,
        NB = 3,
    };

    struct Av3aConfigParams {
        int32_t soundbedChannels = 0;
        int32_t objectNumber = 0;
        int32_t hoaOrder = 0;
        AudioChannelLayout channelLayout = AudioChannelLayout::UNKNOWN;
    };
    
    struct MPEG4Atom {
        int32_t type = 0;
        uint64_t size = 0;
        uint64_t dataSize = 0;
        int64_t dataOffset = 0;
        uint64_t headerSize = 0;
    };

    struct ParseContext {
        int64_t offset = 0; // 当前解析位置
        int64_t dataOffset = 0; // 数据偏移位置
        std::set<uint32_t> compatibleBrands;
        std::vector<uint32_t> path;
        std::vector<std::string> metaKeys;
        int32_t movieDisplayMatrix[3][3] = {{0}}; // movie level 3x3显示变换矩阵
        uint32_t metaKeysCount = 0;
        bool hasMovieDisplayMatrix = false; // 是否有有效的movie显示矩阵
        bool foundHdlrMdta = false;
        bool founditunesMetadata = false;
        bool isUserMeta = false;
        bool foundFileMetaHdlrGltf = false;
        int64_t fileMetaIdatPayloadOffset = -1;
    };
    ParseContext ctx_;

    struct SidxParseContext {
        int64_t firstMoofOffset = 0;
        uint64_t dts = 0;
        uint32_t entryCount = 0;
    };
    SidxParseContext sidxCtx_;

    struct FrameRate {
        double value;
        std::string name;
    };

    struct FrameRateRange {
        double min;
        double max;
        double target;
        const char* name;
    };

    struct MvhdInfo {
        uint64_t creationTime = 0;
        uint64_t duration = 0;
        uint32_t timeScale = 1;
        int32_t matrixOffset = 0;
    };

    uint32_t trackCount_ = 0;
    int64_t moofOffset_ = 0;
    using ParseFunction = Status (MPEG4AtomParser::*)(MPEG4Atom atom, int32_t depth, ParseContext* ctx);
    ParseFunction FindAtomParser(const MPEG4Atom& atom, ParseContext* ctx);
    std::unordered_map<int32_t, ParseFunction> MPEG4ParseTable_;
    std::vector<FragmentEntry> fragmentEntry_;
    std::shared_ptr<DataSource> dataSource_ {nullptr};
    std::shared_ptr<DemuxerDataReader> dataReader_ {nullptr};
    bool isIsom_;
    bool hasMdatBox_;
    bool hasSidxBox_;
    bool isDistributedSidx_;
    bool hasIncompleteSidxReference_;
    bool hasMoofBox_;
    bool moovFound_;
    MediaInfo mediaInfo_;
    Seekable seekable_;
    std::shared_ptr<Meta> userFormat_ = nullptr;
    void InitParseTable();
    void SetCodecConfig(std::shared_ptr<Track> track);
    void AdapterFormat(std::shared_ptr<Track> track);
    bool IsValidTrackIndex(const MediaInfo& mediaInfo, uint32_t index);
    bool UnderMetaPath(const std::vector<uint32_t>& path, int32_t depth);
    bool UnderFileLevelMetaPath(const std::vector<uint32_t>& path, int32_t depth);
    void TrySetGltfOffset(ParseContext* ctx);

    // Display matrix 相关辅助函数
    void CalculateSampleAspectRatio(std::shared_ptr<Track> track, uint32_t width, uint32_t height);
    void CalculateSARFromTrackInfo(std::shared_ptr<Track> track);
    void SetTrackDisplayMatrix(std::shared_ptr<Track> track, const int32_t resultMatrix[3][3],
        uint32_t width, uint32_t height);
    Status ParseDisplayMatrix(const uint8_t* data, int32_t matrix[3][3]);
    // 视频方向解析相关辅助函数
    void ParseOrientationFromMatrix(std::shared_ptr<Track> track);
    void ParseRotationTypeFromMatrix(std::shared_ptr<Track> track);
    void ParseImageTrackData(std::shared_ptr<Track> track);
    // 计算流信息相关辅助函数
    void CalculateVideoFrameRate(std::shared_ptr<Track> track);
    void CalculateTrackBitrates(std::shared_ptr<Track> track);
    bool ShouldSkipCalculatedBitrate(uint32_t trackIndex, std::string& mime);
    int64_t GetBitrateDuration(const std::shared_ptr<Track>& track);
    int64_t GetBitrateTotalSize(const std::shared_ptr<Track>& track);
    bool HasEnoughSamplesForBitrate(const std::shared_ptr<Track>& track, const std::string& mime);
    bool GetBitrateDurationAndSize(std::shared_ptr<Track> track, int64_t& duration, int64_t& totalSize);
    void CalculateVideoDelay(std::shared_ptr<Track> track);
    void NormalizeAudioAttributes(std::shared_ptr<Track> track);
    AudioSampleFormat GetAv3aSampleFormat(int32_t bitsPerRawSample) const;
    AudioChannelLayout GetAv3aChannelLayout(int32_t channelCount, int32_t sampleRate) const;
    void CalculateMPEG4Attributes();
    void SetTrackType(std::shared_ptr<Track> track);
    void SetFileDuration();
    void UpdateTrackStartTime(const std::shared_ptr<Track>& track, int64_t& trackStartTime,
        int64_t& containerStartTimeForTrack);
    void SetStartTime();
    void SetHEVCDefaultValue();
    int64_t GetElstDuration(std::shared_ptr<Track> track);
    int64_t GetMpeg4FileDuration();
    int64_t GetTrackDuration(std::shared_ptr<Track> track, bool calculateBitrate = false);
    int64_t GetSidxTrackStartTime(const std::shared_ptr<Track>& track, int64_t containerStartTime);
    int64_t GetSidxSampleEndTime(const std::shared_ptr<Track>& track, int32_t timeScale);
    int64_t FindMaxTrackDuration(int64_t& maxDuration, uint32_t& maxDurationTrackIndex);
    double CalculateDuration(std::shared_ptr<Track> track, bool isRawFile);
    // 元数据解析辅助函数
    std::string ConvertLanguageCode(uint16_t langcode);
    std::string GetMetadataKey(uint32_t atomType, ParseContext* ctx);
    bool SetMetadataValue(const std::string& key, const std::string& value);
    void ParseLocationString(const std::string& locationStr);
    // 辅助轨相关辅助函数
    void InferTrackTypeFromMime(const std::string& mimeType);
    Status ProcessAuxiliaryHandler(MPEG4Atom currentAtom, ParseContext* parseCtx);
    Status ParseSampleDescriptionEntry(int32_t depth, ParseContext* ctx);
    // 音频属性相关辅助函数
    bool ParseDecoderConfigDescriptor(const uint8_t* data, uint64_t dataSize, uint64_t& offset);
    bool ParseAACConfig(const uint8_t* data, uint32_t size, uint32_t& sampleRate, uint32_t& channels,
        AudioChannelLayout &layout);
    bool ParseAACSpecificConfig(const uint8_t* data, uint32_t size);
    bool ParseFLACExtradata(const uint8_t* data);
    bool ParseOPUSExtradata(const uint8_t* data, uint64_t dataSize);
    void SetAmrAudioAttribute(std::string mime);
    void SetMpegAudioAttribute();
    bool ParseJpegDimensions(const uint8_t* data, uint64_t size, int32_t& width, int32_t& height);
    struct AudioVersionInfoContext {
        ParseContext* ctx = nullptr;
        uint16_t version = 0;
        uint16_t& channelCount;
        uint16_t& sampleSize;
        uint32_t& sampleRate;
        uint32_t& samplePerFrame;
    };
    Status ParseAudioVersionInfo(MPEG4Atom currentAtom, AudioVersionInfoContext& context);
    void SetAudioMimeTypeAndSampleSize(const MPEG4Atom& currentAtom, uint16_t& sampleSize);
    void SetRawPCMSampleFormat(const MPEG4Atom& currentAtom, uint16_t& sampleSize);
    int32_t GetRawPCMBitsPerSample(const MPEG4Atom& currentAtom, uint16_t& sampleSize);
    int64_t GetChannelLayoutByBitmap(uint32_t tag, uint32_t bitmap);
    Status SetVorbisAudioAttribute();
    Status SetAudioAttribute();
    void SetHdrTypeInfo(MPEG4Atom currentAtom);

    Status ParseAtomHeader(MPEG4Atom& atom, std::string& typeStr, int32_t depth, bool &isZeroSizeAtom,
        ParseContext* ctx);
    Status ProcessMetaHandler(MPEG4Atom currentAtom, ParseContext* parseCtx);
    Status ProcessTfhdAtom(const MPEG4Atom& atom);
    Status UpdateDca3TrackMeta(DemuxerBitReader& bitReader, const Av3aConfigParams& params);
    Status ReadDca3Payload(MPEG4Atom currentAtom, ParseContext* ctx,
                                        std::unique_ptr<uint8_t[]>& dca3Data, uint64_t& dataSize);
    Status ParseDca3ContentType(DemuxerBitReader& bitReader, uint8_t contentType, Av3aConfigParams& params);

    bool UpdataMoofInfo(MPEG4Atom& atom, Status& ret, int64_t& currentOffset, FragmentEntry& frag, int64_t sidxDts);
    // 解析函数
    Status ParseContainerAtom(const MPEG4Atom& containerAtom, int32_t depth, ParseContext* ctx);
    Status ParseAudioAttributes(MPEG4Atom currentAtom, ParseContext* parseCtx);
    Status ParseCover(MPEG4Atom currentAtom, uint32_t dataType, uint64_t stringSize, ParseContext* parseCtx);
    Status ParseItunesMetadata(MPEG4Atom currentAtom, std::string& key, ParseContext* parseCtx);
    Status ParseTraditionalMetadata(MPEG4Atom currentAtom, std::string& key, ParseContext* parseCtx);
    Status ReadMetadataString(std::string& key, int64_t offset, uint64_t size, uint32_t dataType, bool isUserMeta);
    Status SetAlacAudioAttributes(const uint8_t* buffer);
    Status ParseAlacEntry(ParseContext& ctx);
    Status ParseDca3Entry(MPEG4Atom currentAtom, ParseContext* ctx);
    Status ParseIccProfile(int64_t offset, uint64_t dataSize);
    Status ParseColorParams(int64_t offset, uint64_t dataSize, const std::string& colorType, ParseContext* parseCtx);
    std::shared_ptr<Track> FindSidxTrack(uint32_t referenceId);
    void CheckSidxReferenceEnd();
    Status ParseSidxEntries(const uint8_t* headerInfo, int64_t entryOffset,
        SidxParseContext& sidxCtx, std::shared_ptr<Track> currentTrack);
    Status FindNextMoof(int64_t& headerOffset, uint32_t& atomSize);
    Status UpdateFragmentEntryAndOffset(bool inEntry, uint32_t fragIndex, FragmentEntry& frag,
        int64_t moofOffset, int64_t& currentOffset);
    Status MPEG4ParseAtom(int32_t depth, ParseContext* ctx);
    Status ParseFtyp(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMoov(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseWide(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMdat(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    MvhdInfo ParseMvhdInfo(const uint8_t* headerInfo) const;
    void SetMvhdMediaInfo(const MvhdInfo& mvhdInfo);
    Status ParseMvhdDisplayMatrix(const MPEG4Atom& currentAtom, const uint8_t* headerInfo, ParseContext* ctx);
    Status ParseMvhd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseTrak(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseTkhd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseEdts(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseElst(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseTref(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMdia(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMdhd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    void UpdateMdhdTimeOffset(uint32_t timeScale);
    Status ParseHdlr(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status SetTrackTypeByHdlr(MPEG4Atom currentAtom, uint32_t handlerType, ParseContext* ctx);
    Status ParseMinf(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseStbl(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseStsd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseVideoSampleEntry(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseCodecConfig(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseAudioSampleEntry(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseSubtitleSampleEntry(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMebx(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseBtrt(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParsePasp(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseEsds(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseDfla(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseDops(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParsePcmc(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseEnda(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseColr(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseAclr(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseGlbl(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseStts(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseStss(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseStsc(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseStsz(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseStco(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseCtts(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseSdtp(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseFiel(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMvex(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseTrex(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseSidx(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseUserDataString(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseUdta(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMeta(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseKeys(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseIlst(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseLoci(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseUdtaGenre(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMoovGenre(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseLabl(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseWave(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseFrma(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseSchi(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseSchm(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseKind(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseChan(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseChap(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseIdat(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseClli(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMdcv(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
};
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_BOX_PARSER_H
