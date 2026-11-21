#include "marker_plate.h"
#include "web_dictionaries.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

TEST(MarkerPlateTest, WebDictionariesMatchNativeForEveryId)
{
    std::size_t offset = 0;
    for (const auto& dictionary : markerplate::supportedDictionaries())
    {
        for (int id = 0; id < dictionary.markerCount; ++id)
        {
            ASSERT_EQ(markerplate::blackCellsForId(dictionary.type, id),
                      markerplate::detail::webDictionaryCells(offset, id, dictionary.payloadSize))
                << dictionary.name << " ID " << id;
        }
        offset += static_cast<std::size_t>(dictionary.markerCount);
    }
    EXPECT_EQ(offset, markerplate::detail::WebDictionaryCodes.size());
}

TEST(MarkerPlateTest, MarkerZeroMatchesTheChevArucogenPattern)
{
    const auto blackCells = markerplate::blackCellsForId(markerplate::DictionaryType::Aruco4x4_1000, 0);
    const std::vector<std::uint8_t> expectedBlackCells{
        0, 1, 0, 0,
        1, 0, 1, 0,
        1, 1, 0, 0,
        1, 1, 0, 1
    };
    EXPECT_EQ(blackCells, expectedBlackCells);
}

TEST(MarkerPlateTest, SupportsId999AndRejectsIdsOutsideTheDictionary)
{
    EXPECT_NO_THROW((void)markerplate::blackCellsForId(markerplate::DictionaryType::Aruco4x4_1000, 999));
    EXPECT_THROW((void)markerplate::blackCellsForId(markerplate::DictionaryType::Aruco4x4_1000, -1), std::invalid_argument);
    EXPECT_THROW((void)markerplate::blackCellsForId(markerplate::DictionaryType::Aruco4x4_1000, 1000), std::invalid_argument);
}

TEST(MarkerPlateTest, CreatesTwoMaterialThreeMfPackage)
{
    const auto bytes = markerplate::make3mf({ 0, 50.0, 2.0, 0.4, markerplate::DictionaryType::Aruco4x4_1000 });
    const std::string package(bytes.begin(), bytes.end());

    ASSERT_GE(bytes.size(), 22u);
    EXPECT_EQ(bytes[0], 0x50);
    EXPECT_EQ(bytes[1], 0x4B);
    EXPECT_NE(package.find("[Content_Types].xml"), std::string::npos);
    EXPECT_NE(package.find("_rels/.rels"), std::string::npos);
    EXPECT_NE(package.find("3D/3dmodel.model"), std::string::npos);
    EXPECT_NE(package.find("displaycolor=\"#FFFFFF\""), std::string::npos);
    EXPECT_NE(package.find("displaycolor=\"#000000\""), std::string::npos);
    EXPECT_NE(package.find("4x4 (50, 100, 250, 1000) marker 0"), std::string::npos);
    EXPECT_NE(package.find("x=\"66.6666667\""), std::string::npos);
    EXPECT_NE(package.find("<components>"), std::string::npos);
}

TEST(MarkerPlateTest, CreatesPackageForId999)
{
    const auto bytes = markerplate::make3mf({ 999, 50.0, 2.0, 0.4, markerplate::DictionaryType::Aruco4x4_1000 });
    const std::string package(bytes.begin(), bytes.end());
    EXPECT_NE(package.find("4x4 (50, 100, 250, 1000) marker 999"), std::string::npos);
}

TEST(MarkerPlateTest, SizesPlateIncludingMarginForDifferentDictionaries)
{
    for (const auto& [dictionary, expectedMarkerSize] :
        std::vector<std::pair<markerplate::DictionaryType, double>>{
            { markerplate::DictionaryType::Aruco4x4_1000, 37.5 },
            { markerplate::DictionaryType::Aruco7x7_1000, 50.0 * 9.0 / 11.0 }
        })
    {
        const double markerSize = markerplate::markerSizeForPlateSize(dictionary, 50.0);
        EXPECT_NEAR(markerSize, expectedMarkerSize, 1e-10);
        EXPECT_NEAR(markerplate::plateSizeForMarkerSize(dictionary, markerSize), 50.0, 1e-10);
        const auto bytes = markerplate::make3mf({ 0, markerSize, 2.0, 0.4, dictionary });
        const std::string package(bytes.begin(), bytes.end());
        EXPECT_NE(package.find("<vertex x=\"50\""), std::string::npos);
    }

    const auto dictionary = markerplate::DictionaryType::Aruco4x4_1000;
    EXPECT_NO_THROW((void)markerplate::make3mf({
        0, markerplate::markerSizeForPlateSize(dictionary, 8.0), 2.0, 0.4, dictionary }));
    EXPECT_THROW((void)markerplate::make3mf({
        0, markerplate::markerSizeForPlateSize(dictionary, 7.9), 2.0, 0.4, dictionary }), std::invalid_argument);
}

