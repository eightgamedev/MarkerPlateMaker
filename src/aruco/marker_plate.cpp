#include "marker_plate.h"

#ifdef __EMSCRIPTEN__
#include "web_dictionaries.h"
#else
#include <opencv2/aruco.hpp>
#endif

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
constexpr std::array<std::uint64_t, 250> Mip36h12Codes{
#include "mip_36h12_codes.inc"
};

constexpr std::array<markerplate::DictionaryInfo, 9> DictionaryList{
        {{markerplate::DictionaryType::Aruco4x4_1000, "ArUco 4x4（1000）", 4, 1000},
         {markerplate::DictionaryType::Aruco5x5_1000, "ArUco 5x5（1000）", 5, 1000},
         {markerplate::DictionaryType::Aruco6x6_1000, "ArUco 6x6（1000）", 6, 1000},
         {markerplate::DictionaryType::Aruco7x7_1000, "ArUco 7x7（1000）", 7, 1000},
         {markerplate::DictionaryType::Mip36h12, "MIP_36h12 (250)", 6, 250},
         {markerplate::DictionaryType::AprilTag16h5, "AprilTag 16h5 (30)", 4, 30},
         {markerplate::DictionaryType::AprilTag25h9, "AprilTag 25h9 (35)", 5, 35},
         {markerplate::DictionaryType::AprilTag36h10, "AprilTag 36h10 (2320)", 6, 2320},
         {markerplate::DictionaryType::AprilTag36h11, "AprilTag 36h11 (587)", 6, 587}}};

const markerplate::DictionaryInfo& dictionaryInfo(markerplate::DictionaryType type) {
    for (const auto& info : DictionaryList) {
        if (info.type == type) {
            return info;
        }
    }
    throw std::invalid_argument("Unknown marker dictionary.");
}

#ifndef __EMSCRIPTEN__
cv::aruco::PREDEFINED_DICTIONARY_NAME openCvDictionary(markerplate::DictionaryType type) {
    switch (type) {
        case markerplate::DictionaryType::Aruco4x4_1000:
            return cv::aruco::DICT_4X4_1000;
        case markerplate::DictionaryType::Aruco5x5_1000:
            return cv::aruco::DICT_5X5_1000;
        case markerplate::DictionaryType::Aruco6x6_1000:
            return cv::aruco::DICT_6X6_1000;
        case markerplate::DictionaryType::Aruco7x7_1000:
            return cv::aruco::DICT_7X7_1000;
        case markerplate::DictionaryType::AprilTag16h5:
            return cv::aruco::DICT_APRILTAG_16h5;
        case markerplate::DictionaryType::AprilTag25h9:
            return cv::aruco::DICT_APRILTAG_25h9;
        case markerplate::DictionaryType::AprilTag36h10:
            return cv::aruco::DICT_APRILTAG_36h10;
        case markerplate::DictionaryType::AprilTag36h11:
            return cv::aruco::DICT_APRILTAG_36h11;
        case markerplate::DictionaryType::Mip36h12:
            break;
    }
    throw std::invalid_argument("MIP_36h12 does not use an OpenCV predefined dictionary enum.");
};

bool isAprilTag(markerplate::DictionaryType type) {
    return type == markerplate::DictionaryType::AprilTag16h5 || type == markerplate::DictionaryType::AprilTag25h9 ||
           type == markerplate::DictionaryType::AprilTag36h10 || type == markerplate::DictionaryType::AprilTag36h11;
}
#endif

struct Vertex {
    double x;
    double y;
    double z;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<std::array<std::uint32_t, 3>> triangles;
};

