/*
* Copyright 2016 Huy Cuong Nguyen
* Copyright 2006 Jeremias Maerki.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "DMECEncoder.h"

#include "ByteArray.h"
#include "DMSymbolInfo.h"
#include "ZXStrConvWorkaround.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

namespace ZXing::DataMatrix {

/**
* Precomputed polynomial factors for ECC 200.
*/
static const std::array<ByteArray, 16> FACTORS = {{
	/*set  1*/ {228, 48, 15, 111, 62},
	/*set  2*/ {23, 68, 144, 134, 240, 92, 254},
	/*set  3*/ {28, 24, 185, 166, 223, 248, 116, 255, 110, 61},
	/*set  4*/ {175, 138, 205, 12, 194, 168, 39, 245, 60, 97, 120},
	/*set  5*/ {41, 153, 158, 91, 61, 42, 142, 213, 97, 178, 100, 242},
	/*set  6*/ {156, 97, 192, 252, 95, 9, 157, 119, 138, 45, 18, 186, 83, 185},
	/*set  7*/ {83, 195, 100, 39, 188, 75, 66, 61, 241, 213, 109, 129, 94, 254, 225, 48, 90, 188},
	/*set  8*/ {15, 195, 244, 9, 233, 71, 168, 2, 188, 160, 153, 145, 253, 79, 108, 82, 27, 174, 186, 172},
	/*set  9*/ {52, 190, 88, 205, 109, 39, 176, 21, 155, 197, 251, 223, 155, 21, 5, 172, 254, 124, 12, 181,
				184, 96, 50, 193},
	/*set 10*/ {211, 231, 43, 97, 71, 96, 103, 174, 37, 151, 170, 53, 75, 34, 249, 121, 17, 138, 110, 213,
				141, 136, 120, 151, 233, 168, 93, 255},
	/*set 11*/ {245, 127, 242, 218, 130, 250, 162, 181, 102, 120, 84, 179, 220, 251, 80, 182, 229, 18, 2, 4,
				68, 33, 101, 137, 95, 119, 115, 44, 175, 184, 59, 25, 225, 98, 81, 112},
	/*set 12*/ {77, 193, 137, 31, 19, 38, 22, 153, 247, 105, 122, 2, 245, 133, 242, 8, 175, 95, 100, 9, 167,
				105, 214, 111, 57, 121, 21, 1, 253, 57, 54, 101, 248, 202, 69, 50, 150, 177, 226, 5, 9, 5},
	/*set 13*/ {245, 132, 172, 223, 96, 32, 117, 22, 238, 133, 238, 231, 205, 188, 237, 87, 191, 106, 16, 147,
				118, 23, 37, 90, 170, 205, 131, 88, 120, 100, 66, 138, 186, 240, 82, 44, 176, 87, 187, 147,
				160, 175, 69, 213, 92, 253, 225, 19},
	/*set 14*/ {175, 9, 223, 238, 12, 17, 220, 208, 100, 29, 175, 170, 230, 192, 215, 235, 150, 159, 36, 223,
				38, 200, 132, 54, 228, 146, 218, 234, 117, 203, 29, 232, 144, 238, 22, 150, 201, 117, 62, 207,
				164, 13, 137, 245, 127, 67, 247, 28, 155, 43, 203, 107, 233, 53, 143, 46},
	/*set 15*/ {242, 93, 169, 50, 144, 210, 39, 118, 202, 188, 201, 189, 143, 108, 196, 37, 185, 112, 134, 230,
				245, 63, 197, 190, 250, 106, 185, 221, 175, 64, 114, 71, 161, 44, 147, 6, 27, 218, 51, 63, 87,
				10, 40, 130, 188, 17, 163, 31, 176, 170, 4, 107, 232, 7, 94, 166, 224, 124, 86, 47, 11, 204},
	/*set 16*/ {220, 228, 173, 89, 251, 149, 159, 56, 89, 33, 147, 244, 154, 36, 73, 127, 213, 136, 248, 180,
				234, 197, 158, 177, 68, 122, 93, 213, 15, 160, 227, 236, 66, 139, 153, 185, 202, 167, 179, 25,
				220, 232, 96, 210, 231, 136, 223, 239, 181, 241, 59, 52, 172, 25, 49, 232, 211, 189, 64, 54,
                108, 153, 132, 63, 96, 103, 82, 186},
}};

