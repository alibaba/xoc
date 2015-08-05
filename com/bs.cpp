/*@
XOC Release License

Copyright (c) 2013-2014, Alibaba Group, All rights reserved.

    compiler@aliexpress.com

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Su Zhenyu nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

author: Su Zhenyu
@*/
#include "ltype.h"
#include "comf.h"
#include "smempool.h"
#include "sstl.h"
#include "bs.h"

#if BITS_PER_BYTE == 8
#define DIVBPB(a) ((a) >> 3)
#define MULBPB(a) ((a) << 3)
#define MODBPB(a) ((a) & 7)
#else
#define DIVBPB(a) ((a) / BITS_PER_BYTE)
#define MULBPB(a) ((a) * BITS_PER_BYTE)
#define MODBPB(a) ((a) % BITS_PER_BYTE)
#endif

//Count of bits in all the one byte numbers.
BYTE const g_bit_count[256] = {
  0, //  0
  1, //  1
  1, //  2
  2, //  3
  1, //  4
  2, //  5
  2, //  6
  3, //  7
  1, //  8
  2, //  9
  2, // 10
  3, // 11
  2, // 12
  3, // 13
  3, // 14
  4, // 15
  1, // 16
  2, // 17
  2, // 18
  3, // 19
  2, // 20
  3, // 21
  3, // 22
  4, // 23
  2, // 24
  3, // 25
  3, // 26
  4, // 27
  3, // 28
  4, // 29
  4, // 30
  5, // 31
  1, // 32
  2, // 33
  2, // 34
  3, // 35
  2, // 36
  3, // 37
  3, // 38
  4, // 39
  2, // 40
  3, // 41
  3, // 42
  4, // 43
  3, // 44
  4, // 45
  4, // 46
  5, // 47
  2, // 48
  3, // 49
  3, // 50
  4, // 51
  3, // 52
  4, // 53
  4, // 54
  5, // 55
  3, // 56
  4, // 57
  4, // 58
  5, // 59
  4, // 60
  5, // 61
  5, // 62
  6, // 63
  1, // 64
  2, // 65
  2, // 66
  3, // 67
  2, // 68
  3, // 69
  3, // 70
  4, // 71
  2, // 72
  3, // 73
  3, // 74
  4, // 75
  3, // 76
  4, // 77
  4, // 78
  5, // 79
  2, // 80
  3, // 81
  3, // 82
  4, // 83
  3, // 84
  4, // 85
  4, // 86
  5, // 87
  3, // 88
  4, // 89
  4, // 90
  5, // 91
  4, // 92
  5, // 93
  5, // 94
  6, // 95
  2, // 96
  3, // 97
  3, // 98
  4, // 99
  3, //100
  4, //101
  4, //102
  5, //103
  3, //104
  4, //105
  4, //106
  5, //107
  4, //108
  5, //109
  5, //110
  6, //111
  3, //112
  4, //113
  4, //114
  5, //115
  4, //116
  5, //117
  5, //118
  6, //119
  4, //120
  5, //121
  5, //122
  6, //123
  5, //124
  6, //125
  6, //126
  7, //127
  1, //128
  2, //129
  2, //130
  3, //131
  2, //132
  3, //133
  3, //134
  4, //135
  2, //136
  3, //137
  3, //138
  4, //139
  3, //140
  4, //141
  4, //142
  5, //143
  2, //144
  3, //145
  3, //146
  4, //147
  3, //148
  4, //149
  4, //150
  5, //151
  3, //152
  4, //153
  4, //154
  5, //155
  4, //156
  5, //157
  5, //158
  6, //159
  2, //160
  3, //161
  3, //162
  4, //163
  3, //164
  4, //165
  4, //166
  5, //167
  3, //168
  4, //169
  4, //170
  5, //171
  4, //172
  5, //173
  5, //174
  6, //175
  3, //176
  4, //177
  4, //178
  5, //179
  4, //180
  5, //181
  5, //182
  6, //183
  4, //184
  5, //185
  5, //186
  6, //187
  5, //188
  6, //189
  6, //190
  7, //191
  2, //192
  3, //193
  3, //194
  4, //195
  3, //196
  4, //197
  4, //198
  5, //199
  3, //200
  4, //201
  4, //202
  5, //203
  4, //204
  5, //205
  5, //206
  6, //207
  3, //208
  4, //209
  4, //210
  5, //211
  4, //212
  5, //213
  5, //214
  6, //215
  4, //216
  5, //217
  5, //218
  6, //219
  5, //220
  6, //221
  6, //222
  7, //223
  3, //224
  4, //225
  4, //226
  5, //227
  4, //228
  5, //229
  5, //230
  6, //231
  4, //232
  5, //233
  5, //234
  6, //235
  5, //236
  6, //237
  6, //238
  7, //239
  4, //240
  5, //241
  5, //242
  6, //243
  5, //244
  6, //245
  6, //246
  7, //247
  5, //248
  6, //249
  6, //250
  7, //251
  6, //252
  7, //253
  7, //254
  8  //255
};


//Mapping from 8 bit unsigned integers to the index of the first one bit.
static BYTE const g_first_one[256] = {
  0, //   no one
  0, //   1 the first bit in 0th pos
  1, //   2 the first bit in 1th pos
  0, //   3 the first bit in 0th pos
  2, //   4 the first bit in 2th pos
  0, //   5
  1, //   6
  0, //   7
  3, //   8
  0, //   9
  1, //  10
  0, //  11
  2, //  12
  0, //  13
  1, //  14
  0, //  15
  4, //  16
  0, //  17
  1, //  18
  0, //  19
  2, //  20
  0, //  21
  1, //  22
  0, //  23
  3, //  24
  0, //  25
  1, //  26
  0, //  27
  2, //  28
  0, //  29
  1, //  30
  0, //  31
  5, //  32
  0, //  33
  1, //  34
  0, //  35
  2, //  36
  0, //  37
  1, //  38
  0, //  39
  3, //  40
  0, //  41
  1, //  42
  0, //  43
  2, //  44
  0, //  45
  1, //  46
  0, //  47
  4, //  48
  0, //  49
  1, //  50
  0, //  51
  2, //  52
  0, //  53
  1, //  54
  0, //  55
  3, //  56
  0, //  57
  1, //  58
  0, //  59
  2, //  60
  0, //  61
  1, //  62
  0, //  63
  6, //  64
  0, //  65
  1, //  66
  0, //  67
  2, //  68
  0, //  69
  1, //  70
  0, //  71
  3, //  72
  0, //  73
  1, //  74
  0, //  75
  2, //  76
  0, //  77
  1, //  78
  0, //  79
  4, //  80
  0, //  81
  1, //  82
  0, //  83
  2, //  84
  0, //  85
  1, //  86
  0, //  87
  3, //  88
  0, //  89
  1, //  90
  0, //  91
  2, //  92
  0, //  93
  1, //  94
  0, //  95
  5, //  96
  0, //  97
  1, //  98
  0, //  99
  2, // 100
  0, // 101
  1, // 102
  0, // 103
  3, // 104
  0, // 105
  1, // 106
  0, // 107
  2, // 108
  0, // 109
  1, // 110
  0, // 111
  4, // 112
  0, // 113
  1, // 114
  0, // 115
  2, // 116
  0, // 117
  1, // 118
  0, // 119
  3, // 120
  0, // 121
  1, // 122
  0, // 123
  2, // 124
  0, // 125
  1, // 126
  0, // 127
  7, // 128
  0, // 129
  1, // 130
  0, // 131
  2, // 132
  0, // 133
  1, // 134
  0, // 135
  3, // 136
  0, // 137
  1, // 138
  0, // 139
  2, // 140
  0, // 141
  1, // 142
  0, // 143
  4, // 144
  0, // 145
  1, // 146
  0, // 147
  2, // 148
  0, // 149
  1, // 150
  0, // 151
  3, // 152
  0, // 153
  1, // 154
  0, // 155
  2, // 156
  0, // 157
  1, // 158
  0, // 159
  5, // 160
  0, // 161
  1, // 162
  0, // 163
  2, // 164
  0, // 165
  1, // 166
  0, // 167
  3, // 168
  0, // 169
  1, // 170
  0, // 171
  2, // 172
  0, // 173
  1, // 174
  0, // 175
  4, // 176
  0, // 177
  1, // 178
  0, // 179
  2, // 180
  0, // 181
  1, // 182
  0, // 183
  3, // 184
  0, // 185
  1, // 186
  0, // 187
  2, // 188
  0, // 189
  1, // 190
  0, // 191
  6, // 192
  0, // 193
  1, // 194
  0, // 195
  2, // 196
  0, // 197
  1, // 198
  0, // 199
  3, // 200
  0, // 201
  1, // 202
  0, // 203
  2, // 204
  0, // 205
  1, // 206
  0, // 207
  4, // 208
  0, // 209
  1, // 210
  0, // 211
  2, // 212
  0, // 213
  1, // 214
  0, // 215
  3, // 216
  0, // 217
  1, // 218
  0, // 219
  2, // 220
  0, // 221
  1, // 222
  0, // 223
  5, // 224
  0, // 225
  1, // 226
  0, // 227
  2, // 228
  0, // 229
  1, // 230
  0, // 231
  3, // 232
  0, // 233
  1, // 234
  0, // 235
  2, // 236
  0, // 237
  1, // 238
  0, // 239
  4, // 240
  0, // 241
  1, // 242
  0, // 243
  2, // 244
  0, // 245
  1, // 246
  0, // 247
  3, // 248
  0, // 249
  1, // 250
  0, // 251
  2, // 252
  0, // 253
  1, // 254
  0, // 255
};