void addBox(Mesh& mesh, double x0, double y0, double x1, double y1, double z0, double z1) {
    const auto first = static_cast<std::uint32_t>(mesh.vertices.size());
    mesh.vertices.insert(
            mesh.vertices.end(),
            {{x0, y0, z0},
             {x1, y0, z0},
             {x1, y1, z0},
             {x0, y1, z0},
             {x0, y0, z1},
             {x1, y0, z1},
             {x1, y1, z1},
             {x0, y1, z1}});

    constexpr std::array<std::array<std::uint32_t, 3>, 12> faces{
            {{0, 2, 1},
             {0, 3, 2},
             {4, 5, 6},
             {4, 6, 7},
             {0, 4, 7},
             {0, 7, 3},
             {1, 2, 6},
             {1, 6, 5},
             {0, 1, 5},
             {0, 5, 4},
             {3, 7, 6},
             {3, 6, 2}}};

    for (const auto& face : faces) {
        mesh.triangles.push_back({first + face[0], first + face[1], first + face[2]});
    }
}

std::string meshXml(const Mesh& mesh) {
    std::ostringstream xml;
    xml.imbue(std::locale::classic());
    xml << std::setprecision(9);
    xml << "<mesh><vertices>";
    for (const auto& vertex : mesh.vertices) {
        xml << "<vertex x=\"" << vertex.x << "\" y=\"" << vertex.y << "\" z=\"" << vertex.z << "\"/>";
    }
    xml << "</vertices><triangles>";
    for (const auto& triangle : mesh.triangles) {
        xml << "<triangle v1=\"" << triangle[0] << "\" v2=\"" << triangle[1] << "\" v3=\"" << triangle[2] << "\"/>";
    }
    xml << "</triangles></mesh>";
    return xml.str();
}

std::string modelXml(const markerplate::Parameters& parameters) {
    const auto& info = dictionaryInfo(parameters.dictionary);
    const auto blackCells = markerplate::blackCellsForId(parameters.dictionary, parameters.id);
    const int markerGridSize = (info.payloadSize + 2);
    const double module = parameters.markerSizeMm / markerGridSize;
    const double boardSize = markerplate::plateSizeForMarkerSize(parameters.dictionary, parameters.markerSizeMm);
    Mesh base;
    Mesh blackCell;
    addBox(base, 0.0, 0.0, boardSize, boardSize, 0.0, parameters.baseThicknessMm);
    addBox(blackCell, 0.0, 0.0, module, module, 0.0, parameters.reliefHeightMm);

    std::ostringstream xml;
    xml.imbue(std::locale::classic());
    xml << std::setprecision(9);
    std::ostringstream components;
    components.imbue(std::locale::classic());
    components << std::setprecision(9);
    for (int row = 0; row < markerGridSize; ++row) {
        for (int column = 0; column < markerGridSize; ++column) {
            bool isBlack = (row == 0) || (row == markerGridSize - 1) || (column == 0) || (column == markerGridSize - 1);
            if (!isBlack) {
                const auto bitIndex = static_cast<std::size_t>((row - 1) * info.payloadSize + (column - 1));
                isBlack = (blackCells[bitIndex] != 0);
            }
            if (isBlack) {
                const double x = module * (column + 1);
                const double y = module * (markerGridSize - row);
                components << "<component objectid=\"3\" transform=\"1 0 0 0 1 0 0 0 1 " << x << " " << y << " "
                           << parameters.baseThicknessMm << "\"/>";
            }
        }
    }

    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        << "<model unit=\"millimeter\" xml:lang=\"en-US\" "
           "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">"
        << "<metadata name=\"Title\">" << info.name << " marker " << parameters.id << "</metadata>"
        << "<resources>"
        << "<basematerials id=\"1\"><base name=\"White\" displaycolor=\"#FFFFFF\"/><base name=\"Black\" "
           "displaycolor=\"#000000\"/></basematerials>"
        << "<object id=\"2\" type=\"model\" name=\"White base\" pid=\"1\" pindex=\"0\">" << meshXml(base) << "</object>"
        << "<object id=\"3\" type=\"model\" name=\"Black cell\" pid=\"1\" pindex=\"1\">" << meshXml(blackCell)
        << "</object>"
        << "<object id=\"4\" type=\"model\" name=\"Black marker\"><components>" << components.str()
        << "</components></object>"
        << "</resources><build><item objectid=\"2\"/><item objectid=\"4\"/></build></model>";
    return xml.str();
}

