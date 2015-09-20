diff --git a/Makefile b/Makefile
index 8fea3e8..817880a 100644
--- a/Makefile
+++ b/Makefile
@@ -42,6 +42,7 @@ OPT_OBJS +=\
       opt/data_type.o \
       opt/option.o\
       opt/region.o\
+      opt/region_mgr.o\
       opt/util.o\
       opt/var.o\
       opt/md.o\
diff --git a/com/comf.cpp b/com/comf.cpp
index 81d359d..108e132 100644
--- a/com/comf.cpp
+++ b/com/comf.cpp
@@ -204,7 +204,7 @@ INT xfloor(INT a, INT b)
 }
 
 
-//Round up to plus infinity
+//Round up to plus infinity.
 INT xceiling(INT a, INT b)
 {
     ASSERT(b != 0, ("div zero"));
@@ -226,7 +226,7 @@ INT xceiling(INT a, INT b)
 }
 
 
-//Great common divisor
+//Great common divisor.
 INT sgcd(INT x, INT y)
 {
     INT t;
@@ -255,15 +255,14 @@ INT slcm(INT x, INT y)
 }
 
 
-INT gcdm(UINT num, Vector<INT, 8> const& a)
+//Great common divisor for values stored in vector.
+INT gcdm(UINT num, Vector<INT, 8> const& values)
 {
-    if (num == 0) {
-        return 0;
-    }
-    INT n1 = a[0];
+    if (num == 0) { return 0; }
+    INT n1 = values[0];
     UINT i = 1;
     while (i < num) {
-        INT n2 = a[i];
+        INT n2 = values[i];
         n1 = sgcd(n1, n2);
         if (n1 == 1) {
             return 1;
@@ -296,6 +295,7 @@ INT gcdm(UINT num, ...)
     return n1;
 }
 
+
 static INT _exgcd(IN INT a, IN INT b, OUT INT & x, OUT INT & y)
 {
     if (b == 0) {
@@ -311,7 +311,7 @@ static INT _exgcd(IN INT a, IN INT b, OUT INT & x, OUT INT & y)
 }
 
 
-//Extended Euclid Method
+//Extended Euclid Method.
 //    ax + by = ay' + b(x' -floor(a/b)*y') = gcd(a,b) = gcd(b, a%b)
 INT exgcd(IN INT a, IN INT b, OUT INT & x, OUT INT & y)
 {
@@ -352,9 +352,9 @@ UINT arra(UINT n, UINT m)
 }
 
 
-/* Combination
-C(n,m)=(n*(n-1)*...*(n-m+1))/m! = n!/m!(n-m)!
-Simplify:C(n,m)=(n*(n-1)*(m+1))/(n-m)! */
+//Combination
+//C(n,m)=(n*(n-1)*...*(n-m+1))/m! = n!/m!(n-m)!
+//Simplify:C(n,m)=(n*(n-1)*(m+1))/(n-m)!
 UINT combin(UINT n, UINT m)
 {
     ASSERT(n != 0 && m != 0 && n >= m, ("illegal param"));
@@ -371,10 +371,8 @@ UINT combin(UINT n, UINT m)
 }
 
 
-/* Convert char value into binary.
-e.g:
-    char p = ' '; p is blank.
-*/
+//Convert char value into binary.
+//e.g: char p = ' '; p is blank.
 INT xctoi(CHAR const* cl)
 {
     #ifndef BYTE_PER_INT
@@ -401,10 +399,10 @@ INT xctoi(CHAR const* cl)
 }
 
 
-/* Convert a string into long integer.
-e.g: cl = '1','2','3','4','5'
-return 12345.
-'is_oct': if true, nptr is octal digits. */
+//Convert a string into long integer.
+//e.g: cl = '1','2','3','4','5'
+//return 12345.
+//'is_oct': if true, nptr is octal digits.
 LONG xatol(CHAR const* nptr, bool is_oct)
 {
     if (nptr == NULL) return 0;
@@ -653,11 +651,8 @@ void dumps_svec(void * vec, UINT ty)
 }
 
 
-/*
-Reverse a DWORD by lexicalgraph
-e.g:if 'd' is 0x12345678
-    return 0x78563412
-*/
+//Reverse a DWORD by lexicalgraph.
+//e.g:if 'd' is 0x12345678, return 0x78563412
 LONG revlong(LONG d)
 {
     CHAR * c = (CHAR*)&d, m;
@@ -715,17 +710,18 @@ UINT getLookupPopCount(ULONGLONG v)
 }
 
 
-//Searchs for sub-string
+//Searchs for sub-string.
 INT findstr(CHAR * src, CHAR * s)
 {
+    if (src == NULL || s == NULL) { return 0; }
+
     // can't have empty or NULL 'old'
     INT srclen = -1;
     CHAR * startp = src, * p, * q;
-    INT l = 0, i = 0;
-    if (src == NULL || s == NULL) { return 0; }
+    INT l = 0;
+    INT i = 0;
     srclen = strlen(src);
     p = startp;
-    i = 0;
     while (p[i] != 0) {
         if (p[i] == s[0]) {
             q = &p[i];
@@ -734,6 +730,7 @@ INT findstr(CHAR * src, CHAR * s)
                 if (q[l] != s[l]) { break; }
                 l++;
             }
+
             if (s[l] == 0) {//match seacrching
                 goto FIND;
             }
@@ -741,15 +738,14 @@ INT findstr(CHAR * src, CHAR * s)
         i++;
     }
     return 0;
+
 FIND:
     return 1;
 }
 
 
-/*
-Ceil rounding alignment.
-e.g  v=17 , align=4 , the result is 20
-*/
+//Ceil rounding alignment.
+//e.g  v=17 , align=4 , the result is 20.
 LONGLONG ceil_align(LONGLONG v, LONGLONG align)
 {
     if (align == 0 || align == 1) return v;
@@ -761,124 +757,130 @@ LONGLONG ceil_align(LONGLONG v, LONGLONG align)
 
 
 //Extract file path.
-//e.g: Given /xxx/yyy/a.file, return /xxx/yyy/
-CHAR * getfilepath(IN CHAR * n, OUT CHAR * buf, UINT bufl)
+//e.g: Given /xx/yy/zz.file, return /xx/yy
+CHAR * getfilepath(CHAR const* n, OUT CHAR * buf, UINT bufl)
 {
-    INT l = strlen(n), i = 0;
-    if (n == NULL) return NULL;
-    i = l;
-    while (i >= 0) {
-        if (n[i] != '\\' && n[i] != '/') {
-            i--;
-        } else {
-            break;
-        }
+    if (n == NULL) { return NULL; }
+
+    ASSERT0(buf);
+    INT l = strlen(n);
+    INT i = l;
+    while (n[i] != '\\' && n[i] != '/' && i >= 0) {
+        i--;
     }
+
     if (i < 0) {
         return NULL;
-    } else {
-        UNUSED(bufl);
-        ASSERT0((UINT)i < bufl);
-        memcpy(buf, n, i);
-        buf[i] = 0;
     }
+
+    UNUSED(bufl);
+    ASSERT0(i < (INT)bufl);
+    memcpy(buf, n, i);
+    buf[i] = 0;
     return buf;
 }
 
 
-/*
-Shift a string to right side or left side
-OFST : greate than zero means shifting SRC to right side,
-       and the displacement is abs(ofst); negative
-       means shifting SRC to left.
-*/
-void strshift(CHAR * src, INT ofst)
+//Shift a string to right side or left side.
+//ofst: great than zero means shifting string to right side,
+//   and the displacement is abs(ofst); negative
+//   means shifting string to left.
+void strshift(IN OUT CHAR * string, INT ofst)
 {
-    INT len = strlen(src), i;
-    if (!src) return;
+    INT len = strlen(string), i;
+    if (string == NULL) { return; }
 
-    if (ofst>=0) { //shift to right
+    if (ofst >= 0) { //shift to right
         if (ofst >= len) {
-            src[0] = 0;
+            string[0] = 0;
         } else {
-            src[len - ofst] = 0;
+            string[len - ofst] = 0;
         }
     } else if (ofst < 0) { //shift to left
         if ((-ofst) >= len) {
-            src[0] = 0;
+            string[0] = 0;
         } else {
             INT diff = len + ofst;
             ofst = -ofst;
             i = 0;
             while (i < diff ) {
-                src[i] = src[ofst + i];
+                string[i] = string[ofst + i];
                 i++;
             }
-            src[i] = 0;
+            string[i] = 0;
         }
     }
 }
 
 
 //Extract file name.
-//e.g: Given /xx/yy/name.foo, return name.
-CHAR * getfilename(CHAR * n, OUT CHAR * buf, UINT bufl)
+//e.g: Given /xx/yy/zz.foo, return zz.
+CHAR * getfilename(CHAR const* path, OUT CHAR * buf, UINT bufl)
 {
     UNUSED(bufl);
-    INT l = strlen(n), i = 0;
-    CHAR * p;
-    if (n == NULL) { return NULL; }
-    i = l;
-    while (i >= 0) {
-        if (n[i] != '\\' && n[i] != '/') {
-            i--;
-        } else {
-            break;
+    if (path == NULL) { return NULL; }
+    INT l = strlen(path);
+    INT i = l;
+    INT dotpos = -1;
+    while (path[i] != '\\' && path[i] != '/' && i >= 0) {
+        if (path[i] == '.') {
+            dotpos = i;
         }
+        i--;
     }
+    i++;
+    ASSERT0(i < (INT)bufl);
+    UINT start;
+    UINT end;
+    if (i < 0) { start = 0; }
+    else { start = i; }
 
-    if (i < 0) {
-        i = l;
-        p = n;
-    } else {
-        p = (CHAR*)ALLOCA(l - i);
-        memcpy(p, n + i + 1, l - i);
-        i = l - i;
-    }
+    if (dotpos < 0) { end = l; }
+    else { end = dotpos; }
 
-    while (i >= 0) {
-        if (p[i] != '.') {
-            i--;
-        } else {
-            break;
-        }
+    UINT len = end - start;
+    if (len > 0) {
+        memcpy(buf, path + start, len);
     }
-
-    ASSERT0((UINT)i < bufl);
-    memcpy(buf, p, i);
-    buf[i] = 0;
+    buf[len] = 0;
     return buf;
 }
 
 
 //Extract file suffix.
 //e.g: Given a.foo, return foo.
-CHAR * getfilesuffix(CHAR * n, OUT CHAR * buf)
+CHAR * getfilesuffix(CHAR const* n, OUT CHAR * buf, UINT bufl)
 {
-    INT l = strlen(n), i = 0;
-    CHAR * v = buf;
-    if (n == NULL) return NULL;
-    i = l;
-    while (i >= 0) {
-        if (n[i] != '.') {
-            i--;
-        } else {
-            break;
+    if (n == NULL) { return NULL; }
+
+    INT l = strlen(n);
+    INT i = l;
+    while (n[i] != '.' && i >= 0) {
+        i--;
+    }
+    UNUSED(bufl);
+    if (i < 0) { return NULL; }
+    ASSERT0((UINT)(l - i -1) < bufl);
+    memcpy(buf, n + i + 1, l - i -1);
+    buf[l - i -1] = 0;
+    return buf;
+}
+
+
+//Extract the right most sub-string which separated by 'separator' from string.
+//e.g: Given string is a\b\c, separator is '\', return c;
+CHAR const* extractRightMostSubString(CHAR const* string, CHAR separator)
+{
+    UINT l = strlen(string);
+    CHAR const* p = string + l;
+    for (; l != 0; l--, p--) {
+        if (*p == separator) {
+            return p + 1;
         }
     }
-    memcpy(v, n + i + 1, l - i -1);
-    v[l - i -1] = 0;
-    return v;
+
+    //Not found method name.
+    return p;
 }
 
 
@@ -907,8 +909,9 @@ bool xstrcmp(CHAR const* p1, CHAR const* p2, INT n)
 }
 
 
-CHAR * upper(CHAR* n)
+CHAR * upper(CHAR * n)
 {
+    if (n == NULL) { return NULL; }
     LONG l = strlen(n);
     l--;
     while (l >= 0) {
@@ -922,6 +925,7 @@ CHAR * upper(CHAR* n)
 
 CHAR * lower(CHAR * n)
 {
+    if (n == NULL) { return NULL; }
     LONG l = strlen(n);
     l--;
     while (l >= 0) {
@@ -933,10 +937,8 @@ CHAR * lower(CHAR * n)
 }
 
 
-/*
-Get the index of the first '1' start at most right side.
-e.g: given m=0x8, the first '1' index is 3.
-*/
+//Get the index of the first '1' start at most right side.
+//e.g: given m=0x8, the first '1' index is 3.
 INT getFirstOneAtRightSide(INT m)
 {
     static const INT dbitpos[32] = {
@@ -950,10 +952,8 @@ INT getFirstOneAtRightSide(INT m)
 //Judge if 'f' is integer conform to IEEE754 spec.
 bool is_integer(float f)
 {
-    /*
-    0000 0000 0111 1111 1111 1111 1111 1111 //mantissa
-    0111 1111 1000 0000 0000 0000 0000 0000 //exp
-    */
+    //0000 0000 0111 1111 1111 1111 1111 1111 //mantissa
+    //0111 1111 1000 0000 0000 0000 0000 0000 //exp
     float * p = &f;
     INT i = *(INT*)p;
     INT m = i & 0x007FFFFF; //mantissa
@@ -995,12 +995,10 @@ bool isPowerOf5(double f)
 }
 
 
-/*
-Return the position in array if find v.
-'array': sorted in incremental order.
-'n': elements size of array.
-'v': search v in array.
-*/
+//Return the position in array if find v.
+//array: sorted in incremental order.
+//n: elements size of array.
+//v: search v in array.
 bool binsearch(INT array[], UINT n, INT v, IN OUT UINT * ppos)
 {
     if (n == 0) return false;
@@ -1390,9 +1388,9 @@ OVER: //We got some problems, and going to the xsprintf.
 
 
 static bool back_slash(CHAR * buf,
-                    UINT buflen,
-                    IN OUT UINT * bufpos,
-                    IN OUT CHAR const** format)
+                       UINT buflen,
+                       IN OUT UINT * bufpos,
+                       IN OUT CHAR const** format)
 {
     CHAR const* pos = *format;
     CHAR ch = *pos++;
@@ -1425,14 +1423,12 @@ OVER: //Get some problems.
 }
 
 
-/*
-Format string and record in buf.
-'buf': output buffer record string.
-'stack_start': point to the first args.
-*/
+//Format string and record in buf.
+//buf: output buffer that record string.
+//buflen: length of output buffer.
 CHAR * xsprintf(IN OUT CHAR * buf,
-                IN UINT buflen,
-                IN CHAR const* format,
+                UINT buflen,
+                CHAR const* format,
                 ...)
 {
     UINT bufpos = 0;
diff --git a/com/comf.h b/com/comf.h
index d5c42f1..55e3a76 100644
--- a/com/comf.h
+++ b/com/comf.h
@@ -131,23 +131,52 @@ template <typename T> void dummy_use(T const&) {}
 
 template <class T, UINT GrowSize> class Vector;
 
+//Arrangement
+//P(n,m)=n*(n-1)*...*(n-m+1)=n!/(n-m)!
 UINT arra(UINT n, UINT m); //Arrangement
+
+//Convert floating point string into binary words.
 void af2i(IN CHAR * f, OUT BYTE * buf, INT buflen, bool is_double);
 
+//Return the position in array if find v.
+//array: sorted in incremental order.
+//n: elements size of array.
+//v: search v in array.
 bool binsearch(INT array[], UINT n, INT v, IN OUT UINT * ppos);
 
+//Combination
+//C(n,m)=(n*(n-1)*...*(n-m+1))/m! = n!/m!(n-m)!
+//Simplify:C(n,m)=(n*(n-1)*(m+1))/(n-m)!
 UINT combin(UINT n, UINT m); //Combination
+
+//Ceil rounding alignment.
+//e.g  v=17 , align=4 , the result is 20.
 LONGLONG ceil_align(LONGLONG v, LONGLONG align);
 
+//Dumpf() for Vector<TY>.
 void dumpf_svec(void * vec, UINT ty, CHAR const* name, bool is_del);
+
+//Dumps() for Vector<TY>.
 void dumps_svec(void * vec, UINT ty);
 
+//Extended Euclid Method.
+//    ax + by = ay' + b(x' -floor(a/b)*y') = gcd(a,b) = gcd(b, a%b)
 INT exgcd(INT a, INT b, OUT INT & x, OUT INT & y);
 
+//Factorial of n, namely, requiring n!.
 UINT fact(UINT n);
+
+//Searchs for sub-string.
 INT findstr(CHAR * src, CHAR * s);
 
+//Extract the right most sub-string which separated by 'separator' from string.
+//e.g: Given string is a\b\c, separator is '\', return c;
+CHAR const* extractRightMostSubString(CHAR const* string, CHAR separator);
+
+//Great common divisor for number of values.
 INT gcdm(UINT num, ...);
+
+//Great common divisor for values stored in vector.
 INT gcdm(UINT num, Vector<INT, 8> const& a);
 
 //Compute the nearest power of 2 that not less than v.
@@ -179,15 +208,34 @@ inline ULONGLONG getNearestPowerOf2(ULONGLONG v)
 
 //Compute the number of 1.
 UINT getLookupPopCount(ULONGLONG v);
+
+//Compute the number of 1.
 UINT getSparsePopCount(ULONGLONG v);
+
+//Compute the power of 2, return the result.
+//Note v must be power of 2.
 UINT getPowerOf2(ULONGLONG v);
 UINT get_const_bit_len(LONGLONG v);
-CHAR * getfilesuffix(CHAR * n, OUT CHAR * buf);
-CHAR * getfilepath(CHAR * n, OUT CHAR * buf, UINT bufl);
-CHAR * getfilename(CHAR * n, OUT CHAR * buf, UINT bufl);
+
+//Extract file suffix.
+//e.g: Given a.foo, return foo.
+CHAR * getfilesuffix(CHAR const* n, OUT CHAR * buf, UINT bufl);
+
+//Extract file path.
+//e.g: Given /xx/yy/zz.file, return /xx/yy
+CHAR * getfilepath(CHAR const* n, OUT CHAR * buf, UINT bufl);
+
+//Extract file name.
+//e.g: Given /xx/yy/zz.foo, return zz.
+CHAR * getfilename(CHAR const* n, OUT CHAR * buf, UINT bufl);
+
+//Get current micro-second.
 ULONGLONG getusec();
 LONG getclockstart();
 float getclockend(LONG start);
+
+//Get the index of the first '1' start at most right side.
+//e.g: given m=0x8, the first '1' index is 3.
 INT getFirstOneAtRightSide(INT m);
 
 inline UINT hash32bit(UINT n)
@@ -201,7 +249,10 @@ inline UINT hash32bit(UINT n)
     return n;
 }
 
+//Judge if 'f' is integer conform to IEEE754 spec.
 bool is_integer(float f);
+
+//Judge if 'd' is integer conform to IEEE754 spec.
 bool is_integerd(double d);
 bool isPowerOf5(double f);
 
@@ -209,24 +260,53 @@ bool isPowerOf5(double f);
 inline bool isPowerOf2(ULONGLONG x)
 { return (x != 0 && (x & (x-1)) == 0); }
 
+//Prime Factorization.
+//e.g: 435234 = 251 * 17 * 17 * 3 * 2.
 void prim(INT m, OUT INT * buf);
+
+//Reverse a DWORD by lexicalgraph.
+//e.g:if 'd' is 0x12345678, return 0x78563412.
 LONG revlong(LONG d);
+
+//Reverse the string.
 UCHAR * reverseString(UCHAR * v);
 CHAR * upper(CHAR * n);
 CHAR * lower(CHAR * n);
 INT sgcd(INT x, INT y);
 INT slcm(INT x, INT y);
+
+//Shift a string to right side or left side.
+//ofst: great than zero means shifting string to right side,
+//   and the displacement is abs(ofst); negative
+//   means shifting string to left.
 void strshift(CHAR * src, INT ofst);
 
 CHAR * xstrcat(CHAR * buf, UINT bufl, CHAR const* info, ...);
 UINT xstrlen(CHAR const* p);
+
+//Compare the first 'n' char of two string.
+//Return true if equal.
 bool xstrcmp(CHAR const* p1, CHAR const* p2, INT n);
+
+//Format string and record in buf.
+//'buf': output buffer record string.
+//'stack_start': point to the first args.
 CHAR * xsprintf(IN OUT CHAR * buf,
                 UINT buflen,
                 IN CHAR const* format,
                 ...);
+
+//Convert a string into long integer.
+//e.g: cl = '1','2','3','4','5'
+//return 12345.
+//'is_oct': if true, nptr is octal digits.
 LONG xatol(CHAR const* nptr, bool is_oct);
+
+//Convert char value into binary.
+//e.g: char p = ' '; p is blank.
 INT xctoi(CHAR const* cl);
+
+//Convert long to string.
 UCHAR * xltoa(LONG v, OUT UCHAR * buf);
 INT xceiling(INT a, INT b);
 INT xfloor(INT a, INT b);
diff --git a/com/smempool.cpp b/com/smempool.cpp
index dc470fd..d679ad5 100644
--- a/com/smempool.cpp
+++ b/com/smempool.cpp
@@ -91,12 +91,12 @@ void dumpPool(SMemPool * handler, FILE * h)
 
 static SMemPool * new_mem_pool(size_t size, MEMPOOLTYPE mpt)
 {
-    SMemPool * mp = NULL;
     INT size_mp = sizeof(SMemPool);
     if (size_mp % WORD_ALIGN) {
         size_mp = (sizeof(SMemPool) / WORD_ALIGN + 1 ) * WORD_ALIGN;
     }
-    mp = (SMemPool*)malloc(size_mp + size + END_BOUND_BYTE);
+
+    SMemPool * mp = (SMemPool*)malloc(size_mp + size + END_BOUND_BYTE);
     ASSERT(mp, ("create mem pool failed, no enough memory"));
     memset(mp, 0, size_mp);
     memset(((CHAR*)mp) + size_mp + size, BOUNDARY_NUM, END_BOUND_BYTE);
@@ -151,6 +151,10 @@ inline static void append_after_smp(SMemPool * marker, SMemPool * tlst)
 }
 
 
+//This function do some initializations if you want to manipulate pool
+//via pool index.
+//Note if you just create pool and manipulate pool via handler,
+//the initialization is dispensable.
 //Hash table must be initialized if one invoked
 //smpoolCreatePoolIndex or smpoolMalloc.
 void smpoolInitPool()
@@ -185,6 +189,8 @@ void smpoolInitPool()
 }
 
 
+//This function perform finialization works if you invoke the
+//smpoolInitPool().
 void smpoolFiniPool()
 {
     if (g_is_pool_init && g_is_pool_hashed) {
diff --git a/com/smempool.h b/com/smempool.h
index 87a3258..8a81908 100644
--- a/com/smempool.h
+++ b/com/smempool.h
@@ -103,7 +103,15 @@ void * smpoolMallocConstSize(size_t elem_size, IN SMemPool * handler);
 //Get whole pool size with byte
 size_t smpoolGetPoolSizeViaIndex(MEMPOOLIDX mpt_idx);
 size_t smpoolGetPoolSize(SMemPool const* handle);
+
+//This function do some initializations if you want to manipulate pool
+//via pool index.
+//Note if you just create pool and manipulate pool via handler,
+//the initialization is dispensable.
 void smpoolInitPool(); //Initializing pool utilities
+
+//This function perform finialization works if you invoke the
+//smpoolInitPool().
 void smpoolFiniPool(); //Finializing pool
 
 void dumpPool(SMemPool * handler, FILE * h);
diff --git a/com/sstl.h b/com/sstl.h
index 1cb75f1..c6676b0 100644
--- a/com/sstl.h
+++ b/com/sstl.h
@@ -486,6 +486,8 @@ public:
 };
 //END FreeList
 
+
+
 /* Dual Linked List.
 NOTICE:
     The following operations are the key points which you should
@@ -1709,16 +1711,17 @@ public:
 /* Single List
 
 NOTICE:
-    The following 3 operations are the key points which you should
-    attention to:
-    1.    If you REMOVE one element, its container will be collect by FREE-List.
-        So if you need a new container, please check the FREE-List first,
-        accordingly, you should first invoke 'get_free_list' which get free
-        containers out from 'm_free_list'.
-      2.    If you want to invoke APPEND, please call 'newXXX' first to
-        allocate a new container memory space, record your elements into
-        the container, then APPEND it at list.
-        newXXX such as:
+    The following operations are the key points which you should attention to:
+
+    1. If you REMOVE one element, its container will be collect by FREE-List.
+       So if you need a new container, please check the FREE-List first,
+       accordingly, you should first invoke 'get_free_list' which get free
+       containers out from 'm_free_list'.
+
+    2. If you want to invoke APPEND, please call 'newXXX' first to
+       allocate a new container memory space, record your elements into
+       the container, then APPEND it at list.
+       newXXX such as:
             T * newXXX(INT type)
             {
                 T * t = get_free_T();
@@ -1726,11 +1729,14 @@ NOTICE:
                 T_type(c) = type;
                 return t;
             }
-    3.  The single linked list is different with dual linked list.
-        the dual linked list does not use mempool to hold the container.
-        Compared to dual linked list, single linked list allocate containers
-        in a const size pool.
-    4.  Compare the iterator with end() to determine if meeting the end of list.
+
+    3. The single linked list is different with dual linked list.
+       the dual linked list does not use mempool to hold the container.
+       Compared to dual linked list, single linked list allocate containers
+       in a const size pool.
+       Invoke init() to do initialization if you allocate SList by malloc().
+
+    4. Compare the iterator with end() to determine if meeting the end of list.
 */
 template <class T> class SList : public SListCore<T> {
 protected:
diff --git a/dex/Android.mk b/dex/Android.mk
index 4ce527e..f52a642 100644
--- a/dex/Android.mk
+++ b/dex/Android.mk
@@ -20,18 +20,17 @@ X_INCLUDE_FILES := \
     $(LLVM_ROOT_PATH)/include \
     ${LOCAL_PATH}/clibs/basic/include \
     ${LOCAL_PATH}/clibs/basic/arch/common \
-    ${LOCAL_PATH}/dex2dex/include \
+    ${LOCAL_PATH}/dex2dex \
     ${LOCAL_PATH}/include \
-    ${LOCAL_PATH}/lir/include \
-    ${LOCAL_PATH}/linealloc/include \
-    ${LOCAL_PATH}/analyse/include \
+    ${LOCAL_PATH}/lir \
+    ${LOCAL_PATH}/linealloc \
     ${LOCAL_PATH}/../com \
     ${LOCAL_PATH}/../opt \
     ${LOCAL_PATH} \
     dalvik
 
 #X_LOCAL_CFLAGS := \
-	-DLOG_TAG=\"DEX2DEX\" \
+	-DLOG_TAG=\"dexpro\" \
 	-D_GNU_SOURCE=1\
 	-D_ENABLE_LOG_\
 	-D_VMWARE_DEBUG_\
@@ -43,7 +42,7 @@ X_INCLUDE_FILES := \
 	-Wstrict-overflow -Wstrict-aliasing=3 -finline-limit=10000000 -Wswitch
 
 X_LOCAL_CFLAGS := \
-	-DLOG_TAG=\"DEX2DEX\" \
+	-DLOG_TAG=\"dexpro\" \
 	-D_GNU_SOURCE=1\
 	-D_VMWARE_DEBUG_\
 	-D_ENABLE_LOG_\
@@ -69,11 +68,9 @@ include $(RELATIVE_AOC)/xoc/dex/config.mk
 
 X_SRC_FILES:= \
     *.cpp\
-    dex2dex/src/cpp/*.cpp\
-    linealloc/src/cpp/*.cpp\
-    lir/src/cpp/*.cpp\
-    analyse/src/*.cpp\
-    analyse/src/*.c\
+    dex2dex/*.cpp\
+    linealloc/*.cpp\
+    lir/*.cpp\
     ../opt/*.cpp\
     ../com/*.cpp
 
@@ -102,14 +99,12 @@ include $(CLEAR_VARS)
 include $(RELATIVE_AOC)/xoc/dex/config.mk
 
 X_SRC_FILES:= \
-    dex2dex/src/cpp/*.cpp\
-    linealloc/src/cpp/*.cpp\
-    lir/src/cpp/*.cpp\
+    dex2dex/*.cpp\
+    linealloc/*.cpp\
+    lir/*.cpp\
     ../opt/*.cpp\
     ../com/*.cpp\
-    *.cpp\
-    analyse/src/*.cpp\
-    analyse/src/*.c
+    *.cpp
 
 LOCAL_CFLAGS += $(X_LOCAL_CFLAGS)
 
@@ -135,17 +130,7 @@ include $(CLEAR_VARS)
 #must include vmkid/config.mk after CLEAR_VARS
 include $(RELATIVE_AOC)/xoc/dex/config.mk
 
-X_SRC_FILES:= \
-    dex2dex/src/cpp/*.cpp\
-    linealloc/src/cpp/*.cpp\
-    lir/src/cpp/*.cpp\
-    analyse/src/*.cpp\
-    analyse/src/*.c\
-    *.cpp\
-    ../com/*.cpp\
-    ../opt/*.cpp
-
-X_SRC_FILES:= dex2dex/src/test/main.cpp
+X_SRC_FILES:= dex2dex/main.cpp
 
 LOCAL_C_INCLUDES := $(X_INCLUDE_FILES)
 LOCAL_SRC_FILES := $(foreach F, $(X_SRC_FILES), $(addprefix $(dir $(F)),$(notdir $(wildcard $(LOCAL_PATH)/$(F)))))
@@ -158,6 +143,6 @@ LOCAL_STATIC_LIBRARIES := libdex libz liblog libcutils
 LOCAL_SHARED_LIBRARIES := libaoc_clibs libaoc_xoc
 
 LOCAL_MODULE_TAGS := optional
-LOCAL_MODULE := aoc_hdex2dex
+LOCAL_MODULE := dexpro
 
 include $(BUILD_HOST_EXECUTABLE)
diff --git a/dex/dex.cpp b/dex/dex.cpp
index 41eb332..2235c9d 100644
--- a/dex/dex.cpp
+++ b/dex/dex.cpp
@@ -35,20 +35,20 @@ author: Su Zhenyu
 #include "dex.h"
 
 BuiltInInfo g_builtin_info[] = {
-	{BLTIN_UNDEF, 				""					},
-	{BLTIN_INVOKE, 				"invoke"			},
-	{BLTIN_NEW,					"#new"				},
-	{BLTIN_NEW_ARRAY,           "#new_array"		},
-	{BLTIN_MOVE_EXP,            "#move_exception"	},
-	{BLTIN_MOVE_RES,            "#move_result"		},
-	{BLTIN_THROW,               "#throw"			},
-	{BLTIN_CHECK_CAST,          "#check_cast"		},
-	{BLTIN_FILLED_NEW_ARRAY,    "#filled_new_array"	},
-	{BLTIN_FILL_ARRAY_DATA,     "#fill_array_data"	},
-	{BLTIN_CONST_CLASS,         "#const_class"		},
-	{BLTIN_ARRAY_LENGTH,        "#array_length"		},
-	{BLTIN_MONITOR_ENTER,       "#monitor_enter"	},
-	{BLTIN_MONITOR_EXIT,        "#monitor_exit"		},
+    {BLTIN_UNDEF,                 ""                    },
+    {BLTIN_INVOKE,                 "invoke"            },
+    {BLTIN_NEW,                    "#new"                },
+    {BLTIN_NEW_ARRAY,           "#new_array"        },
+    {BLTIN_MOVE_EXP,            "#move_exception"    },
+    {BLTIN_MOVE_RES,            "#move_result"        },
+    {BLTIN_THROW,               "#throw"            },
+    {BLTIN_CHECK_CAST,          "#check_cast"        },
+    {BLTIN_FILLED_NEW_ARRAY,    "#filled_new_array"    },
+    {BLTIN_FILL_ARRAY_DATA,     "#fill_array_data"    },
+    {BLTIN_CONST_CLASS,         "#const_class"        },
+    {BLTIN_ARRAY_LENGTH,        "#array_length"        },
+    {BLTIN_MONITOR_ENTER,       "#monitor_enter"    },
+    {BLTIN_MONITOR_EXIT,        "#monitor_exit"        },
     {BLTIN_INSTANCE_OF,         "#instance_of"        },
     {BLTIN_CMP_BIAS,            "#cmp_bias"            },
     {BLTIN_LAST,                ""                    },
diff --git a/dex/dex.h b/dex/dex.h
index dcb6e4c..ed5f39e 100644
--- a/dex/dex.h
+++ b/dex/dex.h
@@ -35,7 +35,7 @@ author: Su Zhenyu
 #define _DEX_H_
 
 #ifdef _ENABLE_LOG_
-#define LOG ALOGE
+#define LOG ALOGI
 #else
 #define LOG
 #endif
diff --git a/dex/dex2ir.cpp b/dex/dex2ir.cpp
index 1b0802d..7b84edb 100644
--- a/dex/dex2ir.cpp
+++ b/dex/dex2ir.cpp
@@ -578,6 +578,7 @@ IR * Dex2IR::convertSget(IN LIR * lir)
     tbaa->type = mapFieldType2Type(LIR_op0(lir));
     ASSERT0(tbaa->type);
     ai->set(tbaa);
+    ASSERT0(rhs->get_ai() == NULL);
     IR_ai(rhs) = ai;
 
     return m_ru->buildStorePR(PR_no(res), IR_dt(res), rhs);
@@ -659,6 +660,7 @@ IR * Dex2IR::convertAput(IN LIR * lir)
     tbaa->type = m_tr->array;
     ASSERT0(tbaa->type);
     ai->set(tbaa);
+    ASSERT0(base->get_ai() == NULL);
     IR_ai(base) = ai;
 
     IR * c = m_ru->buildStoreArray(base, ofst, ty, ty, 1, &enbuf, src);
@@ -696,6 +698,7 @@ IR * Dex2IR::convertAget(IN LIR * lir)
     tbaa->type = m_tr->array;
     ASSERT0(tbaa->type);
     ai->set(tbaa);
+    ASSERT0(base->get_ai() == NULL);
     IR_ai(base) = ai;
 
     IR * c = m_ru->buildStorePR(PR_no(res), IR_dt(res), array);
@@ -935,10 +938,6 @@ IR * Dex2IR::convertIget(IN LIR * lir)
     IR * c = m_ru->buildStorePR(PR_no(res), IR_dt(res), ild);
 
     IR_may_throw(c) = true;
-    if (m_has_catch) {
-        IR * lab = m_ru->buildLabel(m_ru->genIlabel());
-        add_next(&c, lab);
-    }
 
     AttachInfo * ai = m_ru->newAI();
     TbaaAttachInfo * tbaa = (TbaaAttachInfo*)xmalloc(sizeof(TbaaAttachInfo));
@@ -946,8 +945,14 @@ IR * Dex2IR::convertIget(IN LIR * lir)
     tbaa->type = m_tr->ptr;
     ASSERT0(tbaa->type);
     ai->set(tbaa);
+    ASSERT0(obj->get_ai() == NULL);
     IR_ai(obj) = ai;
 
+    if (m_has_catch) {
+        IR * lab = m_ru->buildLabel(m_ru->genIlabel());
+        add_next(&c, lab);
+        attachCatchInfo(c);
+    }
     CHAR const* type_name = get_var_type_name(LIR_op1(lir));
     if (is_array_type(type_name)) {
         //The type of result value of ild is pointer to array type.
@@ -957,6 +962,7 @@ IR * Dex2IR::convertIget(IN LIR * lir)
         tbaa->type = m_tr->array;
         ASSERT0(tbaa->type);
         ai->set(tbaa);
+        ASSERT0(ild->get_ai() == NULL);
         IR_ai(ild) = ai;
     } else if (is_obj_type(type_name)) {
         //The type of result value of ild is pointer to object type.
@@ -2038,15 +2044,15 @@ void Dex2IR::markLabel()
     }
 
     if (m_lircode->triesSize != 0) {
-        TryInfo * head = NULL, * lasti = NULL;
+        TryInfo * tihead = NULL, * lasti = NULL;
         for (UINT i = 0; i < m_lircode->triesSize; i++) {
             LIROpcodeTry * each_try = m_lircode->trys + i;
             TryInfo * ti = (TryInfo*)xmalloc(sizeof(TryInfo));
-            add_next(&head, &lasti, ti);
+            add_next(&tihead, &lasti, ti);
 
-            INT s = each_try->start_pc;
-            ASSERT0(s >= 0 && s < LIRC_num_of_op(m_lircode));
-            LIR * lir = LIRC_op(m_lircode, s);
+            INT pos = each_try->start_pc;
+            ASSERT0(pos >= 0 && pos < LIRC_num_of_op(m_lircode));
+            LIR * lir = LIRC_op(m_lircode, pos);
             List<LabelInfo*> * lst = m_lir2labs.get(lir);
             if (lst == NULL) {
                 lst = new List<LabelInfo*>();
@@ -2056,13 +2062,14 @@ void Dex2IR::markLabel()
             LABEL_INFO_is_try_start(lab) = true;
             lst->append_tail(lab);
             ti->try_start = lab;
+            ti->try_start_pos = pos;
 
-            s = each_try->end_pc;
-            ASSERT0(s >= 0 && s <= LIRC_num_of_op(m_lircode));
+            pos = each_try->end_pc;
+            ASSERT0(pos >= 0 && pos <= LIRC_num_of_op(m_lircode));
             lab = m_ru->genIlabel();
             LABEL_INFO_is_try_end(lab) = true;
-            if (s < LIRC_num_of_op(m_lircode)) {
-                lir = LIRC_op(m_lircode, s);
+            if (pos < LIRC_num_of_op(m_lircode)) {
+                lir = LIRC_op(m_lircode, pos);
                 lst = m_lir2labs.get(lir);
                 if (lst == NULL) {
                     lst = new List<LabelInfo*>();
@@ -2073,6 +2080,7 @@ void Dex2IR::markLabel()
                 m_last_try_end_lab_list.append_tail(lab);
             }
             ti->try_end = lab;
+            ti->try_end_pos = pos;
 
             CatchInfo * last = NULL;
             for (UINT j = 0; j < each_try->catchSize; j++) {
@@ -2109,16 +2117,14 @@ void Dex2IR::markLabel()
 
                 ci->catch_start = lab;
                 ci->kind = each_catch->class_type;
-                CHAR const* catch_descriptor =
-                    ci->kind == kDexNoIndex ?
-                            "<any>" :
-                            dexStringByTypeIdx(m_df, ci->kind);
+                ci->kindname = ci->kind == kDexNoIndex ?
+                               "<any>" : dexStringByTypeIdx(m_df, ci->kind);
 
                 add_next(&ti->catch_list, &last, ci);
                 m_has_catch = true;
             }
         }
-        m_ti = head;
+        m_ti = tihead;
     }
 }
 
@@ -2168,6 +2174,14 @@ IR * Dex2IR::convert(bool * succ)
 
     if (dump) { dump_lir2lab(); }
 
+    INT start = -1;
+    INT end = -1;
+    TryInfo * ti = m_ti;
+    m_current_catch_list  = NULL;
+    if (ti != NULL) {
+        start = ti->try_start_pos;
+        end = ti->try_end_pos - 1;
+    }
     for (INT i = 0; i < LIRC_num_of_op(m_lircode); i++) {
         LIR * lir = LIRC_op(m_lircode, i);
 
@@ -2192,6 +2206,21 @@ IR * Dex2IR::convert(bool * succ)
                 add_next(&ir_list, &last, m_ru->buildLabel(l));
             }
         }
+        if (i > end && ti != NULL) {
+            ti = ti->next;
+            if (ti != NULL) {
+                start = ti->try_start_pos;
+                end = ti->try_end_pos - 1;
+            }
+        }
+
+        if (i >= start && i <= end) {
+            m_current_catch_list = ti->catch_list;
+            ASSERT0(m_current_catch_list);
+        } else {
+            m_current_catch_list = NULL;
+        }
+
         IR * newir = convert(lir);
 
         if (dump) {
diff --git a/dex/dex2ir.h b/dex/dex2ir.h
index 48dcd2c..9ec9b50 100644
--- a/dex/dex2ir.h
+++ b/dex/dex2ir.h
@@ -43,7 +43,8 @@ public:
     CatchInfo * prev;
     CatchInfo * next;
     LabelInfo * catch_start;
-    UINT kind;
+    UINT kind; //record exception type.
+    CHAR const* kindname; //record exception type name.
 };
 
 
@@ -53,6 +54,8 @@ public:
     TryInfo * next;
     LabelInfo * try_start;
     LabelInfo * try_end;
+    UINT try_start_pos;
+    UINT try_end_pos;
     CatchInfo * catch_list;
 };
 
@@ -84,7 +87,7 @@ typedef TMap<CHAR const*, Type const*, CmpStr> Str2Type;
 //To wit, the class declared in class LIR2IR, that will be better.
 class Dex2IR {
 protected:
-    Region * m_ru;
+    DexRegion * m_ru;
     RegionMgr * m_ru_mgr;
     TypeMgr * m_dm;
     DexFile * m_df;
@@ -99,6 +102,7 @@ protected:
     DbxVec const& m_dbxvec;
     Type const* m_ptr_addend;
     UINT m_ofst_addend;
+    CatchInfo * m_current_catch_list;
 
     //Map from typeIdx of type-table to
     //positionIdx in file-class-def-table.
@@ -127,7 +131,7 @@ public:
            DbxVec const& dbxvec) : m_dbxvec(dbxvec)
     {
         ASSERT0(ru && df && fu);
-        m_ru = ru;
+        m_ru = (DexRegion*)ru;
         m_ru_mgr = ru->get_region_mgr();
         m_dm = ru->get_dm();
         m_vm = ru->get_var_mgr();
@@ -141,6 +145,7 @@ public:
         m_ofst_addend = m_dm->get_dtype_bytesize(D_I64);
         m_pr2v.maxreg = fu->maxVars - 1;
         m_pr2v.paramnum = fu->numArgs;
+        m_current_catch_list = NULL;
     }
 
     ~Dex2IR()
diff --git a/dex/dex_util.cpp b/dex/dex_util.cpp
index 56b383d..eebf961 100644
--- a/dex/dex_util.cpp
+++ b/dex/dex_util.cpp
@@ -54,37 +54,37 @@ author: Su Zhenyu
 
 CHAR const* get_dt_name(LIR * ir)
 {
-	switch (LIR_dt(ir)) {
-	case LIR_JDT_unknown:
-		return "UNDEF";
-	case LIR_JDT_void	:
-		return "VOID";
-	case LIR_JDT_int	:
-		return "INT";
-	case LIR_JDT_object	:
-		return "OBJ";
-	case LIR_JDT_boolean:
-		return "BOOL";
-	case LIR_JDT_byte	:
-		return "BYTE";
-	case LIR_JDT_char	:
-		return "CHAR";
-	case LIR_JDT_short	:
-		return "SHORT";
-	case LIR_JDT_float	:
-		return "FLOAT";
-	case LIR_JDT_none	:
-		return "NONE";
-	case LIR_wide:
-		return "WIDE";
-	case LIR_JDT_wide:
-		return "WIDE";
-	case LIR_JDT_long:
-		return "LONG";
-	case LIR_JDT_double:
-		return "DOUBLE";
-	}
-	return NULL;
+    switch (LIR_dt(ir)) {
+    case LIR_JDT_unknown:
+        return "UNDEF";
+    case LIR_JDT_void    :
+        return "VOID";
+    case LIR_JDT_int    :
+        return "INT";
+    case LIR_JDT_object    :
+        return "OBJ";
+    case LIR_JDT_boolean:
+        return "BOOL";
+    case LIR_JDT_byte    :
+        return "BYTE";
+    case LIR_JDT_char    :
+        return "CHAR";
+    case LIR_JDT_short    :
+        return "SHORT";
+    case LIR_JDT_float    :
+        return "FLOAT";
+    case LIR_JDT_none    :
+        return "NONE";
+    case LIR_wide:
+        return "WIDE";
+    case LIR_JDT_wide:
+        return "WIDE";
+    case LIR_JDT_long:
+        return "LONG";
+    case LIR_JDT_double:
+        return "DOUBLE";
+    }
+    return NULL;
 }
 
 
@@ -218,75 +218,106 @@ void extractSeparateParamterType(
 
     //Note outputbuf should big enough to hold the string.
     CHAR * outputbuf = params.get_head();
-    for (; end != 0 && len != 0; len--, end++) {
-        if (*end != ';') { continue; }
-        ASSERT0(outputbuf);
-        UINT s = end - start;
-        memcpy(outputbuf, start, s);
-        outputbuf[s] = 0;
-        outputbuf = params.get_next();
-        start = end + 1;
-    }
-
-    ASSERT0(len == 0 && *end == 0);
-}
-
-
-//Extract the right most sub-string from str.
-CHAR const* extractRightMostString(CHAR const* str, CHAR separator)
-{
-    UINT l = strlen(str);
-    CHAR const* p = str + l;
-    for (; l != 0; l--, p--) {
-        if (*p == separator) {
-            return p + 1;
+    bool is_obj = false;
+    for (; end != 0 && len != 0;) {
+        //Determine if parameter is primitive type.
+        if (is_obj) {
+            //Parse object type.
+            if (*end != ';') {
+                len--;
+                end++;
+                continue;
+            }
+
+            ASSERT0(outputbuf);
+            UINT s = end - start;
+            memcpy(outputbuf, start, s);
+            outputbuf[s] = 0;
+            outputbuf = params.get_next();
+            start = end + 1;
+            is_obj = false;
+        } else {
+            switch (*end) {
+            case 'V': //void
+            case 'Z': //boolean
+            case 'B': //u8
+            case 'S': //i16
+            case 'C': //i8
+            case 'I': //i32
+            case 'J': //i64
+            case 'F': //f32
+            case 'D': //f64
+                if (start == end) {
+                    //Parse primitive type.
+                    ASSERT0(outputbuf);
+                    UINT s = 1;
+                    outputbuf[0] = *end;
+                    outputbuf[1] = 0;
+                    outputbuf = params.get_next();
+                    start = end + 1;
+                }
+                break;
+            case 'L': //object pointer
+                is_obj = true;
+                break;
+            case '[': //array
+                //Parse array type.
+                while (*end == '[' && end != 0) {
+                    len--;
+                    end++;
+                }
+                ASSERT0(end != 0);
+                continue;
+            default: break;
+            }
         }
-    }
+        len--;
+        end++;
+   }
 
-    //Not found method name.
-    return NULL;
+    ASSERT0(len == 0 && *end == 0);
 }
 
 
 //cls_type_idx: typeIdx in string-type-table,
-//	not the idx in class-def-item table.
+//    not the idx in class-def-item table.
 CHAR const* get_class_name(DexFile const* df, UINT cls_type_idx)
 {
-	return dexStringByTypeIdx(df, cls_type_idx);
+    return dexStringByTypeIdx(df, cls_type_idx);
 }
 
 
 //Get the source file path of the class.
 CHAR const* getClassSourceFilePath(
-		DexFile const* df,
-		DexClassDef const* class_info)
+        DexFile const* df,
+        DexClassDef const* class_info)
 {
-	return dexStringById(df, class_info->sourceFileIdx);
+    return dexStringById(df, class_info->sourceFileIdx);
 }
 
 
 CHAR const* get_class_name(DexFile const* df, DexClassDef const* class_info)
 {
-	ASSERT0(class_info);
-	CHAR const* class_name = dexStringByTypeIdx(df, class_info->classIdx);
-	ASSERT0(class_name);
-	return class_name;
+    ASSERT0(class_info);
+    CHAR const* class_name = dexStringByTypeIdx(df, class_info->classIdx);
+    ASSERT0(class_name);
+    return class_name;
 }
 
 
 CHAR const* get_class_name(DexFile const* df, DexMethod const* dm)
 {
-	ASSERT0(df && dm);
-	DexMethodId const* pMethodId = dexGetMethodId(df, dm->methodIdx);
-	return dexStringByTypeIdx(df, pMethodId->classIdx);
+    ASSERT0(df && dm);
+    DexMethodId const* pMethodId = dexGetMethodId(df, dm->methodIdx);
+    return dexStringByTypeIdx(df, pMethodId->classIdx);
 }
 
 
 CHAR const* get_func_name(DexFile const* df, DexMethod const* dm)
 {
-	ASSERT0(df && dm);
-	DexMethodId const* pMethodId = dexGetMethodId(df, dm->methodIdx);
-	return dexStringById(df, pMethodId->nameIdx);
+    ASSERT0(df && dm);
+    DexMethodId const* pMethodId = dexGetMethodId(df, dm->methodIdx);
+    return dexStringById(df, pMethodId->nameIdx);
 }
 
 
@@ -309,932 +340,932 @@ CHAR const* get_func_type(DexFile const* df, DexMethodId const* dmid)
 
 void dump_lir(LIR * lir, DexFile * df, INT pos)
 {
-	if (g_tfile == NULL || lir == NULL) return;
-	fprintf(g_tfile, "\n");
-	dump_lir2(lir, df, pos);
+    if (g_tfile == NULL || lir == NULL) return;
+    fprintf(g_tfile, "\n");
+    dump_lir2(lir, df, pos);
 }
 
 
 //Dump LIR stmts stored in fu->lirList array.
 void dump_lir2(LIR * lir, DexFile * df, INT pos)
 {
-	if (g_tfile == NULL || lir == NULL) return;
-	if (pos < 0) {
-		fprintf(g_tfile, "%s", LIR_name(lir));
-	} else {
-		fprintf(g_tfile, "(%dth)%s", pos, LIR_name(lir));
-	}
-	switch (LIR_opcode(lir)) {
-	case LOP_NOP:
-		break;
-	case LOP_CONST:
-		switch (LIR_dt(lir)) {
-		case LIR_JDT_unknown:
-			fprintf(g_tfile, ", INT");
-			if (is_s4(LIR_int_imm(lir)) && LIR_res(lir) < 16) {
-				//AB
-				fprintf(g_tfile, ", v%d <- %d",
-						LIR_res(lir), (INT)LIR_int_imm(lir));
-			} else if (is_s16(LIR_int_imm(lir))) {
-				//AABBBB
-				fprintf(g_tfile, ", v%d <- %d",
-						LIR_res(lir), (INT)LIR_int_imm(lir));
-			} else {
-				//AABBBBBBBB
-				fprintf(g_tfile, ", v%d <- %d",
-						LIR_res(lir), (INT)LIR_int_imm(lir));
-			}
-			break;
-		case LIR_JDT_wide:
-			fprintf(g_tfile, ", %s", get_dt_name(lir));
-			if (is_swide16(LIR_int_imm(lir))) {
-				//AABBBB
-				fprintf(g_tfile, ", (v%d,v%d) <- %d",
-						LIR_res(lir), LIR_res(lir) + 1, (INT)LIR_int_imm(lir));
-			} else if (is_swide32(LIR_int_imm(lir))) {
-				//AABBBBBBBB
-				fprintf(g_tfile, ", (v%d,v%d) <- %d",
-						LIR_res(lir), LIR_res(lir) + 1, (INT)LIR_int_imm(lir));
-			} else {
-				//AABBBBBBBBBBBBBBBB
-				fprintf(g_tfile, ", (v%d,v%d) <- %lld",
-						LIR_res(lir), LIR_res(lir) + 1, LIR_int_imm(lir));
-			}
-			break;
-		default:
-			/* It seems dex does not distinguish
-			float and integer const. And regard float as
-			32bit integer, double will be 64bit integer. */
-			ASSERT0(0);
-		}
-		break;
-	case LOP_RETURN:
-		{
-			switch (LIR_dt(lir)) {
-			case LIR_JDT_unknown: //return preg.
-				fprintf(g_tfile, ", INT");
-				fprintf(g_tfile, ", v%d", LIR_res(lir));
-				break;
-			case LIR_JDT_void: //No return value.
-				break;
-			case LIR_JDT_object: //return object.
-				fprintf(g_tfile, ", obj_ptr:v%d", LIR_res(lir));
-				break;
-			case LIR_JDT_wide:
-				fprintf(g_tfile, ", %s", get_dt_name(lir));
-				fprintf(g_tfile, ", (v%d,v%d)", LIR_res(lir), LIR_res(lir) + 1);
-				break;
-			default: ASSERT0(0);
-			}
-		}
-		break;
-	case LOP_THROW: //AA
-		//Throws an exception object.
-		//The reference of the exception object is in vx.
-		fprintf(g_tfile, ", v%d", LIR_res(lir));
-		break;
-	case LOP_MONITOR_ENTER  :
-		fprintf(g_tfile, ", v%d", LIR_res(lir));
-		break;
-	case LOP_MONITOR_EXIT   :
-		break;
-	case LOP_MOVE_RESULT    :
-		{
-			//Move function return value to regisiter.
-			//AA
-			LIRAOp * p = (LIRAOp*)lir;
-			switch (LIR_dt(lir)) {
-			case LIR_JDT_unknown: //lexOpcode = lc_mov_result32; break;
-				fprintf(g_tfile, ", INT");
-				fprintf(g_tfile, ", retval -> v%d", LIR_res(lir));
-				break;
-			case LIR_JDT_wide: //lexOpcode = lc_mov_result64; break;
-				fprintf(g_tfile, ", %s", get_dt_name(lir));
-				fprintf(g_tfile, ", retval -> (v%d,v%d)",
-						LIR_res(lir), LIR_res(lir) + 1);
-				break;
-			case LIR_JDT_object: //lexOpcode = lc_mov_result_object; break;
-				fprintf(g_tfile, ", obj-ptr");
-				fprintf(g_tfile, ", retval -> v%d", LIR_res(lir));
-				break;
-			}
-		}
-		break;
-	case LOP_MOVE_EXCEPTION : //AA
-		fprintf(g_tfile, ", v%d", LIR_res(lir));
-		break;
-	case LOP_GOTO		    : //AABBBBBBBB
-		{
-			LIRGOTOOp * p = (LIRGOTOOp*)lir;
-			fprintf(g_tfile, ", (lirIdx)%dth", p->target);
-		}
-		break;
-	case LOP_MOVE		:
-		switch (LIR_dt(lir)) {
-		case LIR_JDT_unknown:
-			fprintf(g_tfile, ", INT");
-			if ((LIR_op0(lir) | LIR_res(lir)) < 16) {
-				//AB
-				fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-			} else if (LIR_res(lir) < 256) {
-				//AABBBB
-				fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-			} else {
-				//AAAABBBB
-				fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-			}
-			break;
-		case LIR_JDT_wide:
-			fprintf(g_tfile, ", %s", get_dt_name(lir));
-			if ((LIR_op0(lir) | LIR_res(lir)) < 16) {
-				//AB
-				fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
-						LIR_res(lir), LIR_res(lir) + 1,
-						LIR_op0(lir), LIR_op0(lir) + 1);
-			} else if (LIR_res(lir) < 256) {
-				//AABBBB
-				fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
-						LIR_res(lir), LIR_res(lir) + 1,
-						LIR_op0(lir), LIR_op0(lir) + 1);
-			} else {
-				//AAAABBBB
-				fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
-						LIR_res(lir), LIR_res(lir) + 1,
-						LIR_op0(lir), LIR_op0(lir) + 1);
-			}
-			break;
-		case LIR_JDT_object:
-			fprintf(g_tfile, ", obj-ptr");
-			if ((LIR_op0(lir) | LIR_res(lir)) < 16) {
-				//AB
-				fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-			} else if (LIR_res(lir) < 256) {
-				//AABBBB
-				fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-			} else {
-				//AAAABBBB
-				fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-			}
-			break;
-		}
-		break;
-	case LOP_NEG        : //AB
-	case LOP_NOT		: //AB
-		fprintf(g_tfile, ", %s", get_dt_name(lir));
-		if (is_wide(lir)) {
-			fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
-					LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir), LIR_op0(lir)+1);
-		} else {
-			fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-		}
-		break;
-	case LOP_CONVERT	: //AB
-		switch (LIR_dt(lir)) {
-		case LIR_convert_i2l:
-			fprintf(g_tfile, ", INT->LONG");
-			fprintf(g_tfile, ", (v%d,v%d) <- v%d",
-					LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir));
-			break;
-		case LIR_convert_i2f: fprintf(g_tfile, ", INT->FLOAT");  break;
-		case LIR_convert_i2d:
-			fprintf(g_tfile, ", INT->DOUBLE");
-			fprintf(g_tfile, ", (v%d,v%d) <- v%d",
-					LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir));
-			break;
-		case LIR_convert_l2i:
-			fprintf(g_tfile, ", LONG->INT");
-			fprintf(g_tfile, ", v%d <- (v%d,v%d)",
-					LIR_res(lir), LIR_op0(lir), LIR_op0(lir)+1);
-			break;
-		case LIR_convert_l2f:
-			fprintf(g_tfile, ", LONG->FLOAT");
-			fprintf(g_tfile, ", v%d <- (v%d,v%d)",
-					LIR_res(lir), LIR_op0(lir), LIR_op0(lir)+1);
-			break;
-		case LIR_convert_l2d:
-			fprintf(g_tfile, ", LONG->DOUBLE");
-			fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
-					LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir), LIR_op0(lir)+1);
-			break;
-		case LIR_convert_f2i: fprintf(g_tfile, ", FLOAT->INT");  break;
-		case LIR_convert_f2l:
-			fprintf(g_tfile, ", FLOAT->LONG");
-			fprintf(g_tfile, ", (v%d,v%d) <- v%d",
-					LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir));
-			break;
-		case LIR_convert_f2d:
-			fprintf(g_tfile, ", FLOAT->DOUBLE");
-			fprintf(g_tfile, ", (v%d,v%d) <- v%d",
-					LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir));
-			break;
-		case LIR_convert_d2i:
-			fprintf(g_tfile, ", DOUBLE->INT");
-			fprintf(g_tfile, ", v%d <- (v%d,v%d)",
-					LIR_res(lir), LIR_op0(lir), LIR_op0(lir)+1);
-			break;
-		case LIR_convert_d2l:
-			fprintf(g_tfile, ", DOUBLE->LONG");
-			fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
-					LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir), LIR_op0(lir)+1);
-			break;
-		case LIR_convert_d2f:
-			fprintf(g_tfile, ", DOUBLE->FLOAT");
-			fprintf(g_tfile, ", v%d <- (v%d,v%d)",
-					LIR_res(lir), LIR_op0(lir), LIR_op0(lir)+1);
-			break;
-		case LIR_convert_i2b:
-			fprintf(g_tfile, ", INT->BOOL");
-			fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-			break;
-		case LIR_convert_i2c:
-			fprintf(g_tfile, ", INT->CHAR");
-			fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-			break;
-		case LIR_convert_i2s:
-			fprintf(g_tfile, ", INT->SHORT");
-			fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-			break;
-		default:
-			ASSERT0(0);
-		}
-		break;
-	case LOP_ADD_ASSIGN :
-	case LOP_SUB_ASSIGN :
-	case LOP_MUL_ASSIGN :
-	case LOP_DIV_ASSIGN :
-	case LOP_REM_ASSIGN :
-	case LOP_AND_ASSIGN :
-	case LOP_OR_ASSIGN  :
-	case LOP_XOR_ASSIGN :
-	case LOP_SHL_ASSIGN :
-	case LOP_SHR_ASSIGN :
-	case LOP_USHR_ASSIGN:
-		fprintf(g_tfile, ", %s", get_dt_name(lir));
-		if (is_wide(lir)) {
-			fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d), (v%d,v%d)",
-					LIR_res(lir), LIR_res(lir)+1,
-					LIR_res(lir), LIR_res(lir)+1,
-					LIR_op0(lir), LIR_op0(lir)+1);
-		} else {
-			fprintf(g_tfile, ", v%d <- v%d, v%d",
-					LIR_res(lir), LIR_res(lir), LIR_op0(lir));
-		}
-		break;
-	case LOP_ARRAY_LENGTH: //AABBBB
-		//Calculates the number of elements of the array referenced by vy
-		//and puts the length value into vx.
-		fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
-		break;
-	case LOP_IFZ         :
-		//AABBBB
-		switch (LIR_dt(lir)) {
-		case LIR_cond_EQ: fprintf(g_tfile, ", =="); break;
-		case LIR_cond_NE: fprintf(g_tfile, ", !="); break;
-		case LIR_cond_LT: fprintf(g_tfile, ", <"); break;
-		case LIR_cond_GE: fprintf(g_tfile, ", >="); break;
-		case LIR_cond_GT: fprintf(g_tfile, ", >"); break;
-		case LIR_cond_LE: fprintf(g_tfile, ", <="); break;
-		}
-		if (is_wide(lir)) {
-			fprintf(g_tfile, ", (v%d,v%d), 0, (lirIdx)%dth",
-					LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir));
-		} else {
-			fprintf(g_tfile, ", v%d, 0, (lirIdx)%dth",
-					LIR_res(lir), LIR_op0(lir));
-		}
-		break;
-	case LOP_NEW_INSTANCE:
-		//AABBBB
-		//LIR_op0(lir) is class-type-id, not class-declaration-id.
-		ASSERT0(df);
-		fprintf(g_tfile, ", (obj_ptr)v%d <- (clsname<%d>)%s",
-				LIR_res(lir),
-				LIR_op0(lir),
-				get_class_name(df, LIR_op0(lir)));
-		break;
-	case LOP_CONST_STRING:
-		//AABBBB or AABBBBBBBB
-		ASSERT0(df);
-		fprintf(g_tfile, ", v%d <- (strofst<%d>)\"%s\"",
-				LIR_res(lir),
-				LIR_op0(lir),
-				dexStringById(df, LIR_op0(lir)));
-		break;
-	case LOP_CONST_CLASS :
-		//AABBBB
-		//const-class vx,type_id
-		//Moves the class object of a class identified by
-		//type_id (e.g. Object.class) into vx.
-		fprintf(g_tfile, ", v%d <- (clsname<%d>)%s",
-				LIR_res(lir),
-				LIR_op0(lir),
-				dexStringByTypeIdx(df, LIR_op0(lir)));
-		break;
-	case LOP_SGET        :
-		//AABBBB
-		fprintf(g_tfile, ", %s", get_dt_name(lir));
-		ASSERT0(df);
-		fprintf(g_tfile, ", v%d <- (ofst<%d>)%s::%s",
-				LIR_res(lir),
-				LIR_op0(lir),
-				get_field_class_name(df, LIR_op0(lir)),
-				get_field_name(df, LIR_op0(lir)));
-		break;
-	case LOP_CHECK_CAST  :
-		//AABBBB
-		ASSERT0(df);
-		fprintf(g_tfile, ", v%d '%s'",
-				LIR_res(lir),
-				dexStringByTypeIdx(df, LIR_op0(lir)));
-		break;
-	case LOP_SPUT        :
-		{
-			//AABBBB
-			LIRABOp * p = (LIRABOp*)lir;
-			fprintf(g_tfile, ", %s", get_dt_name(lir));
-			ASSERT0(df);
-			if (is_wide(lir)) {
-				fprintf(g_tfile, ", (v%d,v%d) -> %s::%s",
-						LIR_res(lir), LIR_res(lir)+1,
-						get_field_class_name(df, LIR_op0(lir)),
-						get_field_name(df, LIR_op0(lir)));
-			} else {
-				fprintf(g_tfile, ", v%d -> %s::%s",
-						LIR_res(lir),
-						get_field_class_name(df, LIR_op0(lir)),
-						get_field_name(df, LIR_op0(lir)));
-			}
-		}
-		break;
-	case LOP_APUT        :
-		//AABBCC
-		fprintf(g_tfile, ", %s", get_dt_name(lir));
-		ASSERT0(df);
-		if (is_wide(lir)) {
-			fprintf(g_tfile, ", (v%d,v%d) -> (array_base_ptr)v%d, (array_elem)v%d",
-					LIR_res(lir), LIR_res(lir)+1,
-					LIR_op0(lir), (UINT)LIR_op1(lir));
-		} else {
-			fprintf(g_tfile, ", v%d -> (array_base_ptr)v%d, (array_elem)v%d",
-					LIR_res(lir), LIR_op0(lir), (UINT)LIR_op1(lir));
-		}
-		break;
-	case LOP_AGET      :
-		//AABBCC
-		fprintf(g_tfile, ", %s", get_dt_name(lir));
-		ASSERT0(df);
-		if (is_wide(lir)) {
-			fprintf(g_tfile, ", (v%d,v%d) <- (array_base_ptr)v%d, (array_elem)v%d",
-					LIR_res(lir), LIR_res(lir)+1,
-					LIR_op0(lir), (UINT)LIR_op1(lir));
-		} else {
-			fprintf(g_tfile, ", v%d <- (array_base_ptr)v%d, (array_elem)v%d",
-					LIR_res(lir), LIR_op0(lir), (UINT)LIR_op1(lir));
-		}
-		break;
-	case LOP_CMPL      :
-	case LOP_CMP_LONG  :
-		//AABBCC
-		ASSERT0(df);
-		switch (LIR_dt(lir)) {
-		case LIR_CMP_float:
-			fprintf(g_tfile, ", FLOAT");
-			fprintf(g_tfile, ", v%d, v%d, %d",
-					LIR_res(lir), LIR_op0(lir), (UINT)LIR_op1(lir));
-			break;
-		case LIR_CMP_double:
-			fprintf(g_tfile, ", DOUBLE");
-			fprintf(g_tfile, ", (v%d,v%d), (v%d,v%d), %d",
-					LIR_res(lir), LIR_res(lir)+1,
-					LIR_op0(lir), LIR_op0(lir)+1,
-					(UINT)LIR_op1(lir));
-			break;
-		default: ASSERT0(0);
-		}
-		break;
-	case LOP_ADD       :
-	case LOP_SUB       :
-	case LOP_MUL       :
-	case LOP_DIV       :
-	case LOP_REM       :
-	case LOP_AND       :
-	case LOP_OR        :
-	case LOP_XOR       :
-	case LOP_SHL       :
-	case LOP_SHR       :
-	case LOP_USHR      :
-		{
-			fprintf(g_tfile, ", %s", get_dt_name(lir));
-			LIRABCOp * p = (LIRABCOp*)lir;
-			if (is_wide(lir)) {
-				fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d), (v%d,v%d)",
-						LIR_res(lir), LIR_res(lir)+1,
-						LIR_op0(lir), LIR_op0(lir)+1,
-						(UINT)LIR_op1(lir), (UINT)LIR_op1(lir)+1);
-			} else {
-				fprintf(g_tfile, ", v%d <- v%d, v%d",
-						LIR_res(lir), LIR_op0(lir), (UINT)LIR_op1(lir));
-			}
-		}
-		break;
-	case LOP_IF        :
-		//ABCCCC
-		switch (LIR_dt(lir)) {
-		case LIR_cond_EQ: fprintf(g_tfile, ", =="); break;
-		case LIR_cond_NE: fprintf(g_tfile, ", !="); break;
-		case LIR_cond_LT: fprintf(g_tfile, ", <"); break;
-		case LIR_cond_GE: fprintf(g_tfile, ", >="); break;
-		case LIR_cond_GT: fprintf(g_tfile, ", >"); break;
-		case LIR_cond_LE: fprintf(g_tfile, ", <="); break;
-		}
-		fprintf(g_tfile, ", v%d, v%d, (lirIdx)%dth",
-				LIR_res(lir), LIR_op0(lir), (UINT)LIR_op1(lir));
-		break;
-	case LOP_ADD_LIT   : //AABBCC, AABBCCCC
-	case LOP_SUB_LIT   : //AABBCC, AABBCCCC
-	case LOP_MUL_LIT   : //AABBCC, AABBCCCC
-	case LOP_DIV_LIT   : //AABBCC, AABBCCCC
-	case LOP_REM_LIT   : //AABBCC, AABBCCCC
-	case LOP_AND_LIT   : //AABBCC, AABBCCCC
-	case LOP_OR_LIT    : //AABBCC, AABBCCCC
-	case LOP_XOR_LIT   : //AABBCC, AABBCCCC
-	case LOP_SHL_LIT   : //AABBCC
-	case LOP_SHR_LIT   : //AABBCC
-	case LOP_USHR_LIT   : //AABBCC
-		{
-			fprintf(g_tfile, ", %s", get_dt_name(lir));
-			LIRABCOp * p = (LIRABCOp*)lir;
-			if (is_wide(lir)) {
-				fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d),",
-						LIR_res(lir), LIR_res(lir)+1,
-						LIR_op0(lir), LIR_op0(lir)+1);
-			} else {
-				fprintf(g_tfile, ", v%d <- v%d,",
-						LIR_res(lir), LIR_op0(lir));
-			}
-
-			if (is_s8((INT)LIR_op1(lir))) {
-				//8bit imm
-				fprintf(g_tfile, "(lit8)%d", (INT)LIR_op1(lir));
-			} else if (is_s16((INT)LIR_op1(lir))) {
-				//16bit imm
-				fprintf(g_tfile, "(lit16)%d", (INT)LIR_op1(lir));
-			} else {
-				ASSERT0(0);
-			}
-		}
-		break;
-	case LOP_IPUT       :
-		fprintf(g_tfile, ", %s", get_dt_name(lir));
-		//ABCCCC
-		ASSERT0(df);
-		if (is_wide(lir)) {
-			fprintf(g_tfile, ", (v%d,v%d) -> (obj_ptr)v%d, (ofst<%d>)%s::%s",
-					LIR_res(lir), LIR_res(lir)+1,
-					LIR_op0(lir), (UINT)LIR_op1(lir),
-					get_field_class_name(df, (UINT)LIR_op1(lir)),
-					get_field_name(df, (UINT)LIR_op1(lir)));
-		} else {
-			fprintf(g_tfile, ", v%d -> (obj_ptr)v%d, (ofst<%d>)%s::%s",
-					LIR_res(lir),
-					LIR_op0(lir),
-					(UINT)LIR_op1(lir),
-					get_field_class_name(df, (UINT)LIR_op1(lir)),
-					get_field_name(df, (UINT)LIR_op1(lir)));
-		}
-		break;
-	case LOP_IGET       :
-		fprintf(g_tfile, ", %s", get_dt_name(lir));
-		//ABCCCC
-		ASSERT0(df);
-		if (is_wide(lir)) {
-			fprintf(g_tfile, ", (v%d,v%d) <- (obj_ptr)v%d, (ofst<%d>)%s::%s",
-					LIR_res(lir), LIR_res(lir)+1,
-					LIR_op0(lir),
-					(UINT)LIR_op1(lir),
-					get_field_class_name(df, (UINT)LIR_op1(lir)),
-					get_field_name(df, (UINT)LIR_op1(lir)));
-		} else {
-			fprintf(g_tfile, ", v%d <- (obj_ptr)v%d, (ofst<%d>)%s::%s",
-					LIR_res(lir),
-					LIR_op0(lir),
-					(UINT)LIR_op1(lir),
-					get_field_class_name(df, (UINT)LIR_op1(lir)),
-					get_field_name(df, (UINT)LIR_op1(lir)));
-		}
-		break;
-	case LOP_INSTANCE_OF:
-		fprintf(g_tfile, ", (pred)v%d <- v%d, (clsname<%d>)'%s'",
-				LIR_res(lir),
-				LIR_op0(lir),
-				(UINT)LIR_op1(lir),
-				dexStringByTypeIdx(df, (UINT)LIR_op1(lir)));
-		break;
-	case LOP_NEW_ARRAY  :
-		//ABCCCC
-		//new-array v%d(res) <- v%d(op0), LCAnimal(op1)
-		fprintf(g_tfile, ", %s", get_dt_name(lir));
-		//ABCCCC
-		ASSERT0(df);
-		fprintf(g_tfile, ", v%d <- (num_of_elem)v%d, (elem_type<%d>)'%s'",
-				LIR_res(lir),
-				LIR_op0(lir),
-				(UINT)LIR_op1(lir),
-				dexStringByTypeIdx(df, (UINT)LIR_op1(lir)));
-		break;
-	case LOP_TABLE_SWITCH:
-		{
-			LIRSwitchOp * p = (LIRSwitchOp*)lir;
-			ASSERT0(LIR_dt(p) == 0x1);
-			fprintf(g_tfile, ", v%d", p->value);
-			USHORT * pdata = p->data;
-
-			//data[0]: flag to indicate switch-table type:
-			//	0x1 TABLE_SWITCH, 0x2 LOOKUP_SWITCH
-			USHORT f = pdata[0];
-			ASSERT0(f == 0x100);
-			//data[1]: the number of CASE entry.
-			USHORT num_of_case = pdata[1];
-
-			//data[2..3]: the base value of case-table
-			UINT base_val = *((UINT*)(&pdata[2]));
-			fprintf(g_tfile, ", basev:%d", base_val);
-
-			//((BYTE*)data)[4..num_of_case*4]:
-			//	the position of the index table is at current instruction.
-			if (num_of_case > 0) {
-				UINT * pcase_entry = (UINT*)&pdata[4];
-				fprintf(g_tfile, " tgtidx:");
-				for (USHORT i = 0; i < num_of_case; i++) {
-					UINT idx_of_insn = pcase_entry[i];
-					fprintf(g_tfile, "%d", idx_of_insn);
-					if (i != num_of_case - 1) {
-						fprintf(g_tfile, ",");
-					}
-				}
-			}
-		}
-		break;
-	case LOP_LOOKUP_SWITCH:
-		{
-			LIRSwitchOp * p = (LIRSwitchOp*)lir;
-			ASSERT0(LIR_dt(p) == 0x2);
-			fprintf(g_tfile, ", v%d", p->value);
-			USHORT * pdata = p->data;
-
-			//pdata[0]: flag to indicate switch-table type:
-			//	0x1 TABLE_SWITCH, 0x2 LOOKUP_SWITCH
-			UINT f = pdata[0];
-			ASSERT0(f == 0x200);
-
-			//pdata[1]: the number of CASE entry.
-			UINT num_of_case = pdata[1];
-			if (num_of_case > 0) {
-				BYTE * tp = (BYTE*)pdata;
-				//((BYTE*)pdata)[4..4+num_of_case*4-1]: the case-value buffer.
-				UINT * pcase_value = (UINT*)&tp[4];
-
-				//((BYTE*)pdata)[4+num_of_case*4, 4+num_of_case*8-1]:
-				//	the position of the index table is at current instruction.
-				UINT * pcase_entry = (UINT*)&tp[4 + num_of_case * 4];
-				fprintf(g_tfile, " val2idx(");
-				for (UINT i = 0; i < num_of_case; i++) {
-					UINT idx_of_insn = pcase_entry[i];
-					fprintf(g_tfile, "%d:%d", pcase_value[i], idx_of_insn);
-					if (i != num_of_case - 1) {
-						fprintf(g_tfile, ",");
-					}
-				}
-				fprintf(g_tfile, ")");
-			}
-		}
-		break;
-	case LOP_FILL_ARRAY_DATA:
-		{
-			fprintf(g_tfile, ", %s", get_dt_name(lir));
-			//AABBBBBBBB
-			//pdata[0]: the magic number of code
-			//0x100 PACKED_SWITCH, 0x200 SPARSE_SWITCH, 0x300 FILL_ARRAY_DATA
-			LIRSwitchOp * r = (LIRSwitchOp*)lir;
-			UInt16 const* pdata = (UInt16 const*)r->data;
-			ASSERT0(pdata[0] == 0x300);
-			//pdata[1]: size of each element.
-			//pdata[2]: the number of element.
-			UINT size_of_elem = pdata[1];
-			UINT num_of_elem = pdata[2];
-			UINT data_size = num_of_elem * size_of_elem;
-			//fprintf(g_tfile, ", (elem_sz<%d>), (elem_num<%d>), (data_ptr)0x%x",
-			fprintf(g_tfile, ", (elem_sz<%d>), (elem_num<%d>)",
-					size_of_elem, num_of_elem);
-		}
-		break;
-	case LOP_INVOKE:
-		{
-			/*
-			ABCCCCDDDD the layout clarifies:
-				A(p4), B(argc), CCCC(method_id), DDDD(p0, p1, p2, p3)
-				where p0,p1,p2,p3,p4 are five parameters.
-
-			AABBBBCCCC the layout clarifies:
-				AA(argc), BBBB(method_id), CCCC(p0,p1,...p(argc-1))
-			*/
-			LIRInvokeOp * r = (LIRInvokeOp*)lir;
-			UINT flags = LIR_dt(lir);
-			UINT flag1 = flags & 0x0F;
-			UINT flag2 = flags & 0xF0;
-			ASSERT(flag1 != 0, ("means LIR is LOP_FILLED_NEW_ARRAY"));
-			DexMethodId const* method_id = dexGetMethodId(df, r->ref);
-			ASSERT0(method_id);
-			CHAR const* method_name = dexStringById(df, method_id->nameIdx);
-			CHAR const* class_name =
-				dexStringByTypeIdx(df, method_id->classIdx);
-			ASSERT0(method_name);
-			DexProtoId const* proto_id =
-				dexGetProtoId(df, method_id->protoIdx);
-			CHAR const* shorty_name = dexStringById(df, proto_id->shortyIdx);
-			fprintf(g_tfile, ", %s::%s", class_name, method_name);
-
-			UINT k = LIR_dt(lir);
-			bool is_range = HAVE_FLAG((k & 0xf0), LIR_Range);
-			if (is_range) {
-				switch (k & 0x0f) {
-				case LIR_invoke_unknown: ASSERT0(0); break;
-				case LIR_invoke_virtual:
-					fprintf(g_tfile, ", virtual-range"); break;
-				case LIR_invoke_direct:
-					fprintf(g_tfile, ", direct-range"); break;
-				case LIR_invoke_super:
-					fprintf(g_tfile, ", super-range"); break;
-				case LIR_invoke_interface:
-					fprintf(g_tfile, ", interface-range"); break;
-				case LIR_invoke_static:
-					fprintf(g_tfile, ", static-range"); break;
-				default: ASSERT0(0);
-				}
-			} else {
-				switch (k & 0x0f) {
-				case LIR_invoke_unknown: ASSERT0(0); break;
-				case LIR_invoke_virtual:
-					fprintf(g_tfile, ", virtual"); break;
-				case LIR_invoke_direct:
-					fprintf(g_tfile, ", direct"); break;
-				case LIR_invoke_super:
-					fprintf(g_tfile, ", super"); break;
-				case LIR_invoke_interface:
-					fprintf(g_tfile, ", interface"); break;
-				case LIR_invoke_static:
-					fprintf(g_tfile, ", static"); break;
-				default: ASSERT0(0);
-				}
-			}
-
-			if (r->argc != 0) {
-				fprintf(g_tfile, ", arg(");
-				for (USHORT i = 0; i < r->argc; i++) {
-					fprintf(g_tfile, "v%d", r->args[i]);
-					if (i != r->argc-1) {
-						fprintf(g_tfile, ",");
-					}
-				}
-				fprintf(g_tfile, ")");
-			}
-		}
-		break;
-	case LOP_FILLED_NEW_ARRAY:
-		{
-			/*
-			AABBBBCCCC or ABCCCCDEFG
-			e.g:
-				A(argc), B,D,E,F,G(parampters), CCCC(class_tyid)
-			*/
-			LIRInvokeOp * r = (LIRInvokeOp*)lir;
-			UINT flags = LIR_dt(lir);
-			CHAR const* class_name = dexStringByTypeIdx(df, r->ref);
-			ASSERT0(class_name);
-			fprintf(g_tfile, ", %s", class_name);
-			if (r->argc != 0) {
-				fprintf(g_tfile, ", arg(");
-				for (USHORT i = 0; i < r->argc; i++) {
-					fprintf(g_tfile, "v%d", r->args[i]);
-					if (i != r->argc-1) {
-						fprintf(g_tfile, ",");
-					}
-				}
-				fprintf(g_tfile, ")");
-			}
-		}
-		break;
-	case LOP_CMPG:
-		//AABBCC
-		ASSERT0(df);
-		switch (LIR_dt(lir)) {
-		case LIR_CMP_float:
-			fprintf(g_tfile, ", FLOAT");
-			fprintf(g_tfile, ", v%d, v%d, %d",
-					LIR_res(lir),
-					LIR_op0(lir),
-					(INT)LIR_op1(lir));
-			break;
-		case LIR_CMP_double:
-			fprintf(g_tfile, ", DOUBLE");
-			fprintf(g_tfile, ", (v%d,v%d), v%d, %d",
-					LIR_res(lir), LIR_res(lir)+1,
-					LIR_op0(lir),
-					(INT)LIR_op1(lir));
-			break;
-		default: ASSERT0(0);
-		}
-		break;
-	case LOP_PHI:
-		ASSERT0(0);
-		break;
-	default: ASSERT0(0);
-	} //end switch
-	fflush(g_tfile);
+    if (g_tfile == NULL || lir == NULL) return;
+    if (pos < 0) {
+        fprintf(g_tfile, "%s", LIR_name(lir));
+    } else {
+        fprintf(g_tfile, "(%dth)%s", pos, LIR_name(lir));
+    }
+    switch (LIR_opcode(lir)) {
+    case LOP_NOP:
+        break;
+    case LOP_CONST:
+        switch (LIR_dt(lir)) {
+        case LIR_JDT_unknown:
+            fprintf(g_tfile, ", INT");
+            if (is_s4(LIR_int_imm(lir)) && LIR_res(lir) < 16) {
+                //AB
+                fprintf(g_tfile, ", v%d <- %d",
+                        LIR_res(lir), (INT)LIR_int_imm(lir));
+            } else if (is_s16(LIR_int_imm(lir))) {
+                //AABBBB
+                fprintf(g_tfile, ", v%d <- %d",
+                        LIR_res(lir), (INT)LIR_int_imm(lir));
+            } else {
+                //AABBBBBBBB
+                fprintf(g_tfile, ", v%d <- %d",
+                        LIR_res(lir), (INT)LIR_int_imm(lir));
+            }
+            break;
+        case LIR_JDT_wide:
+            fprintf(g_tfile, ", %s", get_dt_name(lir));
+            if (is_swide16(LIR_int_imm(lir))) {
+                //AABBBB
+                fprintf(g_tfile, ", (v%d,v%d) <- %d",
+                        LIR_res(lir), LIR_res(lir) + 1, (INT)LIR_int_imm(lir));
+            } else if (is_swide32(LIR_int_imm(lir))) {
+                //AABBBBBBBB
+                fprintf(g_tfile, ", (v%d,v%d) <- %d",
+                        LIR_res(lir), LIR_res(lir) + 1, (INT)LIR_int_imm(lir));
+            } else {
+                //AABBBBBBBBBBBBBBBB
+                fprintf(g_tfile, ", (v%d,v%d) <- %lld",
+                        LIR_res(lir), LIR_res(lir) + 1, LIR_int_imm(lir));
+            }
+            break;
+        default:
+            /* It seems dex does not distinguish
+            float and integer const. And regard float as
+            32bit integer, double will be 64bit integer. */
+            ASSERT0(0);
+        }
+        break;
+    case LOP_RETURN:
+        {
+            switch (LIR_dt(lir)) {
+            case LIR_JDT_unknown: //return preg.
+                fprintf(g_tfile, ", INT");
+                fprintf(g_tfile, ", v%d", LIR_res(lir));
+                break;
+            case LIR_JDT_void: //No return value.
+                break;
+            case LIR_JDT_object: //return object.
+                fprintf(g_tfile, ", obj_ptr:v%d", LIR_res(lir));
+                break;
+            case LIR_JDT_wide:
+                fprintf(g_tfile, ", %s", get_dt_name(lir));
+                fprintf(g_tfile, ", (v%d,v%d)", LIR_res(lir), LIR_res(lir) + 1);
+                break;
+            default: ASSERT0(0);
+            }
+        }
+        break;
+    case LOP_THROW: //AA
+        //Throws an exception object.
+        //The reference of the exception object is in vx.
+        fprintf(g_tfile, ", v%d", LIR_res(lir));
+        break;
+    case LOP_MONITOR_ENTER  :
+        fprintf(g_tfile, ", v%d", LIR_res(lir));
+        break;
+    case LOP_MONITOR_EXIT   :
+        break;
+    case LOP_MOVE_RESULT    :
+        {
+            //Move function return value to regisiter.
+            //AA
+            LIRAOp * p = (LIRAOp*)lir;
+            switch (LIR_dt(lir)) {
+            case LIR_JDT_unknown: //lexOpcode = lc_mov_result32; break;
+                fprintf(g_tfile, ", INT");
+                fprintf(g_tfile, ", retval -> v%d", LIR_res(lir));
+                break;
+            case LIR_JDT_wide: //lexOpcode = lc_mov_result64; break;
+                fprintf(g_tfile, ", %s", get_dt_name(lir));
+                fprintf(g_tfile, ", retval -> (v%d,v%d)",
+                        LIR_res(lir), LIR_res(lir) + 1);
+                break;
+            case LIR_JDT_object: //lexOpcode = lc_mov_result_object; break;
+                fprintf(g_tfile, ", obj-ptr");
+                fprintf(g_tfile, ", retval -> v%d", LIR_res(lir));
+                break;
+            }
+        }
+        break;
+    case LOP_MOVE_EXCEPTION : //AA
+        fprintf(g_tfile, ", v%d", LIR_res(lir));
+        break;
+    case LOP_GOTO            : //AABBBBBBBB
+        {
+            LIRGOTOOp * p = (LIRGOTOOp*)lir;
+            fprintf(g_tfile, ", (lirIdx)%dth", p->target);
+        }
+        break;
+    case LOP_MOVE        :
+        switch (LIR_dt(lir)) {
+        case LIR_JDT_unknown:
+            fprintf(g_tfile, ", INT");
+            if ((LIR_op0(lir) | LIR_res(lir)) < 16) {
+                //AB
+                fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+            } else if (LIR_res(lir) < 256) {
+                //AABBBB
+                fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+            } else {
+                //AAAABBBB
+                fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+            }
+            break;
+        case LIR_JDT_wide:
+            fprintf(g_tfile, ", %s", get_dt_name(lir));
+            if ((LIR_op0(lir) | LIR_res(lir)) < 16) {
+                //AB
+                fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
+                        LIR_res(lir), LIR_res(lir) + 1,
+                        LIR_op0(lir), LIR_op0(lir) + 1);
+            } else if (LIR_res(lir) < 256) {
+                //AABBBB
+                fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
+                        LIR_res(lir), LIR_res(lir) + 1,
+                        LIR_op0(lir), LIR_op0(lir) + 1);
+            } else {
+                //AAAABBBB
+                fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
+                        LIR_res(lir), LIR_res(lir) + 1,
+                        LIR_op0(lir), LIR_op0(lir) + 1);
+            }
+            break;
+        case LIR_JDT_object:
+            fprintf(g_tfile, ", obj-ptr");
+            if ((LIR_op0(lir) | LIR_res(lir)) < 16) {
+                //AB
+                fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+            } else if (LIR_res(lir) < 256) {
+                //AABBBB
+                fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+            } else {
+                //AAAABBBB
+                fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+            }
+            break;
+        }
+        break;
+    case LOP_NEG        : //AB
+    case LOP_NOT        : //AB
+        fprintf(g_tfile, ", %s", get_dt_name(lir));
+        if (is_wide(lir)) {
+            fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
+                    LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir), LIR_op0(lir)+1);
+        } else {
+            fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+        }
+        break;
+    case LOP_CONVERT    : //AB
+        switch (LIR_dt(lir)) {
+        case LIR_convert_i2l:
+            fprintf(g_tfile, ", INT->LONG");
+            fprintf(g_tfile, ", (v%d,v%d) <- v%d",
+                    LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir));
+            break;
+        case LIR_convert_i2f: fprintf(g_tfile, ", INT->FLOAT");  break;
+        case LIR_convert_i2d:
+            fprintf(g_tfile, ", INT->DOUBLE");
+            fprintf(g_tfile, ", (v%d,v%d) <- v%d",
+                    LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir));
+            break;
+        case LIR_convert_l2i:
+            fprintf(g_tfile, ", LONG->INT");
+            fprintf(g_tfile, ", v%d <- (v%d,v%d)",
+                    LIR_res(lir), LIR_op0(lir), LIR_op0(lir)+1);
+            break;
+        case LIR_convert_l2f:
+            fprintf(g_tfile, ", LONG->FLOAT");
+            fprintf(g_tfile, ", v%d <- (v%d,v%d)",
+                    LIR_res(lir), LIR_op0(lir), LIR_op0(lir)+1);
+            break;
+        case LIR_convert_l2d:
+            fprintf(g_tfile, ", LONG->DOUBLE");
+            fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
+                    LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir), LIR_op0(lir)+1);
+            break;
+        case LIR_convert_f2i: fprintf(g_tfile, ", FLOAT->INT");  break;
+        case LIR_convert_f2l:
+            fprintf(g_tfile, ", FLOAT->LONG");
+            fprintf(g_tfile, ", (v%d,v%d) <- v%d",
+                    LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir));
+            break;
+        case LIR_convert_f2d:
+            fprintf(g_tfile, ", FLOAT->DOUBLE");
+            fprintf(g_tfile, ", (v%d,v%d) <- v%d",
+                    LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir));
+            break;
+        case LIR_convert_d2i:
+            fprintf(g_tfile, ", DOUBLE->INT");
+            fprintf(g_tfile, ", v%d <- (v%d,v%d)",
+                    LIR_res(lir), LIR_op0(lir), LIR_op0(lir)+1);
+            break;
+        case LIR_convert_d2l:
+            fprintf(g_tfile, ", DOUBLE->LONG");
+            fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d)",
+                    LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir), LIR_op0(lir)+1);
+            break;
+        case LIR_convert_d2f:
+            fprintf(g_tfile, ", DOUBLE->FLOAT");
+            fprintf(g_tfile, ", v%d <- (v%d,v%d)",
+                    LIR_res(lir), LIR_op0(lir), LIR_op0(lir)+1);
+            break;
+        case LIR_convert_i2b:
+            fprintf(g_tfile, ", INT->BOOL");
+            fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+            break;
+        case LIR_convert_i2c:
+            fprintf(g_tfile, ", INT->CHAR");
+            fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+            break;
+        case LIR_convert_i2s:
+            fprintf(g_tfile, ", INT->SHORT");
+            fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+            break;
+        default:
+            ASSERT0(0);
+        }
+        break;
+    case LOP_ADD_ASSIGN :
+    case LOP_SUB_ASSIGN :
+    case LOP_MUL_ASSIGN :
+    case LOP_DIV_ASSIGN :
+    case LOP_REM_ASSIGN :
+    case LOP_AND_ASSIGN :
+    case LOP_OR_ASSIGN  :
+    case LOP_XOR_ASSIGN :
+    case LOP_SHL_ASSIGN :
+    case LOP_SHR_ASSIGN :
+    case LOP_USHR_ASSIGN:
+        fprintf(g_tfile, ", %s", get_dt_name(lir));
+        if (is_wide(lir)) {
+            fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d), (v%d,v%d)",
+                    LIR_res(lir), LIR_res(lir)+1,
+                    LIR_res(lir), LIR_res(lir)+1,
+                    LIR_op0(lir), LIR_op0(lir)+1);
+        } else {
+            fprintf(g_tfile, ", v%d <- v%d, v%d",
+                    LIR_res(lir), LIR_res(lir), LIR_op0(lir));
+        }
+        break;
+    case LOP_ARRAY_LENGTH: //AABBBB
+        //Calculates the number of elements of the array referenced by vy
+        //and puts the length value into vx.
+        fprintf(g_tfile, ", v%d <- v%d", LIR_res(lir), LIR_op0(lir));
+        break;
+    case LOP_IFZ         :
+        //AABBBB
+        switch (LIR_dt(lir)) {
+        case LIR_cond_EQ: fprintf(g_tfile, ", =="); break;
+        case LIR_cond_NE: fprintf(g_tfile, ", !="); break;
+        case LIR_cond_LT: fprintf(g_tfile, ", <"); break;
+        case LIR_cond_GE: fprintf(g_tfile, ", >="); break;
+        case LIR_cond_GT: fprintf(g_tfile, ", >"); break;
+        case LIR_cond_LE: fprintf(g_tfile, ", <="); break;
+        }
+        if (is_wide(lir)) {
+            fprintf(g_tfile, ", (v%d,v%d), 0, (lirIdx)%dth",
+                    LIR_res(lir), LIR_res(lir)+1, LIR_op0(lir));
+        } else {
+            fprintf(g_tfile, ", v%d, 0, (lirIdx)%dth",
+                    LIR_res(lir), LIR_op0(lir));
+        }
+        break;
+    case LOP_NEW_INSTANCE:
+        //AABBBB
+        //LIR_op0(lir) is class-type-id, not class-declaration-id.
+        ASSERT0(df);
+        fprintf(g_tfile, ", (obj_ptr)v%d <- (clsname<%d>)%s",
+                LIR_res(lir),
+                LIR_op0(lir),
+                get_class_name(df, LIR_op0(lir)));
+        break;
+    case LOP_CONST_STRING:
+        //AABBBB or AABBBBBBBB
+        ASSERT0(df);
+        fprintf(g_tfile, ", v%d <- (strofst<%d>)\"%s\"",
+                LIR_res(lir),
+                LIR_op0(lir),
+                dexStringById(df, LIR_op0(lir)));
+        break;
+    case LOP_CONST_CLASS :
+        //AABBBB
+        //const-class vx,type_id
+        //Moves the class object of a class identified by
+        //type_id (e.g. Object.class) into vx.
+        fprintf(g_tfile, ", v%d <- (clsname<%d>)%s",
+                LIR_res(lir),
+                LIR_op0(lir),
+                dexStringByTypeIdx(df, LIR_op0(lir)));
+        break;
+    case LOP_SGET        :
+        //AABBBB
+        fprintf(g_tfile, ", %s", get_dt_name(lir));
+        ASSERT0(df);
+        fprintf(g_tfile, ", v%d <- (ofst<%d>)%s::%s",
+                LIR_res(lir),
+                LIR_op0(lir),
+                get_field_class_name(df, LIR_op0(lir)),
+                get_field_name(df, LIR_op0(lir)));
+        break;
+    case LOP_CHECK_CAST  :
+        //AABBBB
+        ASSERT0(df);
+        fprintf(g_tfile, ", v%d '%s'",
+                LIR_res(lir),
+                dexStringByTypeIdx(df, LIR_op0(lir)));
+        break;
+    case LOP_SPUT        :
+        {
+            //AABBBB
+            LIRABOp * p = (LIRABOp*)lir;
+            fprintf(g_tfile, ", %s", get_dt_name(lir));
+            ASSERT0(df);
+            if (is_wide(lir)) {
+                fprintf(g_tfile, ", (v%d,v%d) -> %s::%s",
+                        LIR_res(lir), LIR_res(lir)+1,
+                        get_field_class_name(df, LIR_op0(lir)),
+                        get_field_name(df, LIR_op0(lir)));
+            } else {
+                fprintf(g_tfile, ", v%d -> %s::%s",
+                        LIR_res(lir),
+                        get_field_class_name(df, LIR_op0(lir)),
+                        get_field_name(df, LIR_op0(lir)));
+            }
+        }
+        break;
+    case LOP_APUT        :
+        //AABBCC
+        fprintf(g_tfile, ", %s", get_dt_name(lir));
+        ASSERT0(df);
+        if (is_wide(lir)) {
+            fprintf(g_tfile, ", (v%d,v%d) -> (array_base_ptr)v%d, (array_elem)v%d",
+                    LIR_res(lir), LIR_res(lir)+1,
+                    LIR_op0(lir), (UINT)LIR_op1(lir));
+        } else {
+            fprintf(g_tfile, ", v%d -> (array_base_ptr)v%d, (array_elem)v%d",
+                    LIR_res(lir), LIR_op0(lir), (UINT)LIR_op1(lir));
+        }
+        break;
+    case LOP_AGET      :
+        //AABBCC
+        fprintf(g_tfile, ", %s", get_dt_name(lir));
+        ASSERT0(df);
+        if (is_wide(lir)) {
+            fprintf(g_tfile, ", (v%d,v%d) <- (array_base_ptr)v%d, (array_elem)v%d",
+                    LIR_res(lir), LIR_res(lir)+1,
+                    LIR_op0(lir), (UINT)LIR_op1(lir));
+        } else {
+            fprintf(g_tfile, ", v%d <- (array_base_ptr)v%d, (array_elem)v%d",
+                    LIR_res(lir), LIR_op0(lir), (UINT)LIR_op1(lir));
+        }
+        break;
+    case LOP_CMPL      :
+    case LOP_CMP_LONG  :
+        //AABBCC
+        ASSERT0(df);
+        switch (LIR_dt(lir)) {
+        case LIR_CMP_float:
+            fprintf(g_tfile, ", FLOAT");
+            fprintf(g_tfile, ", v%d, v%d, %d",
+                    LIR_res(lir), LIR_op0(lir), (UINT)LIR_op1(lir));
+            break;
+        case LIR_CMP_double:
+            fprintf(g_tfile, ", DOUBLE");
+            fprintf(g_tfile, ", (v%d,v%d), (v%d,v%d), %d",
+                    LIR_res(lir), LIR_res(lir)+1,
+                    LIR_op0(lir), LIR_op0(lir)+1,
+                    (UINT)LIR_op1(lir));
+            break;
+        default: ASSERT0(0);
+        }
+        break;
+    case LOP_ADD       :
+    case LOP_SUB       :
+    case LOP_MUL       :
+    case LOP_DIV       :
+    case LOP_REM       :
+    case LOP_AND       :
+    case LOP_OR        :
+    case LOP_XOR       :
+    case LOP_SHL       :
+    case LOP_SHR       :
+    case LOP_USHR      :
+        {
+            fprintf(g_tfile, ", %s", get_dt_name(lir));
+            LIRABCOp * p = (LIRABCOp*)lir;
+            if (is_wide(lir)) {
+                fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d), (v%d,v%d)",
+                        LIR_res(lir), LIR_res(lir)+1,
+                        LIR_op0(lir), LIR_op0(lir)+1,
+                        (UINT)LIR_op1(lir), (UINT)LIR_op1(lir)+1);
+            } else {
+                fprintf(g_tfile, ", v%d <- v%d, v%d",
+                        LIR_res(lir), LIR_op0(lir), (UINT)LIR_op1(lir));
+            }
+        }
+        break;
+    case LOP_IF        :
+        //ABCCCC
+        switch (LIR_dt(lir)) {
+        case LIR_cond_EQ: fprintf(g_tfile, ", =="); break;
+        case LIR_cond_NE: fprintf(g_tfile, ", !="); break;
+        case LIR_cond_LT: fprintf(g_tfile, ", <"); break;
+        case LIR_cond_GE: fprintf(g_tfile, ", >="); break;
+        case LIR_cond_GT: fprintf(g_tfile, ", >"); break;
+        case LIR_cond_LE: fprintf(g_tfile, ", <="); break;
+        }
+        fprintf(g_tfile, ", v%d, v%d, (lirIdx)%dth",
+                LIR_res(lir), LIR_op0(lir), (UINT)LIR_op1(lir));
+        break;
+    case LOP_ADD_LIT   : //AABBCC, AABBCCCC
+    case LOP_SUB_LIT   : //AABBCC, AABBCCCC
+    case LOP_MUL_LIT   : //AABBCC, AABBCCCC
+    case LOP_DIV_LIT   : //AABBCC, AABBCCCC
+    case LOP_REM_LIT   : //AABBCC, AABBCCCC
+    case LOP_AND_LIT   : //AABBCC, AABBCCCC
+    case LOP_OR_LIT    : //AABBCC, AABBCCCC
+    case LOP_XOR_LIT   : //AABBCC, AABBCCCC
+    case LOP_SHL_LIT   : //AABBCC
+    case LOP_SHR_LIT   : //AABBCC
+    case LOP_USHR_LIT   : //AABBCC
+        {
+            fprintf(g_tfile, ", %s", get_dt_name(lir));
+            LIRABCOp * p = (LIRABCOp*)lir;
+            if (is_wide(lir)) {
+                fprintf(g_tfile, ", (v%d,v%d) <- (v%d,v%d),",
+                        LIR_res(lir), LIR_res(lir)+1,
+                        LIR_op0(lir), LIR_op0(lir)+1);
+            } else {
+                fprintf(g_tfile, ", v%d <- v%d,",
+                        LIR_res(lir), LIR_op0(lir));
+            }
+
+            if (is_s8((INT)LIR_op1(lir))) {
+                //8bit imm
+                fprintf(g_tfile, "(lit8)%d", (INT)LIR_op1(lir));
+            } else if (is_s16((INT)LIR_op1(lir))) {
+                //16bit imm
+                fprintf(g_tfile, "(lit16)%d", (INT)LIR_op1(lir));
+            } else {
+                ASSERT0(0);
+            }
+        }
+        break;
+    case LOP_IPUT       :
+        fprintf(g_tfile, ", %s", get_dt_name(lir));
+        //ABCCCC
+        ASSERT0(df);
+        if (is_wide(lir)) {
+            fprintf(g_tfile, ", (v%d,v%d) -> (obj_ptr)v%d, (ofst<%d>)%s::%s",
+                    LIR_res(lir), LIR_res(lir)+1,
+                    LIR_op0(lir), (UINT)LIR_op1(lir),
+                    get_field_class_name(df, (UINT)LIR_op1(lir)),
+                    get_field_name(df, (UINT)LIR_op1(lir)));
+        } else {
+            fprintf(g_tfile, ", v%d -> (obj_ptr)v%d, (ofst<%d>)%s::%s",
+                    LIR_res(lir),
+                    LIR_op0(lir),
+                    (UINT)LIR_op1(lir),
+                    get_field_class_name(df, (UINT)LIR_op1(lir)),
+                    get_field_name(df, (UINT)LIR_op1(lir)));
+        }
+        break;
+    case LOP_IGET       :
+        fprintf(g_tfile, ", %s", get_dt_name(lir));
+        //ABCCCC
+        ASSERT0(df);
+        if (is_wide(lir)) {
+            fprintf(g_tfile, ", (v%d,v%d) <- (obj_ptr)v%d, (ofst<%d>)%s::%s",
+                    LIR_res(lir), LIR_res(lir)+1,
+                    LIR_op0(lir),
+                    (UINT)LIR_op1(lir),
+                    get_field_class_name(df, (UINT)LIR_op1(lir)),
+                    get_field_name(df, (UINT)LIR_op1(lir)));
+        } else {
+            fprintf(g_tfile, ", v%d <- (obj_ptr)v%d, (ofst<%d>)%s::%s",
+                    LIR_res(lir),
+                    LIR_op0(lir),
+                    (UINT)LIR_op1(lir),
+                    get_field_class_name(df, (UINT)LIR_op1(lir)),
+                    get_field_name(df, (UINT)LIR_op1(lir)));
+        }
+        break;
+    case LOP_INSTANCE_OF:
+        fprintf(g_tfile, ", (pred)v%d <- v%d, (clsname<%d>)'%s'",
+                LIR_res(lir),
+                LIR_op0(lir),
+                (UINT)LIR_op1(lir),
+                dexStringByTypeIdx(df, (UINT)LIR_op1(lir)));
+        break;
+    case LOP_NEW_ARRAY  :
+        //ABCCCC
+        //new-array v%d(res) <- v%d(op0), LCAnimal(op1)
+        fprintf(g_tfile, ", %s", get_dt_name(lir));
+        //ABCCCC
+        ASSERT0(df);
+        fprintf(g_tfile, ", v%d <- (num_of_elem)v%d, (elem_type<%d>)'%s'",
+                LIR_res(lir),
+                LIR_op0(lir),
+                (UINT)LIR_op1(lir),
+                dexStringByTypeIdx(df, (UINT)LIR_op1(lir)));
+        break;
+    case LOP_TABLE_SWITCH:
+        {
+            LIRSwitchOp * p = (LIRSwitchOp*)lir;
+            ASSERT0(LIR_dt(p) == 0x1);
+            fprintf(g_tfile, ", v%d", p->value);
+            USHORT * pdata = p->data;
+
+            //data[0]: flag to indicate switch-table type:
+            //    0x1 TABLE_SWITCH, 0x2 LOOKUP_SWITCH
+            USHORT f = pdata[0];
+            ASSERT0(f == 0x100);
+            //data[1]: the number of CASE entry.
+            USHORT num_of_case = pdata[1];
+
+            //data[2..3]: the base value of case-table
+            UINT base_val = *((UINT*)(&pdata[2]));
+            fprintf(g_tfile, ", basev:%d", base_val);
+
+            //((BYTE*)data)[4..num_of_case*4]:
+            //    the position of the index table is at current instruction.
+            if (num_of_case > 0) {
+                UINT * pcase_entry = (UINT*)&pdata[4];
+                fprintf(g_tfile, " tgtidx:");
+                for (USHORT i = 0; i < num_of_case; i++) {
+                    UINT idx_of_insn = pcase_entry[i];
+                    fprintf(g_tfile, "%d", idx_of_insn);
+                    if (i != num_of_case - 1) {
+                        fprintf(g_tfile, ",");
+                    }
+                }
+            }
+        }
+        break;
+    case LOP_LOOKUP_SWITCH:
+        {
+            LIRSwitchOp * p = (LIRSwitchOp*)lir;
+            ASSERT0(LIR_dt(p) == 0x2);
+            fprintf(g_tfile, ", v%d", p->value);
+            USHORT * pdata = p->data;
+
+            //pdata[0]: flag to indicate switch-table type:
+            //    0x1 TABLE_SWITCH, 0x2 LOOKUP_SWITCH
+            UINT f = pdata[0];
+            ASSERT0(f == 0x200);
+
+            //pdata[1]: the number of CASE entry.
+            UINT num_of_case = pdata[1];
+            if (num_of_case > 0) {
+                BYTE * tp = (BYTE*)pdata;
+                //((BYTE*)pdata)[4..4+num_of_case*4-1]: the case-value buffer.
+                UINT * pcase_value = (UINT*)&tp[4];
+
+                //((BYTE*)pdata)[4+num_of_case*4, 4+num_of_case*8-1]:
+                //    the position of the index table is at current instruction.
+                UINT * pcase_entry = (UINT*)&tp[4 + num_of_case * 4];
+                fprintf(g_tfile, " val2idx(");
+                for (UINT i = 0; i < num_of_case; i++) {
+                    UINT idx_of_insn = pcase_entry[i];
+                    fprintf(g_tfile, "%d:%d", pcase_value[i], idx_of_insn);
+                    if (i != num_of_case - 1) {
+                        fprintf(g_tfile, ",");
+                    }
+                }
+                fprintf(g_tfile, ")");
+            }
+        }
+        break;
+    case LOP_FILL_ARRAY_DATA:
+        {
+            fprintf(g_tfile, ", %s", get_dt_name(lir));
+            //AABBBBBBBB
+            //pdata[0]: the magic number of code
+            //0x100 PACKED_SWITCH, 0x200 SPARSE_SWITCH, 0x300 FILL_ARRAY_DATA
+            LIRSwitchOp * r = (LIRSwitchOp*)lir;
+            UInt16 const* pdata = (UInt16 const*)r->data;
+            ASSERT0(pdata[0] == 0x300);
+            //pdata[1]: size of each element.
+            //pdata[2]: the number of element.
+            UINT size_of_elem = pdata[1];
+            UINT num_of_elem = pdata[2];
+            UINT data_size = num_of_elem * size_of_elem;
+            //fprintf(g_tfile, ", (elem_sz<%d>), (elem_num<%d>), (data_ptr)0x%x",
+            fprintf(g_tfile, ", (elem_sz<%d>), (elem_num<%d>)",
+                    size_of_elem, num_of_elem);
+        }
+        break;
+    case LOP_INVOKE:
+        {
+            /*
+            ABCCCCDDDD the layout clarifies:
+                A(p4), B(argc), CCCC(method_id), DDDD(p0, p1, p2, p3)
+                where p0,p1,p2,p3,p4 are five parameters.
+
+            AABBBBCCCC the layout clarifies:
+                AA(argc), BBBB(method_id), CCCC(p0,p1,...p(argc-1))
+            */
+            LIRInvokeOp * r = (LIRInvokeOp*)lir;
+            UINT flags = LIR_dt(lir);
+            UINT flag1 = flags & 0x0F;
+            UINT flag2 = flags & 0xF0;
+            ASSERT(flag1 != 0, ("means LIR is LOP_FILLED_NEW_ARRAY"));
+            DexMethodId const* method_id = dexGetMethodId(df, r->ref);
+            ASSERT0(method_id);
+            CHAR const* method_name = dexStringById(df, method_id->nameIdx);
+            CHAR const* class_name =
+                dexStringByTypeIdx(df, method_id->classIdx);
+            ASSERT0(method_name);
+            DexProtoId const* proto_id =
+                dexGetProtoId(df, method_id->protoIdx);
+            CHAR const* shorty_name = dexStringById(df, proto_id->shortyIdx);
+            fprintf(g_tfile, ", %s::%s", class_name, method_name);
+
+            UINT k = LIR_dt(lir);
+            bool is_range = HAVE_FLAG((k & 0xf0), LIR_Range);
+            if (is_range) {
+                switch (k & 0x0f) {
+                case LIR_invoke_unknown: ASSERT0(0); break;
+                case LIR_invoke_virtual:
+                    fprintf(g_tfile, ", virtual-range"); break;
+                case LIR_invoke_direct:
+                    fprintf(g_tfile, ", direct-range"); break;
+                case LIR_invoke_super:
+                    fprintf(g_tfile, ", super-range"); break;
+                case LIR_invoke_interface:
+                    fprintf(g_tfile, ", interface-range"); break;
+                case LIR_invoke_static:
+                    fprintf(g_tfile, ", static-range"); break;
+                default: ASSERT0(0);
+                }
+            } else {
+                switch (k & 0x0f) {
+                case LIR_invoke_unknown: ASSERT0(0); break;
+                case LIR_invoke_virtual:
+                    fprintf(g_tfile, ", virtual"); break;
+                case LIR_invoke_direct:
+                    fprintf(g_tfile, ", direct"); break;
+                case LIR_invoke_super:
+                    fprintf(g_tfile, ", super"); break;
+                case LIR_invoke_interface:
+                    fprintf(g_tfile, ", interface"); break;
+                case LIR_invoke_static:
+                    fprintf(g_tfile, ", static"); break;
+                default: ASSERT0(0);
+                }
+            }
+
+            if (r->argc != 0) {
+                fprintf(g_tfile, ", arg(");
+                for (USHORT i = 0; i < r->argc; i++) {
+                    fprintf(g_tfile, "v%d", r->args[i]);
+                    if (i != r->argc-1) {
+                        fprintf(g_tfile, ",");
+                    }
+                }
+                fprintf(g_tfile, ")");
+            }
+        }
+        break;
+    case LOP_FILLED_NEW_ARRAY:
+        {
+            /*
+            AABBBBCCCC or ABCCCCDEFG
+            e.g:
+                A(argc), B,D,E,F,G(parampters), CCCC(class_tyid)
+            */
+            LIRInvokeOp * r = (LIRInvokeOp*)lir;
+            UINT flags = LIR_dt(lir);
+            CHAR const* class_name = dexStringByTypeIdx(df, r->ref);
+            ASSERT0(class_name);
+            fprintf(g_tfile, ", %s", class_name);
+            if (r->argc != 0) {
+                fprintf(g_tfile, ", arg(");
+                for (USHORT i = 0; i < r->argc; i++) {
+                    fprintf(g_tfile, "v%d", r->args[i]);
+                    if (i != r->argc-1) {
+                        fprintf(g_tfile, ",");
+                    }
+                }
+                fprintf(g_tfile, ")");
+            }
+        }
+        break;
+    case LOP_CMPG:
+        //AABBCC
+        ASSERT0(df);
+        switch (LIR_dt(lir)) {
+        case LIR_CMP_float:
+            fprintf(g_tfile, ", FLOAT");
+            fprintf(g_tfile, ", v%d, v%d, %d",
+                    LIR_res(lir),
+                    LIR_op0(lir),
+                    (INT)LIR_op1(lir));
+            break;
+        case LIR_CMP_double:
+            fprintf(g_tfile, ", DOUBLE");
+            fprintf(g_tfile, ", (v%d,v%d), v%d, %d",
+                    LIR_res(lir), LIR_res(lir)+1,
+                    LIR_op0(lir),
+                    (INT)LIR_op1(lir));
+            break;
+        default: ASSERT0(0);
+        }
+        break;
+    case LOP_PHI:
+        ASSERT0(0);
+        break;
+    default: ASSERT0(0);
+    } //end switch
+    fflush(g_tfile);
 }
 
 
 //Dump LIR stmts stored in fu->lirList array.
 void dump_all_lir(LIRCode * fu, DexFile * df, DexMethod const* dm)
 {
-	if (g_tfile == NULL) return;
-	ASSERT0(fu && df && dm);
-	CHAR const* class_name = get_class_name(df, dm);
-	CHAR const* func_name = get_func_name(df, dm);
-	ASSERT0(class_name && func_name);
-	fprintf(g_tfile, "\n==---- DUMP LIR of %s::%s ----== maxreg:%d ",
-			class_name, func_name, fu->maxVars - 1);
-	if (fu->maxVars > 0) {
-		fprintf(g_tfile, "(");
-		for (INT i = fu->maxVars - fu->numArgs; i < (INT)fu->maxVars; i++) {
-			ASSERT0(i >= 0);
-			fprintf(g_tfile, "v%d,", i);
-		}
-		fprintf(g_tfile, ")");
-	}
-	fprintf(g_tfile, " ====");
-	if (fu->triesSize != 0) {
-		for (UINT i = 0; i < fu->triesSize; i++) {
-			fprintf(g_tfile, "\ntry%d, ", i);
-			LIROpcodeTry * each_try = fu->trys + i;
-			fprintf(g_tfile, "start_idx=%dth, end_idx=%dth",
-					each_try->start_pc, each_try->end_pc);
-			for (UINT j = 0; j < each_try->catchSize; j++) {
-				LIROpcodeCatch * each_catch = each_try->catches + j;
-				fprintf(g_tfile, "\n    catch%d, kind=%d, start_idx=%dth", j,
-						each_catch->class_type,
-						each_catch->handler_pc);
-			}
-		}
-	}
-	for (INT i = 0; i < LIRC_num_of_op(fu); i++) {
-		LIR * lir = LIRC_op(fu, i);
-		ASSERT0(lir);
-		fprintf(g_tfile, "\n");
-		dump_lir2(lir, df, i);
-	}
-	fflush(g_tfile);
+    if (g_tfile == NULL) return;
+    ASSERT0(fu && df && dm);
+    CHAR const* class_name = get_class_name(df, dm);
+    CHAR const* func_name = get_func_name(df, dm);
+    ASSERT0(class_name && func_name);
+    fprintf(g_tfile, "\n==---- DUMP LIR of %s::%s ----== maxreg:%d ",
+            class_name, func_name, fu->maxVars - 1);
+    if (fu->maxVars > 0) {
+        fprintf(g_tfile, "(");
+        for (INT i = fu->maxVars - fu->numArgs; i < (INT)fu->maxVars; i++) {
+            ASSERT0(i >= 0);
+            fprintf(g_tfile, "v%d,", i);
+        }
+        fprintf(g_tfile, ")");
+    }
+    fprintf(g_tfile, " ====");
+    if (fu->triesSize != 0) {
+        for (UINT i = 0; i < fu->triesSize; i++) {
+            fprintf(g_tfile, "\ntry%d, ", i);
+            LIROpcodeTry * each_try = fu->trys + i;
+            fprintf(g_tfile, "start_idx=%dth, end_idx=%dth",
+                    each_try->start_pc, each_try->end_pc);
+            for (UINT j = 0; j < each_try->catchSize; j++) {
+                LIROpcodeCatch * each_catch = each_try->catches + j;
+                fprintf(g_tfile, "\n    catch%d, kind=%d, start_idx=%dth", j,
+                        each_catch->class_type,
+                        each_catch->handler_pc);
+            }
+        }
+    }
+    for (INT i = 0; i < LIRC_num_of_op(fu); i++) {
+        LIR * lir = LIRC_op(fu, i);
+        ASSERT0(lir);
+        fprintf(g_tfile, "\n");
+        dump_lir2(lir, df, i);
+    }
+    fflush(g_tfile);
 }
 
 
 //'finfo': field info.
 static void dumpf_field(DexFile * df, DexField * finfo, INT indent)
 {
-	if (g_tfile == NULL) { return; }
-	CHAR const* fname = get_field_name(df, finfo);
-	CHAR const* tname = get_field_type_name(df, finfo);
-	ASSERT0(fname && tname);
-	while (indent-- >= 0) { fprintf(g_tfile, " "); }
-	fprintf(g_tfile, "%dth, %s, Type:%s",
-			finfo->fieldIdx, fname, tname);
-	fflush(g_tfile);
+    if (g_tfile == NULL) { return; }
+    CHAR const* fname = get_field_name(df, finfo);
+    CHAR const* tname = get_field_type_name(df, finfo);
+    ASSERT0(fname && tname);
+    while (indent-- >= 0) { fprintf(g_tfile, " "); }
+    fprintf(g_tfile, "%dth, %s, Type:%s",
+            finfo->fieldIdx, fname, tname);
+    fflush(g_tfile);
 }
 
 
 //finfo: field-info in class.
 CHAR const* get_field_name(DexFile * df, DexField * finfo)
 {
-	//Get field's name via nameIdx in symbol table.
-	return get_field_name(df, finfo->fieldIdx);
+    //Get field's name via nameIdx in symbol table.
+    return get_field_name(df, finfo->fieldIdx);
 }
 
 
 //finfo: field-info in class.
 CHAR const* get_field_type_name(DexFile * df, DexField * finfo)
 {
-	return get_field_type_name(df, finfo->fieldIdx);
+    return get_field_type_name(df, finfo->fieldIdx);
 }
 
 
 //finfo: field-info in class.
 CHAR const* get_field_class_name(DexFile * df, DexField * finfo)
 {
-	return get_field_class_name(df, finfo->fieldIdx);
+    return get_field_class_name(df, finfo->fieldIdx);
 }
 
 
 //field_id: field number in dexfile.
 CHAR const* get_field_type_name(DexFile * df, UINT field_id)
 {
-	DexFieldId const* fid = dexGetFieldId(df, field_id);
-	ASSERT0(fid);
+    DexFieldId const* fid = dexGetFieldId(df, field_id);
+    ASSERT0(fid);
 
-	//Get field's type name.
-	return dexStringByTypeIdx(df, fid->typeIdx);
+    //Get field's type name.
+    return dexStringByTypeIdx(df, fid->typeIdx);
 }
 
 
 //Return the class name which 'field_id' was place in.
 CHAR const* get_field_class_name(DexFile * df, UINT field_id)
 {
-	DexFieldId const* fid = dexGetFieldId(df, field_id);
-	ASSERT0(fid);
-	return dexStringByTypeIdx(df, fid->classIdx);
+    DexFieldId const* fid = dexGetFieldId(df, field_id);
+    ASSERT0(fid);
+    return dexStringByTypeIdx(df, fid->classIdx);
 }
 
 
 //field_id: field number in dexfile.
 CHAR const* get_field_name(DexFile * df, UINT field_id)
 {
-	DexFieldId const* fid = dexGetFieldId(df, field_id);
-	ASSERT0(fid);
+    DexFieldId const* fid = dexGetFieldId(df, field_id);
+    ASSERT0(fid);
 
-	//Get field's name via nameIdx in symbol table.
-	return dexStringById(df, fid->nameIdx);
+    //Get field's name via nameIdx in symbol table.
+    return dexStringById(df, fid->nameIdx);
 }
 
 
 //Dump all classes declared in DEX file 'df'.
 void dump_all_class_and_field(DexFile * df)
 {
-	if (g_tfile == NULL) { return; }
-	fprintf(g_tfile, "\n==---- DUMP ALL CLASS and FIELD ----==");
-	//Walk through each class declaration via class-index in file.
-	ASSERT0(df && df->pHeader);
-	for (UINT i = 0; i < df->pHeader->classDefsSize; i++) {
-		fprintf(g_tfile, "\n");
-		//Show section header.
-		//dumpClassDef(pDexFile, i);
-
-		//Dump class name.
-		DexClassDef const* class_info = dexGetClassDef(df, i);
-		//ASSERT(i == class_info->classIdx, ("they might be equal"));
-		//CHAR const* class_name = dexStringByTypeIdx(df, class_info->classIdx);
-		//ASSERT0(class_name);
-		fprintf(g_tfile, "source file:%s", getClassSourceFilePath(df, class_info));
-		fprintf(g_tfile, "\nclass %s {", get_class_name(df, class_info));
-
-		//Dump class fields.
-		BYTE const* encoded_data = dexGetClassData(df, class_info);
-		if (encoded_data != NULL) {
-			DexClassData * class_data =
-				dexReadAndVerifyClassData(&encoded_data, NULL);
-			if (class_data != NULL) {
-				for (UINT i = 0; i < class_data->header.instanceFieldsSize; i++) {
-					fprintf(g_tfile, "\n");
-					DexField * finfo = &class_data->instanceFields[i];
-					ASSERT0(finfo);
-					dumpf_field(df, finfo, 4);
-					fflush(g_tfile);
-				}
-			} else {
-				fprintf(g_tfile, "\n\t--");
-			}
-		} else {
-			fprintf(g_tfile, "\n\t--");
-		}
-		fprintf(g_tfile, "\n");
-		fprintf(g_tfile, "}");
-		fflush(g_tfile);
-	}
-	fflush(g_tfile);
+    if (g_tfile == NULL) { return; }
+    fprintf(g_tfile, "\n==---- DUMP ALL CLASS and FIELD ----==");
+    //Walk through each class declaration via class-index in file.
+    ASSERT0(df && df->pHeader);
+    for (UINT i = 0; i < df->pHeader->classDefsSize; i++) {
+        fprintf(g_tfile, "\n");
+        //Show section header.
+        //dumpClassDef(pDexFile, i);
+
+        //Dump class name.
+        DexClassDef const* class_info = dexGetClassDef(df, i);
+        //ASSERT(i == class_info->classIdx, ("they might be equal"));
+        //CHAR const* class_name = dexStringByTypeIdx(df, class_info->classIdx);
+        //ASSERT0(class_name);
+        fprintf(g_tfile, "source file:%s", getClassSourceFilePath(df, class_info));
+        fprintf(g_tfile, "\nclass %s {", get_class_name(df, class_info));
+
+        //Dump class fields.
+        BYTE const* encoded_data = dexGetClassData(df, class_info);
+        if (encoded_data != NULL) {
+            DexClassData * class_data =
+                dexReadAndVerifyClassData(&encoded_data, NULL);
+            if (class_data != NULL) {
+                for (UINT i = 0; i < class_data->header.instanceFieldsSize; i++) {
+                    fprintf(g_tfile, "\n");
+                    DexField * finfo = &class_data->instanceFields[i];
+                    ASSERT0(finfo);
+                    dumpf_field(df, finfo, 4);
+                    fflush(g_tfile);
+                }
+            } else {
+                fprintf(g_tfile, "\n\t--");
+            }
+        } else {
+            fprintf(g_tfile, "\n\t--");
+        }
+        fprintf(g_tfile, "\n");
+        fprintf(g_tfile, "}");
+        fflush(g_tfile);
+    }
+    fflush(g_tfile);
 }
 
 
 /*
 ---dex file map-------
-	header!
-	string id!
-	type id!
-	proto id!
-	field id!
-	method id!
-	class def id!
-
-	annotation set ref list
-	annotation set item list !
-
-	code item !
-
-	annotations directory item!
-	type list info!
-	string data item list !
-	debug info!
-	annotation item list !
-	encodearray item list !
-
-	class Data item !
-	map list!
+    header!
+    string id!
+    type id!
+    proto id!
+    field id!
+    method id!
+    class def id!
+
+    annotation set ref list
+    annotation set item list !
+
+    code item !
+
+    annotations directory item!
+    type list info!
+    string data item list !
+    debug info!
+    annotation item list !
+    encodearray item list !
+
+    class Data item !
+    map list!
 */
 
 //
