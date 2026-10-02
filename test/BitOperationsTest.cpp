#include <gtest/gtest.h>

#include "rqt_multiplot/BitOperations.hpp"

namespace {

using rqt_multiplot::BitOperations;

TEST(BitOperations, revertByteShortAndIntRoundTrip) {
  EXPECT_EQ(BitOperations::revertByte(0), 0u);
  EXPECT_EQ(BitOperations::revertByte(0x01), 0x80u);
  EXPECT_EQ(BitOperations::revertByte(0xF0), 0x0Fu);
  EXPECT_EQ(BitOperations::revertByte(BitOperations::revertByte(0xA5)), 0xA5u);

  EXPECT_EQ(BitOperations::revertShort(0), 0u);
  EXPECT_EQ(BitOperations::revertShort(0x0001), 0x8000u);
  EXPECT_EQ(BitOperations::revertShort(BitOperations::revertShort(0x1234)), 0x1234u);

  EXPECT_EQ(BitOperations::revertInt(0u), 0u);
  EXPECT_EQ(BitOperations::revertInt(0x00000001u), 0x80000000u);
  EXPECT_EQ(BitOperations::revertInt(BitOperations::revertInt(0xF00DF00Du)), 0xF00DF00Du);
}

}  // namespace