TEST(MarkerPlateTest, AddsExtraMarginToEverySideAndConvertsPlateSizeBackToMarkerSize)
{
    const auto dictionary = markerplate::DictionaryType::Aruco4x4_1000;
    constexpr double markerSize = 50.0;
    constexpr double additionalMargin = 2.0;
    const double expectedPlateSize = markerSize * 8.0 / 6.0 + additionalMargin * 2.0;

    EXPECT_NEAR(
        markerplate::plateSizeForMarkerSize(dictionary, markerSize, additionalMargin), expectedPlateSize, 1e-10);
    EXPECT_NEAR(
        markerplate::markerSizeForPlateSize(dictionary, expectedPlateSize, additionalMargin), markerSize, 1e-10);

    const auto bytes = markerplate::make3mf({ 0, markerSize, 2.0, 0.4, dictionary, additionalMargin });
    const std::string package(bytes.begin(), bytes.end());
    EXPECT_NE(package.find("<vertex x=\"70.6666667\""), std::string::npos);
}

TEST(MarkerPlateTest, SupportsEveryListedDictionary)
{
    for (const auto& dictionary : markerplate::supportedDictionaries())
    {
        const auto blackCells = markerplate::blackCellsForId(dictionary.type, 0);
        EXPECT_EQ(blackCells.size(), static_cast<std::size_t>(dictionary.payloadSize * dictionary.payloadSize));
        EXPECT_EQ(
            markerplate::blackCellsForId(dictionary.type, dictionary.markerCount - 1).size(),
            static_cast<std::size_t>(dictionary.payloadSize * dictionary.payloadSize));
        EXPECT_NO_THROW((void)markerplate::make3mf({ 0, 50.0, 2.0, 0.4, dictionary.type }));
        EXPECT_THROW(
            (void)markerplate::blackCellsForId(dictionary.type, dictionary.markerCount),
            std::invalid_argument);
    }
}

TEST(MarkerPlateTest, Mip36h12IdZeroMatchesThePublishedCode)
{
    const auto blackCells = markerplate::blackCellsForId(markerplate::DictionaryType::Mip36h12, 0);
    const std::vector<std::uint8_t> expectedBlackCells{
        0, 0, 1, 0, 1, 1,
        0, 1, 0, 1, 0, 0,
        1, 0, 0, 1, 1, 1,
        0, 0, 0, 1, 0, 1,
        1, 1, 1, 1, 0, 1,
        1, 0, 0, 0, 1, 0
    };
    EXPECT_EQ(blackCells, expectedBlackCells);
}

TEST(MarkerPlateTest, AprilTagIdZeroMatchesTheChevArucoGenOrientation)
{
    const std::vector<std::pair<markerplate::DictionaryType, std::vector<std::string>>> patterns{
        { markerplate::DictionaryType::AprilTag16h5, { "1101", "1100", "1110", "0100" } },
        { markerplate::DictionaryType::AprilTag25h9, { "01010", "10100", "01101", "00000", "01110" } },
        { markerplate::DictionaryType::AprilTag36h10, { "111000", "110101", "011011", "010101", "100101", "111000" } },
        { markerplate::DictionaryType::AprilTag36h11, { "001010", "100010", "100111", "010111", "101001", "111011" } }
    };

    for (const auto& [dictionary, rows] : patterns)
    {
        std::vector<std::uint8_t> expectedBlackCells;
        for (const auto& row : rows)
        {
            for (const char cell : row)
            {
                expectedBlackCells.push_back(static_cast<std::uint8_t>(cell == '1'));
            }
        }
        EXPECT_EQ(markerplate::blackCellsForId(dictionary, 0), expectedBlackCells);
    }
}

TEST(MarkerPlateTest, RejectsInvalidDimensions)
{
    EXPECT_THROW(markerplate::make3mf({ 0, 5.0, 2.0, 0.4 }), std::invalid_argument);
    EXPECT_THROW(markerplate::make3mf({ 0, 50.0, 0.0, 0.4 }), std::invalid_argument);
    EXPECT_THROW(markerplate::make3mf({ 0, 50.0, 2.0, 0.0 }), std::invalid_argument);
    EXPECT_THROW(markerplate::make3mf({ 0, 50.0, 2.0, 0.4, markerplate::DictionaryType::Aruco4x4_1000, -1.0 }),
                 std::invalid_argument);
}
