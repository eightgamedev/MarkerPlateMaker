#include "../functional_test/numeric_input.h"

#include <gtest/gtest.h>

TEST(NumericInputTest, FiltersPastedTextAndPreservesCursorPosition)
{
    std::u32string text = U"1あ2.３4x5";
    std::size_t cursor = 5;

    EXPECT_TRUE(filterNumericInput(text, cursor, true));
    EXPECT_EQ(text, U"12.45");
    EXPECT_EQ(cursor, 3u);
}

TEST(NumericInputTest, IdRejectsDecimalPointAndNonAsciiDigits)
{
    std::u32string text = U"1.２3";
    std::size_t cursor = text.size();

    EXPECT_TRUE(filterNumericInput(text, cursor, false));
    EXPECT_EQ(text, U"13");
    EXPECT_EQ(cursor, 2u);
}

TEST(NumericInputTest, ValidInputAndCursorAreUnchanged)
{
    std::u32string text = U"12.50";
    std::size_t cursor = 2;

    EXPECT_FALSE(filterNumericInput(text, cursor, true));
    EXPECT_EQ(text, U"12.50");
    EXPECT_EQ(cursor, 2u);
}
