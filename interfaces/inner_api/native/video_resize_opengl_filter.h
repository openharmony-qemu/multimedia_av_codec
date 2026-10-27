/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef FILTERS_VIDEO_RESIZE_OPENGL_FILTER_H
#define FILTERS_VIDEO_RESIZE_OPENGL_FILTER_H

#include "opengl_video_filter_base.h"
#include "shader_program.h"
#include "surface/native_buffer.h"

namespace OHOS {
namespace Media {
namespace Pipeline {

/**
 * @brief 基于 OpenGL 的视频降分辨率 filter，用于转码管线。
 *
 * 继承 OpenGLVideoFilterBase 复用通用 EGL/GL/渲染线程/视频直通等基础设施，
 * 通过 virtual hook 注入 HDR / 10bit 转码管线（零拷贝 YUV shader、P010 OES FBO、
 * 帧级 HDR 元数据透传），与水印文件完全隔离。
 *
 * HDR 路径仅支持零拷贝（GPU 直写编码器 DMA buffer），要求 GPU 支持
 * GL_EXT_YUV_target 扩展且 P010 OES 纹理可作为 FBO 颜色附件。能力检测在
 * OnGLResourcesCreated 阶段完成，失败则拒绝进入 HDR 模式。
 *
 * SDR 降分辨率路径：基类 RenderFrame 默认仅渲染视频直通，不执行任何水印叠加。
 * 转码引擎通过 AddVideoResize 创建此 filter，与 WaterMarkFilter 通路解耦。
 */
class VideoResizeOpenGLFilter : public OpenGLVideoFilterBase {
public:
    explicit VideoResizeOpenGLFilter(std::string name, FilterType type);
    ~VideoResizeOpenGLFilter() override;

    Status DoPrepare() override;

protected:
    // ---- OpenGLVideoFilterBase virtual hook overrides ----
    bool SetupEglContextForMode() override;
    bool OnGLResourcesCreated() override;
    void ProcessFrame() override;
    void ConfigureInputSurface() override;
    void ConfigureOutputSurface() override;
    void OnSetCodecFormat(const std::shared_ptr<Meta> &format) override;
    void OnReleaseGpuResources() override;
    // ------------------------------------------------

private:
    bool isHdrMode_ = false;
    int32_t dstColorSpace_ = 0;
    GLuint outputFBO_ = 0;
    GLuint outputTexId_ = DEFINED_VALUE;
    std::unique_ptr<ShaderProgram> yuvConvertProgram_;  // RGB10_A2 → YUV 零拷贝转换shader
    bool useDirectBt2020Shader_ = false;                // true=直接BT.2020 shader, false=itu_709+预补偿回退

    // HDR2HDR 帧级元数据透传（仅 isHdrMode_ == true 时使用）
    struct HdrMetaData {
        std::vector<uint8_t> dynamicMetadata;   // OH_HDR_DYNAMIC_METADATA / ATTRKEY_HDR_DYNAMIC_METADATA
        std::vector<uint8_t> staticMetadata;    // OH_HDR_STATIC_METADATA / ATTRKEY_HDR_STATIC_METADATA
        std::vector<uint8_t> metadataType;      // OH_HDR_METADATA_TYPE / ATTRKEY_HDR_METADATA_TYPE
        bool valid = false;
    };
    HdrMetaData cachedHdrMetaData_;  // 当前帧的 HDR 元数据缓存

    bool InitializeEGLContextHdr();
    bool CreateOutputTexturesAndFBO();
    bool DetectZeroCopyCapability();
    bool IsHdrColorSpace(int32_t colorSpace) const;
    void ConfigureHdrSurface();
    void RenderHdrFrame();
    void RenderHdrFrameZeroCopy(int32_t viewWidth, int32_t viewHeight, int64_t pts);
    bool RequestEncoderBuffer(OHNativeWindowBuffer *&outBuffer);
    void FlushEncoderBuffer(OHNativeWindowBuffer *outBuffer,
        int32_t viewWidth, int32_t viewHeight);
    bool AcquireInputBufferWithMetadata(OHNativeWindowBuffer *&inBuffer, int64_t &pts);
    void ReadHdrMetadataFromBuffer(OHNativeWindowBuffer *inBuffer);
    void WriteHdrMetadataToBuffer(OHNativeWindowBuffer *outBuffer);
    void ReleaseInputBuffer(OHNativeWindowBuffer *inBuffer);

    // ---- HDR metadata helper methods ----
    void ReadSingleMetadata(OH_NativeBuffer *nativeBuffer, OH_NativeBuffer_MetadataKey key,
                            std::vector<uint8_t> &dest);
    void WriteSingleMetadata(OH_NativeBuffer *nativeBuffer, OH_NativeBuffer_MetadataKey key,
                             std::vector<uint8_t> &src, const char *metadataName);
    // ---- Zero-copy YUV FBO helper methods ----
    bool SetupYuvFbo(OHNativeWindowBuffer *outBuffer, GLuint &outTex, GLuint &yuvFbo,
                     EGLImageKHR &outImg);
    void ReleaseYuvFboResources(GLuint outTex, GLuint yuvFbo, EGLImageKHR outImg);
};

} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // FILTERS_VIDEO_RESIZE_OPENGL_FILTER_H
