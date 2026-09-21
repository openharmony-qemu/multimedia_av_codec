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

#include <cstdint>
#include <memory>
#include <set>
#include <map>
#include <string>
#include "converter.h"
#include "mpeg4_audio_parser.h"
#include "mpeg4_sample.h"
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
        bool hasValidExtradata = false;
        std::unique_ptr<uint8_t[]> data = nullptr;
        int64_t extradataSize = 0;
        int64_t annexbExtradataSize = 0;
        TrackType trackType = INVALID_TYPE;
        HdrBoxInfo hdrBoxInfo {false, false};
        CodecParams() = default;
    };

    struct Mpeg4DrmSubSample {
        uint32_t clearHeaderLen = 0;
        uint32_t payLoadLen = 0;
    };

    struct Mpeg4DrmCencInfo {
        uint32_t algo = 0;
        uint8_t keyId[16] = {0};
        uint32_t keyIdLen = 0;
        uint8_t iv[16] = {0};
        uint32_t ivLen = 0;
        uint32_t encryptBlocks = 0;
        uint32_t skipBlocks = 0;
        std::vector<Mpeg4DrmSubSample> subSamples;
        uint32_t subSampleNum = 0;
        uint32_t mode = 0;
    };

    struct Mpeg4SampleEncryptionEntry {
        std::vector<uint8_t> iv;
        std::vector<Mpeg4DrmSubSample> subSamples;
    };

    struct Mpeg4TrackDrmInfo {
        bool isEncrypted = false;
        uint32_t schemeType = 0;
        uint32_t perSampleIvSize = 0;
        Mpeg4DrmCencInfo defaultCencInfo;
        std::vector<Mpeg4SampleEncryptionEntry> sampleEncryptionEntries;
        // For saiz/saio auxiliary info parsing
        uint8_t auxiliaryInfoDefaultSize = 0;
        std::vector<uint8_t> auxiliaryInfoSizes;
        std::vector<uint64_t> auxiliaryOffsets;
        uint32_t auxiliaryInfoSampleCount = 0;
        uint32_t auxiliaryOffsetsCount = 0;
    };

    struct Track {
        std::shared_ptr<Track> next = nullptr;
        uint32_t trackIndex = 0;
        int32_t trackId = 0;
        std::shared_ptr<MPEG4SampleHelper> sampleHelper = nullptr;
        std::unique_ptr<int32_t[]> displayMatrix = nullptr;  // 3x3 display matrix (9个int32_t), 按行存储
        bool hasDisplayMatrix = false;  // 是否有有效的display matrix
        uint32_t currentSampleIndex = 0;
        int32_t currentSamplePos = 0; // 当前帧读取拷贝位置
        uint32_t currentFragmentIndexBase = 0; // 当前fragment在全局sample中的起始索引
        int64_t duration = 0; // track的duration，mdhd box中定义的
        int32_t timeScale = 1; // mdhd timescale, kept internal and not exported to track meta
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
        uint16_t audioSampleEntryChannelCount = 0;
        uint16_t audioSampleEntryBitsPerCodedSample = 0;
        CodecParams codecParms{};
        std::vector<std::vector<uint8_t>> codecConfigs;
        bool hasMultipleCodecConfigs = false;
        uint32_t currentCodecConfigIndex = UINT32_MAX;
        Mpeg4TrackDrmInfo drmInfo;
        std::shared_ptr<SampleCache> cache = nullptr;

        Track() = default;
    };

    struct Mpeg4PsshInfo {
        uint8_t uuid[16] = {0};
        uint32_t uuidLen = 0;
        std::vector<uint8_t> psshData;
        uint32_t psshLen = 0;
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
    int32_t GetMovieTimeScale() const;
    Status MPEG4ParseHeader(const std::shared_ptr<DataSource>& source, Seekable seekable, int64_t readTimeoutMs = -1,
        Plugins::DemuxerMode mode = Plugins::DemuxerMode::FULL_PARSE);
    Status ParseMoof(const std::shared_ptr<Track>& track, int64_t offset);
    void ClearTrackSaizSaioState();
    void PadSampleEncryptionEntries();
    const std::vector<Mpeg4PsshInfo>& GetPsshList() const;
    void NotifyInitializationCompleted();
    
private:
    std::shared_ptr<Track> firstTrack_;
    std::shared_ptr<Track> lastTrack_;
    uint32_t currentStsdEntryIndex_ = 0;
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
        uint64_t size = 0; // Declared atom size, normalized for size==0 atoms.
        uint64_t effectiveSize = 0; // Atom size used for consumption, including the header.
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
    std::vector<Mpeg4PsshInfo> psshList_;
    std::shared_ptr<DataSource> dataSource_ {nullptr};
    std::shared_ptr<DemuxerDataReader> dataReader_ {nullptr};
    bool isIsom_;
    bool hasMdatBox_;
    bool hasSidxBox_;
    bool isDistributedSidx_;
    bool hasIncompleteSidxReference_;
    bool hasMoofBox_;
    bool moovFound_;
    Status pendingSampleTableStatus_ {Status::OK};
    Plugins::DemuxerMode demuxerMode_ {Plugins::DemuxerMode::FULL_PARSE};
    int32_t movieTimeScale_ = 1;
    MediaInfo mediaInfo_;
    Seekable seekable_;
    std::shared_ptr<Meta> userFormat_ = nullptr;
    void InitParseTable();
    void SetCodecConfig(std::shared_ptr<Track> track);
    void AdapterFormat(std::shared_ptr<Track> track);
    bool IsValidTrackIndex(const MediaInfo& mediaInfo, uint32_t index);
    bool IsLastTrackValid();
    bool UnderMetaPath(const std::vector<uint32_t>& path, int32_t depth);
    bool UnderFileLevelMetaPath(const std::vector<uint32_t>& path, int32_t depth);
    void TrySetGltfOffset(ParseContext* ctx);

    // Display matrix 相关辅助函数
    void CalculateSampleAspectRatio(std::shared_ptr<Track> track, uint32_t width, uint32_t height);
    void CalculateSARFromTrackInfo(std::shared_ptr<Track> track);
    void SetTrackDisplayMatrix(std::shared_ptr<Track> track, const int32_t resultMatrix[3][3],
        uint32_t width, uint32_t height, const size_t len);
    Status ParseDisplayMatrix(const uint8_t* data, int32_t matrix[3][3]);
    // 视频方向解析相关辅助函数
    void ParseOrientationFromMatrix(std::shared_ptr<Track> track);
    void ParseRotationTypeFromMatrix(std::shared_ptr<Track> track);
    void ParseImageTrackData(std::shared_ptr<Track> track);
    // 计算流信息相关辅助函数
    void CalculateVideoFrameRate(std::shared_ptr<Track> track);
    void CalculateTrackBitrates(std::shared_ptr<Track> track);
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
    bool SetMetadataValue(const std::string& key, const std::string& value, bool isTrackMetadata = false);
    void ParseLocationString(const std::string& locationStr);
    // 辅助轨相关辅助函数
    void InferTrackTypeFromMime(const std::string& mimeType);
    Status ProcessAuxiliaryHandler(MPEG4Atom currentAtom, ParseContext* parseCtx);
    Status ParseSampleDescriptionEntry(int32_t depth, ParseContext* ctx);
    // 音频属性相关辅助函数
    bool IsSupportEsds();
    bool IsSupportObjectTypeId(uint8_t objectTypeId);
    Status ParseDecoderConfigDescriptor(const uint8_t* data, uint64_t dataSize, uint64_t& offset);
    bool ParseAACConfig(const uint8_t* data, uint32_t size, uint32_t& sampleRate, uint32_t& channels,
        AudioChannelLayout &layout);
    bool ParseAACSpecificConfig(const uint8_t* data, uint32_t size);
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
    int64_t GetChannelLayoutByBitmap(uint32_t tag, uint32_t bitmap);
    Status SetAudioAttribute();
    void SetHdrTypeInfo(MPEG4Atom currentAtom);

    Status ParseAtomHeader(MPEG4Atom& atom, std::string& typeStr, int32_t depth, bool &isZeroSizeAtom,
        ParseContext* ctx);
    Status ParseAtomSize(MPEG4Atom& atom, int32_t depth, bool& isZeroSizeAtom, ParseContext* ctx);
    Status ValidateAtomSourceRange(MPEG4Atom& atom, const std::string& typeStr, int32_t depth, ParseContext* ctx,
        uint64_t atomInfoSize);
    Status ProcessMetaHandler(MPEG4Atom currentAtom, ParseContext* parseCtx);
    Status ProcessTfhdAtom(const MPEG4Atom& atom);
    Status UpdateDca3TrackMeta(DemuxerBitReader& bitReader, const Av3aConfigParams& params);
    Status ReadDca3Payload(MPEG4Atom currentAtom, ParseContext* ctx,
                                        std::unique_ptr<uint8_t[]>& dca3Data, uint64_t& dataSize);
    Status ParseDca3ContentType(DemuxerBitReader& bitReader, uint8_t contentType, Av3aConfigParams& params);
    void SetMime(std::string mimeType);

    bool UpdataMoofInfo(MPEG4Atom& atom, Status& ret, int64_t& currentOffset, FragmentEntry& frag, int64_t sidxDts);
    bool ProcessMoofMfhd(MPEG4Atom& atom, Status& ret, int64_t& currentOffset);
    bool ProcessMoofTfhd(MPEG4Atom& atom, Status& ret, int64_t& currentOffset);
    bool ProcessMoofTfdt(MPEG4Atom& atom, Status& ret, int64_t& currentOffset);
    bool ProcessMoofTrun(MPEG4Atom& atom, Status& ret, int64_t& currentOffset, FragmentEntry& frag, int64_t sidxDts);
    bool ProcessMoofSenc(MPEG4Atom& atom, Status& ret, int64_t& currentOffset);
    bool ProcessMoofSaiz(MPEG4Atom& atom, Status& ret, int64_t& currentOffset);
    bool ProcessMoofSaio(MPEG4Atom& atom, Status& ret, int64_t& currentOffset);
    // 解析函数
    Status ParseContainerAtom(const MPEG4Atom& containerAtom, int32_t depth, ParseContext* ctx);
    Status PrepareChildAtomInContainer(int64_t endOffset, ParseContext* ctx, bool& isPadding);
    Status ReadExtendedChildAtomSize(int64_t childHeaderEndOffset, int64_t endOffset, ParseContext* ctx,
        uint64_t& childSize);
    Status __attribute__((optnone)) ParseAudioAttributes(MPEG4Atom currentAtom, ParseContext* parseCtx);
    Status ParseCover(MPEG4Atom currentAtom, uint32_t dataType, uint64_t stringSize, ParseContext* parseCtx);
    Status ParseItunesMetadata(MPEG4Atom currentAtom, std::string& key, ParseContext* parseCtx);
    Status ParseTraditionalMetadata(MPEG4Atom currentAtom, std::string& key, ParseContext* parseCtx);
    Status ReadMetadataString(
        std::string& key, int64_t offset, uint64_t size, uint32_t dataType, ParseContext* parseCtx);
    Status SetMetadata(std::string& key, uint8_t* buffer, uint64_t size, uint32_t dataType);
    Status ParseDca3Entry(MPEG4Atom currentAtom, ParseContext* ctx);
    Status ParseColorParams(int64_t offset, uint64_t dataSize, const std::string& colorType, ParseContext* parseCtx);
    std::shared_ptr<Track> FindSidxTrack(uint32_t referenceId);
    void CheckSidxReferenceEnd();
    Status ReadSidxHeaderInfo(MPEG4Atom currentAtom, ParseContext* ctx, std::unique_ptr<uint8_t[]>& headerInfo,
        std::shared_ptr<Track>& currentTrack, uint8_t& version);
    Status ParseSidxContext(MPEG4Atom currentAtom, ParseContext* ctx, const uint8_t* headerInfo, uint8_t version,
        uint64_t& entryOffset);
    Status ParseSidxEntries(const uint8_t* headerInfo, uint64_t entryOffset, uint64_t bufferSize,
        SidxParseContext& sidxCtx, std::shared_ptr<Track> currentTrack);
    Status GetSidxEntryOffset(uint64_t entryOffset, uint64_t bufferSize, uint32_t index,
        uint64_t& currentEntryOffset);
    Status ParseSidxEntry(const uint8_t* headerInfo, uint64_t currentEntryOffset, SidxParseContext& sidxCtx,
        std::shared_ptr<Track> currentTrack, uint32_t index);
    Status FindNextMoof(int64_t& headerOffset, uint32_t& atomSize);
    Status UpdateFragmentEntryAndOffset(bool inEntry, uint32_t fragIndex, FragmentEntry& frag,
        int64_t moofOffset, int64_t& currentOffset);
    Status MPEG4ParseAtom(int32_t depth, ParseContext* ctx);
    Status ParseFtyp(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMoov(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseWide(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMdat(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    MvhdInfo ParseMvhdInfo(const uint8_t* headerInfo, const size_t len) const;
    void SetMvhdMediaInfo(const MvhdInfo& mvhdInfo);
    Status ParseMvhdDisplayMatrix(const MPEG4Atom& currentAtom, const uint8_t* headerInfo, ParseContext* ctx,
        const size_t len);
    Status ParseMvhd(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseTrak(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status InitCurrentTrackForTrak();
    void UpdateParsedTrackVideoInfo();
    Status FinalizeParsedTrackSampleTables();
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
    Status ParseWave(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseFrma(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseSchi(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseSchm(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseSinf(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParsePssh(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status BuildPsshInfo(int64_t boxOffset, const uint8_t uuidData[16], uint32_t boxSize);
    Status ParseTenc(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ReadTencDefaultConstantIV(ParseContext* ctx, uint32_t kidOffset, uint8_t defaultIsProtected,
        uint8_t defaultPerSampleIvSize, bool ivSizeInvalid);
    Status ParseSenc(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseSencSampleEntry(uint32_t& dataOffset, size_t entryIndex, bool useSubsamples,
        uint32_t atomEndOffset);
    Status ParseSaiz(const MPEG4Atom& atom);
    Status ParseSaizBody(int64_t dataOffset, const MPEG4Atom& atom);
    Status ParseSaio(const MPEG4Atom& atom);
    Status ReadSaioOffsetEntries(int64_t dataOffset, uint8_t version, const MPEG4Atom& atom);
    Status ParseAuxiliaryInfo(const std::shared_ptr<Track>& track);
    Status ParseAuxiliarySampleEntry(int64_t& auxiliaryOffset, size_t entryIndex, uint32_t sampleInfoSize,
        const std::shared_ptr<Track>& track);
    Status ParseChan(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseIdat(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseClli(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
    Status ParseMdcv(MPEG4Atom currentAtom, int32_t depth, ParseContext* ctx);
};
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_BOX_PARSER_H
