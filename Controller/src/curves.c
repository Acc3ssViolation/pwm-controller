#include "curves.h"

/** Generated using Dr LUT - Free Lookup Table Generator
 * https://github.com/ppelikan/drlut
 **/

// Formula: (t/16)^2
const uint8_t _curveExp[256 - 26] = {
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6,
    6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10,
    11, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 23, 23,
    24, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32,
    32, 33, 34, 35, 35, 36, 37, 38, 38, 39, 40, 41, 41,
    42, 43, 44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53,
    53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65,
    66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 77, 78, 79,
    80, 81, 82, 83, 84, 86, 87, 88, 89, 90, 91, 93, 94,
    95, 96, 98, 99, 100, 101, 103, 104, 105, 106, 108, 109, 110,
    112, 113, 114, 116, 117, 118, 120, 121, 122, 124, 125, 127, 128,
    129, 131, 132, 134, 135, 137, 138, 140, 141, 143, 144, 146, 147,
    149, 150, 152, 153, 155, 156, 158, 159, 161, 163, 164, 166, 167,
    169, 171, 172, 174, 176, 177, 179, 181, 182, 184, 186, 187, 189,
    191, 193, 194, 196, 198, 200, 201, 203, 205, 207, 208, 210, 212,
    214, 216, 218, 219, 221, 223, 225, 227, 229, 231, 233, 234, 236,
    238, 240, 242, 244, 246, 248, 250, 252, 254};

uint8_t Curves_Exp(uint8_t in)
{
  if (in <= 12)
    return 0;
  if (in <= 20)
    return 1;
  if (in <= 26)
    return 2;
  return _curveExp[in];
}

uint8_t Curves_Linear(uint8_t in)
{
  return in;
}