@@ -1242,100 +1273,100 @@ void dump_all_class_and_field(DexFile * df)
 //
 class DEX_CP : public IR_CP {
 public:
-	DEX_CP(Region * ru) : IR_CP(ru) {}
-	virtual ~DEX_CP() {}
-
-	//Check if ir is appropriate for propagation.
-	virtual bool canBeCandidate(IR const* ir) const
-	{
-		//Prop const imm may generate code which is not legal dex format.
-		//TODO: Perform more code normalization before ir2dex.
-		//return ir->is_const() || ir->is_pr();
-
-		return ir->is_pr();
-	}
+    DEX_CP(Region * ru) : IR_CP(ru) {}
+    virtual ~DEX_CP() {}
+
+    //Check if ir is appropriate for propagation.
+    virtual bool canBeCandidate(IR const* ir) const
+    {
+        //Prop const imm may generate code which is not legal dex format.
+        //TODO: Perform more code normalization before ir2dex.
+        //return ir->is_const() || ir->is_pr();
+
+        return ir->is_pr();
+    }
 };
 //END DEX_CP
 
 
 class DEX_RP : public IR_RP {
-	bool m_has_insert_stuff;
+    bool m_has_insert_stuff;
 public:
-	DEX_RP(Region * ru, IR_GVN * gvn) : IR_RP(ru, gvn)
-	{ m_has_insert_stuff = false; }
-	virtual ~DEX_RP() {}
-
-	/*
-	virtual bool is_promotable(IR const* ir) const
-	{
-		if (ir->is_array()) {
-			IR * sub = ARR_sub_list(ir);
-			ASSERT0(sub);
-			if (cnt_list(sub) == 1) {
-				if (sub->is_pr() && PR_no(sub) == 2) {
-					return false;
-				}
-			}
-		}
-		return IR_RP::is_promotable(ir);
-	}
-
-	void insert_stuff_code(IR const* ref, Region * ru, IR_GVN * gvn)
-	{
-		ASSERT0(IR_type(ref) == IR_ARRAY);
-
-		IR * stmt = ref->get_stmt();
-		ASSERT0(stmt);
-		IRBB * stmt_bb = stmt->get_bb();
-		ASSERT0(stmt_bb);
-
-		IR_DU_MGR * dumgr = ru->get_du_mgr();
-
-		C<IR*> * ct = NULL;
-		BB_irlist(stmt_bb).find(stmt, &ct);
-		ASSERT0(ct != NULL);
-
-		//Insert stuff code as you need. It will slow down the benchmark.
-		UINT num_want_to_insert = 30;
-		for (UINT i = 0; i < num_want_to_insert; i++) {
-			IR * newref = ru->dupIRTree(ref);
-			dumgr->copyIRTreeDU(newref, ref, true);
-			IR * stpr = ru->buildStorePR(ru->buildPrno(IR_dt(newref)),
-											IR_dt(newref), newref);
-			ru->allocRefForPR(stpr);
-			IR_may_throw(stpr) = true;
-
-			//New IR has same VN with original one.
-			gvn->set_mapIR2VN(stpr, gvn->mapIR2VN(ref));
-
-			BB_irlist(stmt_bb).insert_before(stpr, ct);
-		}
-	}
-
-	virtual void handleAccessInBody(IR * ref, IR * delegate, IR * delegate_pr,
-									TMap<IR*, SList<IR*>*> &
-											delegate2has_outside_uses_ir_list,
-									TTab<IR*> & restore2mem,
-									List<IR*> & fixup_list,
-									TMap<IR*, IR*> & delegate2stpr,
-									LI<IRBB> const* li,
-									IRIter & ii)
-	{
-		if (!m_has_insert_stuff &&
-			IR_type(ref) == IR_ARRAY &&
-			(m_ru->isRegionName(
-				"Lsoftweg/hw/performance/CPUTest;::arrayElementsDouble") ||
-			 m_ru->isRegionName(
-			 	"Lsoftweg/hw/performance/CPUTest;::arrayElementsSingle"))) {
-			m_has_insert_stuff = true;
-			insert_stuff_code(ref, m_ru, m_gvn);
-		}
-		IR_RP::handleAccessInBody(ref, delegate, delegate_pr,
-								delegate2has_outside_uses_ir_list,
-								restore2mem, fixup_list,
-								delegate2stpr, li, ii);
-	}
-	*/
+    DEX_RP(Region * ru, IR_GVN * gvn) : IR_RP(ru, gvn)
+    { m_has_insert_stuff = false; }
+    virtual ~DEX_RP() {}
+
+    /*
+    virtual bool is_promotable(IR const* ir) const
+    {
+        if (ir->is_array()) {
+            IR * sub = ARR_sub_list(ir);
+            ASSERT0(sub);
+            if (cnt_list(sub) == 1) {
+                if (sub->is_pr() && PR_no(sub) == 2) {
+                    return false;
+                }
+            }
+        }
+        return IR_RP::is_promotable(ir);
+    }
+
+    void insert_stuff_code(IR const* ref, Region * ru, IR_GVN * gvn)
+    {
+        ASSERT0(IR_type(ref) == IR_ARRAY);
+
+        IR * stmt = ref->get_stmt();
+        ASSERT0(stmt);
+        IRBB * stmt_bb = stmt->get_bb();
+        ASSERT0(stmt_bb);
+
+        IR_DU_MGR * dumgr = ru->get_du_mgr();
+
+        C<IR*> * ct = NULL;
+        BB_irlist(stmt_bb).find(stmt, &ct);
+        ASSERT0(ct != NULL);
+
+        //Insert stuff code as you need. It will slow down the benchmark.
+        UINT num_want_to_insert = 30;
+        for (UINT i = 0; i < num_want_to_insert; i++) {
+            IR * newref = ru->dupIRTree(ref);
+            dumgr->copyIRTreeDU(newref, ref, true);
+            IR * stpr = ru->buildStorePR(ru->buildPrno(IR_dt(newref)),
+                                            IR_dt(newref), newref);
+            ru->allocRefForPR(stpr);
+            IR_may_throw(stpr) = true;
+
+            //New IR has same VN with original one.
+            gvn->set_mapIR2VN(stpr, gvn->mapIR2VN(ref));
+
+            BB_irlist(stmt_bb).insert_before(stpr, ct);
+        }
+    }
+
+    virtual void handleAccessInBody(IR * ref, IR * delegate, IR * delegate_pr,
+                                    TMap<IR*, SList<IR*>*> &
+                                            delegate2has_outside_uses_ir_list,
+                                    TTab<IR*> & restore2mem,
+                                    List<IR*> & fixup_list,
+                                    TMap<IR*, IR*> & delegate2stpr,
+                                    LI<IRBB> const* li,
+                                    IRIter & ii)
+    {
+        if (!m_has_insert_stuff &&
+            IR_type(ref) == IR_ARRAY &&
+            (m_ru->isRegionName(
+                "Lsoftweg/hw/performance/CPUTest;::arrayElementsDouble") ||
+             m_ru->isRegionName(
+                 "Lsoftweg/hw/performance/CPUTest;::arrayElementsSingle"))) {
+            m_has_insert_stuff = true;
+            insert_stuff_code(ref, m_ru, m_gvn);
+        }
+        IR_RP::handleAccessInBody(ref, delegate, delegate_pr,
+                                delegate2has_outside_uses_ir_list,
+                                restore2mem, fixup_list,
+                                delegate2stpr, li, ii);
+    }
+    */
 };
 
 //