static BYTE const g_last_one[256] = {
  0, //  0
  0, //  1
  1, //  2
  1, //  3
  2, //  4
  2, //  5
  2, //  6
  2, //  7
  3, //  8
  3, //  9
  3, // 10
  3, // 11
  3, // 12
  3, // 13
  3, // 14
  3, // 15
  4, // 16
  4, // 17
  4, // 18
  4, // 19
  4, // 20
  4, // 21
  4, // 22
  4, // 23
  4, // 24
  4, // 25
  4, // 26
  4, // 27
  4, // 28
  4, // 29
  4, // 30
  4, // 31
  5, // 32
  5, // 33
  5, // 34
  5, // 35
  5, // 36
  5, // 37
  5, // 38
  5, // 39
  5, // 40
  5, // 41
  5, // 42
  5, // 43
  5, // 44
  5, // 45
  5, // 46
  5, // 47
  5, // 48
  5, // 49
  5, // 50
  5, // 51
  5, // 52
  5, // 53
  5, // 54
  5, // 55
  5, // 56
  5, // 57
  5, // 58
  5, // 59
  5, // 60
  5, // 61
  5, // 62
  5, // 63
  6, // 64
  6, // 65
  6, // 66
  6, // 67
  6, // 68
  6, // 69
  6, // 70
  6, // 71
  6, // 72
  6, // 73
  6, // 74
  6, // 75
  6, // 76
  6, // 77
  6, // 78
  6, // 79
  6, // 80
  6, // 81
  6, // 82
  6, // 83
  6, // 84
  6, // 85
  6, // 86
  6, // 87
  6, // 88
  6, // 89
  6, // 90
  6, // 91
  6, // 92
  6, // 93
  6, // 94
  6, // 95
  6, // 96
  6, // 97
  6, // 98
  6, // 99
  6, //100
  6, //101
  6, //102
  6, //103
  6, //104
  6, //105
  6, //106
  6, //107
  6, //108
  6, //109
  6, //110
  6, //111
  6, //112
  6, //113
  6, //114
  6, //115
  6, //116
  6, //117
  6, //118
  6, //119
  6, //120
  6, //121
  6, //122
  6, //123
  6, //124
  6, //125
  6, //126
  6, //127
  7, //128
  7, //129
  7, //130
  7, //131
  7, //132
  7, //133
  7, //134
  7, //135
  7, //136
  7, //137
  7, //138
  7, //139
  7, //140
  7, //141
  7, //142
  7, //143
  7, //144
  7, //145
  7, //146
  7, //147
  7, //148
  7, //149
  7, //150
  7, //151
  7, //152
  7, //153
  7, //154
  7, //155
  7, //156
  7, //157
  7, //158
  7, //159
  7, //160
  7, //161
  7, //162
  7, //163
  7, //164
  7, //165
  7, //166
  7, //167
  7, //168
  7, //169
  7, //170
  7, //171
  7, //172
  7, //173
  7, //174
  7, //175
  7, //176
  7, //177
  7, //178
  7, //179
  7, //180
  7, //181
  7, //182
  7, //183
  7, //184
  7, //185
  7, //186
  7, //187
  7, //188
  7, //189
  7, //190
  7, //191
  7, //192
  7, //193
  7, //194
  7, //195
  7, //196
  7, //197
  7, //198
  7, //199
  7, //200
  7, //201
  7, //202
  7, //203
  7, //204
  7, //205
  7, //206
  7, //207
  7, //208
  7, //209
  7, //210
  7, //211
  7, //212
  7, //213
  7, //214
  7, //215
  7, //216
  7, //217
  7, //218
  7, //219
  7, //220
  7, //221
  7, //222
  7, //223
  7, //224
  7, //225
  7, //226
  7, //227
  7, //228
  7, //229
  7, //230
  7, //231
  7, //232
  7, //233
  7, //234
  7, //235
  7, //236
  7, //237
  7, //238
  7, //239
  7, //240
  7, //241
  7, //242
  7, //243
  7, //244
  7, //245
  7, //246
  7, //247
  7, //248
  7, //249
  7, //250
  7, //251
  7, //252
  7, //253
  7, //254
  7, //255
};


//
//START BITSET
//
void * BITSET::realloc(IN void * src, size_t orgsize, size_t newsize)
{
	if (orgsize >= newsize) {
		clean();
		return src;
	}
	void * p = ::malloc(newsize);
	if (src != NULL) {
		IS_TRUE0(orgsize > 0);
		::memcpy(p, src, orgsize);
		::free(src);
		::memset(((BYTE*)p) + orgsize, 0, newsize - orgsize);
	} else {
		::memset(p, 0, newsize);
	}
	return p;
}


//Allocate bytes
void BITSET::alloc(UINT size)
{
	m_size = size;
	if (m_ptr != NULL) { ::free(m_ptr);	}
	if (size != 0) {
		m_ptr = (BYTE*)::malloc(size);
		::memset(m_ptr, 0, m_size);
	} else {
		m_ptr = NULL;
	}
}


//Returns a new set which is the union of set1 and set2,
//and modify set1 as result operand.
void BITSET::bunion(BITSET const& bs)
{
	IS_TRUE0(this != &bs);
	if (bs.m_ptr == NULL) { return; }
	UINT cp_sz = bs.m_size; //size need to union.
	if (m_size < bs.m_size) {
		//src's last byte pos.
		INT l = bs.get_last();
		if (l < 0) { return; }
		cp_sz = l / BITS_PER_BYTE + 1;
		if (m_size < cp_sz) {
			m_ptr = (BYTE*)realloc(m_ptr, m_size, cp_sz);
			m_size = cp_sz;
		}
	}
	UINT const num_of_uint = cp_sz / BYTES_PER_UINT; //floor-div.
	IS_TRUE(m_ptr, ("not yet init"));
	UINT * uint_ptr_this = (UINT*)m_ptr;
	UINT * uint_ptr_bs = (UINT*)bs.m_ptr;
	for (UINT i = 0; i < num_of_uint; i++) {
		uint_ptr_this[i] |= uint_ptr_bs[i];
	}
	for (UINT i = num_of_uint * BYTES_PER_UINT; i < cp_sz; i++) {
		m_ptr[i] |= bs.m_ptr[i];
	}
}


//Add a element which corresponding to 'elem' bit, and set this bit.
void BITSET::bunion(INT elem)
{
	IS_TRUE(elem >= 0,("bit set cannot be NULL"));
	UINT const first_byte = DIVBPB(elem);
	if (m_size < (first_byte+1)) {
		m_ptr = (BYTE*)realloc(m_ptr, m_size, first_byte + 1);
		m_size = first_byte+1;
	}
	elem = MODBPB(elem);
	m_ptr[first_byte] |= (BYTE)(1 << elem);
}


