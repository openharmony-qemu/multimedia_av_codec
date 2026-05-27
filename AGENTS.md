# AGENTS.md - multimedia_av_codec_b2

## Project Overview
- **OHOS multimedia av_codec component** - provides audio/video codec, demuxer, muxer capabilities
- **Build system**: GN (OhOS build), not CMake/npm/Make
- **Language**: C++17, no RTTI, no exceptions
- **License**: Apache 2.0

## Key Directories
- `frameworks/` - client-side framework code
- `services/` - service-side codec implementation
- `interfaces/` - public APIs (inner_api/native, kits/c)
- `test/` - unittest, moduletest, fuzztest, nativedemo

## Build & Test Commands
- Tests are built via GN targets, run on OHOS device/emulator
- Main test groups: `av_codec_unit_test`, `av_codec_module_test`, `av_codec_fuzz_test`, `video_codec_unit_test`
- Video encoder tests: `test/unittest/video_test/vcodec_framework_test/`
- Test binary: `vcodec_framework_test`

## Code Conventions
- **Line width**: 120 columns max (codecheck enforced)
- **Comments**: Use `//` not `/* */` for single-line; numeric annotations like `// 3: offset` explain magic numbers
- **Logging macros**: `UNITTEST_INFO_LOG`, `UNITTEST_CHECK_AND_RETURN_RET_LOG`, `UNITTEST_CHECK_AND_BREAK_LOG`
- **Error codes**: `AV_ERR_OK`, `AV_ERR_UNKNOWN`, `AV_ERR_INVALID_VAL`, `AV_ERR_TRY_AGAIN_LATER`
- **Null checks**: Use `== nullptr` / `!= nullptr` explicitly

## Test Patterns
- **Sample classes**: `VideoEncAsyncSample`, `VideoEncSyncSample` - encapsulate encoder test logic
- **Callback classes**: `VEncCallbackTest`, `VEncCallbackTestExt` - mock codec callbacks
- **Signal struct**: `VEncSignal` in `func_sample_encoder_base.h` - shared state for async coordination
- **Mock factories**: `VCodecMockFactory::CreateVideoEncMockByMime/ByName`
- **SHA verification**: Tests can verify output integrity via SHA512 (`needCheckSHA_`, `CheckSHA()`)

## Feature Flags
- Defined in `config.gni` via `declare_args()`
- Key flags: `av_codec_support_test`, `av_codec_support_hcodec`, `av_codec_support_software_codec`
- Codec-specific: `av_codec_enable_codec_eac3/dts/truehd/rv`, `av_codec_support_vp8/vp9/av1_decoder`
- Tests guarded by `if (av_codec_support_test)` in BUILD.gn

## Compiler Flags (from BUILD.gn)
- `-std=c++17 -fno-rtti -fno-exceptions -Wall -Werror -Wextra`
- `-Dprivate=public -Dprotected=public` (test access to private members)
- Sanitizers available: `av_codec_test_sanitize`

## Common Gotchas
- `VEncSignal` uses `std::atomic` for thread-safe counters (`pendingInputCount_`, `maxPendingInputCount_`)
- When updating atomic max values, use `compare_exchange_weak` loop pattern - extract to helper function to avoid bloating callbacks
- `OnNeedInputData` in async mode tracks pending input count for MAX_FRAME_DELAY_COUNT verification
- Surface mode vs buffer mode: `isAVBufferMode_` determines which API path is used
- `VIDEOENC_CAPI_UNIT_TEST` define switches between CAPI mock and inner API mock
