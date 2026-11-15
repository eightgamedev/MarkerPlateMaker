#pragma once

#include "marker_plate.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace markerplate::detail {
inline constexpr std::array<std::uint64_t, 7222> WebDictionaryCodes{
#include "web_dictionary_codes.inc"
};

inline std::vector<std::uint8_t> webDictionaryCells(std::size_t offset, int id, int payloadSize) {
    const auto code = WebDictionaryCodes.at(offset + static_cast<std::size_t>(id));
    std::vector<std::uint8_t> cells(static_cast<std::size_t>(payloadSize * payloadSize));
    for (std::size_t index = 0; index < cells.size(); ++index) {
        cells[index] = static_cast<std::uint8_t>((code >> index) & 1u);
    }
    return cells;
}
}  // namespace markerplate::detail