@@ -1343,66 +1374,66 @@ public:
 //
 CHAR const* AocDxMgr::get_string(UINT str_idx)
 {
-	return dexStringById(m_df, str_idx);
+    return dexStringById(m_df, str_idx);
 }
 
 
 CHAR const* AocDxMgr::get_type_name(UINT idx)
 {
-	return dexStringByTypeIdx(m_df, idx);
+    return dexStringByTypeIdx(m_df, idx);
 }
 
 
 //field_id: field number in dexfile.
 CHAR const* AocDxMgr::get_field_name(UINT field_idx)
 {
-	DexFieldId const* fid = dexGetFieldId(m_df, field_idx);
-	ASSERT0(fid);
+    DexFieldId const* fid = dexGetFieldId(m_df, field_idx);
+    ASSERT0(fid);
 
-	//Get field's name via nameIdx in symbol table.
-	return dexStringById(m_df, fid->nameIdx);
+    //Get field's name via nameIdx in symbol table.
+    return dexStringById(m_df, fid->nameIdx);
 }
 
 
 CHAR const* AocDxMgr::get_method_name(UINT method_idx)
 {
-	DexMethodId const* method_id = dexGetMethodId(m_df, method_idx);
-	ASSERT0(method_id);
-	CHAR const* method_name = dexStringById(m_df, method_id->nameIdx);
-	ASSERT0(method_name);
-	return method_name;
+    DexMethodId const* method_id = dexGetMethodId(m_df, method_idx);
+    ASSERT0(method_id);
+    CHAR const* method_name = dexStringById(m_df, method_id->nameIdx);
+    ASSERT0(method_name);
+    return method_name;
 }
 
 
 CHAR const* AocDxMgr::get_class_name(UINT class_type_idx)
 {
-	return dexStringByTypeIdx(m_df, class_type_idx);
+    return dexStringByTypeIdx(m_df, class_type_idx);
 }
 
 
 CHAR const* AocDxMgr::get_class_name_by_method_id(UINT method_idx)
 {
-	DexMethodId const* method_id = dexGetMethodId(m_df, method_idx);
-	ASSERT0(method_id);
-	CHAR const* class_name = dexStringByTypeIdx(m_df, method_id->classIdx);
-	return class_name;
+    DexMethodId const* method_id = dexGetMethodId(m_df, method_idx);
+    ASSERT0(method_id);
+    CHAR const* class_name = dexStringByTypeIdx(m_df, method_id->classIdx);
+    return class_name;
 }
 
 
 CHAR const* AocDxMgr::get_class_name_by_field_id(UINT field_idx)
 {
-	DexFieldId const* fid = dexGetFieldId(m_df, field_idx);
-	ASSERT0(fid);
-	return dexStringByTypeIdx(m_df, fid->classIdx);
+    DexFieldId const* fid = dexGetFieldId(m_df, field_idx);
+    ASSERT0(fid);
+    return dexStringByTypeIdx(m_df, fid->classIdx);
 }
 
 
 CHAR const* AocDxMgr::get_class_name_by_declaration_id(UINT cls_def_idx)
 {
-	DexClassDef const* class_info = dexGetClassDef(m_df, cls_def_idx);
-	CHAR const* class_name = dexStringByTypeIdx(m_df, class_info->classIdx);
-	ASSERT0(class_name);
-	return class_name;
+    DexClassDef const* class_info = dexGetClassDef(m_df, cls_def_idx);
+    CHAR const* class_name = dexStringByTypeIdx(m_df, class_info->classIdx);
+    ASSERT0(class_name);
+    return class_name;
 }
 //END AocDxMgr
 
