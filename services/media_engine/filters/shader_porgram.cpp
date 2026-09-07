/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "shader_program.h"
#include "common/log.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "WaterMarkFilter" };
}

namespace OHOS {
namespace Media {

ShaderProgram::ShaderProgram(const std::string &vertexShader, const std::string &fragShader)
{
    auto vShaderCode = vertexShader.c_str();
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");

    auto fShaderCode = fragShader.c_str();
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");
    
    id_ = glCreateProgram();
    glAttachShader(id_, vertex);
    glAttachShader(id_, fragment);
    glBindAttribLocation(id_, 0, "position");
    glBindAttribLocation(id_, 1, "texCoord");
    glLinkProgram(id_);
    CheckCompileErrors(id_, "PROGRAM");
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

ShaderProgram::~ShaderProgram() noexcept
{
    if (Valid()) {
        glDeleteProgram(id_);
    }
}

void ShaderProgram::CheckCompileErrors(GLuint shader, const std::string &shaderType)
{
    int success = 0;
    if (shaderType != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        FALSE_RETURN_MSG(success, "ERROR::SHADER_COMPILATION_ERROR of glGetShaderiv");
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        FALSE_RETURN_MSG(success, "ERROR::PROGRAM_LINKING_ERROR of glGetProgramiv");
    }
}

void ShaderProgram::SetInt(const std::string &name, int value)
{
    glUniform1i(glGetUniformLocation(id_, name.c_str()), static_cast<GLint>(value));
}

void ShaderProgram::SetVec2(const std::string &name, float v0, float v1)
{
    glUniform2f(glGetUniformLocation(id_, name.c_str()), v0, v1);
}

void ShaderProgram::SetMatrix4v(const std::string &name, const float *matrix, int cnt, bool transpose)
{
    FALSE_RETURN_MSG(matrix != nullptr, "matrix is null");
    GLboolean glTranspose = transpose ? GL_TRUE : GL_FALSE;
    glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, glTranspose, matrix);
}
}
}