std::uint32_t crc32(std::string_view data) {
    std::uint32_t crc = 0xFFFFFFFFu;
    for (const unsigned char byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

void appendU16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
}

void appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    appendU16(bytes, static_cast<std::uint16_t>(value & 0xFFFFu));
    appendU16(bytes, static_cast<std::uint16_t>(value >> 16));
}

void appendText(std::vector<std::uint8_t>& bytes, std::string_view text) {
    bytes.insert(bytes.end(), text.begin(), text.end());
}

struct ZipEntry {
    std::string_view name;
    std::string data;
    std::uint32_t checksum;
    std::uint32_t localOffset = 0;
};

std::vector<std::uint8_t> makeZip(std::vector<ZipEntry> entries) {
    std::vector<std::uint8_t> bytes;
    for (auto& entry : entries) {
        entry.checksum = crc32(entry.data);
        entry.localOffset = static_cast<std::uint32_t>(bytes.size());
        appendU32(bytes, 0x04034B50u);
        appendU16(bytes, 20);
        appendU16(bytes, 0);
        appendU16(bytes, 0);
        appendU16(bytes, 0);
        appendU16(bytes, 0x0021);
        appendU32(bytes, entry.checksum);
        appendU32(bytes, static_cast<std::uint32_t>(entry.data.size()));
        appendU32(bytes, static_cast<std::uint32_t>(entry.data.size()));
        appendU16(bytes, static_cast<std::uint16_t>(entry.name.size()));
        appendU16(bytes, 0);
        appendText(bytes, entry.name);
        appendText(bytes, entry.data);
    }

    const auto centralOffset = static_cast<std::uint32_t>(bytes.size());
    for (const auto& entry : entries) {
        appendU32(bytes, 0x02014B50u);
        appendU16(bytes, 20);
        appendU16(bytes, 20);
        appendU16(bytes, 0);
        appendU16(bytes, 0);
        appendU16(bytes, 0);
        appendU16(bytes, 0x0021);
        appendU32(bytes, entry.checksum);
        appendU32(bytes, static_cast<std::uint32_t>(entry.data.size()));
        appendU32(bytes, static_cast<std::uint32_t>(entry.data.size()));
        appendU16(bytes, static_cast<std::uint16_t>(entry.name.size()));
        appendU16(bytes, 0);
        appendU16(bytes, 0);
        appendU16(bytes, 0);
        appendU16(bytes, 0);
        appendU32(bytes, 0);
        appendU32(bytes, entry.localOffset);
        appendText(bytes, entry.name);
    }
    const auto centralSize = static_cast<std::uint32_t>(bytes.size()) - centralOffset;
    appendU32(bytes, 0x06054B50u);
    appendU16(bytes, 0);
    appendU16(bytes, 0);
    appendU16(bytes, static_cast<std::uint16_t>(entries.size()));
    appendU16(bytes, static_cast<std::uint16_t>(entries.size()));
    appendU32(bytes, centralSize);
    appendU32(bytes, centralOffset);
    appendU16(bytes, 0);
    return bytes;
}

void validate(const markerplate::Parameters& parameters) {
    const auto& info = dictionaryInfo(parameters.dictionary);
    if (parameters.id < 0 || parameters.id >= info.markerCount) {
        throw std::invalid_argument("ID is outside the selected dictionary.");
    }
    if (!std::isfinite(parameters.markerSizeMm) || parameters.markerSizeMm < (info.payloadSize + 2.0) ||
        parameters.markerSizeMm > 1000.0) {
        throw std::invalid_argument("Marker size must be at least one millimeter per marker cell and at most 1000 mm.");
    }
    if (!std::isfinite(parameters.baseThicknessMm) || parameters.baseThicknessMm <= 0.0 ||
        parameters.baseThicknessMm > 100.0) {
        throw std::invalid_argument("Base thickness must be greater than 0 and at most 100 mm.");
    }
    if (!std::isfinite(parameters.reliefHeightMm) || parameters.reliefHeightMm <= 0.0 ||
        parameters.reliefHeightMm > 10.0) {
        throw std::invalid_argument("Relief height must be greater than 0 and at most 10 mm.");
    }
}
}  // namespace