@@ -1412,86 +1443,86 @@ CHAR const* AocDxMgr::get_class_name_by_declaration_id(UINT cls_def_idx)
 //
 Pass * DexPassMgr::allocDCE()
 {
-	IR_DCE * pass = new IR_DCE(m_ru);
-	pass->set_elim_cfs(true);
-	return pass;
+    IR_DCE * pass = new IR_DCE(m_ru);
+    pass->set_elim_cfs(true);
+    return pass;
 }
 
 
 Pass * DexPassMgr::allocCopyProp()
 {
-	Pass * pass = new DEX_CP(m_ru);
-	SimpCTX simp;
-	pass->set_simp_cont(&simp);
-	return pass;
+    Pass * pass = new DEX_CP(m_ru);
+    SimpCTX simp;
+    pass->set_simp_cont(&simp);
+    return pass;
 }
 
 
 Pass * DexPassMgr::allocRP()
 {
-	Pass * pass = new DEX_RP(m_ru, (IR_GVN*)registerPass(PASS_GVN));
-	return pass;
+    Pass * pass = new DEX_RP(m_ru, (IR_GVN*)registerPass(PASS_GVN));
+    return pass;
 }
 
 
 int xtime = 1;
 void DexPassMgr::performScalarOpt(OptCTX & oc)
 {
-	List<Pass*> passlist; //A list of optimization.
-	IR_SSA_MGR * ssamgr = (IR_SSA_MGR*)registerPass(PASS_SSA_MGR);
-	bool is_ssa_avail = false;
-	if (ssamgr != NULL) {
-		is_ssa_avail = ssamgr->is_ssa_constructed();
-	}
-
-	passlist.append_tail(registerPass(PASS_CP));
-	passlist.append_tail(registerPass(PASS_DCE));
-	passlist.append_tail(registerPass(PASS_RP));
-	passlist.append_tail(registerPass(PASS_CP));
-	passlist.append_tail(registerPass(PASS_DCE));
-	passlist.append_tail(registerPass(PASS_RP));
-	passlist.append_tail(registerPass(PASS_CP));
-	passlist.append_tail(registerPass(PASS_DCE));
-	passlist.append_tail(registerPass(PASS_LOOP_CVT));
-	passlist.append_tail(registerPass(PASS_LICM));
-	passlist.append_tail(registerPass(PASS_GCSE));
-
-	((IR_DCE*)registerPass(PASS_DCE))->set_elim_cfs(false);
-
-	if (passlist.get_elem_count() != 0) {
-		LOG("\tScalar optimizations for '%s'", m_ru->get_ru_name());
-	}
-
-	bool change;
-	UINT count = 0;
-	//do {
-		change = false;
-		for (Pass * pass = passlist.get_head();
-			 pass != NULL; pass = passlist.get_next()) {
-			CHAR const* passname = pass->get_pass_name();
-			LOG("\t\tpass %s", passname);
-			ASSERT0(verifyIRandBB(m_ru->get_bb_list(), m_ru));
-			ULONGLONG t = getusec();
-
-			//dumpBBList(m_ru->get_bb_list(), m_ru, "before");
-			//m_ru->get_cfg()->dump_vcg("before.vcg");
-
-			bool doit = pass->perform(oc);
-
-			//dumpBBList(m_ru->get_bb_list(), m_ru, "after");
-			//m_ru->get_cfg()->dump_vcg("after.vcg");
-
-			appendTimeInfo(pass->get_pass_name(), getusec() - t);
-			if (doit) {
-				LOG("\t\t\tchanged");
-				change = true;
-				ASSERT0(verifyIRandBB(m_ru->get_bb_list(), m_ru));
-				ASSERT0(m_ru->get_cfg()->verify());
-			}
-		}
-		count++;
-	//} while (change && count < 20);
-	//ASSERT0(!change);
+    List<Pass*> passlist; //A list of optimization.
+    IR_SSA_MGR * ssamgr = (IR_SSA_MGR*)registerPass(PASS_SSA_MGR);
+    bool is_ssa_avail = false;
+    if (ssamgr != NULL) {
+        is_ssa_avail = ssamgr->is_ssa_constructed();
+    }
+
+    passlist.append_tail(registerPass(PASS_CP));
+    passlist.append_tail(registerPass(PASS_DCE));
+    passlist.append_tail(registerPass(PASS_RP));
+    passlist.append_tail(registerPass(PASS_CP));
+    passlist.append_tail(registerPass(PASS_DCE));
+    passlist.append_tail(registerPass(PASS_RP));
+    passlist.append_tail(registerPass(PASS_CP));
+    passlist.append_tail(registerPass(PASS_DCE));
+    passlist.append_tail(registerPass(PASS_LOOP_CVT));
+    passlist.append_tail(registerPass(PASS_LICM));
+    passlist.append_tail(registerPass(PASS_GCSE));
+
+    ((IR_DCE*)registerPass(PASS_DCE))->set_elim_cfs(false);
+
+    if (passlist.get_elem_count() != 0) {
+        LOG("\tScalar optimizations for '%s'", m_ru->get_ru_name());
+    }
+
+    bool change;
+    UINT count = 0;
+    //do {
+        change = false;
+        for (Pass * pass = passlist.get_head();
+             pass != NULL; pass = passlist.get_next()) {
+            CHAR const* passname = pass->get_pass_name();
+            LOG("\t\tpass %s", passname);
+            ASSERT0(verifyIRandBB(m_ru->get_bb_list(), m_ru));
+            ULONGLONG t = getusec();
+
+            //dumpBBList(m_ru->get_bb_list(), m_ru, "before");
+            //m_ru->get_cfg()->dump_vcg("before.vcg");
+
+            bool doit = pass->perform(oc);
+
+            //dumpBBList(m_ru->get_bb_list(), m_ru, "after");
+            //m_ru->get_cfg()->dump_vcg("after.vcg");
+
+            appendTimeInfo(pass->get_pass_name(), getusec() - t);
+            if (doit) {
+                LOG("\t\t\tchanged");
+                change = true;
+                ASSERT0(verifyIRandBB(m_ru->get_bb_list(), m_ru));
+                ASSERT0(m_ru->get_cfg()->verify());
+            }
+        }
+        count++;
+    //} while (change && count < 20);
+    //ASSERT0(!change);
 }
 //END DexPassMgr
 