/* The difference operation calculates the elements that
distinguish one set from another.
Remove a element which map with 'elem' bit, and clean this bit. */
void BITSET::diff(UINT elem)
{
	UINT first_byte = DIVBPB(elem);
	if ((first_byte + 1) > m_size) {
		return;
	}
	elem = MODBPB(elem);
	IS_TRUE(m_ptr != NULL, ("not yet init"));
	m_ptr[first_byte] &= (BYTE)(~(1 << elem));
}


/* The difference operation calculates the elements that
distinguish one set from another.
Subtracting set2 from set1
Returns a new set which is
	{ x : member( x, 'set1' ) & ~ member( x, 'set2' ) }. */
void BITSET::diff(BITSET const& bs)
{
	IS_TRUE0(this != &bs);
	if (m_size == 0 || bs.m_size == 0) { return; }
	UINT minsize = MIN(m_size, bs.m_size);
	//Common part: copy the inverse bits.
	//for (UINT i = 0; i < minsize; i++) {
	//	m_ptr[i] &= ~bs.m_ptr[i];
	//}
	UINT num_of_uint = minsize / BYTES_PER_UINT;
	UINT * uint_ptr_this = (UINT*)m_ptr;
	UINT * uint_ptr_bs = (UINT*)bs.m_ptr;
	for (UINT i = 0; i < num_of_uint; i++) {
		UINT d = uint_ptr_bs[i];
		if (d != 0) {
			uint_ptr_this[i] &= ~d;
		}
	}

	IS_TRUE(m_ptr != NULL, ("not yet init"));
	for (UINT i = num_of_uint * BYTES_PER_UINT; i < minsize; i++) {
		BYTE d = bs.m_ptr[i];
		if (d != 0) {
			m_ptr[i] &= ~d;
		}
	}
}


//Returns the a new set which is intersection of 'set1' and 'set2'.
void BITSET::intersect(BITSET const& bs)
{
	IS_TRUE0(this != &bs);
	if (m_ptr == NULL) { return; }
	if (m_size > bs.m_size) {
		for (UINT i = 0; i < bs.m_size; i++) {
			m_ptr[i] &= bs.m_ptr[i];
		}
		::memset(m_ptr + bs.m_size, 0, m_size - bs.m_size);
	} else {
		for (UINT i = 0; i < m_size; i++) {
			m_ptr[i] &= bs.m_ptr[i];
		}
	}
}


/* Reverse each bit.
e.g: 1001 to 0110
'last_bit_pos': start at 0, e.g:given '101', last bit pos is 2. */
void BITSET::rev(UINT last_bit_pos)
{
	IS_TRUE(m_ptr != NULL, ("can not reverse empty set"));
	UINT const last_byte_pos = last_bit_pos / BITS_PER_BYTE;
	IS_TRUE0(last_byte_pos < m_size);

	UINT const n = last_byte_pos / BYTES_PER_UINT;
	for (UINT i = 0; i < n; i++) {
		((UINT*)m_ptr)[i] = ~(((UINT*)m_ptr)[i]);
	}

	for (UINT i = n * BYTES_PER_UINT; i < last_byte_pos; i++) {
		m_ptr[i] = ~m_ptr[i];
	}

	//Use uint type to avoid byte truncate operation.
	UINT last_byte = m_ptr[last_byte_pos];
	UINT const mask = (1 << (last_bit_pos % BITS_PER_BYTE + 1)) - 1;
	UINT const rev_last_byte = (~last_byte) & mask;
	last_byte = (last_byte & (~mask)) | rev_last_byte;
	m_ptr[last_byte_pos] = last_byte;
}


//Complement set of s = univers - s.
void BITSET::complement(IN BITSET const& univers)
{
	BITSET tmp(univers);
	tmp.diff(*this);
	copy(tmp);
}


