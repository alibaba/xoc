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
#ifndef _COMF_H_
#define _COMF_H_

namespace xcom {


/* Singler timer, show const string before timer start.
e.g:
	START_TIMER("My Pass");
	Run mypass();
	END_TIMER(); */
#define START_TIMER(s)  			\
	LONG _start_time_count_ = 0;	\
	if (g_show_comp_time) {			\
		_start_time_count_ =		\
			getclockstart();		\
		printf("\n==-- %s Time:", (s));	\
	}
#define END_TIMER()					\
	if (g_show_comp_time) {			\
		printf("%fsec", getclockend(_start_time_count_)); \
	}


/* Single timer, show const string after timer finish.
e.g:
	START_TIMER_AFTER();
	Run mypass();
	END_TIMER_AFTER("My Pass"); */
#define START_TIMER_AFTER() 		\
	LONG _start_time_count_ = 0;	\
	if (g_show_comp_time) {			\
		_start_time_count_ =		\
				getclockstart();	\
	}
#define END_TIMER_AFTER(s)			\
	if (g_show_comp_time) {			\
		printf("\n==-- %s Time:%fsec", \
			   (s), getclockend(_start_time_count_)); \
	}


/* Single timer, show format string after timer finish.
e.g:
	START_TIMER();
	Run mypass();
	END_TIMER_FMT(("My Pass Name%s", get_pass_name())); */
#define START_TIMER_FMT()  			\
	LONG _start_time_count_ = 0;	\
	if (g_show_comp_time) {			\
		_start_time_count_ = getclockstart();	\
	}
#define END_TIMER_FMT(s)			\
	if (g_show_comp_time) {			\
		printf("\n==-- ");			\
		printf s;					\
		printf(" Time:%fsec",	\
			   getclockend(_start_time_count_)); \
	}


/* Define multiple const string timers,
and show const string before timer start.
e.g:
	START_TIMERS("My Pass", local_timer);
	Run mypass();
	END_TIMERS(local_timer); */
#define START_TIMERS(s, _timer_timer_)	\
	LONG _timer_timer_ = 0; 				\
	if (g_show_comp_time) {					\
		_timer_timer_ =						\
			getclockstart();				\
		printf("\n==-- %s Time:", (s));		\
	}
#define END_TIMERS(_timer_timer_)			\
	if (g_show_comp_time) {					\
		printf("%fsec", getclockend(_timer_timer_)); \
	}

//This macro declare copy constructor for class.
#define COPY_CONSTRUCTOR(class_name)  \
	class_name(class_name const&);  \
	class_name const& operator = (class_name const&)

//Used to avoid warning: unreferenced variable if set
//-Werror=unused-variable.
//#define UNUSED(v) (v)
template <typename T> void dummy_use(T const&) {}
#define UNUSED(v) dummy_use(v)


#ifdef _DEBUG_
#define CK_USE(a)	ASSERT0(a)
#else
#define CK_USE(a)	UNUSED(a)
#endif


template <class T, UINT GrowSize> class Vector;

UINT arra(UINT n, UINT m); //Arrangement
void af2i(IN CHAR * f, OUT BYTE * buf, INT buflen, bool is_double);

bool binsearch(INT array[], UINT n, INT v, IN OUT UINT * ppos);

UINT combin(UINT n, UINT m); //Combination
LONGLONG ceil_align(LONGLONG v, LONGLONG align);

void dumpf_svec(void * vec, UINT ty, CHAR const* name, bool is_del);
void dumps_svec(void * vec, UINT ty);

INT exgcd(INT a, INT b, OUT INT & x, OUT INT & y);

UINT fact(UINT n);
INT findstr(CHAR * src, CHAR * s);

INT gcdm(UINT num, ...);
INT gcdm(UINT num, Vector<INT, 8> const& a);

//Compute the nearest power of 2 that not less than v.
inline UINT getNearestPowerOf2(UINT v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

//Compute the nearest power of 2 that not less than v.
inline ULONGLONG getNearestPowerOf2(ULONGLONG v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	v++;
	return v;
}

//Compute the number of 1.
UINT getLookupPopCount(ULONGLONG v);
UINT getSparsePopCount(ULONGLONG v);
UINT getPowerOf2(ULONGLONG v);
UINT get_const_bit_len(LONGLONG v);
CHAR * getfilesuffix(CHAR * n, OUT CHAR * buf);
CHAR * getfilepath(CHAR * n, OUT CHAR * buf, UINT bufl);
CHAR * getfilename(CHAR * n, OUT CHAR * buf, UINT bufl);
ULONGLONG getusec();
LONG getclockstart();
float getclockend(LONG start);
INT getFirstOneAtRightSide(INT m);

inline UINT hash32bit(UINT n)
{
	n = (n+0x7ed55d16) + (n<<12);
	n = (n^0xc761c23c) ^ (n>>19);
	n = (n+0x165667b1) + (n<<5);
	n = (n+0xd3a2646c) ^ (n<<9);
	n = (n+0xfd7046c5) + (n<<3);
	n = (n^0xb55a4f09) ^ (n>>16);
	return n;
}

bool is_integer(float f);
bool is_integerd(double d);
bool isPowerOf5(double f);

//inline is necessary to avoid multiple define.
inline bool isPowerOf2(ULONGLONG x)
{ return (x != 0 && (x & (x-1)) == 0); }

void prim(INT m, OUT INT * buf);
LONG revlong(LONG d);
UCHAR * reverseString(UCHAR * v);
CHAR * upper(CHAR * n);
CHAR * lower(CHAR * n);
INT sgcd(INT x, INT y);
INT slcm(INT x, INT y);
void strshift(CHAR * src, INT ofst);

CHAR * xstrcat(CHAR * buf, UINT bufl, CHAR const* info, ...);
UINT xstrlen(CHAR const* p);
bool xstrcmp(CHAR const* p1, CHAR const* p2, INT n);
CHAR * xsprintf(IN OUT CHAR * buf,
				UINT buflen,
				IN CHAR const* format,
				...);
LONG xatol(CHAR const* nptr, bool is_oct);
INT xctoi(CHAR const* cl);
UCHAR * xltoa(LONG v, OUT UCHAR * buf);
INT xceiling(INT a, INT b);
INT xfloor(INT a, INT b);
INT xstrstr(CHAR const* src, CHAR const* par, INT i);
void xstrcpy(CHAR * tgt, CHAR const* src, UINT size);
inline bool xisspace(CHAR c) { return c == ' ' || c == '\t'; }
inline bool xisdigit(CHAR c) { return c >= '0' && c <= '9'; }
inline bool xisdigithex(CHAR d)
{
	if (xisdigit(d)) return true;
	else if ((d >= 'a' && d <= 'f') || (d >= 'A' && d <= 'F')) return true;
	return false;
}
inline bool xisalpha(CHAR c)
{ return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
LONGLONG xabs(LONGLONG a);


//Exported Data Structures
class ASCII {
public:
	UCHAR val;
	CHAR ch;
};
extern ASCII g_asc1[];

} //namespace xcom
#endif