@@ -1504,77 +1535,77 @@ void DexPassMgr::performScalarOpt(OptCTX & oc)
 typedef TMap<Type const*, MD const*> Type2MD;
 
 class DEX_AA : public IR_AA {
-	Type2MD m_type2md;
+    Type2MD m_type2md;
 public:
-	DEX_AA(Region * ru) : IR_AA(ru) {}
-
-	//Attemp to compute POINT-TO set via data type.
-	virtual MD const* computePointToViaType(IR const* ir)
-	{
-		AttachInfo * ai = IR_ai(ir);
-		ASSERT0(ir && ai);
-
-		TbaaAttachInfo * ty = (TbaaAttachInfo*)ai->get(AI_TBAA);
-		if (ty == NULL) { return NULL; }
-
-		ASSERT(ty->type, ("Type should not be given if "
-						  "you intend to annotate it as TBAA."));
-		MD const* md = m_type2md.get(ty->type);
-		if (md != NULL) {
-			return md;
-		}
-
-		CHAR buf[64];
-		sprintf(buf, "dummy%d", (UINT)(size_t)ty->type);
-		VAR * dummy = m_var_mgr->registerVar(
-						buf, ty->type, 1, VAR_GLOBAL);
-		VAR_is_addr_taken(dummy) = true;
-		VAR_allocable(dummy) = false;
-		m_ru->addToVarTab(dummy);
-
-		MD dummy_md;
-		MD_base(&dummy_md) = dummy;
-		MD_size(&dummy_md) = 0;
-		MD_ty(&dummy_md) = MD_UNBOUND;
-		MD const* entry = m_md_sys->registerMD(dummy_md);
-		m_type2md.set(ty->type, entry);
-		return entry;
-	}
-
-	void handle_ld(IR * ld, MD2MDSet * mx)
-	{
-		ASSERT0(ld->is_ld() && mx);
-		AttachInfo * ai = IR_ai(ld);
-		if (ai == NULL) { return; }
-
-		TbaaAttachInfo * ty = (TbaaAttachInfo*)ai->get(AI_TBAA);
-		if (ty == NULL) { return; }
-
-		MD const* md = m_type2md.get(ty->type);
-		if (md != NULL) {
-			MD const* t = allocLoadMD(ld);
-			setPointToMDSetByAddMD(t, *mx, md);
-			return;
-		}
-
-		CHAR buf[64];
-		sprintf(buf, "dummy%d", (UINT)(size_t)ty->type);
-		VAR * dummy = m_var_mgr->registerVar(
-					buf, m_dm->getMCType(0), 1, VAR_GLOBAL);
-		VAR_is_addr_taken(dummy) = true;
-		VAR_allocable(dummy) = false;
-		m_ru->addToVarTab(dummy);
-
-		MD dummy_md;
-		MD_base(&dummy_md) = dummy;
-		MD_size(&dummy_md) = 0;
-		MD_ty(&dummy_md) = MD_UNBOUND;
-		MD const* entry = m_md_sys->registerMD(dummy_md);
-		m_type2md.set(ty->type, entry);
-
-		MD const* t = allocLoadMD(ld);
-		setPointToMDSetByAddMD(t, *mx, entry);
-	}
+    DEX_AA(Region * ru) : IR_AA(ru) {}
+
+    //Attemp to compute POINT-TO set via data type.
+    virtual MD const* computePointToViaType(IR const* ir)
+    {
+        AttachInfo * ai = IR_ai(ir);
+        ASSERT0(ir && ai);
+
+        TbaaAttachInfo * ty = (TbaaAttachInfo*)ai->get(AI_TBAA);
+        if (ty == NULL) { return NULL; }
+
+        ASSERT(ty->type, ("Type should not be given if "
+                          "you intend to annotate it as TBAA."));
+        MD const* md = m_type2md.get(ty->type);
+        if (md != NULL) {
+            return md;
+        }
+
+        CHAR buf[64];
+        sprintf(buf, "dummy%d", (UINT)(size_t)ty->type);
+        VAR * dummy = m_var_mgr->registerVar(
+                        buf, ty->type, 1, VAR_GLOBAL);
+        VAR_is_addr_taken(dummy) = true;
+        VAR_allocable(dummy) = false;
+        m_ru->addToVarTab(dummy);
+
+        MD dummy_md;
+        MD_base(&dummy_md) = dummy;
+        MD_size(&dummy_md) = 0;
+        MD_ty(&dummy_md) = MD_UNBOUND;
+        MD const* entry = m_md_sys->registerMD(dummy_md);
+        m_type2md.set(ty->type, entry);
+        return entry;
+    }
+
+    void handle_ld(IR * ld, MD2MDSet * mx)
+    {
+        ASSERT0(ld->is_ld() && mx);
+        AttachInfo * ai = IR_ai(ld);
+        if (ai == NULL) { return; }
+
+        TbaaAttachInfo * ty = (TbaaAttachInfo*)ai->get(AI_TBAA);
+        if (ty == NULL) { return; }
+
+        MD const* md = m_type2md.get(ty->type);
+        if (md != NULL) {
+            MD const* t = allocLoadMD(ld);
+            setPointToMDSetByAddMD(t, *mx, md);
+            return;
+        }
+
+        CHAR buf[64];
+        sprintf(buf, "dummy%d", (UINT)(size_t)ty->type);
+        VAR * dummy = m_var_mgr->registerVar(
+                    buf, m_dm->getMCType(0), 1, VAR_GLOBAL);
+        VAR_is_addr_taken(dummy) = true;
+        VAR_allocable(dummy) = false;
+        m_ru->addToVarTab(dummy);
+
+        MD dummy_md;
+        MD_base(&dummy_md) = dummy;
+        MD_size(&dummy_md) = 0;
+        MD_ty(&dummy_md) = MD_UNBOUND;
+        MD const* entry = m_md_sys->registerMD(dummy_md);
+        m_type2md.set(ty->type, entry);
+
+        MD const* t = allocLoadMD(ld);
+        setPointToMDSetByAddMD(t, *mx, entry);
+    }
 };
 //END DEX_AA
 
