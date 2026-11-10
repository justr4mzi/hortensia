// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "core/hle/service/nx2_present.h"
#include "video_core/engines/native_upscale.h"

TEST_CASE("Nx2 presentation crops preserve the native allocation extent", "[nx2_video]") {
    const auto crop = Service::Nx2PresentCrop(0, 1279 | (719 << 16), 2, 3840, 2160);
    CHECK(crop.left == 0);
    CHECK(crop.top == 0);
    CHECK(crop.right == 1280);
    CHECK(crop.bottom == 720);
    const auto disabled = Service::Nx2PresentCrop(0, 1279 | (719 << 16), 1, 3840, 2160);
    CHECK(disabled.right == 3840);
    CHECK(disabled.bottom == 2160);
}

TEST_CASE("Nx2 presentation crops decode inclusive coordinates and clip to the image", "[nx2_video]") {
    const auto crop = Service::Nx2PresentCrop(16 | (32 << 16), 4095 | (4095 << 16), 2, 3840, 2160);
    CHECK(crop.left == 16);
    CHECK(crop.top == 32);
    CHECK(crop.right == 3840);
    CHECK(crop.bottom == 2160);
    const auto pixel = Service::Nx2PresentCrop(16 | (32 << 16), 16 | (32 << 16), 2, 3840, 2160);
    CHECK(pixel.GetWidth() == 1);
    CHECK(pixel.GetHeight() == 1);
    const auto maximum = Service::Nx2PresentCrop(0, 0xffffffff, 2, 65536, 65536);
    CHECK(maximum.right == 65536);
    CHECK(maximum.bottom == 65536);
}

TEST_CASE("Nx2 presentation ignores empty or reversed crop metadata", "[nx2_video]") {
    for (const u32 origin : {3840U, 2160U << 16, 32U | (32U << 16)}) {
        const auto crop = Service::Nx2PresentCrop(origin, 15 | (15 << 16), 2, 3840, 2160);
        CHECK(crop.left == 0);
        CHECK(crop.top == 0);
        CHECK(crop.right == 3840);
        CHECK(crop.bottom == 2160);
    }
}

TEST_CASE("Nx2 presentation honors vertical inversion independently of crop flags", "[nx2_video]") {
    using Service::android::BufferTransformFlags;
    CHECK(Service::Nx2PresentTransform(0x00040003) == BufferTransformFlags::FlipV);
    CHECK(Service::Nx2PresentTransform(0x00040000) == BufferTransformFlags::FlipV);
    CHECK(Service::Nx2PresentTransform(0x00000003) == BufferTransformFlags::Unset);
    CHECK(Service::Nx2PresentTransform(0) == BufferTransformFlags::Unset);
}

TEST_CASE("Native upscale uses active regions inside larger texture allocations", "[nx2_video]") {
    const std::array<u32, 12> parameters{128, 40, 23, 1, 0, 0, 959, 539, 0, 0, 1919, 1079};
    const auto regions = Tegra::Engines::DecodeNativeUpscaleRegions(
        parameters, {128, 1, 1}, {40, 23, 1}, {960, 540}, {3840, 2160});
    REQUIRE(regions);
    CHECK(regions->source == std::array<u32, 4>{0, 0, 960, 540});
    CHECK(regions->destination == std::array<u32, 4>{0, 0, 1920, 1080});

    auto offset = parameters;
    offset[4] = 16;
    offset[5] = 8;
    offset[8] = 32;
    offset[9] = 16;
    const auto subregion = Tegra::Engines::DecodeNativeUpscaleRegions(
        offset, {128, 1, 1}, {40, 23, 1}, {960, 540}, {3840, 2160});
    REQUIRE(subregion);
    CHECK(subregion->source[0] == 16);
    CHECK(subregion->destination[1] == 16);

    // Handheld Xenoblade Chronicles 2: word 0 is 160 although its blocks are 128 wide.
    const std::array<u32, 12> xenoblade{160, 40, 15, 1, 0, 0, 1279, 719, 0, 0, 1919, 1079};
    const auto handheld = Tegra::Engines::DecodeNativeUpscaleRegions(
        xenoblade, {128, 1, 1}, {40, 15, 1}, {1280, 720}, {3840, 2160});
    REQUIRE(handheld);
    CHECK(handheld->source == std::array<u32, 4>{0, 0, 1280, 720});
    CHECK(handheld->destination == std::array<u32, 4>{0, 0, 1920, 1080});
}

TEST_CASE("Native upscale rejects incompatible or invalid region parameters", "[nx2_video]") {
    const std::array<u32, 12> parameters{128, 40, 23, 1, 0, 0, 959, 539, 0, 0, 1919, 1079};
    CHECK_FALSE(Tegra::Engines::DecodeNativeUpscaleRegions(
        parameters, {128, 1, 1}, {41, 23, 1}, {960, 540}, {3840, 2160}));
    CHECK_FALSE(Tegra::Engines::DecodeNativeUpscaleRegions(
        parameters, {64, 2, 1}, {40, 23, 1}, {960, 540}, {3840, 2160}));
    CHECK_FALSE(Tegra::Engines::DecodeNativeUpscaleRegions(
        parameters, {64, 1, 2}, {40, 23, 1}, {960, 540}, {3840, 2160}));
    CHECK_FALSE(Tegra::Engines::DecodeNativeUpscaleRegions(
        std::span{parameters}.first(11), {128, 1, 1}, {40, 23, 1}, {960, 540}, {3840, 2160}));
    for (const std::pair<std::size_t, u32> invalid : {
             std::pair<std::size_t, u32>{6, 960}, {7, 540}, {10, 3840}, {11, 2160},
             {10, 0xffffffff}, {4, 960}, {8, 1920}, {10, 100}}) {
        auto malformed = parameters;
        malformed[invalid.first] = invalid.second;
        CHECK_FALSE(Tegra::Engines::DecodeNativeUpscaleRegions(
            malformed, {128, 1, 1}, {40, 23, 1}, {960, 540}, {3840, 2160}));
    }
}