/* Return the element count in 'set'
Add up the population count of each byte in the set.  We get the
population counts from the table above.  Great for a machine with
effecient loadbyte instructions. */
UINT BITSET::get_elem_count() const
{
	if (m_ptr == NULL) { return 0; }
	UINT count = 0;
//#define HAMMING_WEIGHT_METHOD
#ifdef HAMMING_WEIGHT_METHOD
	IS_TRUE0(BYTES_PER_UINT == 4);
	UINT const m = m_size / BYTES_PER_UINT;
	for (UINT i = 0; i < m; i++) {
		INT v = ((INT*)(m_ptr))[i];
		v = v - ((v >> 1) & 0x55555555);
		v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
		count += (((v + (v >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
	}
	for (UINT i = m * BYTES_PER_UINT; i < m_size; i++) {
		count += g_bit_count[m_ptr[i]];
	}
#else
	for (UINT i = 0; i < m_size; i++) {
		count += g_bit_count[m_ptr[i]];
	}
#endif
	return count;
}


bool BITSET::is_equal(BITSET const& bs) const
{
	IS_TRUE0(this != &bs);
	UINT size1 = m_size , size2 = bs.m_size;
	if (size1 == 0) {
		if (size2 == 0) { return true; }
		return bs.is_empty();
	}
	if (size2 == 0) {
		if (size1 == 0) { return true; }
		return is_empty();
	}

	BYTE const* ptr1 = m_ptr;
	BYTE const* ptr2 = bs.m_ptr;
	//To guarantee set1 is the smaller.
	if (size1 > size2) {
		BYTE const* tmp = ptr1;
		ptr1 = ptr2;
		ptr2 = tmp;

		UINT tmp1 = size1;
		size1 = size2;
		size2 = tmp1;
	}

	UINT v1 = *(UINT*)ptr1;
	UINT v2 = *(UINT*)ptr2;
	switch (size1) {
	case 1:
		if ((v1&0xff) != (v2&0xff)) { return false;	}
		break;
	case 2:
		if ((v1&0xffff) != (v2&0xffff)) { return false;	}
		break;
	case 3:
		if ((v1&0xffffff) != (v2&0xffffff)) { return false; }
		break;
	case 4:
		if ((v1&0xffffffff) != (v2&0xffffffff)) { return false; }
		break;
	default:
		if (::memcmp(ptr1, ptr2, size1) != 0) { return false; }
	}

	for (UINT i = size1; i < size2; i++) {
		if (ptr2[i] != 0) {
			return false;
		}
	}
	return true;
}


//Return true if this contain elem.
bool BITSET::is_contain(UINT elem) const
{
	if (m_ptr == NULL) { return false; }
	if (elem >= (MULBPB(m_size))) {
		return false;
	}
	if ((m_ptr[DIVBPB(elem)] & (1 << (MODBPB(elem)))) != 0) {
		return true;
	}
	return false;
}


/* Return true if 'this' contains 'bs'.
'strict': If it is false, we say the bitset contains bs;
if it is true, the bitset must have at least one
element that does not belong to 'bs'. */
bool BITSET::is_contain(BITSET const& bs, bool strict) const
{
	IS_TRUE0(this != &bs);
	bool scon = false; //Set to true if 'this' strictly contained 'bs'.
	INT const first_bit = get_first();
	if (first_bit == -1) {
		return false;
	}
	INT const bs_first_bit = bs.get_first();
	if (bs_first_bit == -1) {
		//NULL set be contained for any set.
		return true;
	}

	if (first_bit > bs_first_bit) {
		return false;
	} else if (strict && first_bit < bs_first_bit) {
		scon = true;
	}

	INT const start_bit = MAX(first_bit, bs_first_bit);
	INT const start_byte = DIVBPB(start_bit);

	if (m_size < bs.m_size) {//'this' is smaller
		if (bs.get_next(MULBPB(m_size) - 1) != -1) {
			//'bs' has more elements than 'this'.
			return false;
		}
	} else if (strict && m_size > bs.m_size) {//'bs' is smaller
		if (get_next(MULBPB(bs.m_size) - 1) != -1) {
			//'this' has more elements than 'this'.
			scon = true;
		}
	}

	INT const minsize = MIN(m_size, bs.m_size);
	for (INT i = start_byte; i < minsize; i++) {
		if ((m_ptr[i] & bs.m_ptr[i]) != bs.m_ptr[i]) {
			return false;
		}
		if (strict && m_ptr[i] != bs.m_ptr[i]) {
			scon = true;
		}
	}

	if (strict && !scon) {
		return false;
	}
	return true;
}


bool BITSET::is_empty() const
{
	if (m_ptr == NULL) { return true; }
	UINT num_of_uint = m_size / BYTES_PER_UINT;
	UINT * uint_ptr = (UINT*)m_ptr;
	for (UINT i = 0; i < num_of_uint; i++) {
		if (uint_ptr[i] != 0) {
	 		return false;
		}
	}
	for (UINT i = num_of_uint * BYTES_PER_UINT; i < m_size; i++) {
		if (m_ptr[i] != (BYTE)0) {
	 		return false;
		}
	}
	return true;
}


bool BITSET::is_intersect(BITSET const& bs) const
{
	IS_TRUE0(this != &bs);
	INT const first_bit = get_first();
	if (first_bit == -1) {
		return false;
	}
	INT const bs_first_bit = bs.get_first();
	if (bs_first_bit == -1) { //Empty set
		return false;
	}

	UINT const start_bit = MAX(first_bit, bs_first_bit);
	UINT const end_bit = MIN(get_last(), bs.get_last());
	UINT const start_byte = DIVBPB(start_bit);
	UINT const end_byte = DIVBPB(end_bit);

	for (UINT i = start_byte; i <= end_byte; i++) {
		if ((m_ptr[i] & bs.m_ptr[i]) != (BYTE)0) {
			return true;
		}
	}
	return false;
}


//Return true if 'this' contained in range between 'low' and 'high'.
//'strict': 'this' strictly contained in range.
bool BITSET::is_contained_in_range(UINT low, UINT high, bool strict) const
{
	IS_TRUE(low <= high, ("Invalid bit set"));
	INT const set_low = get_first();
	if (set_low == -1) {
		return false;
	}
	INT const set_high = get_last();
	//In case:
	// low						   high
	// |---------------------------|     given range
	//     set_low        set_high
	//        |--------------|           given bitset
	if (strict) {
		if ((UINT)set_low > low && (UINT)set_high < high) {
			return true;
		}
	} else {
		if ((UINT)set_low >= low && (UINT)set_high <= high) {
			return true;
		}
	}
	return false;
}


//Return true if 'this' contained range between 'low' and 'high'.
bool BITSET::is_contain_range(UINT low, UINT high, bool strict) const
{
	IS_TRUE(low <= high, ("Invalid bit set"));
	INT const set_low = get_first();
	if (set_low == -1) {
		return false;
	}
	INT const set_high = get_last();
	//In case:
	//         low		      high
	//          |--------------|			//given range
	// set_low                     set_high
	//    |---------------------------|		//given set
	if (strict) {
		if ((UINT)set_low < low && (UINT)set_high > high) {
			return true;
		}
	} else {
		if ((UINT)set_low <= low && (UINT)set_high >= high) {
			return true;
		}
	}
	return false;
}


/* Return true if range between first_bit of 'this' and
last_bit of 'this' overlapped with the range between
'low' and 'high'. */
bool BITSET::is_overlapped(UINT low, UINT high) const
{
	IS_TRUE(low <= high, ("Invalid bit set"));
	INT const set_low = get_first();
	if (set_low == -1) {
		return false;
	}
	INT const set_high = get_last();

	//In case:
	// low						   high
	// |---------------------------|     given range
	//     set_low        set_high
	//        |--------------|           given bitset
	if ((UINT)set_low >= low && (UINT)set_high <= high) {
		return true;
	}

	//In case:
	//         low		      high
	//          |--------------|
	// set_low                     set_high
	//    |---------------------------|
	if ((UINT)set_low <= low && (UINT)set_high >= high) {
		return true;
	}

	//In case:
	//         low		      high
	//          |--------------|
	// set_low        set_high
	//    |--------------|
	if ((UINT)set_high >= low && (UINT)set_high <= high) {
		return true;
	}

	//In case:
	//       low		    high
	//        |--------------|
	//            set_low          set_high
	//               |----------------|
	if ((UINT)set_low >= low && (UINT)set_low <= high) {
		return true;
	}
	return false;
}


//Return true if in the range between 'low' and 'high' has
//any elements.
bool BITSET::has_elem_in_range(UINT low, UINT high) const
{
	IS_TRUE(low <= high, ("out of boundary"));
	INT const first_bit = get_first();
	if (first_bit == -1 || (UINT)first_bit > high) {
		return false;
	}
	if ((UINT)first_bit >= low && (UINT)first_bit <= high) {
		return true;
	}
	INT const last_bit = get_last();
	if ((UINT)last_bit < low) {
		return false;
	}
	INT const start_bit = low;
	INT const end_bit = MIN((UINT)last_bit, high);
	if (is_contain(start_bit)) {
		return true;
	}
	if (get_next(start_bit) <= end_bit) {
		return true;
	}
	return false;
}


//Return position of first element, start from '0'.
//Return -1 if the bitset is empty.
INT BITSET::get_first() const
{
	if (m_size == 0) return -1;
	UINT i = 0;
	UINT m = m_size / BYTES_PER_UINT * BYTES_PER_UINT;
	for (UINT const* uint_ptr = ((UINT const*)(m_ptr));
		 i < m; uint_ptr++, i += 4) {
		if (*uint_ptr == 0) { continue; }
		for (BYTE const* bptr = ((BYTE*)(uint_ptr));
			 i < m; bptr++, i++) {
			BYTE byte = *bptr;
			if (byte != (BYTE)0) {
				return g_first_one[byte] + (MULBPB(i));
			}
		}
		IS_TRUE(0, ("not arrival"));
	}
	IS_TRUE0(i <= m_size);
	for (; i < m_size; i++) {
		BYTE byte = m_ptr[i];
		if (byte != (BYTE)0) {
			return g_first_one[byte] + (MULBPB(i));
		}
	}
	return -1;
}


//Get bit postition of the last element.
INT BITSET::get_last() const
{
	if (m_size == 0) return -1;
	INT i = m_size - 1;
	UINT m = m_size % BYTES_PER_UINT;
	if (m == 0) {
		m = BYTES_PER_UINT;
	}
	for (; i >= (INT)(m_size - m); i--) {
		BYTE byte = m_ptr[i];
		if (byte != (BYTE)0) {
			return g_last_one[byte] + (MULBPB(i));
		}
	}
	if (i < 0) { return -1; }
	UINT const* uint_ptr;
	for (uint_ptr = ((UINT const*)(m_ptr)) + i / BYTES_PER_UINT;
		 i >= 0; uint_ptr--, i -= 4) {
		if (*uint_ptr == 0) { continue; }
		for (BYTE const* bptr = ((BYTE*)(uint_ptr)) + BYTES_PER_UINT - 1;
			 i >= 0; bptr--, i--) {
			BYTE byte = *bptr;
			if (byte != (BYTE)0) {
				return g_last_one[byte] + (MULBPB(i));
			}
		}
	}
	IS_TRUE0(m_ptr == ((BYTE*)uint_ptr) + sizeof(UINT));
	return -1;
}


//Extract subset in range between 'low' and 'high'.
BITSET * BITSET::get_subset_in_range(IN UINT low, IN UINT high,
									 OUT BITSET & subset)
{
	IS_TRUE(low <= high, ("Invalid bit set"));
	IS_TRUE(&subset != this, ("overlapped!"));

	subset.clean();
	INT first_bit = get_first();
	if (first_bit == -1) {
		return &subset;
	}
	INT last_bit = get_last();
	if ((low > (UINT)last_bit) || (high < (UINT)first_bit)) {
		return &subset;
	}

	UINT const sb_last_byte = DIVBPB(high); //last byte of subset
	UINT const sb_first_byte = DIVBPB(low); //first byte of subset
	UINT const last_byte = DIVBPB(last_bit); //last byte of 'this'
	UINT const first_byte = DIVBPB(first_bit); //first byte of 'this'
	if ((sb_last_byte + 1) > subset.m_size) {
		subset.alloc(sb_last_byte + 1);
	}

	//START and END byte-pos of duplication and which apart from
	//the first and last byte of the extracted range.
	INT start, end;
	if (first_byte > sb_first_byte) {
		/*
		'this':             first_byte, ...
		subset: first_byte, ...
		*/
		start = first_byte;
		if (last_byte < sb_last_byte) {
			/*
			'this': last_byte,
			subset:       ,...,last_byte
			*/
			end = last_byte;
			::memcpy(subset.m_ptr + start,
					m_ptr + start, end - start + 1);
		} else {
			/*
			'this':         ,...,last_byte
			subset: last_byte,
			or:
			'this': last_byte,
			subset: last_byte,
			 */
			end = sb_last_byte - 1;
			if (end >= start) {
				//Copy the content of 'this', except for
				//the first and last byte.
				::memcpy(subset.m_ptr + start,
						m_ptr + start, end - start + 1);
			}
			BYTE byte = m_ptr[sb_last_byte];
			UINT ofst = MODBPB(high);
			ofst = BITS_PER_BYTE - ofst - 1;
			byte <<= ofst;
			byte >>= ofst;
			subset.m_ptr[sb_last_byte] = byte;
		}
	} else {
		/*
		'this': first_byte, ...,
		subset:             first_byte, ...
		or:
		'this': first_byte, ...
		subset: first_byte, ...
		*/
		BYTE byte = m_ptr[sb_first_byte];
		UINT ofst = MODBPB(low);
		byte >>= ofst;
		byte <<= ofst;
		subset.m_ptr[sb_first_byte] = byte;

		start = sb_first_byte + 1;
		if (last_byte < sb_last_byte) {
			/*
			'this': last_byte,
			subset:          ,...,last_byte
			 */
			end = last_byte;
			::memcpy(subset.m_ptr + start, m_ptr + start, end - start + 1);
		} else {
			/*
			'this':          ,...,last_byte
			subset: last_byte,
			or:
			'this': last_byte,
			subset: last_byte,
			 */
			end = sb_last_byte - 1;
			BYTE byte = 0;
			if (end >= start) {
				//Copy the content of 'this', except
				//for the first and last byte.
				::memcpy(subset.m_ptr + start,
						m_ptr + start, end - start + 1);
				byte = m_ptr[sb_last_byte];
			} else {
				/*
				There are two cases:
				CASE1: Both subset's first and last byte are the same one.
					'this': first/last_byte,
					subset: first/last_byte,
				CASE2: first and last byte are sibling.
					'this': first_byte, last_byte,
					subset: first_byte, last_byte,
				*/
				if (sb_first_byte == sb_last_byte) {
					byte = subset.m_ptr[sb_last_byte];
				} else if (sb_first_byte + 1 == sb_last_byte) {
					byte = m_ptr[sb_last_byte];
				} else {
					IS_TRUE0(0);
				}
			}
			UINT ofst = MODBPB(high);
			ofst = BITS_PER_BYTE - ofst - 1;
			byte <<= ofst;
			byte >>= ofst;
			subset.m_ptr[sb_last_byte] = byte;
		}
	} //end else
	return &subset;
}


//Return -1 if it has no other element.
//'elem': return next one to current element.
INT BITSET::get_next(UINT elem) const
{
	if (m_size == 0) return -1;
	INT first_byte = DIVBPB(elem);
	if (((UINT)first_byte + 1) > m_size) {
		return -1;
	}
	BYTE byte = m_ptr[first_byte];
	elem = MODBPB(elem) + 1; //index in 'first_byte'.
	byte >>= elem; //Erase low 'elem' bits.
	byte <<= elem;
	if (byte == 0) {
		//No elements in this byte.
		UINT i = first_byte + 1;
		while (i < m_size) {
			byte = m_ptr[i];
			if (byte != 0) {
				first_byte = i;
				break;
			}
			i++;
		}
		if (byte == 0) {
			return -1;
		}
	}
	return g_first_one[byte] + (MULBPB(first_byte));
}


void BITSET::clean()
{
	if (m_ptr == NULL) return;
	::memset(m_ptr, 0, m_size);
}


//Do copy from 'src' to 'des'.
void BITSET::copy(BITSET const& src)
{
	IS_TRUE(this != &src, ("copy self"));
	if (src.m_size == 0) {
		::memset(m_ptr, 0, m_size);
		return;
	}

	UINT cp_sz = src.m_size; //size need to copy.
	if (m_size < src.m_size) {
		//src's last byte pos.
		INT l = src.get_last();
		if (l < 0) {
			::memset(m_ptr, 0, m_size);
			return;
		}

		cp_sz = l / BITS_PER_BYTE + 1;
		if (m_size < cp_sz) {
			::free(m_ptr);
			m_ptr = (BYTE*)::malloc(cp_sz);
			m_size = cp_sz;
		} else if (m_size > cp_sz) {
			::memset(m_ptr + cp_sz, 0, m_size - cp_sz);
		}
	} else if (m_size > src.m_size) {
		cp_sz = src.m_size;
		::memset(m_ptr + cp_sz, 0, m_size - cp_sz);
	}

	IS_TRUE(m_ptr != NULL, ("not yet init"));
	::memcpy(m_ptr, src.m_ptr, cp_sz);
}


//Support concatenation assignment such as: a=b=c
BITSET const& BITSET::operator = (BITSET const& src)
{
	copy(src);
	return *this;
}


void BITSET::dump(CHAR const* name, bool is_del, UINT flag, INT last_pos) const
{
	if (name == NULL) {
		name = "zbs.cxx";
	}
	if (is_del) {
		unlink(name);
	}
	FILE * h = fopen(name, "a+");
	IS_TRUE(h != NULL, ("%s create failed!!!", name));
	dump(h, flag, last_pos);
	fclose(h);
}


void BITSET::dump(FILE * h, UINT flag, INT last_pos) const
{
	if (h == NULL) { return; }
	IS_TRUE0(last_pos < 0 || (last_pos / BITS_PER_BYTE) < (INT)m_size);

	INT elem = get_last();
	if (last_pos != -1) {
		elem = last_pos;
	}
	if (elem < 0) {
		if (HAVE_FLAG(flag, BS_DUMP_BITSET)) {
			fprintf(h, "\nbitset:[]");
		}
		if (HAVE_FLAG(flag, BS_DUMP_POS)) {
			fprintf(h, "\npos(start at 0):");
		}
	} else {
		INT i = 0;
		/*
		fprintf(h, "\nbitset(hex):\n\t");
		fprintf(h, "[0x");
		for (BYTE byte = m_ptr[i]; i <= DIVBPB(elem); byte = m_ptr[++i]) {
			fprintf(h, "%x", byte);
		}
		fprintf(h, "]");
		*/

		//Print as binary
		if (HAVE_FLAG(flag, BS_DUMP_BITSET)) {
			fprintf(h, "\nbitset:[");
			i = 0;
			do {
				if (is_contain(i)) {
					fprintf(h, "1");
				} else {
					fprintf(h, "0");
				}
				i++;
			} while (i <= elem);
			fprintf(h, "]");
		}

		//Print as position
		if (HAVE_FLAG(flag, BS_DUMP_POS)) {
			fprintf(h, "\npos(start at 0):");
			elem = get_first();
			do {
				fprintf(h, "%d ", elem);
				elem = get_next(elem);
			} while (elem >= 0);
		}
	}
	fflush(h);
}
//END BITSET


//
//START BITSET_MGR
//
//'h': dump mem usage detail to file.
UINT BITSET_MGR::count_mem(FILE * h)
{
	UINT count = 0;
	C<BITSET*> * ct;
	for (BITSET const* bs = m_bs_list.get_head(&ct);
		 bs != NULL; bs = m_bs_list.get_next(&ct)) {
		count += bs->count_mem();
	}
	#ifdef _DEBUG_
    if (h != NULL) {
        //Dump mem usage into file.
		LIST<UINT> lst;
		for (BITSET const* bs = m_bs_list.get_head();
			 bs != NULL; bs = m_bs_list.get_next()) {
			UINT c = bs->count_mem();
			C<UINT> * ct;
			UINT n = lst.get_elem_count();
			lst.get_head(&ct);
			UINT i;
			for (i = 0; i < n; i++, lst.get_next(&ct)) {
				if (c >= C_val(ct)) {
					lst.insert_before(c, ct);
					break;
				}
			}
			if (i == n) {
				lst.append_head(c);
			}
		}
		UINT v = lst.get_head();
		fprintf(h, "\n== DUMP BITSET_MGR: total %d "
				   "bitsets, mem usage are:\n",
				   m_bs_list.get_elem_count());
		UINT b = 0;
		UINT n = lst.get_elem_count();
		for (UINT i = 0; i < n; i++, v = lst.get_next(), b++) {
			if (b == 20) {
				fprintf(h, "\n");
				b = 0;
			}
			if (v < 1024) {
				fprintf(h, "%dB,", v);
			} else if (v < 1024 * 1024) {
				fprintf(h, "%dKB,", v/1024);
			} else {
				fprintf(h, "%dMB,", v/1024/1024);
			}
		}
		fflush(h);
    }
	#endif
	return count;
}
//END BITSET_MGR



//
//Binary Operations
//
//Returns a new set which is the union of set1 and set2,
//and modify 'res' as result.
BITSET * bs_union(IN BITSET const& set1, IN BITSET const& set2,
				  OUT BITSET & res)
{
	IS_TRUE(set1.m_ptr != NULL && set2.m_ptr != NULL && res.m_ptr != NULL,
			("not yet init"));
	if (&res == &set1) {
		res.bunion(set2);
	} else if (&res == &set2) {
		res.bunion(set1);
	} else if (set1.m_size < set2.m_size) {
		res.copy(set2);
		res.bunion(set1);
	} else {
		res.copy(set1);
		res.bunion(set2);
	}
	return &res;
}


//Subtracting set2 from set1
//Returns a new set which is { x : member( x, 'set1' ) & ~ member( x, 'set2' ) }.
BITSET * bs_diff(IN BITSET const& set1, IN BITSET const& set2, OUT BITSET & res)
{
	IS_TRUE(set1.m_ptr != NULL &&
			set2.m_ptr != NULL &&
			res.m_ptr != NULL, ("not yet init"));
	if (&res == &set1) {
		res.diff(set2);
	} else if (&res == &set2) {
		BITSET tmp(set1);
		tmp.diff(set2);
		res.copy(tmp);
	} else {
		res.copy(set1);
		res.diff(set2);
	}
	return &res;
}


//Returns a new set which is intersection of 'set1' and 'set2'.
BITSET * bs_intersect(IN BITSET const& set1,
					  IN BITSET const& set2,
					  OUT BITSET & res)
{
	IS_TRUE(set1.m_ptr != NULL &&
			set2.m_ptr != NULL &&
			res.m_ptr != NULL, ("not yet init"));
	if (&res == &set1) {
		res.intersect(set2);
	} else if (&res == &set2) {
		res.intersect(set1);
	} else {
		res.copy(set1);
		res.intersect(set2);
	}
	return &res;
}


//
//START SDBITSET_MGR
//
//'h': dump mem usage detail to file.
UINT SDBITSET_MGR::count_mem(FILE * h) const
{
	UINT count = 0;
	SC<SBITSET*> * st;
	for (SBITSET const* bs = m_sbitset_list.get_head(&st);
		 bs != NULL; bs = m_sbitset_list.get_next(&st)) {
		count += bs->count_mem();
	}

	SC<DBITSET*> * dt;
	for (DBITSET const* d = m_dbitset_list.get_head(&dt);
		 d != NULL; d = m_dbitset_list.get_next(&dt)) {
		count += d->count_mem();
	}

	//DBITSETC and SBITSETC are allocated in the pool.
	count += smpool_get_pool_size_handle(m_sbitsetc_pool);
	count += smpool_get_pool_size_handle(m_dbitsetc_pool);
	count += smpool_get_pool_size_handle(ptr_pool);
	count += sm.count_mem();

	#ifdef _DEBUG_
	if (h != NULL) {
		//Dump mem usage into file.
		LIST<UINT> lst;
		SC<SBITSET*> * st;
		for (SBITSET const* bs = m_sbitset_list.get_head(&st);
			 bs != NULL; bs = m_sbitset_list.get_next(&st)) {
			UINT c = bs->count_mem();
			C<UINT> * ct;
			UINT n = lst.get_elem_count();
			lst.get_head(&ct);
			UINT i;
			for (i = 0; i < n; i++, lst.get_next(&ct)) {
				if (c >= C_val(ct)) {
					lst.insert_before(c, ct);
					break;
				}
			}
			if (i == n) {
				lst.append_head(c);
			}
		}
		UINT v = lst.get_head();
		fprintf(h, "\n== DUMP BITSET_MGR: total %d "
					"bitsets, mem usage are:\n",
					m_sbitset_list.get_elem_count());
		UINT b = 0;
		UINT n = lst.get_elem_count();
		for (UINT i = 0; i < n; i++, v = lst.get_next(), b++) {
			if (b == 20) {
				fprintf(h, "\n");
				b = 0;
			}
			if (v < 1024) {
				fprintf(h, "%dB,", v);
			} else if (v < 1024 * 1024) {
				fprintf(h, "%dKB,", v/1024);
			} else {
				fprintf(h, "%dMB,", v/1024/1024);
			}
		}
		fflush(h);
	}
	#endif
	return count;
}
//END SDBITSET_MGR


//
//START SBITSETC
//
//'free_list': free list for SC<SEG*>
//'pool': be used to alloc SC<SEG*>
void SBITSETC::bunion(SBITSETC const& src, SEG_MGR * sm,
					  SC<SEG*> ** free_list, SMEM_POOL * pool)
{
	IS_TRUE(this != &src, ("operate on same set"));
	SC<SEG*> * srcst;
	SC<SEG*> * tgtst, * prev_st = NULL;
	SEG * t = segs.get_head(&tgtst);
	for (SEG * s = src.segs.get_head(&srcst);
		 s != NULL; s = src.segs.get_next(&srcst)) {
		UINT src_start = s->start;

		bool handled = false;
		for (; t != NULL; prev_st = tgtst, t = segs.get_next(&tgtst)) {
			UINT tgt_start = t->start;
			if (src_start < tgt_start) {
				/*
				s: |----|
				t:       |----|
				*/
				SEG * x = sm->new_seg();
				x->copy(*s);
				if (prev_st == NULL) {
					prev_st = segs.append_head(x, free_list, pool);
				} else {
					prev_st = segs.insert_after(x, prev_st, free_list, pool);
				}
				handled = true; //current tgt segment has been handled.
				break;
			} else if (src_start == tgt_start) {
				t->bs.bunion(s->bs);
				prev_st = tgtst;
				t = segs.get_next(&tgtst);
				handled = true; //current tgt segment has been handled.
				break;
			}
		}

		if (tgtst == NULL && !handled) {
			//Append rest of src segments to tail.
			SEG * x = sm->new_seg();
			x->copy(*s);
			segs.append_tail(x, free_list, pool);
		}
	}
}


//'free_list': free list for SC<SEG*>
//'pool': be used to alloc SC<SEG*>
void SBITSETC::bunion(UINT elem, SEG_MGR * sm,
					  SC<SEG*> ** free_list, SMEM_POOL * pool)
{
	SC<SEG*> * sct, * next_sct, * prev_sct = NULL;
	for (segs.get_head(&sct), next_sct = sct;
		 sct != NULL; prev_sct = sct, sct = next_sct) {
		SEG * s = SC_val(sct);
		segs.get_next(&next_sct);
		UINT start = s->get_start();
		if (elem < start) { break; }
		UINT last = s->get_end();
		if (elem <= last) {
			s->bs.bunion(elem - start);
			return;
		}
	}

	SEG * x = sm->new_seg();
	if (sct != NULL) {
		if (prev_sct == NULL) {
			segs.append_head(x, free_list, pool);
		} else {
			segs.insert_after(x, prev_sct, free_list, pool);
		}
	} else {
		segs.append_tail(x, free_list, pool);
	}
	x->start = elem / BITS_PER_SEG * BITS_PER_SEG;
	x->bs.bunion(elem - x->start);
}


void SBITSETC::copy(SBITSETC const& src, SEG_MGR * sm,
					SC<SEG*> ** free_list, SMEM_POOL * pool)
{
	IS_TRUE(this != &src, ("operate on same set"));
	clean(sm, free_list);
	SC<SEG*> * st;
	for (SEG * s = src.segs.get_head(&st);
		 s != NULL; s = src.segs.get_next(&st)) {
		SEG * t = sm->new_seg();
		t->copy(*s);
		segs.append_tail(t, free_list, pool);
	}
}


void SBITSETC::clean(SEG_MGR * sm, SC<SEG*> ** free_list)
{
	SC<SEG*> * st;
	for (SEG * s = segs.get_head(&st); s != NULL; s = segs.get_next(&st)) {
		sm->free(s);
	}

	//Free the list of container of SEG* back to SEG_MGR.
	segs.clean(free_list);
}


void SBITSETC::destroy_seg_and_clean(SEG_MGR * sm,
												SC<SEG*> ** free_list)
{
	SC<SEG*> * iter;
	for (SEG * s = segs.get_head(&iter);
		 s != NULL; s = segs.get_next(&iter)) {
		//We are not going to give it back to SEG_MGR.
		#ifdef _DEBUG_
		sm->dec_seg_count();
		#endif
		delete s;
	}

	//Because SC<SEG*> is always allocated from pool, recycle
	//it to free_list, at last give it back to the pool.
	segs.clean(free_list);
}


UINT SBITSETC::count_mem() const
{
	SC<SEG*> * st;
	UINT c = 0;
	for (SEG * s = segs.get_head(&st); s != NULL; s = segs.get_next(&st)) {
		c += s->count_mem();
	}
	c += segs.count_mem();
	return c;
}


void SBITSETC::diff(UINT elem, SEG_MGR * sm, SC<SEG*> ** free_list)
{
	SC<SEG*> * sct, * next_sct, * prev_sct = NULL;
	for (segs.get_head(&sct), next_sct = sct;
		 sct != NULL; prev_sct = sct, sct = next_sct) {
		SEG * s = SC_val(sct);
		segs.get_next(&next_sct);
		UINT start = s->get_start();
		if (elem < start) { break; }
		UINT last = s->get_end();
		if (elem <= last) {
			s->bs.diff(elem - start);
			if (s->bs.is_empty()) {
				segs.remove(prev_sct, sct, free_list);
				sm->free(s);
			}
			return;
		}
	}
}


//Difference between current bitset and 'src', current bitset
//will be modified.
void SBITSETC::diff(SBITSETC const& src, SEG_MGR * sm, SC<SEG*> ** free_list)
{
	IS_TRUE(this != &src, ("operate on same set"));
	SC<SEG*> * tgtst, * prev_st = NULL, * next_st;
	segs.get_head(&tgtst);
	next_st = tgtst;
	SC<SEG*> * next_srcst = NULL;
	SC<SEG*> * srcst = NULL;
	for (src.segs.get_head(&next_srcst), srcst = next_srcst;
		 srcst != NULL; srcst = next_srcst) {
		src.segs.get_next(&next_srcst);
		SEG * s = SC_val(srcst);
		UINT src_start = s->start;
		for (; tgtst != NULL;) {
			SEG * t = SC_val(tgtst);
			UINT tgt_start = t->start;
			if (src_start < tgt_start) {
				break;
			}
			if (src_start > tgt_start) {
				segs.get_next(&next_st);
				prev_st = tgtst;
				tgtst = next_st;
				continue;
			}
			segs.get_next(&next_st);
			t->bs.diff(s->bs);
			if (t->bs.is_empty()) {
				segs.remove(prev_st, tgtst, free_list);
				//prev_st keep unchanged.
				tgtst = next_st;
				sm->free(t);
			} else {
				prev_st = tgtst;
				tgtst = next_st;
			}
		}
	}
}


void SBITSETC::dump2(FILE * h) const
{
	IS_TRUE0(h);
	fprintf(h, "\n");
	if (segs.get_elem_count() == 0) {
		fprintf(h, "empty");
		fflush(h);
		return;
	}
	dump(h);
}


void SBITSETC::dump(FILE * h) const
{
	IS_TRUE0(h);
	SC<SEG*> * sct;
	for (SEG * s = segs.get_head(&sct);
		 s != NULL; s = segs.get_next(&sct)) {
		fprintf(h, " [");
		INT n;
		for (INT i = s->bs.get_first(); i >= 0; i = n) {
			n = s->bs.get_next(i);
			fprintf(h, "%d", i + s->get_start());
			if (n >= 0) {
				fprintf(h, ",");
			}
		}
		fprintf(h, "]");
	}
	fflush(h);
}


UINT SBITSETC::get_elem_count() const
{
	UINT c = 0;
	SC<SEG*> * st;
	for (SEG * s = segs.get_head(&st); s != NULL; s = segs.get_next(&st)) {
		c += s->bs.get_elem_count();
	}
	return c;
}


INT SBITSETC::get_first(SC<SEG*> ** cur) const
{
	IS_TRUE0(cur);
	SEG * s = segs.get_head(cur);
	if (s == NULL) {
		IS_TRUE0(segs.get_elem_count() == 0);
		return -1;
	}
	IS_TRUE0(!s->bs.is_empty());
	return s->get_start() + s->bs.get_first();
}


INT SBITSETC::get_last(SC<SEG*> ** cur) const
{
	IS_TRUE0(cur);
	SEG * s = segs.get_tail(cur);
	if (s == NULL) {
		IS_TRUE0(segs.get_elem_count() == 0);
		return -1;
	}
	IS_TRUE0(!s->bs.is_empty());
	return s->get_start() + s->bs.get_last();
}


INT SBITSETC::get_next(UINT elem, SC<SEG*> ** cur) const
{
	if (cur == NULL) {
		SC<SEG*> * st;
		for (SEG * s = segs.get_head(&st);
			 s != NULL; s = segs.get_next(&st)) {
			UINT start = s->get_start();
			if (elem < start) { continue; }
			UINT last = s->get_end();
			if (elem <= last) {
				INT n = s->bs.get_next(elem - start);
				if (n >= 0) { return start + (UINT)n; }

				segs.get_next(&st);
				if (st == NULL) {
					return -1;
				}
				start = SC_val(st)->get_start();
				n = SC_val(st)->bs.get_first();
				IS_TRUE0(n >= 0);
				return start + (UINT)n;
			}
		}
		return -1;
	}
	SC<SEG*> * cp = *cur;
	if (cp == NULL) { return -1; }
	UINT start = SC_val(cp)->get_start();
	INT n = SC_val(cp)->bs.get_next(elem - start);
	if (n >= 0) {
		return start + (UINT)n;
	}
	segs.get_next(&cp);
	if (cp == NULL) {
		*cur = NULL;
		return -1;
	}
	*cur = cp;
	start = SC_val(cp)->get_start();
	n = SC_val(cp)->bs.get_first();
	IS_TRUE0(n >= 0);
	return start + (UINT)n;
}


bool SBITSETC::is_equal(SBITSETC const& src) const
{
	IS_TRUE(this != &src, ("operate on same set"));
	SC<SEG*> * srcst;
	SC<SEG*> * tgtst;
	src.segs.get_head(&srcst);
	segs.get_head(&tgtst);
	while (srcst != NULL || tgtst != NULL)  {
		if ((srcst == NULL) ^ (tgtst == NULL)) {
			return false;
		}
		IS_TRUE0(srcst);
		if (SC_val(srcst)->start != SC_val(tgtst)->start) {
			return false;
		}
		if (!SC_val(srcst)->bs.is_equal(SC_val(tgtst)->bs)) {
			return false;
		}
		src.segs.get_next(&srcst);
		segs.get_next(&tgtst);
	}
	return true;
}


bool SBITSETC::is_intersect(SBITSETC const& src) const
{
	IS_TRUE(this != &src, ("operate on same set"));
	SC<SEG*> * srcst;
	SC<SEG*> * tgtst;
	src.segs.get_head(&srcst);
	segs.get_head(&tgtst);
	while (srcst != NULL && tgtst != NULL) {
		if (SC_val(srcst)->start < SC_val(tgtst)->start) {
			src.segs.get_next(&srcst);
			continue;
		} else if (SC_val(srcst)->start == SC_val(tgtst)->start) {
			if (SC_val(srcst)->bs.is_intersect(SC_val(tgtst)->bs)) {
				return true;
			}
			src.segs.get_next(&srcst);
			segs.get_next(&tgtst);
			continue;
		} else {
			//srcst's start > tgtst's start
			segs.get_next(&tgtst);
			continue;
		}
	}
	return false;
}


bool SBITSETC::is_contain(UINT elem) const
{
	SC<SEG*> * st;
	segs.get_head(&st);
	while (st != NULL) {
		SEG * seg = SC_val(st);
        UINT start = seg->get_start();
        if (elem < start) { return false; }
        if (elem >= start && elem <= seg->get_end()) {
            return seg->bs.is_contain(elem - start);
        }
		segs.get_next(&st);
	}
	return false;
}


bool SBITSETC::is_empty() const
{
	SC<SEG*> * st;
	segs.get_head(&st);
	while (st != NULL) {
		if (!SC_val(st)->bs.is_empty()) {
			return false;
		}
		segs.get_next(&st);
	}
	return true;
}


//Do intersection for current bitset and 'src', current bitset
//will be modified.
void SBITSETC::intersect(SBITSETC const& src, SEG_MGR * sm,
						 SC<SEG*> ** free_list)
{
	IS_TRUE(this != &src, ("operate on same set"));
	SC<SEG*> * tgtst, * prev_st = NULL, * next_st;
	segs.get_head(&tgtst);
	next_st = tgtst;
	SC<SEG*> * next_srcst = NULL;
	SC<SEG*> * srcst = NULL;
	for (src.segs.get_head(&next_srcst), srcst = next_srcst;
		 srcst != NULL; srcst = next_srcst) {
		src.segs.get_next(&next_srcst);
		SEG * s = SC_val(srcst);
		UINT src_start = s->start;
		for (; tgtst != NULL;) {
			SEG * t = SC_val(tgtst);
			UINT tgt_start = t->start;
			if (src_start < tgt_start) {
				if (next_srcst == NULL) {
					//the last segment of src.
					segs.get_next(&next_st);
					segs.remove(prev_st, tgtst, free_list);
					//prev_st keep unchanged.
					tgtst = next_st;
					sm->free(t);
					continue;
				}
				break;
			}
			if (src_start > tgt_start) {
				segs.get_next(&next_st);
				segs.remove(prev_st, tgtst, free_list);
				//prev_st keep unchanged.
				tgtst = next_st;
				sm->free(t);
				continue;
			}
			segs.get_next(&next_st);
			t->bs.intersect(s->bs);
			if (t->bs.is_empty()) {
				segs.remove(prev_st, tgtst, free_list);
				//prev_st keep unchanged.
				tgtst = next_st;
				sm->free(t);
			} else {
				prev_st = tgtst;
				tgtst = next_st;
			}
		}
	}
	if (next_srcst == NULL) {
		//tgt list is longer than src. So tgt list still have
		//element to cope with.
		for (; tgtst != NULL; tgtst = next_st) {
			SEG * t = SC_val(tgtst);
			segs.get_next(&next_st);
			segs.remove(prev_st, tgtst, free_list);
			//prev_st keep unchanged.
			tgtst = next_st;
			sm->free(t);
		}
	}
}
//END SBITSETC


//
//START DBITSETC
//
INT DBITSETC::get_first(SC<SEG*> ** cur) const
{
	IS_TRUE0(cur);
	SEG * s = segs.get_head(cur);
	if (s == NULL) {
		IS_TRUE0(segs.get_elem_count() == 0);
		return -1;
	}
	//DBITSETC allow bs is empty if it is not sparse.
	//IS_TRUE0(!s->bs.is_empty());
	return s->get_start() + s->bs.get_first();
}


INT DBITSETC::get_last(SC<SEG*> ** cur) const
{
	IS_TRUE0(cur);
	SEG * s = segs.get_tail(cur);
	if (s == NULL) {
		IS_TRUE0(segs.get_elem_count() == 0);
		return -1;
	}
	//DBITSETC allow bs is empty if it is not sparse.
	//IS_TRUE0(!s->bs.is_empty());
	return s->get_start() + s->bs.get_last();
}
//END DBITSETC


#ifdef _DEBUG_
void bs_test()
{
	extern FILE * g_tfile;
	SEG_MGR sm;

	DBITSET a(&sm);
	DBITSET b(&sm);
	a.set_sparse(false);
	b.set_sparse(false);
	a.bunion(5);
	a.bunion(511);
	a.bunion(515);
	b.bunion(21);
	a.dump(g_tfile);
	int u = a.count_mem();
	b.dump(g_tfile);
	a.bunion(b);
	a.dump(g_tfile);


	DBITSET x(&sm, 20);
	x.set_sparse(false);
	x.bunion(1999);
	//x.bunion(1);
	x.bunion(2000);
	//x.bunion(2007);
	//x.bunion(2016);
	x.bunion(2014);
	//x.bunion(2025);
	//x.bunion(2033);
	x.bunion(2048);
	//x.bunion(1997);
	//x.bunion(1991);
	x.bunion(1983);
	x.bunion(2112);
	x.bunion(2113);
	x.diff(2048);
	x.bunion(2048);
	x.bunion(13555);
	x.dump(g_tfile);
	int n = x.get_elem_count();
	SC<SEG*> * ct;
	n = x.get_first(&ct);
	n = x.get_next(n, &ct);
	n = x.get_next(n, &ct);
	n = x.get_next(n, &ct);
	n = x.get_next(n, &ct);
	n = x.get_next(n, &ct);
	n = x.get_next(n, &ct);
	n = x.get_last(&ct);

	DBITSET y(&sm, 20);
	y.set_sparse(false);
	y.bunion(23);
	y.bunion(1990);
	y.bunion(2113);
	y.bunion(12345);
	y.bunion(10000);
	y.bunion(14330);
	y.bunion(15330);
	y.bunion(16330);

	fprintf(g_tfile, "\n");
	x.dump(g_tfile);
	fprintf(g_tfile, "\n");
	y.dump(g_tfile);
	fprintf(g_tfile, "\n");

	y.bunion(x);
	fprintf(g_tfile, "\n=====\n");
	y.dump(g_tfile);

	//y.intersect(x);
	x.bunion(0);
	x.bunion(100000000);
	fprintf(g_tfile, "\ny:"); y.dump(g_tfile);
	fprintf(g_tfile, "\nx:"); x.dump(g_tfile);
	x.diff(y);
	fprintf(g_tfile, "\n=====\n");
	x.dump(g_tfile);
}


void bs_test2()
{
	extern FILE * g_tfile;
	SEG_MGR sm;
	SBITSET a(&sm),b(&sm);
	a.bunion(8);
	a.bunion(512);
	a.dump2(g_tfile);
	a.intersect(b);
	a.dump2(g_tfile);


	SBITSET c(&sm),d(&sm);
	c.bunion(8);
	c.bunion(512);
	c.dump2(g_tfile);
	d.diff(c);
	d.dump2(g_tfile);


	BITSET e,f;
	e.bunion(64);
	int i = e.get_first();


	SBITSET g(&sm),h(&sm);
	g.bunion(1);
	g.bunion(100);
	g.bunion(600);
	g.bunion(1500);
	g.dump2(g_tfile);
	h.bunion(1501);
	h.bunion(2500);
	h.bunion(3500);
	g.is_intersect(h);
	return;
}


#ifdef DEBUG_SEG
extern FILE * g_tfile;
void dump_seg(SEG_MGR & m)
{
	if (g_tfile == NULL) { return; }
	SC<SEG*> * sc;
	fprintf(g_tfile, "\n====start %d:%d===\n",
			m.get_free_list()->get_elem_count(),
			m.get_seg_count());
	BITSET x;
	SLIST<SEG*> const* flst = m.get_free_list();
	for (SEG const* s = flst->get_head(&sc); s != NULL; s = flst->get_next(&sc)) {
		fprintf(g_tfile, "%d,", s->id);
		x.bunion(s->id);
	}
	fflush(g_tfile);
	x.dump(g_tfile);
}
#endif
#endif