@@ -1615,26 +1646,25 @@ void DexRegionMgr::processProgramRegion(Region * program)
 
     //Function region has been handled. And call list should be available.
     CallGraph * callg = initCallGraph(program, false);
-
 }
 //START DexRegion
 //
 PassMgr * DexRegion::allocPassMgr()
 {
-	return new DexPassMgr(this);
+    return new DexPassMgr(this);
 }
 
 
 //Initialize alias analysis.
 IR_AA * DexRegion::allocAliasAnalysis()
 {
-	return new DEX_AA(this);
+    return new DEX_AA(this);
 }
 
 
 bool DexRegion::MiddleProcess(OptCTX & oc)
 {
-	return Region::MiddleProcess(oc);
+    return Region::MiddleProcess(oc);
 }
 
 
@@ -1692,115 +1722,115 @@ void DexRegion::HighProcessImpl(OptCTX & oc)
 
 bool DexRegion::HighProcess(OptCTX & oc)
 {
-	CHAR const* ru_name = get_ru_name();
-	g_indent = 0;
-	SimpCTX simp;
-	SIMP_if(&simp) = 1;
-	SIMP_do_loop(&simp) = 1;
-	SIMP_do_while(&simp) = 1;
-	SIMP_while_do(&simp) = 1;
-	SIMP_switch(&simp) = 0;
-	SIMP_break(&simp) = 1;
-	SIMP_continue(&simp) = 1;
+    CHAR const* ru_name = get_ru_name();
+    g_indent = 0;
+    SimpCTX simp;
+    SIMP_if(&simp) = 1;
+    SIMP_do_loop(&simp) = 1;
+    SIMP_do_while(&simp) = 1;
+    SIMP_while_do(&simp) = 1;
+    SIMP_switch(&simp) = 0;
+    SIMP_break(&simp) = 1;
+    SIMP_continue(&simp) = 1;
 
-	REGION_analysis_instrument(this)->m_ir_list =
-					simplifyStmtList(get_ir_list(), &simp);
+    REGION_analysis_instrument(this)->m_ir_list =
+                    simplifyStmtList(get_ir_list(), &simp);
 
-	ASSERT0(verify_simp(get_ir_list(), simp));
-	ASSERT0(verify_irs(get_ir_list(), NULL, this));
+    ASSERT0(verify_simp(get_ir_list(), simp));
+    ASSERT0(verify_irs(get_ir_list(), NULL, this));
 
-	constructIRBBlist();
+    constructIRBBlist();
 
-	ASSERT0(verifyIRandBB(get_bb_list(), this));
+    ASSERT0(verifyIRandBB(get_bb_list(), this));
 
-	//All IRs have been moved to each IRBB.
-	REGION_analysis_instrument(this)->m_ir_list = NULL;
+    //All IRs have been moved to each IRBB.
+    REGION_analysis_instrument(this)->m_ir_list = NULL;
 
     HighProcessImpl(oc);
-	return true;
+    return true;
 }
 
 
 //All global prs must be mapped.
 bool DexRegion::verifyRAresult(RA & ra, Prno2Vreg & prno2v)
 {
-	GltMgr * gltm = ra.get_gltm();
-	Vector<GLT*> * gltv = gltm->get_gltvec();
-	for (UINT i = 0; i < gltm->get_num_of_glt(); i++) {
-		GLT * g = gltv->get(i);
-		if (g == NULL) { continue; }
-		ASSERT0(g->has_allocated());
-		if (GLT_bbs(g) == NULL) {
-			//parameter may be have no occ.
-			continue;
-		}
-		bool find;
-		prno2v.get(GLT_prno(g), &find);
-		ASSERT0(find);
-	}
-
-	BBList * bbl = get_bb_list();
-	for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
-		LTMgr * ltm = gltm->map_bb2ltm(bb);
-		if (ltm == NULL) { continue; }
-		Vector<LT*> * lvec = ltm->get_lt_vec();
-		for (INT i = 0; i <= lvec->get_last_idx(); i++) {
-			LT * l = lvec->get(i);
-			if (l == NULL) { continue; }
-			ASSERT0(l->has_allocated());
-			bool find;
-			prno2v.get(LT_prno(l), &find);
-			ASSERT0(find);
-		}
-	}
-	return true;
+    GltMgr * gltm = ra.get_gltm();
+    Vector<GLT*> * gltv = gltm->get_gltvec();
+    for (UINT i = 0; i < gltm->get_num_of_glt(); i++) {
+        GLT * g = gltv->get(i);
+        if (g == NULL) { continue; }
+        ASSERT0(g->has_allocated());
+        if (GLT_bbs(g) == NULL) {
+            //parameter may be have no occ.
+            continue;
+        }
+        bool find;
+        prno2v.get(GLT_prno(g), &find);
+        ASSERT0(find);
+    }
+
+    BBList * bbl = get_bb_list();
+    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
+        LTMgr * ltm = gltm->map_bb2ltm(bb);
+        if (ltm == NULL) { continue; }
+        Vector<LT*> * lvec = ltm->get_lt_vec();
+        for (INT i = 0; i <= lvec->get_last_idx(); i++) {
+            LT * l = lvec->get(i);
+            if (l == NULL) { continue; }
+            ASSERT0(l->has_allocated());
+            bool find;
+            prno2v.get(LT_prno(l), &find);
+            ASSERT0(find);
+        }
+    }
+    return true;
 }
 
 
 void DexRegion::updateRAresult(IN RA & ra, OUT Prno2Vreg & prno2v)
 {
-	prno2v.maxreg = ra.get_maxreg();
-	prno2v.paramnum = ra.get_paramnum();
-	GltMgr * gltm = ra.get_gltm();
-	BBList * bbl = get_bb_list();
-	prno2v.clean();
-	for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
-		LTMgr * ltm = gltm->map_bb2ltm(bb);
-		if (ltm == NULL) { continue; }
-		Vector<LT*> * lvec = ltm->get_lt_vec();
-		for (INT i = 0; i <= lvec->get_last_idx(); i++) {
-			LT * l = lvec->get(i);
-			if (l == NULL) { continue; }
-			ASSERT0(l->has_allocated());
-			bool find;
-			UINT v = prno2v.get(LT_prno(l), &find);
-			if (find) {
-				//each prno is corresponding to a unqiue vreg.
-				ASSERT0(v == LT_phy(l));
-			} else {
-				prno2v.set(LT_prno(l), LT_phy(l));
-			}
-		}
-	}
-	prno2v.dump();
-	ASSERT0(verifyRAresult(ra, prno2v));
+    prno2v.maxreg = ra.get_maxreg();
+    prno2v.paramnum = ra.get_paramnum();
+    GltMgr * gltm = ra.get_gltm();
+    BBList * bbl = get_bb_list();
+    prno2v.clean();
+    for (IRBB * bb = bbl->get_head(); bb != NULL; bb = bbl->get_next()) {
+        LTMgr * ltm = gltm->map_bb2ltm(bb);
+        if (ltm == NULL) { continue; }
+        Vector<LT*> * lvec = ltm->get_lt_vec();
+        for (INT i = 0; i <= lvec->get_last_idx(); i++) {
+            LT * l = lvec->get(i);
+            if (l == NULL) { continue; }
+            ASSERT0(l->has_allocated());
+            bool find;
+            UINT v = prno2v.get(LT_prno(l), &find);
+            if (find) {
+                //each prno is corresponding to a unqiue vreg.
+                ASSERT0(v == LT_phy(l));
+            } else {
+                prno2v.set(LT_prno(l), LT_phy(l));
+            }
+        }
+    }
+    //prno2v.dump();
+    ASSERT0(verifyRAresult(ra, prno2v));
 }
 
 
 void DexRegion::processSimply()
 {
-	LOG("DexRegion::processSimply %s", get_ru_name());
-	if (get_ir_list() == NULL) { return ; }
-	OptCTX oc;
-	OC_show_comp_time(oc) = g_show_comp_time;
+    LOG("DexRegion::processSimply %s", get_ru_name());
+    if (get_ir_list() == NULL) { return ; }
+    OptCTX oc;
+    OC_show_comp_time(oc) = g_show_comp_time;
 
-	CHAR const* ru_name = get_ru_name();
+    CHAR const* ru_name = get_ru_name();
 
-	constructIRBBlist();
+    constructIRBBlist();
 
-	ASSERT0(verifyIRandBB(get_bb_list(), this));
+    ASSERT0(verifyIRandBB(get_bb_list(), this));
 
-	REGION_analysis_instrument(this)->m_ir_list = NULL; //All IRs have been moved to each IRBB.
+    REGION_analysis_instrument(this)->m_ir_list = NULL; //All IRs have been moved to each IRBB.
 
     PassMgr * passmgr = initPassMgr();
     ASSERT0(passmgr);
@@ -1812,7 +1842,7 @@ void DexRegion::processSimply()
     ASSERT0(g_do_cfg_dom);
     cfg->LoopAnalysis(oc);
 
-	destroyPassMgr();
+    destroyPassMgr();
 
     //Do not allocate register.
     getPrno2Vreg()->clean();
@@ -1824,59 +1854,59 @@ void DexRegion::processSimply()
 //This function outputs Prno2Vreg after Dex register allocation.
 void DexRegion::process()
 {
-	if (get_ir_list() == NULL) { return; }
-	OptCTX oc;
-	OC_show_comp_time(oc) = g_show_comp_time;
+    if (get_ir_list() == NULL) { return; }
+    OptCTX oc;
+    OC_show_comp_time(oc) = g_show_comp_time;
 
-	g_indent = 0;
-	LOG("DexRegion process %s", get_ru_name());
+    g_indent = 0;
+    LOG("DexRegion process %s", get_ru_name());
     //note("\n==---- REGION_NAME:%s ----==", get_ru_name());
-	prescan(get_ir_list());
+    prescan(get_ir_list());
 
-	REGION_is_pr_unique_for_same_number(this) = true;
+    REGION_is_pr_unique_for_same_number(this) = true;
 
     PassMgr * passmgr = initPassMgr();
 
-	HighProcess(oc);
-	MiddleProcess(oc);
+    HighProcess(oc);
+    MiddleProcess(oc);
 
-	ASSERT0(get_pass_mgr());
-	IR_SSA_MGR * ssamgr = (IR_SSA_MGR*)get_pass_mgr()->query_opt(PASS_SSA_MGR);
-	if (ssamgr != NULL && ssamgr->is_ssa_constructed()) {
-		ssamgr->destructionInBBListOrder();
-	}
+    ASSERT0(get_pass_mgr());
+    IR_SSA_MGR * ssamgr = (IR_SSA_MGR*)get_pass_mgr()->query_opt(PASS_SSA_MGR);
+    if (ssamgr != NULL && ssamgr->is_ssa_constructed()) {
+        ssamgr->destructionInBBListOrder();
+    }
 
-	//Destroy PassMgr.
-	destroyPassMgr();
+    //Destroy PassMgr.
+    destroyPassMgr();
 
     if (!is_function()) { return; }
 
-	BBList * bbl = get_bb_list();
-	if (bbl->get_elem_count() == 0) { return; }
-
-	ASSERT0(verifyIRandBB(bbl, this));
-
-	RefineCTX rf;
-	RC_insert_cvt(rf) = false; //Do not insert cvt for DEX code.
-	refineBBlist(bbl, rf);
-	ASSERT0(verifyIRandBB(bbl, this));
-
-	if (g_do_dex_ra) {
-		Prno2Vreg * original_prno2vreg = getDex2IR()->getPR2Vreg();
-		RA ra(this,
-			  getTypeIndexRep(),
-			  getParamNum(),
-			  getOrgVregNum(),
-			  getDex2IR()->getVreg2PR(),
-			  original_prno2vreg,
-			  &m_var2pr);
-		LOG("\t\tdo DEX Register Allcation for '%s'", get_ru_name());
-		ra.perform(oc);
-		updateRAresult(ra, *getPrno2Vreg());
-	} else {
-		//Do not allocate register.
-		getPrno2Vreg()->clean();
-		getPrno2Vreg()->copy(*getDex2IR()->getPR2Vreg());
-	}
+    BBList * bbl = get_bb_list();
+    if (bbl->get_elem_count() == 0) { return; }
+
+    ASSERT0(verifyIRandBB(bbl, this));
+
+    RefineCTX rf;
+    RC_insert_cvt(rf) = false; //Do not insert cvt for DEX code.
+    refineBBlist(bbl, rf);
+    ASSERT0(verifyIRandBB(bbl, this));
+
+    if (g_do_dex_ra) {
+        Prno2Vreg * original_prno2vreg = getDex2IR()->getPR2Vreg();
+        RA ra(this,
+              getTypeIndexRep(),
+              getParamNum(),
+              getOrgVregNum(),
+              getDex2IR()->getVreg2PR(),
+              original_prno2vreg,
+              &m_var2pr);
+        LOG("\t\tdo DEX Register Allcation for '%s'", get_ru_name());
+        ra.perform(oc);
+        updateRAresult(ra, *getPrno2Vreg());
+    } else {
+        //Do not allocate register.
+        getPrno2Vreg()->clean();
+        getPrno2Vreg()->copy(*getDex2IR()->getPR2Vreg());
+    }
 }
 //END DexRegion
diff --git a/dex/dex_util.h b/dex/dex_util.h
index 334ab1c..e5de283 100644
--- a/dex/dex_util.h
+++ b/dex/dex_util.h
@@ -38,14 +38,14 @@ class Dex2IR;
 
 class DexPassMgr : public PassMgr {
 public:
-	DexPassMgr(Region * ru) : PassMgr(ru) {}
-	virtual ~DexPassMgr() {}
+    DexPassMgr(Region * ru) : PassMgr(ru) {}
+    virtual ~DexPassMgr() {}
 
-	virtual Pass * allocDCE();
-	virtual Pass * allocCopyProp();
-	virtual Pass * allocRP();
+    virtual Pass * allocDCE();
+    virtual Pass * allocCopyProp();
+    virtual Pass * allocRP();
 
-	virtual void performScalarOpt(OptCTX & oc);
+    virtual void performScalarOpt(OptCTX & oc);
 };
 
 
@@ -61,11 +61,12 @@ protected:
     Dex2IR * m_dex2ir;
     Prno2Vreg * m_prno2v;
     TypeIndexRep * m_type_index_rep;
-	UINT m_paramnum;
-	UINT m_org_vregnum;
-	CHAR const* m_src_path; //record source file path of class.
+    UINT m_paramnum;
+    UINT m_org_vregnum;
+    CHAR const* m_src_path; //record source file path of class.
     DexFile const* m_dexfile;
     DexMethod const* m_dexmethod;
+    SMemPool * m_sc_pool; //a pool to hold the SC<LabelInfo*>
 
 protected:
     bool is_64bit(IR const* ir)
@@ -78,63 +79,65 @@ public:
     CHAR const* m_classname;
 
 public:
-	DexRegion(REGION_TYPE rt, RegionMgr * rm) : Region(rt, rm)
-	{
-		m_dex2ir = NULL;
-		m_prno2v = NULL;
-		m_type_index_rep = NULL;
-		m_paramnum = 0;
-		m_org_vregnum = 0;
-		m_src_path;
-		m_dexfile = NULL;
-		m_dexmethod = NULL;
+    DexRegion(REGION_TYPE rt, RegionMgr * rm) : Region(rt, rm)
+    {
+        m_dex2ir = NULL;
+        m_prno2v = NULL;
+        m_type_index_rep = NULL;
+        m_paramnum = 0;
+        m_org_vregnum = 0;
+        m_src_path;
+        m_dexfile = NULL;
+        m_dexmethod = NULL;
         m_classname = NULL;
         m_funcname = NULL;
         m_functype = NULL;
-	}
-	virtual ~DexRegion() {}
+        m_sc_pool = smpoolCreate(sizeof(SC<LabelInfo*>), MEM_CONST_SIZE);
+    }
+    virtual ~DexRegion() { smpoolDelete(m_sc_pool); }
 
-	IR * gen_and_add_sib(IR * ir, UINT prno);
-	DexPassMgr * getDexPassMgr() { return (DexPassMgr*)get_pass_mgr(); }
+    IR * gen_and_add_sib(IR * ir, UINT prno);
+    DexPassMgr * getDexPassMgr() { return (DexPassMgr*)get_pass_mgr(); }
 
-	virtual bool HighProcess(OptCTX & oc);
-	virtual bool MiddleProcess(OptCTX & oc);
+    virtual bool HighProcess(OptCTX & oc);
+    virtual bool MiddleProcess(OptCTX & oc);
 
-	virtual PassMgr * allocPassMgr();
-	virtual IR_AA * allocAliasAnalysis();
+    virtual PassMgr * allocPassMgr();
+    virtual IR_AA * allocAliasAnalysis();
 
-	void setDex2IR(Dex2IR * dex2ir) { m_dex2ir = dex2ir; }
-	Dex2IR * getDex2IR() { return m_dex2ir; }
+    SMemPool * get_sc_pool() const { return m_sc_pool; }
+    void setDex2IR(Dex2IR * dex2ir) { m_dex2ir = dex2ir; }
+    Dex2IR * getDex2IR() { return m_dex2ir; }
 
-	void setPrno2Vreg(Prno2Vreg * p2v) { m_prno2v = p2v; }
-	Prno2Vreg * getPrno2Vreg() { return m_prno2v; }
+    void setPrno2Vreg(Prno2Vreg * p2v) { m_prno2v = p2v; }
+    Prno2Vreg * getPrno2Vreg() { return m_prno2v; }
 
-	void setTypeIndexRep(TypeIndexRep * tr) { m_type_index_rep = tr; }
-	TypeIndexRep * getTypeIndexRep() { return m_type_index_rep; }
+    void setTypeIndexRep(TypeIndexRep * tr) { m_type_index_rep = tr; }
+    TypeIndexRep * getTypeIndexRep() { return m_type_index_rep; }
 
-	void setParamNum(UINT num) { m_paramnum = num; }
-	UINT getParamNum() const { return m_paramnum; }
+    void setParamNum(UINT num) { m_paramnum = num; }
+    UINT getParamNum() const { return m_paramnum; }
 
-	void setOrgVregNum(UINT num) { m_org_vregnum = num; }
-	UINT getOrgVregNum() const { return m_org_vregnum; }
+    void setOrgVregNum(UINT num) { m_org_vregnum = num; }
+    UINT getOrgVregNum() const { return m_org_vregnum; }
 
-	void setClassSourceFilePath(CHAR const* path) { m_src_path = path; }
-	CHAR const* getClassSourceFilePath() const { return m_src_path; }
+    void setClassSourceFilePath(CHAR const* path) { m_src_path = path; }
+    CHAR const* getClassSourceFilePath() const { return m_src_path; }
 
-	void setDexFile(DexFile const* df) { m_dexfile = df; }
-	DexFile const* getDexFile() const { return m_dexfile; }
+    void setDexFile(DexFile const* df) { m_dexfile = df; }
+    DexFile const* getDexFile() const { return m_dexfile; }
 
-	void setDexMethod(DexMethod const* dm) { m_dexmethod = dm; }
-	DexMethod const* getDexMethod() const { return m_dexmethod; }
+    void setDexMethod(DexMethod const* dm) { m_dexmethod = dm; }
+    DexMethod const* getDexMethod() const { return m_dexmethod; }
 
-	void updateRAresult(RA & ra, Prno2Vreg & prno2v);
+    void updateRAresult(RA & ra, Prno2Vreg & prno2v);
 
-	bool verifyRAresult(RA & ra, Prno2Vreg & prno2v);
+    bool verifyRAresult(RA & ra, Prno2Vreg & prno2v);
 
-	void process_group_bb(IRBB * bb, List<IR*> & lst);
-	void process_group();
-	void processSimply();
-	virtual void process();
+    void process_group_bb(IRBB * bb, List<IR*> & lst);
+    void process_group();
+    void processSimply();
+    virtual void process();
 };
 //END DexRegion
 
@@ -173,9 +176,9 @@ protected:
 public:
     DexRegionMgr();
     COPY_CONSTRUCTOR(DexRegionMgr);
-	virtual ~DexRegionMgr() {}
-	virtual Region * allocRegion(REGION_TYPE rt)
-	{ return new DexRegion(rt, this); }
+    virtual ~DexRegionMgr() {}
+    virtual Region * allocRegion(REGION_TYPE rt)
+    { return new DexRegion(rt, this); }
     virtual CallGraph * allocCallGraph(UINT edgenum, UINT vexnum)
     { return new DexCallGraph(edgenum, vexnum, this); }
 
@@ -195,57 +198,57 @@ public:
 };
 //END DexRegionMgr
 
-#define OBJ_MC_SIZE			16
-#define ARRAY_MC_SIZE		32
+#define OBJ_MC_SIZE            16
+#define ARRAY_MC_SIZE        32
 
-#define LIRC_num_of_op(l)	((l)->instrCount)
-#define LIRC_op(l, i)		((l)->lirList[i])
+#define LIRC_num_of_op(l)    ((l)->instrCount)
+#define LIRC_op(l, i)        ((l)->lirList[i])
 
-#define LIR_opcode(ir)		((ir)->opcode)
-#define LIR_name(ir)		(gLIROpcodeInfo.opNames[LIR_opcode(ir)])
-#define LIR_dt(ir)			((ir)->flags)
+#define LIR_opcode(ir)        ((ir)->opcode)
+#define LIR_name(ir)        (gLIROpcodeInfo.opNames[LIR_opcode(ir)])
+#define LIR_dt(ir)            ((ir)->flags)
 
 //For CONST
-#define LIR_int_imm(ir)		(((LIRConstOp*)ir)->vB)
+#define LIR_int_imm(ir)        (((LIRConstOp*)ir)->vB)
 
 //For SPUT
-#define LIR_sym_ptr(ir)		(((LIRABOp*)ir)->ptr)
-#define LIR_val(ir)			(((LIRABOp*)ir)->vA) //store value
+#define LIR_sym_ptr(ir)        (((LIRABOp*)ir)->ptr)
+#define LIR_val(ir)            (((LIRABOp*)ir)->vA) //store value
 
 //For ADD,SUB,MUL,DIV,REM,AND,OR,SHL,SHR,USHR,
 //XOR,CONST,NEW_INSTANCE.
-#define LIR_res(ir)				(((LIRABCOp*)ir)->vA)
+#define LIR_res(ir)                (((LIRABCOp*)ir)->vA)
 
 //For ADD,SUB,MUL,DIV,REM,AND,OR,SHL,SHR,USHR,
 //XOR,NEW_INSTANCE.
-#define LIR_op0(ir)				(((LIRABCOp*)ir)->vB)
+#define LIR_op0(ir)                (((LIRABCOp*)ir)->vB)
 
 //For ADD,SUB,MUL,DIV,REM,AND,OR,SHL,SHR,USHR,XOR
-#define LIR_op1(ir)				(((LIRABCOp*)ir)->vC)
+#define LIR_op1(ir)                (((LIRABCOp*)ir)->vC)
 
 
 #define LIR_fill_array_data_mark(ir) \
-		(((USHORT*)(((LIRSwitchOp*)ir)->data))[0])
+        (((USHORT*)(((LIRSwitchOp*)ir)->data))[0])
 #define LIR_switch_kind(ir) \
-		(((USHORT*)(((LIRSwitchOp*)ir)->data))[0])
+        (((USHORT*)(((LIRSwitchOp*)ir)->data))[0])
 #define LIR_case_num(ir) \
-		(((USHORT*)(((LIRSwitchOp*)ir)->data))[1])
+        (((USHORT*)(((LIRSwitchOp*)ir)->data))[1])
 #define LIR_packed_switch_base_value(ir) \
-		(*((INT*)(&(((LIRSwitchOp*)ir)->data[2]))))
+        (*((INT*)(&(((LIRSwitchOp*)ir)->data[2]))))
 #define LIR_packed_switch_case_entry(ir) \
