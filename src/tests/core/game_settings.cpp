// SPDX-FileCopyrightText: Copyright 2026 Orchid Project
// SPDX-License-Identifier: GPL-3.0-or-later

// Per-title overrides in Core::GameSettings::LoadOverrides. Only the renderer's vendor string
// is read there, so minimal fakes stand in for the window and renderer.

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "common/settings.h"
#include "core/frontend/emu_window.h"
#include "core/frontend/graphics_context.h"
#include "core/game_settings.h"
#include "video_core/renderer_base.h"

namespace {

class FakeWindow final : public Core::Frontend::EmuWindow {
public:
    std::unique_ptr<Core::Frontend::GraphicsContext> CreateSharedContext() const override {
        return nullptr;
    }
    bool IsShown() const override {
        return true;
    }
};

class FakeRenderer final : public VideoCore::RendererBase {
public:
    explicit FakeRenderer(Core::Frontend::EmuWindow& window) : RendererBase(window, nullptr) {}
    void Composite(std::span<const Tegra::FramebufferConfig>) override {}
    std::vector<u8> GetAppletCaptureBuffer() override {
        return {};
    }
    VideoCore::RasterizerInterface* ReadRasterizer() override {
        return nullptr;
    }
    std::string GetDeviceVendor() const override {
        return "AMD Radeon RX 7600 (RADV NAVI33)";
    }
};

bool PreemptiveReadsFor(u64 program_id) {
    FakeWindow window;
    FakeRenderer renderer{window};
    Core::GameSettings::LoadOverrides(program_id, renderer);
    return Settings::values.preemptive_cpu_reads;
}

} // namespace

TEST_CASE("GameSettings: Xenoblade DE Switch 2 Edition skips ordered CPU reads", "[game_settings]") {
    // Title ID as the game reports it (not the enum), so a wrong ID in the table fails here.
    // The game reads a GPU-written texture every frame; ordered reads drained the host GPU.
    REQUIRE(PreemptiveReadsFor(0x0400FF500E34A001ULL));
    // The Switch 1 release keeps ordered reads.
    REQUIRE_FALSE(PreemptiveReadsFor(0x0100FF500E34A000ULL));
}