static const uint8_t LOG[] = {
	  0, 255,   1, 240,   2, 225, 241,  53,   3,  38, 226, 133, 242,  43,  54, 210,
	  4, 195,  39, 114, 227, 106, 134,  28, 243, 140,  44,  23,  55, 118, 211, 234,
	  5, 219, 196,  96,  40, 222, 115, 103, 228,  78, 107, 125, 135,   8,  29, 162,
	244, 186, 141, 180,  45,  99,  24,  49,  56,  13, 119, 153, 212, 199, 235,  91,
	  6,  76, 220, 217, 197,  11,  97, 184,  41,  36, 223, 253, 116, 138, 104, 193,
	229,  86,  79, 171, 108, 165, 126, 145, 136,  34,   9,  74,  30,  32, 163,  84,
	245, 173, 187, 204, 142,  81, 181, 190,  46,  88, 100, 159,  25, 231,  50, 207,
	 57, 147,  14,  67, 120, 128, 154, 248, 213, 167, 200,  63, 236, 110,  92, 176,
	  7, 161,  77, 124, 221, 102, 218,  95, 198,  90,  12, 152,  98,  48, 185, 179,
	 42, 209,  37, 132, 224,  52, 254, 239, 117, 233, 139,  22, 105,  27, 194, 113,
	230, 206,  87, 158,  80, 189, 172, 203, 109, 175, 166,  62, 127, 247, 146,  66,
	137, 192,  35, 252,  10, 183,  75, 216,  31,  83,  33,  73, 164, 144,  85, 170,
	246,  65, 174,  61, 188, 202, 205, 157, 143, 169,  82,  72, 182, 215, 191, 251,
	 47, 178,  89, 151, 101,  94, 160, 123,  26, 112, 232,  21,  51, 238, 208, 131,
	 58,  69, 148,  18,  15,  16,  68,  17, 121, 149, 129,  19, 155,  59, 249,  70,
	214, 250, 168,  71, 201, 156,  64,  60, 237, 130, 111,  20,  93, 122, 177, 150,
};

static const uint8_t ALOG[] = {
	  1,   2,   4,   8,  16,  32,  64, 128,  45,  90, 180,  69, 138,  57, 114, 228,
	229, 231, 227, 235, 251, 219, 155,  27,  54, 108, 216, 157,  23,  46,  92, 184,
	 93, 186,  89, 178,  73, 146,   9,  18,  36,  72, 144,  13,  26,  52, 104, 208,
	141,  55, 110, 220, 149,   7,  14,  28,  56, 112, 224, 237, 247, 195, 171, 123,
	246, 193, 175, 115, 230, 225, 239, 243, 203, 187,  91, 182,  65, 130,  41,  82,
	164, 101, 202, 185,  95, 190,  81, 162, 105, 210, 137,  63, 126, 252, 213, 135,
	 35,  70, 140,  53, 106, 212, 133,  39,  78, 156,  21,  42,  84, 168, 125, 250,
	217, 159,  19,  38,  76, 152,  29,  58, 116, 232, 253, 215, 131,  43,  86, 172,
	117, 234, 249, 223, 147,  11,  22,  44,  88, 176,  77, 154,  25,  50, 100, 200,
	189,  87, 174, 113, 226, 233, 255, 211, 139,  59, 118, 236, 245, 199, 163, 107,
	214, 129,  47,  94, 188,  85, 170, 121, 242, 201, 191,  83, 166,  97, 194, 169,
	127, 254, 209, 143,  51, 102, 204, 181,  71, 142,  49,  98, 196, 165, 103, 206,
	177,  79, 158,  17,  34,  68, 136,  61, 122, 244, 197, 167,  99, 198, 161, 111,
	222, 145,  15,  30,  60, 120, 240, 205, 183,  67, 134,  33,  66, 132,  37,  74,
	148,   5,  10,  20,  40,  80, 160, 109, 218, 153,  31,  62, 124, 248, 221, 151,
	  3,   6,  12,  24,  48,  96, 192, 173, 119, 238, 241, 207, 179,  75, 150,   1,
};

static uint8_t mult(uint8_t a, uint8_t b)
{
	if(a == 0 || b == 0)
		return 0;
	return ALOG[(LOG[a] + LOG[b]) % 255];
}

//TODO: replace this duplicated code with ReedSolomonEncoder
static void CreateECCBlock(ByteArray& data, int codeOffset, int codeLength, int eccOffset, int eccLength, int stride)
{
	// binary search for the poly vector with length numECWords
	auto iter = std::lower_bound(FACTORS.begin(), FACTORS.end(), eccLength,
							   [](const ByteArray& vec, size_t size) { return vec.size() < size; });
	if (iter == FACTORS.end())
		throw std::invalid_argument("Illegal number of error correction codewords specified: " + std::to_string(eccLength));

	auto& poly = *iter;
	ByteArray ecc(eccLength);
	for (int i = 0; i < codeLength; ++i) {
		const auto m = ecc.back() ^ data[codeOffset + i * stride];
		for (size_t k = ecc.size() - 1; k > 0; k--)
			ecc[k] = ecc[k - 1] ^ mult(m, poly[k]);
		ecc[0] = mult(m, poly[0]);
	}
	for (int i = 0; i < eccLength; ++i)
		data[eccOffset + i * stride] = ecc[eccLength - 1 - i];
}

void EncodeECC200(ByteArray& codewords, const SymbolInfo& symbolInfo)
{
	if (codewords.size() != (size_t)symbolInfo.dataCapacity()) {
		throw std::invalid_argument("The number of codewords does not match the selected symbol");
	}
	codewords.resize(symbolInfo.codewordCount(), 0);
	int blockCount = symbolInfo.interleavedBlockCount();
	if (blockCount == 1) {
		CreateECCBlock(codewords, 0, symbolInfo.dataCapacity(), symbolInfo.dataCapacity(), symbolInfo.errorCodewords(), 1);
	}
	else {
		for (int block = 0; block < blockCount; block++)
			CreateECCBlock(codewords, block, symbolInfo.dataLengthForInterleavedBlock(block + 1),
						   symbolInfo.dataCapacity() + block, symbolInfo.errorLengthForInterleavedBlock(), blockCount);
	}
}

} // namespace ZXing::DataMatrix