-		((UINT*)(((USHORT*)((LIRSwitchOp*)ir)->data) + 4))
+        ((UINT*)(((USHORT*)((LIRSwitchOp*)ir)->data) + 4))
 #define LIR_sparse_switch_case_value(ir) \
-		((UINT*)(((USHORT*)(((LIRSwitchOp*)ir)->data)) + 2))
+        ((UINT*)(((USHORT*)(((LIRSwitchOp*)ir)->data)) + 2))
 #define LIR_sparse_switch_case_entry(ir) \
-		(UINT*)&(((BYTE*)(((LIRSwitchOp*)ir)->data))[4 + LIR_case_num(ir) * 4])
+        (UINT*)&(((BYTE*)(((LIRSwitchOp*)ir)->data))[4 + LIR_case_num(ir) * 4])
 
 typedef LIRBaseOp LIR;
 
 //Backfill data.
 class BackFillData {
 public:
-	IR * ir;
-	LIR * lir;
+    IR * ir;
+    LIR * lir;
 };
 
 //Export Functions.
@@ -255,9 +258,9 @@ void dump_all_lir(LIRCode * fu, DexFile * df, DexMethod const* dm);
 void dump_all_class_and_field(DexFile * df);
 inline bool is_wide(LIR * lir)
 {
-	return LIR_dt(lir) == LIR_JDT_wide ||
-			LIR_dt(lir) == LIR_JDT_double ||
-			LIR_dt(lir) == LIR_JDT_long;
+    return LIR_dt(lir) == LIR_JDT_wide ||
+            LIR_dt(lir) == LIR_JDT_double ||
+            LIR_dt(lir) == LIR_JDT_long;
 }
 
 //and Function Name.
@@ -274,8 +277,6 @@ void extractFunctionTypeFromFunctionTypeString(
         OUT CHAR * param_type,
         OUT CHAR * return_type);
 
-//Extract the right most sub-string from str.
-CHAR const* extractRightMostString(CHAR const* str, CHAR separator);
 void extractFunctionTypeFromRegionName(
         CHAR const* runame,
         OUT CHAR * param_type,
diff --git a/dex/dx_mgr.h b/dex/dx_mgr.h
index 4ccbecf..ee8cc53 100644
--- a/dex/dx_mgr.h
+++ b/dex/dx_mgr.h
@@ -405,21 +405,18 @@ Since the low 8-bit in metadata may look like DX_NOP, we
 need to check both the low and whole half-word
 to determine whether it is code or data.
 
-If instruction's low byte is NOP, and high byte is not zero,
+If instruction's low byte is NOP(0x0), and high byte is not zero,
 it is the inline data.
 */
 inline bool is_inline_data(USHORT const* ptr)
 {
     USHORT data = *ptr;
     DX_OPC opcode = (DX_OPC)(data & 0x00ff);
-    USHORT hw = data & 0xff00;
-    //if (!(opcode != DX_NOP || data == 0)) {
-    if (opcode == DX_NOP && hw != 0) {
-        /*
-        There are three kinds of inline data,
-        PACKED_SWITCH_DATA(0x100), SPARSE_SWITCH_DATA(0x200),
-        FILLED_ARRAY_DATA(0x300).
-        */
+    USHORT highbyte = data & 0xff00;
+    if (opcode == DX_NOP && highbyte != 0) {
+        //There are three kinds of inline data,
+        //PACKED_SWITCH_DATA(0x100), SPARSE_SWITCH_DATA(0x200),
+        //FILLED_ARRAY_DATA(0x300).
         return true;
     }
     return false;
diff --git a/dex/ir2dex.cpp b/dex/ir2dex.cpp
index b33ced6..436e99c 100644
--- a/dex/ir2dex.cpp
+++ b/dex/ir2dex.cpp
@@ -50,7 +50,7 @@ author: Su Zhenyu
 
 //Use outside pool to alloc memory. The pool will
 //be freed by caller.
-void * IR2Dex::_ymalloc(UINT size)
+void * IR2Dex::ymalloc(UINT size)
 {
     void * p = LIRMALLOC(size);
     ASSERT0(p);
@@ -67,7 +67,7 @@ LIR * IR2Dex::buildConstString(IN IR ** ir)
     UINT vx = get_vreg(STPR_no(tir));
     VAR * v = ID_info(LDA_base(STPR_rhs(tir)));
     CHAR const* n = SYM_name(VAR_name(v));
-    LIRABOp * lir = (LIRABOp*)_ymalloc(sizeof(LIRABOp));
+    LIRABOp * lir = (LIRABOp*)ymalloc(sizeof(LIRABOp));
     lir->opcode = LOP_CONST_STRING;
     LIR_dt(lir) = LIR_JDT_unknown; //see dir2lir.c
     lir->vA = vx;
@@ -85,7 +85,7 @@ LIR * IR2Dex::buildSput(IN IR ** ir)
     #ifdef _DEBUG_
     CHAR const* n = SYM_name(VAR_name(ST_idinfo(tir)));
     #endif
-    LIRABOp * lir = (LIRABOp*)_ymalloc(sizeof(LIRABOp));
+    LIRABOp * lir = (LIRABOp*)ymalloc(sizeof(LIRABOp));
     lir->opcode = LOP_SPUT;
     LIR_dt(lir) = get_lir_ty(TY_dtype(tir->get_type()));
     lir->vA = get_vreg(ST_rhs(tir));
@@ -105,7 +105,7 @@ LIR * IR2Dex::buildSgetBasicTypeVar(IN IR ** ir)
     VAR * v = LD_idinfo(STPR_rhs(tir));
     CHAR const* n = SYM_name(VAR_name(v));
     UINT field_id = m_var2ofst->mget(v);
-    LIRABOp * lir = (LIRABOp*)_ymalloc(sizeof(LIRABOp));
+    LIRABOp * lir = (LIRABOp*)ymalloc(sizeof(LIRABOp));
     lir->opcode = LOP_SGET;
     LIR_dt(lir) = get_lir_ty(TY_dtype(tir->get_type()));
     lir->vA = vx;
@@ -125,7 +125,7 @@ LIR * IR2Dex::buildSgetObj(IN IR ** ir)
     VAR * v = ID_info(LDA_base(STPR_rhs(tir)));
     CHAR const* n = SYM_name(VAR_name(v));
     UINT field_id = m_var2ofst->mget(v);
-    LIRABOp * lir = (LIRABOp*)_ymalloc(sizeof(LIRABOp));
+    LIRABOp * lir = (LIRABOp*)ymalloc(sizeof(LIRABOp));
     lir->opcode = LOP_SGET;
     LIR_dt(lir) = LIR_JDT_object;
     lir->vA = vx;
@@ -147,7 +147,7 @@ LIR * IR2Dex::buildMove(IN IR ** ir)
         *ir = IR_next(*ir);
         return NULL;
     }
-    LIRABOp * lir = (LIRABOp*)_ymalloc(sizeof(LIRABOp));
+    LIRABOp * lir = (LIRABOp*)ymalloc(sizeof(LIRABOp));
     lir->opcode = LOP_MOVE;
     lir->vA = tgt;
     lir->vB = src;
@@ -176,7 +176,7 @@ LIR * IR2Dex::buildCvt(IN IR ** ir)
     IR * rhs = STPR_rhs(tir);
     UINT vx = get_vreg(STPR_no(tir));
     UINT vy = get_vreg(CVT_exp(rhs));
-    LIRABOp * lir = (LIRABOp*)_ymalloc(sizeof(LIRABOp));
+    LIRABOp * lir = (LIRABOp*)ymalloc(sizeof(LIRABOp));
     lir->opcode = LOP_CONVERT;
     lir->vA = vx;
     lir->vB = vy;
@@ -236,7 +236,7 @@ LIR * IR2Dex::buildCvt(IN IR ** ir)
 LIR * IR2Dex::buildMoveResult(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_MOVE_RES));
-    LIRAOp * lir = (LIRAOp*)_ymalloc(sizeof(LIRAOp));
+    LIRAOp * lir = (LIRAOp*)ymalloc(sizeof(LIRAOp));
     lir->opcode = LOP_MOVE_RESULT;
 
     ASSERT0((*ir)->hasReturnValue());
@@ -264,7 +264,7 @@ LIR * IR2Dex::buildMoveResult(IN IR ** ir)
 LIR * IR2Dex::buildThrow(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_THROW));
-    LIRAOp * lir = (LIRAOp*)_ymalloc(sizeof(LIRAOp));
+    LIRAOp * lir = (LIRAOp*)ymalloc(sizeof(LIRAOp));
     lir->opcode = LOP_THROW;
     LIR_dt(lir) = LIR_JDT_unknown;
     IR * p = CALL_param_list(*ir);
@@ -280,7 +280,7 @@ LIR * IR2Dex::buildThrow(IN IR ** ir)
 LIR * IR2Dex::buildMonitorExit(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_MONITOR_EXIT));
-    LIRAOp * lir = (LIRAOp*)_ymalloc(sizeof(LIRAOp));
+    LIRAOp * lir = (LIRAOp*)ymalloc(sizeof(LIRAOp));
     lir->opcode = LOP_MONITOR_EXIT;
     LIR_dt(lir) = LIR_JDT_unknown;
     IR * p = CALL_param_list(*ir);
@@ -297,7 +297,7 @@ LIR * IR2Dex::buildMonitorExit(IN IR ** ir)
 LIR * IR2Dex::buildMonitorEnter(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_MONITOR_ENTER));
-    LIRAOp * lir = (LIRAOp*)_ymalloc(sizeof(LIRAOp));
+    LIRAOp * lir = (LIRAOp*)ymalloc(sizeof(LIRAOp));
     lir->opcode = LOP_MONITOR_ENTER;
     LIR_dt(lir) = LIR_JDT_unknown;
     IR * p = CALL_param_list(*ir);
@@ -314,7 +314,7 @@ LIR * IR2Dex::buildMonitorEnter(IN IR ** ir)
 LIR * IR2Dex::buildMoveException(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_MOVE_EXP));
-    LIRAOp * lir = (LIRAOp*)_ymalloc(sizeof(LIRAOp));
+    LIRAOp * lir = (LIRAOp*)ymalloc(sizeof(LIRAOp));
     lir->opcode = LOP_MOVE_EXCEPTION;
 
     ASSERT0((*ir)->hasReturnValue());
@@ -344,7 +344,7 @@ LIR * IR2Dex::buildMoveException(IN IR ** ir)
 LIR * IR2Dex::buildArrayLength(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_ARRAY_LENGTH));
-    LIRABOp * lir = (LIRABOp*)_ymalloc(sizeof(LIRABOp));
+    LIRABOp * lir = (LIRABOp*)ymalloc(sizeof(LIRABOp));
     lir->opcode = LOP_ARRAY_LENGTH; //see genInstruction()
     LIR_dt(lir) = LIR_JDT_unknown; //see genInstruction()
     IR * tir = *ir;
@@ -366,7 +366,7 @@ LIR * IR2Dex::buildArrayLength(IN IR ** ir)
 LIR * IR2Dex::buildCheckCast(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_CHECK_CAST));
-    LIRABOp * lir = (LIRABOp*)_ymalloc(sizeof(LIRABOp));
+    LIRABOp * lir = (LIRABOp*)ymalloc(sizeof(LIRABOp));
     lir->opcode = LOP_CHECK_CAST;
     IR * tir = *ir;
     IR * p = CALL_param_list(tir);
@@ -395,7 +395,7 @@ LIR * IR2Dex::buildConst(IN IR ** ir)
     ASSERT0(tir->is_stpr());
     ASSERT0(IR_type(STPR_rhs(tir)) == IR_CONST);
     UINT vx = get_vreg(STPR_no(tir));
-    LIRConstOp * lir = (LIRConstOp*)_ymalloc(sizeof(LIRConstOp));
+    LIRConstOp * lir = (LIRConstOp*)ymalloc(sizeof(LIRConstOp));
     lir->opcode = LOP_CONST;
     lir->vA = vx;
     lir->vB = CONST_int_val(STPR_rhs(tir));
@@ -493,7 +493,7 @@ LIR * IR2Dex::buildIget(IN IR ** ir)
     field_id /= m_d2ir->get_ofst_addend();
     ASSERT0(IR_type(objptr) == IR_PR);
     UINT vy = get_vreg(objptr);
-    LIRABCOp * lir = (LIRABCOp*)_ymalloc(sizeof(LIRABCOp));
+    LIRABCOp * lir = (LIRABCOp*)ymalloc(sizeof(LIRABCOp));
     lir->opcode = LOP_IGET;
     lir->vA = vx; //result reg
     lir->vB = vy; //base_ptr
@@ -514,7 +514,7 @@ LIR * IR2Dex::buildArray(IN IR ** ir)
     IR * base = ARR_base(STPR_rhs(tir));
     IR * ofst = ARR_sub_list(STPR_rhs(tir));
     ASSERT0(base->is_pr() && ofst->is_pr());
-    LIRABCOp * lir = (LIRABCOp*)_ymalloc(sizeof(LIRABCOp));
+    LIRABCOp * lir = (LIRABCOp*)ymalloc(sizeof(LIRABCOp));
     lir->opcode = LOP_AGET;
     LIR_dt(lir) = get_lir_ty(TY_dtype(tir->get_type()));
     lir->vA = get_vreg(STPR_no(tir));
@@ -554,7 +554,7 @@ LIR * IR2Dex::buildBinRegLit(IN IR ** ir)
     default: ASSERT0(0);
     }
 
-    LIRABCOp * lir = (LIRABCOp*)_ymalloc(sizeof(LIRABCOp));
+    LIRABCOp * lir = (LIRABCOp*)ymalloc(sizeof(LIRABCOp));
     lir->opcode = lty;
     lir->vA = vA;
     lir->vB = vB;
@@ -610,12 +610,12 @@ LIR * IR2Dex::buildBinRegReg(IN IR ** ir)
 
     LIR * lir;
     if (is_assign_equ) {
-        lir = (LIR*)_ymalloc(sizeof(LIRABOp));
+        lir = (LIR*)ymalloc(sizeof(LIRABOp));
         lir->opcode = lty;
         ((LIRABOp*)lir)->vA = vA;
         ((LIRABOp*)lir)->vB = vC;
     } else {
-        lir = (LIR*)_ymalloc(sizeof(LIRABCOp));
+        lir = (LIR*)ymalloc(sizeof(LIRABCOp));
         lir->opcode = lty;
         ((LIRABCOp*)lir)->vA = vA;
         ((LIRABCOp*)lir)->vB = vB;
@@ -661,7 +661,7 @@ LIR * IR2Dex::buildUniOp(IN IR ** ir)
         ASSERT0(0);
     }
 
-    LIR * lir = (LIR*)_ymalloc(sizeof(LIRABOp));
+    LIR * lir = (LIR*)ymalloc(sizeof(LIRABOp));
     lir->opcode = lty;
     ((LIRABOp*)lir)->vA = get_vreg(STPR_no(tir));
     ((LIRABOp*)lir)->vB = get_vreg(op0);
@@ -750,7 +750,7 @@ LIR * IR2Dex::convertStoreArray(IN OUT IR ** ir, IN IR2DexCtx * cont)
 {
     IR * tir = *ir;
     ASSERT0(tir->is_starray());
-    LIRABCOp * lir = (LIRABCOp*)_ymalloc(sizeof(LIRABCOp));
+    LIRABCOp * lir = (LIRABCOp*)ymalloc(sizeof(LIRABCOp));
     IR * rhs = STARR_rhs(tir);
     IR * lhs = ARR_base(tir);
     ASSERT0(rhs->is_pr());
@@ -779,7 +779,7 @@ LIR * IR2Dex::convertIstore(IN OUT IR ** ir, IN IR2DexCtx * cont)
 {
     IR * tir = *ir;
     ASSERT0(tir->is_ist());
-    LIRABCOp * lir = (LIRABCOp*)_ymalloc(sizeof(LIRABCOp));
+    LIRABCOp * lir = (LIRABCOp*)ymalloc(sizeof(LIRABCOp));
     IR * rhs = IST_rhs(tir);
     IR * lhs = IST_base(tir);
     ASSERT0(rhs->is_pr());
@@ -800,7 +800,7 @@ LIR * IR2Dex::convertIstore(IN OUT IR ** ir, IN IR2DexCtx * cont)
 LIR * IR2Dex::buildInvoke(IN IR ** ir)
 {
     IR * tir = *ir;
-    LIRInvokeOp * lir = (LIRInvokeOp*)_ymalloc(sizeof(LIRInvokeOp));
+    LIRInvokeOp * lir = (LIRInvokeOp*)ymalloc(sizeof(LIRInvokeOp));
     lir->opcode = LOP_INVOKE;
 
     IR * p = CALL_param_list(tir);
@@ -847,7 +847,7 @@ LIR * IR2Dex::buildInvoke(IN IR ** ir)
         t = IR_next(t);
     }
     if (i > 0) {
-        lir->args = (USHORT*)_ymalloc(i * sizeof(USHORT));
+        lir->args = (USHORT*)ymalloc(i * sizeof(USHORT));
     }
     lir->argc = i;
     INT j = 0;
@@ -872,7 +872,7 @@ LIR * IR2Dex::buildInvoke(IN IR ** ir)
 LIR * IR2Dex::buildNewInstance(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_NEW));
-    LIRABOp * lir = (LIRABOp*)_ymalloc(sizeof(LIRABOp));
+    LIRABOp * lir = (LIRABOp*)ymalloc(sizeof(LIRABOp));
     lir->opcode = LOP_NEW_INSTANCE;
     IR * tir = *ir;
     //class-id
@@ -901,7 +901,7 @@ NOTE: it is very different with filled-new-array.
 LIR * IR2Dex::buildFillArrayData(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_FILL_ARRAY_DATA));
-    LIRSwitchOp * lir = (LIRSwitchOp*)_ymalloc(sizeof(LIRSwitchOp));
+    LIRSwitchOp * lir = (LIRSwitchOp*)ymalloc(sizeof(LIRSwitchOp));
     lir->opcode = LOP_FILL_ARRAY_DATA;
     IR * tir = *ir;
     IR * p = CALL_param_list(tir);
@@ -942,7 +942,7 @@ filled-new-array instruction.
 LIR * IR2Dex::buildFilledNewArray(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_FILLED_NEW_ARRAY));
-    LIRInvokeOp * lir = (LIRInvokeOp*)_ymalloc(sizeof(LIRInvokeOp));
+    LIRInvokeOp * lir = (LIRInvokeOp*)ymalloc(sizeof(LIRInvokeOp));
     lir->opcode = LOP_FILLED_NEW_ARRAY;
     IR * tir = *ir;
     IR * p = CALL_param_list(tir);
@@ -966,7 +966,7 @@ LIR * IR2Dex::buildFilledNewArray(IN IR ** ir)
         i++;
     }
     lir->argc = i;
-    lir->args = (UInt16*)_ymalloc(sizeof(UInt16) * i);
+    lir->args = (UInt16*)ymalloc(sizeof(UInt16) * i);
     i = 0;
     while (p != NULL) {
         lir->args[i] = get_vreg(p);
@@ -988,7 +988,7 @@ LIR * IR2Dex::buildConstClass(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_CONST_CLASS));
     IR * tir = *ir;
-    LIRABOp * lir = (LIRABOp*)_ymalloc(sizeof(LIRABOp));
+    LIRABOp * lir = (LIRABOp*)ymalloc(sizeof(LIRABOp));
     lir->opcode = LOP_CONST_CLASS;
     LIR_dt(lir) = LIR_JDT_unknown;
 
@@ -1013,7 +1013,7 @@ LIR * IR2Dex::buildNewArray(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_NEW_ARRAY));
     IR * tir = *ir;
-    LIRABCOp * lir = (LIRABCOp*)_ymalloc(sizeof(LIRABCOp));
+    LIRABCOp * lir = (LIRABCOp*)ymalloc(sizeof(LIRABCOp));
     lir->opcode = LOP_NEW_ARRAY;
     LIR_dt(lir) = LIR_JDT_unknown; //see dir2lir.c
 
@@ -1048,7 +1048,7 @@ LIR * IR2Dex::buildInstanceOf(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_INSTANCE_OF));
     IR * tir = *ir;
-    LIRABCOp * lir = (LIRABCOp*)_ymalloc(sizeof(LIRABCOp));
+    LIRABCOp * lir = (LIRABCOp*)ymalloc(sizeof(LIRABCOp));
     lir->opcode = LOP_INSTANCE_OF;
     LIR_dt(lir) = LIR_JDT_unknown;
 
