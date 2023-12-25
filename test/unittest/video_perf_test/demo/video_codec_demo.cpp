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

#include <iostream>
#include "arg_parser.h"
#include "arg_checker.h"
#include "sample_helper.h"
#include "av_codec_sample_error.h"

using namespace OHOS::MediaAVCodec::Sample;

int main(int argc, char *argv[])
{
    auto info = ParseDemoArg(argc, argv);
    if (!SampleInfoChecker(info)) {
        std::cout << "Demo arg check failed, exit" << std::endl;
        return 1;
    }

    for (uint32_t times = 1; times <= info.repeatTimes; times++) {
        std::cout << "\rRepeat times: (" << times << "/" << info.repeatTimes << ")" << std::flush;

        int ret = RunSample(info);
        if (ret != AVCODEC_SAMPLE_ERR_OK) {
            std::cout << "Demo run failed!" << std::endl;
            return 1;
        }
    }
    return 0;
}