namespace markerplate {
std::span<const DictionaryInfo> supportedDictionaries() noexcept { return DictionaryList; }

double markerSizeForPlateSize(DictionaryType type, double plateSizeMm) {
    const auto& info = dictionaryInfo(type);
    return plateSizeMm * (info.payloadSize + 2.0) / (info.payloadSize + 4.0);
}

double plateSizeForMarkerSize(DictionaryType type, double markerSizeMm) {
    const auto& info = dictionaryInfo(type);
    return markerSizeMm * (info.payloadSize + 4.0) / (info.payloadSize + 2.0);
}

std::vector<std::uint8_t> blackCellsForId(DictionaryType type, int id) {
    const auto& info = dictionaryInfo(type);
    if (id < 0 || id >= info.markerCount) {
        throw std::invalid_argument("ID is outside the selected dictionary.");
    }

#ifdef __EMSCRIPTEN__
    std::size_t offset = 0;
    for (const auto& dictionary : DictionaryList) {
        if (dictionary.type == type) {
            return detail::webDictionaryCells(offset, id, info.payloadSize);
        }
        offset += static_cast<std::size_t>(dictionary.markerCount);
    }
    throw std::invalid_argument("Unknown marker dictionary.");
#else
    std::vector<std::uint8_t> blackCells(static_cast<std::size_t>(info.payloadSize * info.payloadSize));
    if (type == DictionaryType::Mip36h12) {
        const auto code = Mip36h12Codes[static_cast<std::size_t>(id)];
        for (int index = 0; index < 36; ++index) {
            blackCells[static_cast<std::size_t>(index)] = static_cast<std::uint8_t>(((code >> (35 - index)) & 1u) == 0);
        }
        return blackCells;
    }

    const auto dictionary = cv::aruco::getPredefinedDictionary(openCvDictionary(type));
    const auto bitGrid =
            cv::aruco::Dictionary::getBitsFromByteList(dictionary->bytesList.row(id), dictionary->markerSize);
    for (int row = 0; row < info.payloadSize; ++row) {
        for (int column = 0; column < info.payloadSize; ++column) {
            const auto bitIndex = static_cast<std::size_t>(row * info.payloadSize + column);
            const int sourceRow = isAprilTag(type) ? info.payloadSize - row - 1 : row;
            const int sourceColumn = isAprilTag(type) ? info.payloadSize - column - 1 : column;
            blackCells[bitIndex] = static_cast<std::uint8_t>(bitGrid.at<std::uint8_t>(sourceRow, sourceColumn) == 0);
        }
    }
    return blackCells;
#endif
}

std::vector<std::uint8_t> make3mf(const Parameters& parameters) {
    validate(parameters);
    const std::string contentTypes =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
            "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
            "<Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
            "</Types>";
    const std::string relationships =
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
            "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
            "<Relationship Target=\"/3D/3dmodel.model\" Id=\"rel0\" "
            "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/>"
            "</Relationships>";

    return makeZip(
            {{"[Content_Types].xml", contentTypes, 0},
             {"_rels/.rels", relationships, 0},
             {"3D/3dmodel.model", modelXml(parameters), 0}});
}

void save3mf(const std::filesystem::path& path, const Parameters& parameters) {
    const auto bytes = make3mf(parameters);
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open the output file.");
    }
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) {
        throw std::runtime_error("Could not write the output file.");
    }
}
}  // namespace markerplate