@@ -1086,7 +1086,7 @@ LIR * IR2Dex::buildCmpBias(IN IR ** ir)
 {
     ASSERT0(m_d2ir->is_builtin(*ir, BLTIN_CMP_BIAS));
     IR * tir = *ir;
-    LIRABCOp * lir = (LIRABCOp*)_ymalloc(sizeof(LIRABCOp));
+    LIRABCOp * lir = (LIRABCOp*)ymalloc(sizeof(LIRABCOp));
 
     //cmp-kind
     IR * p = CALL_param_list(tir);
@@ -1184,7 +1184,7 @@ LIR * IR2Dex::convertIcall(IN OUT IR ** ir, IN IR2DexCtx * cont)
 //return vAA
 LIR * IR2Dex::convertReturn(IN OUT IR ** ir, IN IR2DexCtx * cont)
 {
-    LIRAOp * lir = (LIRAOp*)_ymalloc(sizeof(LIRAOp));
+    LIRAOp * lir = (LIRAOp*)ymalloc(sizeof(LIRAOp));
     lir->opcode = LOP_RETURN;
 
     IR * retval = RET_exp(*ir);
@@ -1215,7 +1215,7 @@ LIR * IR2Dex::convertReturn(IN OUT IR ** ir, IN IR2DexCtx * cont)
 //AABBBBBBBB
 LIR * IR2Dex::convertGoto(IN OUT IR ** ir, IN IR2DexCtx * cont)
 {
-    LIRGOTOOp * lir = (LIRGOTOOp*)_ymalloc(sizeof(LIRGOTOOp));
+    LIRGOTOOp * lir = (LIRGOTOOp*)ymalloc(sizeof(LIRGOTOOp));
     lir->opcode = LOP_GOTO;
     lir->target = 0xFFFFffff; //backfill after all LIR generated.
 
@@ -1236,13 +1236,13 @@ LIR * IR2Dex::convertBranch(bool is_truebr, IN OUT IR ** ir,
     IR * det = BR_det(*ir);
     if (IR_type(BIN_opnd1(det)) == IR_CONST &&
         CONST_int_val(BIN_opnd1(det)) == 0) {
-        lir = (LIR*)_ymalloc(sizeof(LIRABOp));
+        lir = (LIR*)ymalloc(sizeof(LIRABOp));
         LIR_opcode(lir) = LOP_IFZ;
         LIR_res(lir) = get_vreg(BIN_opnd0(det));
         LIR_op0(lir) = (UInt32)0xFFFFffff; //target.
     } else {
         ASSERT0(IR_type(BIN_opnd1(det)) == IR_PR);
-        lir = (LIR*)_ymalloc(sizeof(LIRABCOp));
+        lir = (LIR*)ymalloc(sizeof(LIRABCOp));
         LIR_opcode(lir) = LOP_IF;
         LIR_res(lir) = get_vreg(BIN_opnd0(det));
         LIR_op0(lir) = get_vreg(BIN_opnd1(det));
@@ -1283,7 +1283,7 @@ LIR * IR2Dex::convertBranch(bool is_truebr, IN OUT IR ** ir,
 //AABBBBBBBB
 LIR * IR2Dex::convertSwitch(IN OUT IR ** ir, IN IR2DexCtx * cont)
 {
-    LIRSwitchOp * lir = (LIRSwitchOp*)_ymalloc(sizeof(LIRSwitchOp));
+    LIRSwitchOp * lir = (LIRSwitchOp*)ymalloc(sizeof(LIRSwitchOp));
     IR * vexp = SWITCH_vexp(*ir);
     ASSERT0(IR_type(vexp) == IR_PR);
     lir->value = get_vreg(vexp);
@@ -1316,22 +1316,24 @@ LIR * IR2Dex::convertSwitch(IN OUT IR ** ir, IN IR2DexCtx * cont)
         }
 
         if (is_packed) {
-            lir->data = (USHORT*)_ymalloc((1+1+2)*sizeof(USHORT) +
+            lir->data = (USHORT*)ymalloc((1+1+2)*sizeof(USHORT) +
                                            num_of_case*sizeof(UINT));
             LIR_switch_kind(lir) = 0x100;
             LIR_case_num(lir) = num_of_case;
             LIR_packed_switch_base_value(lir) = base_val;
             UINT * pcase_entry = LIR_packed_switch_case_entry(lir);
+            ASSERT(sizeof(UINT) == 4, ("case entry is 32bit"));
             for (UINT i = 0; i < num_of_case; i++) {
-                pcase_entry[i] = 0xFFFFffff; //case entry is 32bit.
+                pcase_entry[i] = (UINT)0xffffFFFF; //case entry is 32bit.
             }
         } else {
-            lir->data = (USHORT*)_ymalloc((1+1)*sizeof(USHORT) +
-                                           num_of_case*sizeof(UINT)*2);
+            lir->data = (USHORT*)ymalloc((1+1)*sizeof(USHORT) +
+                                          num_of_case*sizeof(UINT)*2);
             LIR_switch_kind(lir) = 0x200;
             LIR_case_num(lir) = num_of_case;
             //((BYTE*)data)[4..4+num_of_case*4-1]: the case-value buffer.
             UINT * pcase_value = LIR_sparse_switch_case_value(lir);
+            ASSERT(sizeof(UINT) == 4, ("case value is 32bit"));
             IR * x = case_list;
             for (UINT i = 0; i < num_of_case; i++) {
                 pcase_value[i] = CONST_int_val(CASE_vexp(x));
@@ -1342,7 +1344,7 @@ LIR * IR2Dex::convertSwitch(IN OUT IR ** ir, IN IR2DexCtx * cont)
             //    the position of the index table is at current instruction.
             UINT * pcase_entry = LIR_sparse_switch_case_entry(lir);
             for (UINT i = 0; i < num_of_case; i++) {
-                pcase_entry[i] = 0xFFFFffff; //case entry is 32bit.
+                pcase_entry[i] = (UINT)0xFFFFffff; //case entry is 32bit.
             }
         }
     }
diff --git a/dex/ir2dex.h b/dex/ir2dex.h
index caab37c..19efca9 100644
--- a/dex/ir2dex.h
+++ b/dex/ir2dex.h
@@ -94,7 +94,7 @@ protected:
 
     //Use outside pool to alloc memory. The pool will
     //be freed by caller.
-    void * _ymalloc(UINT size);
+    void * ymalloc(UINT size);
 public:
     IR2Dex(IN Region * ru, IN DexFile * df)
     {
diff --git a/opt/ai.h b/opt/ai.h
index 782b7e9..064be5c 100644
--- a/opt/ai.h
+++ b/opt/ai.h
@@ -73,9 +73,10 @@ public:
 };
 
 
+typedef SimpleVec<BaseAttachInfo*, 1> AICont;
 class AttachInfo {
-public:
-    SimpleVec<BaseAttachInfo*, 1> cont;
+protected:
+    AICont cont;
 
 public:
     AttachInfo() { init(); }
@@ -151,6 +152,23 @@ public:
         }
         return NULL;
     }
+
+    AICont const& read_cont() const { return cont; }
+
+    CHAR const* get_ai_name(AI_TYPE type) const
+    {
+        switch (type) {
+        case AI_UNDEF: return "Undef";
+        case AI_DBX: return "Dbx";
+        case AI_PROF: return "Prof";
+        case AI_TBAA: return "Tbaa";
+        case AI_EH_LABEL: return "EH";
+        case AI_USER_DEF: return "User";
+        case AI_LAST:;
+        default: ASSERT0(0);
+        }
+        return NULL;
+    }
 };
 
 
@@ -164,13 +182,16 @@ public:
     { init(pool); }
     COPY_CONSTRUCTOR(EHLabelAttachInfo);
 
+    //This function must be invoked during construction.
     void init(SMemPool * pool)
     {
         BaseAttachInfo::init(AI_EH_LABEL);
+        labels.init();
         labels.set_pool(pool);
     }
 
-    SList<LabelInfo*> const& get_labels() const { return labels; }
+    SList<LabelInfo*> const& read_labels() const { return labels; }
+    SList<LabelInfo*> & get_labels() { return labels; }
 };
 
 
diff --git a/opt/cfg.h b/opt/cfg.h
index 6802641..cde30ee 100644
--- a/opt/cfg.h
+++ b/opt/cfg.h
@@ -237,18 +237,17 @@ public:
             BB * bb = ct->val();
             ASSERT0(bb);
 
-            Vertex * vex = get_vertex(bb->id);
-            ASSERT(vex, ("No vertex corresponds to BB%d", bb->id));
-            if (comp_entry) {
-                if (VERTEX_in_list(vex) == NULL &&
-                    (is_ru_entry(bb) || bb->is_exp_handling())) {
-                    m_entry_list.append_tail(bb);
-                }
+            ASSERT(get_vertex(bb->id),
+                   ("No vertex corresponds to BB%d", bb->id));
+
+            if (comp_entry && is_ru_entry(bb)) {
+                m_entry_list.append_tail(bb);
             }
-            if (comp_exit) {
-                if (VERTEX_out_list(vex) == NULL) {
-                    m_exit_list.append_tail(bb);
-                }
+
+            if (comp_exit &&
+                (is_ru_exit(bb) ||
+                 VERTEX_out_list(get_vertex(bb->id)) == NULL)) {
+                m_exit_list.append_tail(bb);
             }
         }
     }
@@ -1116,22 +1115,11 @@ bool CFG<BB, XR>::removeUnreachBB()
 
     ASSERT(m_entry_list.get_elem_count() > 0,
             ("call computeEntryAndExit first"));
+    ASSERT(m_entry_list.get_elem_count() == 1, ("Only support SEME"));
 
-    if (m_entry_list.get_elem_count() == 1) {
-        BB * entry = m_entry_list.get_head();
-        ASSERT(entry != NULL, ("need entry bb"));
-        if (!visited.is_contain(entry->id)) {
-            _remove_unreach_bb(entry->id, visited);
-        }
-    } else { //multiple entries
-        C<BB*> * ct;
-        for (m_entry_list.get_head(&ct);
-             ct != m_entry_list.end(); ct = m_entry_list.get_next(ct)) {
-            BB * bb = ct->val();
-            if (!visited.is_contain(bb->id)) {
-                _remove_unreach_bb(bb->id, visited);
-            }
-        }
+    BB * entry = m_entry_list.get_head();
+    if (!visited.is_contain(entry->id)) {
+        _remove_unreach_bb(entry->id, visited);
     }
 
     C<BB*> * next_ct;
@@ -1140,8 +1128,10 @@ bool CFG<BB, XR>::removeUnreachBB()
         BB * bb = ct->val();
         next_ct = m_bb_list->get_next(ct);
         if (!visited.is_contain(bb->id)) {
-            ASSERT(!bb->is_exp_handling(),
-                ("For conservative purpose, exception handler should be reserved."));
+            //EH may be redundant and can be removed.
+            //ASSERT(!bb->is_exp_handling(),
+            // ("For conservative purpose, exception handler should be reserved."));
+
             bb->removeSuccessorPhiOpnd(this);
             remove_bb(ct);
             removed = true;
diff --git a/opt/ir.cpp b/opt/ir.cpp
index 61d40f3..3a59bb3 100644
--- a/opt/ir.cpp
+++ b/opt/ir.cpp
@@ -47,64 +47,64 @@ namespace xoc {
 #endif
 
 IRDesc const g_ir_desc[] = {
-    {IR_UNDEF,        "undef",        0x0,    0,    0,                                                0                    },
-    {IR_CONST,        "const",        0x0,    0,    IRT_IS_LEAF,                                    sizeof(CConst)        },
-    {IR_ID,            "id",            0x0,    0,    IRT_IS_LEAF,                                    sizeof(CId)            },
-    {IR_LD,            "ld",            0x0,    0,    IRT_IS_MEM_REF|IRT_IS_MEM_OPND|IRT_IS_LEAF,        sizeof(CLd)            },
-    {IR_ILD,        "ild",            0x1,    1,    IRT_IS_UNA|IRT_IS_MEM_REF|IRT_IS_MEM_OPND,        sizeof(CIld)        },
-    {IR_PR,         "pr",            0x0,    0,    IRT_IS_MEM_REF|IRT_IS_MEM_OPND|IRT_IS_LEAF,     sizeof(CPr)         },
-    {IR_ARRAY,        "array",        0x3,    2,    IRT_IS_MEM_REF|IRT_IS_MEM_OPND,                 sizeof(CArray)        },
-    {IR_ST,            "st",            0x1,    1,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,        sizeof(CSt)            },
-    {IR_STPR,        "stpr",            0x1,    1,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,        sizeof(CStpr)        },
-    {IR_STARRAY,    "starray",        0x7,    3,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,        sizeof(CStArray)    },
-    {IR_IST,        "ist",            0x3,    2,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,        sizeof(CIst)        },
-    {IR_SETELEM,     "setepr",        0x3,    2,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,        sizeof(CSetElem)     },
-    {IR_GETELEM,     "getepr",        0x3,    2,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,        sizeof(CGetElem)     },
-    {IR_CALL,        "call",            0x1,    1,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,        sizeof(CCall)        },
-    {IR_ICALL,        "icall",        0x3,    2,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,        sizeof(CICall)        },
-    {IR_LDA,        "lda",            0x1,    1,    IRT_IS_UNA,                                     sizeof(CLda)        },
-    {IR_ADD,        "add",            0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE,    sizeof(CBin)    },
-    {IR_SUB,        "sub",            0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE,                        sizeof(CBin)    },
-    {IR_MUL,        "mul",            0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE,    sizeof(CBin)    },
-    {IR_DIV,        "div",            0x3,    2,    IRT_IS_BIN,                                        sizeof(CBin)        },
-    {IR_REM,        "rem",            0x3,    2,    IRT_IS_BIN,                                        sizeof(CBin)        },
-    {IR_MOD,        "mod",            0x3,    2,    IRT_IS_BIN,                                        sizeof(CBin)        },
-    {IR_LAND,        "land",            0x3,    2,    IRT_IS_BIN|IRT_IS_LOGICAL,                        sizeof(CBin)        },
-    {IR_LOR,        "lor",            0x3,    2,    IRT_IS_BIN|IRT_IS_LOGICAL,                        sizeof(CBin)        },
-    {IR_BAND,        "band",            0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE,    sizeof(CBin)    },
-    {IR_BOR,        "bor",            0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE,                        sizeof(CBin)    },
-    {IR_XOR,        "xor",            0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE,    sizeof(CBin)    },
-    {IR_ASR,        "asr",            0x3,    2,    IRT_IS_BIN,                                     sizeof(CBin)        },
-    {IR_LSR,        "lsr",            0x3,    2,    IRT_IS_BIN,                                     sizeof(CBin)        },
-    {IR_LSL,        "lsl",            0x3,    2,    IRT_IS_BIN,                                     sizeof(CBin)        },
-    {IR_LT,         "lt",            0x3,    2,    IRT_IS_BIN|IRT_IS_RELATION,                     sizeof(CBin)        },
-    {IR_LE,         "le",            0x3,    2,    IRT_IS_BIN|IRT_IS_RELATION,                     sizeof(CBin)        },
-    {IR_GT,         "gt",            0x3,    2,    IRT_IS_BIN|IRT_IS_RELATION,                     sizeof(CBin)        },
-    {IR_GE,         "ge",            0x3,    2,    IRT_IS_BIN|IRT_IS_RELATION,                     sizeof(CBin)        },
-    {IR_EQ,         "eq",            0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE|IRT_IS_RELATION,    sizeof(CBin)        },
-    {IR_NE,         "ne",            0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE|IRT_IS_RELATION,    sizeof(CBin)        },
-    {IR_BNOT,        "bnot",            0x1,    1,    IRT_IS_UNA,                                        sizeof(CUna)        },
-    {IR_LNOT,        "lnot",            0x1,    1,    IRT_IS_UNA|IRT_IS_LOGICAL,                        sizeof(CUna)        },
-    {IR_NEG,        "neg",            0x1,    1,    IRT_IS_UNA,                                        sizeof(CUna)        },
-    {IR_CVT,        "cvt",            0x1,    1,    IRT_IS_UNA,                                     sizeof(CCvt)        },
-    {IR_GOTO,        "goto",            0x0,    0,    IRT_IS_STMT,                                    sizeof(CGoto)        },
-    {IR_IGOTO,        "igoto",        0x3,    2,    IRT_IS_STMT,                                    sizeof(CIGoto)        },
-    {IR_DO_WHILE,    "do_while",        0x3,    2,    IRT_IS_STMT,                                    sizeof(CDoWhile)    },
-    {IR_WHILE_DO,    "while_do",        0x3,    2,    IRT_IS_STMT,                                    sizeof(CWhileDo)    },
-    {IR_DO_LOOP,    "do_loop",        0xF,    4,    IRT_IS_STMT,                                    sizeof(CDoLoop)    },
-    {IR_IF,            "if",            0x7,    3,    IRT_IS_STMT,                                    sizeof(CIf)            },
-    {IR_LABEL,        "label",        0x0,    0,    IRT_IS_STMT,                                    sizeof(CLab)        },
-    {IR_SWITCH,        "switch",        0x7,    3,    IRT_IS_STMT,                                    sizeof(CSwitch)        },
-    {IR_CASE,        "case",            0x1,    1,    0,                                                sizeof(CCase)        },
-    {IR_TRUEBR,        "truebr",        0x1,    1,    IRT_IS_STMT,                                    sizeof(CTruebr)        },
-    {IR_FALSEBR,    "falsebr",        0x1,    1,    IRT_IS_STMT,                                    sizeof(CFalsebr)    },
-    {IR_RETURN,        "return",        0x1,    1,    IRT_IS_STMT,                                    sizeof(CRet)        },
-    {IR_SELECT,        "select",        0x7,    3,    0,                                                sizeof(CSelect)        },
-    {IR_BREAK,        "break",        0x0,    0,    IRT_IS_STMT,                                    sizeof(CBreak)        },
-    {IR_CONTINUE,    "continue",        0x0,    0,    IRT_IS_STMT,                                    sizeof(CContinue)    },
-    {IR_PHI,        "phi",            0x1,    1,    IRT_IS_STMT|IRT_HAS_RESULT|IRT_IS_MEM_REF,        sizeof(CPhi)        },
-    {IR_REGION,        "region",        0x0,    0,    IRT_IS_STMT,                                    sizeof(CRegion)        },
-    {IR_TYPE_NUM,    "LAST IR Type",    0x0,    0,    0,                                                0                    },
+    {IR_UNDEF,      "undef",     0x0,    0,    0,                                              0                },
+    {IR_CONST,      "const",     0x0,    0,    IRT_IS_LEAF,                                    sizeof(CConst)   },
+    {IR_ID,         "id",        0x0,    0,    IRT_IS_LEAF,                                    sizeof(CId)      },
+    {IR_LD,         "ld",        0x0,    0,    IRT_IS_MEM_REF|IRT_IS_MEM_OPND|IRT_IS_LEAF,     sizeof(CLd)      },
+    {IR_ILD,        "ild",       0x1,    1,    IRT_IS_UNA|IRT_IS_MEM_REF|IRT_IS_MEM_OPND,      sizeof(CIld)     },
+    {IR_PR,         "pr",        0x0,    0,    IRT_IS_MEM_REF|IRT_IS_MEM_OPND|IRT_IS_LEAF,     sizeof(CPr)      },
+    {IR_ARRAY,      "array",     0x3,    2,    IRT_IS_MEM_REF|IRT_IS_MEM_OPND,                 sizeof(CArray)   },
+    {IR_ST,         "st",        0x1,    1,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,      sizeof(CSt)      },
+    {IR_STPR,       "stpr",      0x1,    1,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,      sizeof(CStpr)    },
+    {IR_STARRAY,    "starray",   0x7,    3,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,      sizeof(CStArray) },
+    {IR_IST,        "ist",       0x3,    2,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,      sizeof(CIst)     },
+    {IR_SETELEM,    "setepr",    0x3,    2,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,      sizeof(CSetElem) },
+    {IR_GETELEM,    "getepr",    0x3,    2,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,      sizeof(CGetElem) },
+    {IR_CALL,       "call",      0x1,    1,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,      sizeof(CCall)    },
+    {IR_ICALL,      "icall",     0x3,    2,    IRT_IS_STMT|IRT_IS_MEM_REF|IRT_HAS_RESULT,      sizeof(CICall)   },
+    {IR_LDA,        "lda",       0x1,    1,    IRT_IS_UNA,                                     sizeof(CLda)     },
+    {IR_ADD,        "add",       0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE, sizeof(CBin)   },
+    {IR_SUB,        "sub",       0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE,                    sizeof(CBin)   },
+    {IR_MUL,        "mul",       0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE, sizeof(CBin)   },
+    {IR_DIV,        "div",       0x3,    2,    IRT_IS_BIN,                                       sizeof(CBin)   },
+    {IR_REM,        "rem",       0x3,    2,    IRT_IS_BIN,                                       sizeof(CBin)   },
+    {IR_MOD,        "mod",       0x3,    2,    IRT_IS_BIN,                                       sizeof(CBin)   },
+    {IR_LAND,       "land",      0x3,    2,    IRT_IS_BIN|IRT_IS_LOGICAL,                        sizeof(CBin)   },
+    {IR_LOR,        "lor",       0x3,    2,    IRT_IS_BIN|IRT_IS_LOGICAL,                        sizeof(CBin)   },
+    {IR_BAND,       "band",      0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE, sizeof(CBin)   },
+    {IR_BOR,        "bor",       0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE,                    sizeof(CBin)   },
+    {IR_XOR,        "xor",       0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE, sizeof(CBin)   },
+    {IR_ASR,        "asr",       0x3,    2,    IRT_IS_BIN,                                       sizeof(CBin)   },
+    {IR_LSR,        "lsr",       0x3,    2,    IRT_IS_BIN,                                       sizeof(CBin)   },
+    {IR_LSL,        "lsl",       0x3,    2,    IRT_IS_BIN,                                       sizeof(CBin)   },
+    {IR_LT,         "lt",        0x3,    2,    IRT_IS_BIN|IRT_IS_RELATION,                       sizeof(CBin)   },
+    {IR_LE,         "le",        0x3,    2,    IRT_IS_BIN|IRT_IS_RELATION,                       sizeof(CBin)   },
+    {IR_GT,         "gt",        0x3,    2,    IRT_IS_BIN|IRT_IS_RELATION,                       sizeof(CBin)   },
+    {IR_GE,         "ge",        0x3,    2,    IRT_IS_BIN|IRT_IS_RELATION,                       sizeof(CBin)   },
+    {IR_EQ,         "eq",        0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE|IRT_IS_RELATION,  sizeof(CBin)        },
+    {IR_NE,         "ne",        0x3,    2,    IRT_IS_BIN|IRT_IS_ASSOCIATIVE|IRT_IS_COMMUTATIVE|IRT_IS_RELATION,  sizeof(CBin)        },
+    {IR_BNOT,       "bnot",      0x1,    1,    IRT_IS_UNA,                                     sizeof(CUna)     },
+    {IR_LNOT,       "lnot",      0x1,    1,    IRT_IS_UNA|IRT_IS_LOGICAL,                      sizeof(CUna)     },
+    {IR_NEG,        "neg",       0x1,    1,    IRT_IS_UNA,                                     sizeof(CUna)     },
+    {IR_CVT,        "cvt",       0x1,    1,    IRT_IS_UNA,                                     sizeof(CCvt)     },
+    {IR_GOTO,       "goto",      0x0,    0,    IRT_IS_STMT,                                    sizeof(CGoto)    },
+    {IR_IGOTO,      "igoto",     0x3,    2,    IRT_IS_STMT,                                    sizeof(CIGoto)   },
+    {IR_DO_WHILE,   "do_while",  0x3,    2,    IRT_IS_STMT,                                    sizeof(CDoWhile) },
+    {IR_WHILE_DO,   "while_do",  0x3,    2,    IRT_IS_STMT,                                    sizeof(CWhileDo) },
+    {IR_DO_LOOP,    "do_loop",   0xF,    4,    IRT_IS_STMT,                                    sizeof(CDoLoop)  },
+    {IR_IF,         "if",        0x7,    3,    IRT_IS_STMT,                                    sizeof(CIf)      },
+    {IR_LABEL,      "label",     0x0,    0,    IRT_IS_STMT,                                    sizeof(CLab)     },
+    {IR_SWITCH,     "switch",    0x7,    3,    IRT_IS_STMT,                                    sizeof(CSwitch)  },
+    {IR_CASE,       "case",      0x1,    1,    0,                                              sizeof(CCase)    },
+    {IR_TRUEBR,     "truebr",    0x1,    1,    IRT_IS_STMT,                                    sizeof(CTruebr)  },
+    {IR_FALSEBR,    "falsebr",   0x1,    1,    IRT_IS_STMT,                                    sizeof(CFalsebr) },
+    {IR_RETURN,     "return",    0x1,    1,    IRT_IS_STMT,                                    sizeof(CRet)     },
+    {IR_SELECT,     "select",    0x7,    3,    0,                                              sizeof(CSelect)  },
+    {IR_BREAK,      "break",     0x0,    0,    IRT_IS_STMT,                                    sizeof(CBreak)   },
+    {IR_CONTINUE,   "continue",  0x0,    0,    IRT_IS_STMT,                                    sizeof(CContinue)},
+    {IR_PHI,        "phi",       0x1,    1,    IRT_IS_STMT|IRT_HAS_RESULT|IRT_IS_MEM_REF,      sizeof(CPhi)     },
+    {IR_REGION,     "region",    0x0,    0,    IRT_IS_STMT,                                    sizeof(CRegion)  },
+    {IR_TYPE_NUM,   "LAST IR Type", 0x0, 0,    0,                                              0                },
 };
 
 
@@ -403,7 +403,6 @@ void dump_irs(IRList & ir_list, TypeMgr const* dm)
 }
 
 
-#ifdef _DEBUG_
 static void ir_dump_lab(IR const* ir)
 {
     LabelInfo * li = ir->get_label();
@@ -417,13 +416,37 @@ static void ir_dump_lab(IR const* ir)
         ASSERT(0, ("unknown label type"));
     }
 }
-#endif
+
+
+static void dump_ai(OUT CHAR * buf, IR const* ir)
+{
+    ASSERT0(ir && buf);
+    AttachInfo const* ai = ir->get_ai();
+    if (ai == NULL) { return; }
+
+    AICont const& cont = ai->read_cont();
+
+    if (!cont.is_init()) { return; }
+
+    strcat(buf, " ai:");
+    CHAR * p = buf + strlen(buf);
+    for (UINT i = 0; i < cont.get_capacity(); i++) {
+        BaseAttachInfo * ac = cont.get(i);
+        if (ac == NULL) { continue; }
+        sprintf(p, "%s,", ai->get_ai_name(ac->type));
+        p = p + strlen(p);
+    }
+}
 
 
 //Dump IR and all of its kids.
 //'attr': miscellaneous string which following 'ir'.
-void dump_ir(IN IR const* ir, TypeMgr const* dm, IN CHAR * attr,
-             bool dump_kid, bool dump_src_line, bool dump_addr)
+void dump_ir(IN IR const* ir,
+             TypeMgr const* dm,
+             IN CHAR * attr,
+             bool dump_kid,
+             bool dump_src_line,
+             bool dump_addr)
 {
     UNUSED(dump_addr);
     UNUSED(dump_src_line);
@@ -432,7 +455,6 @@ void dump_ir(IN IR const* ir, TypeMgr const* dm, IN CHAR * attr,
     UNUSED(dm);
     UNUSED(ir);
 
-    #ifdef _DEBUG_
     ASSERT0(dm);
     UINT dn = 4;
     if (g_tfile == NULL || ir == NULL) { return; }
@@ -465,6 +487,8 @@ void dump_ir(IN IR const* ir, TypeMgr const* dm, IN CHAR * attr,
         strcat(p, " sideeffect");
     }
 
+    dump_ai(p, ir);
+
     //Record type info and var decl.
     static CHAR buf[MAX_BUF_LEN];
     static CHAR buf2[MAX_BUF_LEN];
@@ -1177,7 +1201,6 @@ void dump_ir(IN IR const* ir, TypeMgr const* dm, IN CHAR * attr,
         return ;
     } //end switch
     fflush(g_tfile);
-    #endif
 } //end dump_ir
 
 
diff --git a/opt/ir_cfg.cpp b/opt/ir_cfg.cpp
index bd9d9a5..4d74d02 100644
--- a/opt/ir_cfg.cpp
+++ b/opt/ir_cfg.cpp
@@ -142,7 +142,7 @@ void IR_CFG::initCfg(OptCTX & oc)
 {
     //cfg->removeEmptyBB();
     build(oc);
-    buildEHEdgeNaive();
+    buildEHEdge();
 
     //Rebuild cfg may generate redundant empty bb, it
     //disturb the computation of entry and exit.
@@ -186,6 +186,7 @@ void IR_CFG::initCfg(OptCTX & oc)
     }
     ASSERT0(!change);
     ASSERT0(verify());
+    ASSERT(get_entry_list()->get_elem_count() <= 1, ("Must be single entry"));
 }
 
 
@@ -469,14 +470,23 @@ bool IR_CFG::removeTrampolinBB()
             get_preds(preds, bb);
             for (IRBB * pred = preds.get_head();
                  pred != NULL; pred = preds.get_next()) {
+                unionLabels(bb, next);
+
                 if (BB_is_fallthrough(pred) && prev == pred) {
                     removeEdge(pred, bb);
-                    addEdge(BB_id(pred), BB_id(next));
+
+                    //Add normal control flow edge.
+                    Edge * e = addEdge(BB_id(pred), BB_id(next));
+                    EI * ei = (EI*)EDGE_info(e);
+                    if (ei != NULL && EI_is_eh(ei)) {
+                        //If there is already an edge, check if it is an
+                        //exception edge. If it is, change the exception edge
+                        //to be normal control flow edge.
+                        EI_is_eh(ei) = false;
+                    }
                     continue;
                 }
 
-                unionLabels(bb, next);
-
                 //Revise branch target LabelInfo of xr in 'pred'.
                 IR * last_xr_of_pred = get_last_xr(pred);
 
@@ -486,8 +496,16 @@ bool IR_CFG::removeTrampolinBB()
                 ASSERT0(last_xr_of_pred->get_label() != NULL);
                 last_xr_of_pred->set_label(br->get_label());
                 removeEdge(pred, bb);
-                addEdge(BB_id(pred), BB_id(next));
+                Edge * e = addEdge(BB_id(pred), BB_id(next));
+                EI * ei = (EI*)EDGE_info(e);
+                if (ei != NULL && EI_is_eh(ei)) {
+                    //If there is already an edge, check if it is an
+                    //exception edge. If it is, change the exception edge
+                    //to be normal control flow edge.
+                    EI_is_eh(ei) = false;
+                }
             } //end for each pred of BB.
+
             remove_bb(bb);
             removed = true;
             bb = m_bb_list->get_head(&ct); //reprocessing BB list.
@@ -920,7 +938,6 @@ void IR_CFG::dump_vcg(CHAR const* name, bool detail, bool dump_eh)
         INT scale = 1;
         CHAR const* color = "gold";
         if (BB_is_entry(bb) || BB_is_exit(bb)) {
-
             font = "Times Bold";
             scale = 2;
             color = "cyan";
@@ -1008,7 +1025,10 @@ void IR_CFG::dump_vcg(CHAR const* name, bool detail, bool dump_eh)
                         VERTEX_id(EDGE_from(e)), VERTEX_id(EDGE_to(e)));
             }
         } else {
-            ASSERT(0, ("unsupport EDGE_INFO"));
+            fprintf(h,
+                    "\nedge: { sourcename:\"%d\" targetname:\"%d\" "
+                    " thickness:4 color:darkred }",
+                    VERTEX_id(EDGE_from(e)), VERTEX_id(EDGE_to(e)));
         }
     }
     g_tfile = old;
diff --git a/opt/ir_cfg.h b/opt/ir_cfg.h
index 0db5da3..0b7021d 100644
--- a/opt/ir_cfg.h
+++ b/opt/ir_cfg.h
@@ -90,10 +90,14 @@ public:
             if (x != NULL && x->is_may_throw() && x->get_ai() != NULL) {
                 EHLabelAttachInfo const* ehlab =
                     (EHLabelAttachInfo const*)x->get_ai()->get(AI_EH_LABEL);
+                if (ehlab == NULL) { continue; }
+
                 SC<LabelInfo*> * sc;
-                SList<LabelInfo*> const& labs = ehlab->get_labels();
+                SList<LabelInfo*> const& labs = ehlab->read_labels();
                 for (sc = labs.get_head();
-                     sc != labs.end(); sc = labs.get_next(sc)) {
+                     sc != labs.end();
+                     sc = labs.get_next(sc)) {
+                    ASSERT0(sc);
                     IRBB * tgt = findBBbyLabel(sc->val());
                     ASSERT0(tgt);
                     Edge * e = addEdge(BB_id(bb), BB_id(tgt));
@@ -171,27 +175,19 @@ public:
     virtual void cf_opt();
     void computeDomAndIdom(IN OUT OptCTX & oc, BitSet const* uni = NULL);
     void computePdomAndIpdom(IN OUT OptCTX & oc, BitSet const* uni = NULL);
+
+    //Record the Exit BB here.
     virtual void computeEntryAndExit(bool comp_entry, bool comp_exit)
     {
-        ASSERT0(comp_entry | comp_exit);
-        if (comp_entry) { m_entry_list.clean(); }
-        if (comp_exit) { m_exit_list.clean(); }
-
-        for (IRBB * bb = m_bb_list->get_head();
-             bb != NULL; bb = m_bb_list->get_next()) {
-            Vertex * vex = get_vertex(bb->id);
-            ASSERT(vex, ("No vertex corresponds to BB%d", bb->id));
-            if (comp_entry) {
-                if (VERTEX_in_list(vex) == NULL &&
-                    (is_ru_entry(bb) || bb->is_exp_handling())) {
-                    m_entry_list.append_tail(bb);
-                }
-            }
-            if (comp_exit) {
-                if (VERTEX_out_list(vex) == NULL) {
-                    m_exit_list.append_tail(bb);
-                    BB_is_exit(bb) = true;
-                }
+        CFG<IRBB, IR>::computeEntryAndExit(comp_entry, comp_exit);
+        if (comp_exit) {
+            C<IRBB*> * ct;
+            for (m_exit_list.get_head(&ct);
+                 ct != m_exit_list.end();
+                 ct = m_exit_list.get_next(ct)) {
+                IRBB * bb = ct->val();
+                ASSERT0(bb);
+                BB_is_exit(bb) = true;
             }
         }
     }
diff --git a/opt/ir_opt.h b/opt/ir_opt.h
deleted file mode 100644
index bfc3fbf..0000000
--- a/opt/ir_opt.h
+++ /dev/null
@@ -1,70 +0,0 @@
-/*@
-XOC Release License
-
-Copyright (c) 2013-2014, Alibaba Group, All rights reserved.
-
-    compiler@aliexpress.com
-
-Redistribution and use in source and binary forms, with or without
-modification, are permitted provided that the following conditions are met:
-
-    * Redistributions of source code must retain the above copyright
-      notice, this list of conditions and the following disclaimer.
-    * Redistributions in binary form must reproduce the above copyright
-      notice, this list of conditions and the following disclaimer in the
-      documentation and/or other materials provided with the distribution.
-    * Neither the name of the Su Zhenyu nor the names of its contributors
-      may be used to endorse or promote products derived from this software
-      without specific prior written permission.
-
-THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
-WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
-OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
-DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
-BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
-CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
-OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
-BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
-WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
-OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
-IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-
-author: Su Zhenyu
-@*/
-#ifndef __IR_OPT_H__
-#define __IR_OPT_H__
-
-namespace xoc {
-
-//Basis Class of pass.
-class Pass {
-protected:
-	SimpCTX * m_simp;
-public:
-	Pass() { m_simp = NULL; }
-	virtual ~Pass() {}
-	COPY_CONSTRUCTOR(Pass);
-
-	virtual CHAR const* get_pass_name() const
-	{
-		ASSERT(0, ("Optimization Dependent Code"));
-		return NULL;
-	}
-
-	virtual PASS_TYPE get_pass_type() const
-	{
-		ASSERT(0, ("Optimization Dependent Code"));
-		return PASS_UNDEF;
-	}
-
-	void set_simp_cont(SimpCTX * simp) { m_simp = simp; }
-
-	virtual bool perform(OptCTX &)
-	{
-		ASSERT(0, ("Optimization Dependent Code"));
-		return false;
-	}
-};
-
-} //namespace xoc
-#endif
