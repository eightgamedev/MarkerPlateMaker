#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

namespace markerplate
{
    enum class DictionaryType
    {
        Aruco4x4_1000,
        Aruco5x5_1000,
        Aruco6x6_1000,
        Aruco7x7_1000,
        Mip36h12,
        AprilTag16h5,
        AprilTag25h9,
        AprilTag36h10,
        AprilTag36h11
    };

    struct DictionaryInfo
    {
        DictionaryType type;
        std::string_view name;
        int payloadSize;
        int markerCount;
    };

    struct Parameters
    {
        int id = 0;
        double markerSizeMm = 50.0;
        double baseThicknessMm = 2.0;
        double reliefHeightMm = 0.4;
        DictionaryType dictionary = DictionaryType::Aruco4x4_1000;
        double additionalMarginMm = 0.0;
    };

    [[nodiscard]] std::span<const DictionaryInfo> supportedDictionaries() noexcept;
    [[nodiscard]] double markerSizeForPlateSize(
            DictionaryType dictionary, double plateSizeMm, double additionalMarginMm = 0.0);
    [[nodiscard]] double plateSizeForMarkerSize(
            DictionaryType dictionary, double markerSizeMm, double additionalMarginMm = 0.0);
    [[nodiscard]] std::vector<std::uint8_t> blackCellsForId(DictionaryType dictionary, int id);
    [[nodiscard]] std::vector<std::uint8_t> make3mf(const Parameters& parameters);
    void save3mf(const std::filesystem::path& path, const Parameters& parameters);
}
