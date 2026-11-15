#pragma once

#include <algorithm>
#include <cstddef>
#include <string>

inline bool filterNumericInput(std::u32string& text, std::size_t& cursorPos, bool allowDecimalPoint)
{
    const std::size_t oldCursor = std::min(cursorPos, text.size());
    std::size_t writePos = 0;
    std::size_t newCursor = 0;
    for (std::size_t readPos = 0; readPos < text.size(); ++readPos)
    {
        const char32_t ch = text[readPos];
        if ((ch >= U'0' && ch <= U'9') || (allowDecimalPoint && ch == U'.'))
        {
            text[writePos++] = ch;
            if (readPos < oldCursor)
            {
                ++newCursor;
            }
        }
    }

    const bool rejected = writePos != text.size();
    text.resize(writePos);
    cursorPos = newCursor;
    return rejected;
}
