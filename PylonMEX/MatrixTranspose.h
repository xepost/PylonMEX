#pragma once

#define REC_SZ 16


inline void transpose16x16_uint8(uint8_t* dst, const uint8_t* src, size_t height, size_t width) {
	__m128i row[16];
	uint8_t i;
	uint8_t j;

	__m128i tmp;
	//load data -> AB...OP ; AB...;...
	for (i = 0; i < 16; ++i) {
		row[i] = _mm_lddqu_si128((__m128i*)&src[i * width]);
	}

	//first interleave -> AACC...;IIJJ...;AACC...
	for (i = 0; i < 16; i += 2) {
		tmp = _mm_unpacklo_epi8(row[i], row[i + 1]);
		row[i + 1] = _mm_unpackhi_epi8(row[i], row[i + 1]);
		row[i] = tmp;
	}

	//second interleave ->A;I;B;J;A...
	for (i = 0; i < 16; i += 4) {
		for (j = 0; j < 2; ++j) {
			tmp = _mm_unpacklo_epi16(row[i + j], row[i + 2 + j]);
			row[i + 2 + j] = _mm_unpackhi_epi16(row[i + j], row[i + 2 + j]);
			row[i + j] = tmp;
		}
	}

	//third interleave -> A;I;B;J;C;K;D;L;A...
	for (i = 0; i < 16; i += 8) {
		for (j = 0; j < 4; ++j) {
			tmp = _mm_unpacklo_epi32(row[i + j], row[i + 4 + j]);
			row[i + 4 + j] = _mm_unpackhi_epi32(row[i + j], row[i + 4 + j]);
			row[i + j] = tmp;
		}
	}

	//fourth interleave
	for (i = 0; i < 8; ++i) {
		tmp = _mm_unpacklo_epi64(row[i], row[i + 8]);
		row[i + 8] = _mm_unpackhi_epi64(row[i], row[i + 8]);
		row[i] = tmp;
	}

	//set data
	_mm_storeu_si128((__m128i*)&dst[0 * height], row[0]);//a
	_mm_storeu_si128((__m128i*)&dst[1 * height], row[8]);//b
	_mm_storeu_si128((__m128i*)&dst[2 * height], row[4]);//c
	_mm_storeu_si128((__m128i*)&dst[3 * height], row[12]);//d
	_mm_storeu_si128((__m128i*)&dst[4 * height], row[2]);//e
	_mm_storeu_si128((__m128i*)&dst[5 * height], row[10]);//f
	_mm_storeu_si128((__m128i*)&dst[6 * height], row[6]);//g
	_mm_storeu_si128((__m128i*)&dst[7 * height], row[14]);//h
	_mm_storeu_si128((__m128i*)&dst[8 * height], row[1]);//i
	_mm_storeu_si128((__m128i*)&dst[9 * height], row[9]);//j
	_mm_storeu_si128((__m128i*)&dst[10 * height], row[5]);//k
	_mm_storeu_si128((__m128i*)&dst[11 * height], row[13]);//l
	_mm_storeu_si128((__m128i*)&dst[12 * height], row[3]);//m
	_mm_storeu_si128((__m128i*)&dst[13 * height], row[11]);//n
	_mm_storeu_si128((__m128i*)&dst[14 * height], row[7]);//o
	_mm_storeu_si128((__m128i*)&dst[15 * height], row[15]);//p

}

//Transpose uint8_t matrix using SSE operations
//Assumes data is row-major oriented
inline void transposeSSE(uint8_t* dst, const uint8_t* src, size_t height, size_t width) {
	if (height < 16 || width < 16) {
		throw "Height and width should be >= 16. Ideally multiples of 16";
	}
#pragma omp parallel for
	for (int j = 0; j < height - 16; j += 16) {
		//j = j + 16 < height ? j : height - 16; //if not multiple of 16, last 2 rows will overlap
		for (int i = 0; i < width - 16; i += 16) {
			//i = i + 16 < width ? i : width - 16; //if not multiple of 16, last 2 cols will overlap
			//cout << "i:" << (int)i << " j:" << (int)j << endl;
			transpose16x16_uint8(&dst[i*height + j], &src[j*width + i], height, width);
		}
	}
	//process last col, except corner
#pragma omp parallel for
	for (int j = 0; j < height - 16; j += 16) {
		int i = width - 16;
		transpose16x16_uint8(&dst[i*height + j], &src[j*width + i], height, width);
	}
	//process last row, except corner
#pragma omp parallel for
	for (int i = 0; i < width - 16; i += 16) {
		int j = height - 16;
		transpose16x16_uint8(&dst[i*height + j], &src[j*width + i], height, width);
	}
	//process corner
	transpose16x16_uint8(&dst[(width - 16)*height + (height - 16)], &src[(height - 16)*width + (width - 16)], height, width);

}

//transpose matrix using cache optimize routine
template <typename T>
void cachetran(size_t r_start, size_t r_end, size_t c_start, size_t c_end, const T* src, T* dst, size_t src_rows, size_t src_cols) {
	size_t delr = r_end - r_start;
	size_t delc = c_end - c_start;
	if (delr <= REC_SZ && delc <= REC_SZ) {
		for (size_t r = r_start; r < r_end; ++r) {
			for (size_t c = c_start; c < c_end; ++c) {
				dst[c*src_rows + r] = src[r*src_cols + c];
			}
		}
	}
	else if (delr >= delc) {
		cachetran(r_start, r_start + delr / 2, c_start, c_end, src, dst, src_rows, src_cols);
		cachetran(r_start + delr / 2, r_end, c_start, c_end, src, dst, src_rows, src_cols);
	}
	else {
		cachetran(r_start, r_end, c_start, c_start + delc / 2, src, dst, src_rows, src_cols);
		cachetran(r_start, r_end, c_start + delc / 2, c_end, src, dst, src_rows, src_cols);
	}
}


//Transpose uint8_t matrix
//Assumes data is row-major oriented
//if width and height >= 16 then SSE optimized transpose is used
//otherwise cache optimized transpose is used
void transpose(const uint8_t* in, uint8_t* out, size_t in_height, size_t in_width) {
	if (in_height < 16 || in_width < 16) {
		cachetran(0, in_height, 0, in_width, in, out, in_height, in_width);
	}
	else {
		transposeSSE(out, in, in_height, in_width);
	}
}

//transpose array
template <typename T>
void transpose(const T* in, T* out, size_t in_m, size_t in_n) {
	cachetran(0, in_m, 0, in_n, in, out, in_m, in_n);
}

