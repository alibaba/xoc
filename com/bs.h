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
#ifndef __BITSET_H__
#define __BITSET_H__

#define BS_ZERO			0
#define BS_DUMP_BITSET	1
#define BS_DUMP_POS		2
#define BITS_PER_BYTE	8
#define BYTES_PER_UINT	4
#define BYTES_PER_SEG	64
#define BITS_PER_SEG	(BITS_PER_BYTE * BYTES_PER_SEG)

class BITSET;
class BITSET_MGR;

class BITSET
{
	friend BITSET * bs_union(IN BITSET const& set1, IN BITSET const& set2,
							OUT BITSET & res);
	friend BITSET * bs_diff(IN BITSET const& set1, IN BITSET const& set2,
							OUT BITSET & res);
	friend BITSET * bs_intersect(IN BITSET const& set1, IN BITSET const& set2,
							OUT BITSET & res);
protected:
	UINT m_size;
	BYTE * m_ptr;

	void * realloc(IN void * src, size_t orgsize, size_t newsize);
public:
	BITSET(UINT init_pool_size = 1)
	{
		m_ptr = 0;
		init(init_pool_size);
	}

	//Copy constructor
	BITSET(BITSET const& bs)
	{
		m_ptr = 0;
		init();
		copy(bs);
	}

	~BITSET() { destroy(); }

	void init(UINT init_pool_size = 1)
	{
		if (m_ptr != NULL) return;
		m_size = init_pool_size;
		if (init_pool_size == 0) return;
		m_ptr = (BYTE*)::malloc(init_pool_size);
		::memset(m_ptr, 0, m_size);
	}

	void destroy()
	{
		if (m_ptr == NULL) return;
		IS_TRUE(m_size > 0, ("bitset is invalid"));
		::free(m_ptr);
		m_ptr = NULL;
		m_size = 0;
	}

	void alloc(UINT size);
	void bunion(BITSET const& bs);
	void bunion(INT elem);

	void copy(BITSET const& src);
	void clean();
	UINT count_mem() const { return get_byte_size() + sizeof(BITSET); }
	void complement(IN BITSET const& univers);

	void diff(UINT elem);
	void diff(BITSET const& bs);
	void dump(CHAR const* name = NULL, bool is_del = false,
			  UINT flag = BS_DUMP_BITSET | BS_DUMP_POS,
			  INT last_pos = -1) const;
	void dump(FILE * h, UINT flag, INT last_pos) const;
	void dump(FILE * h) const { dump(h, BS_DUMP_BITSET|BS_DUMP_POS, -1); }

	UINT get_elem_count() const;
	INT get_first() const;
	INT get_last() const;
	BITSET * get_subset_in_range(UINT low, UINT high, OUT BITSET & subset);
	INT get_next(UINT elem) const;
	UINT get_byte_size() const { return m_size; }

	bool has_elem_in_range(UINT low, UINT high) const;

	void intersect(BITSET const& bs);
	bool is_equal(BITSET const& bs) const;
	bool is_contain(UINT elem) const;
	bool is_contain(BITSET const& bs, bool strict = false) const;
	bool is_contained_in_range(UINT low, UINT high, bool strict) const;
	bool is_contain_range(UINT low, UINT high, bool strict) const;
	bool is_intersect(BITSET const& bs) const;
	bool is_overlapped(UINT low, UINT high) const;
	bool is_empty() const;

	BITSET const& operator = (BITSET const& src);

	void rev(UINT last_bit_pos);
};


class BITSET_MGR
{
protected:
	SMEM_POOL * m_pool;
	LIST<BITSET*> m_bs_list;
	LIST<BITSET*> m_free_list;

	inline void * xmalloc(size_t size)
	{
		IS_TRUE(m_pool != NULL, ("LIST not yet initialized."));
		void * p = smpool_malloc_h_const_size(size, m_pool);
		IS_TRUE(p != NULL, ("malloc failed"));
		memset(p, 0, size);
		return p;
	}
public:
	BITSET_MGR()
	{
		m_pool = NULL;
		init();
	}
	~BITSET_MGR() { destroy(); }

	inline void init()
	{
		if (m_pool != NULL) { return; }
		m_pool = smpool_create_handle(sizeof(BITSET) * 4, MEM_CONST_SIZE);
		m_bs_list.init();
		m_free_list.init();
	}

	void destroy()
	{
		if (m_pool == NULL) { return; }
		for (BITSET * bs = m_bs_list.get_head();
			 bs != NULL; bs = m_bs_list.get_next()) {
			bs->destroy();
		}
		m_bs_list.destroy();
		m_free_list.destroy();
		smpool_free_handle(m_pool);
		m_pool = NULL;
	}

	BITSET * create(UINT init_sz = 0)
	{
		IS_TRUE(m_pool, ("not yet init"));
		BITSET * p = m_free_list.remove_head();
		if (p == NULL) {
			p = (BITSET*)xmalloc(sizeof(BITSET));
			p->init(init_sz);
			m_bs_list.append_tail(p);
		}
		return p;
	}

	inline BITSET * copy(BITSET const& bs)
	{
		IS_TRUE(m_pool, ("not yet init"));
		BITSET * p = create();
		p->copy(bs);
		return p;
	}

	inline void clean()
	{
		IS_TRUE(m_pool, ("not yet init"));
		destroy();
		init();
	}
	UINT count_mem(FILE * h = NULL);

	inline void free(IN BITSET * bs) //free bs for next use.
	{
		if (bs == NULL) { return; }
		#ifdef _DEBUG_
		for (BITSET * x = m_free_list.get_head();
			 x != NULL; x = m_free_list.get_next()) {
			IS_TRUE(x != bs, ("Already have been freed."));
		}
		#endif
		bs->clean();
		m_free_list.append_tail(bs);
	}
};


//
//START BVEC
//
template <class T> class BVEC : public SVECTOR<T> {
protected:
	BITSET m_bs; //Record position set by 'set()'
public:
	BVEC();
	BVEC(INT size);
	~BVEC();
	void init();
	void destroy();
	inline void copy(LIST<T> & list);
	inline void clone(BVEC<T> & vec);
	UINT count_mem() const;
	inline void clean();
	inline void set(INT i, T elem);
	inline T & operator[](INT i);

	//Get the first index number and return the element.
	inline T get_first(OUT INT * idx);
	inline INT get_first() const; //Get first number of index of element.

	//Get next index number and return the next element at the same time.
	inline T get_next(INT * curidx);
	inline INT get_next(UINT curidx) const; //Get next index number.
	inline UINT get_elem_count() const; //Get number of elements in vector.
	inline BITSET * get_bs();

	//Clear bit of position 'i', and set new value 't' for the position.
	inline void remove(INT i, T t = (T)0);
	void dump(CHAR const* name = NULL, bool is_del = false) const
	{ m_bs.dump(name, is_del); }
	void dump(FILE * h) const
	{ m_bs.dump(h); }
};


template <class T>
BVEC<T>::BVEC()
{
	init();
}


template <class T>
BVEC<T>::BVEC(INT size)
{
	init();
	SVECTOR<T>::grow(size);
}


template <class T>
BVEC<T>::~BVEC()
{
	destroy();
}


template <class T>
void BVEC<T>::init()
{
	if (SVECTOR<T>::m_is_init) return;
	SVECTOR<T>::init();
	m_bs.init();
}


template <class T>
void BVEC<T>::destroy()
{
	if (!SVECTOR<T>::m_is_init) return;
	m_bs.destroy();
	SVECTOR<T>::destroy();
}


template <class T>
void BVEC<T>::copy(LIST<T> & list)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	INT count = 0;
	set(list.get_elem_count()-1, 0); //Alloc memory right away.
	for (T elem = list.get_head();
		 elem != (T)0;
		 elem = list.get_next(), count++) {
		set(count, elem);
	}
}


template <class T>
void BVEC<T>::clone(BVEC<T> & vec)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	SVECTOR<T>::copy(vec);
	m_bs.copy(vec.m_bs);
}


template <class T>
void BVEC<T>::clean()
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	SVECTOR<T>::clean();
	m_bs.clean();
}


template <class T>
UINT BVEC<T>::count_mem() const
{
	return m_bs.count_mem() + SVECTOR<T>::count_mem();
}


template <class T>
void BVEC<T>::set(INT i, T elem)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	SVECTOR<T>::set(i, elem);
	m_bs.bunion(i);
}


/*
Overloaded [] for non-const array reference return.
Create an lvalue, equal to 'set()'
*/
template <class T>
T & BVEC<T>::operator[](INT i)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	if (i >= SVECTOR<T>::m_size) {
		set(i, (T)0);
	}
	return SVECTOR<T>::m_vec[i];
}


template <class T>
INT BVEC<T>::get_first() const
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	return m_bs.get_first();
}


template <class T>
T BVEC<T>::get_first(OUT INT * idx)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	INT i = m_bs.get_first();
	if (idx) { *idx = i; }
	return SVECTOR<T>::get(i);
}


//Return next element related to current 'idx'.
template <class T>
T BVEC<T>::get_next(OUT INT * curidx)
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	*curidx = m_bs.get_next(*curidx);
	return SVECTOR<T>::get(*curidx);
}


template <class T>
INT BVEC<T>::get_next(UINT curidx) const
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	return m_bs.get_next(curidx);
}


//Get number of elements in vector.
template <class T>
UINT BVEC<T>::get_elem_count() const
{
	IS_TRUE(SVECTOR<T>::m_is_init, ("VECTOR not yet initialized."));
	return m_bs.get_elem_count();
}


/*
Clear bit of position 'i', and set new value 't' for the position.
Default placeholder of clear bit is NULL.
*/
template <class T>
void BVEC<T>::remove(INT i, T t)
{
	m_bs.diff(i);
	SVECTOR<T>::set(i, t);
}


template <class T>
BITSET * BVEC<T>::get_bs()
{
	return &m_bs;
}
//END BVEC


//
//START BVEC_MGR
//
template <class T> class BVEC_MGR {
protected:
	LIST<BVEC<T>*> m_bs_list;
	LIST<BVEC<T>*> m_free_list;
	bool m_is_init;
public:
	BVEC_MGR()
	{
		m_is_init = false;
		init();
	}
	~BVEC_MGR(){ destroy(); }

	inline void init()
	{
		if (m_is_init) return;
		m_bs_list.init();
		m_is_init = true;
	}
	void destroy()
	{
		if (!m_is_init) return;
		for (BVEC<T> * bs = m_bs_list.get_head();
			 bs != NULL; bs = m_bs_list.get_next()) {
			bs->destroy();
		}
		m_bs_list.destroy();
		m_is_init = false;
	}
	BVEC<T> * create();
	inline void clean()
	{
		IS_TRUE(m_is_init, ("not yet init"));
		destroy();
		init();
	}
	UINT count_mem(FILE * h = NULL);
	inline void free(IN BVEC<T> * bs) //free bs for next use.
	{
		if (bs == NULL) return;
		bs->clean();
		m_free_list.append_tail(bs);
	}
};


template <class T>
UINT BVEC_MGR<T>::count_mem(FILE * h)
{
	UINT count = 0;
	for (BVEC<T> const* bs = m_bs_list.get_head();
		 bs != NULL; bs = m_bs_list.get_next()) {
		count += bs->count_mem();
	}
	#ifdef _DEBUG_
    if (h != NULL) {
        //Dump mem usage into file.
        LIST<UINT> lst;
        for (BVEC<T> const* bs = m_bs_list.get_head();
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


template <class T>
BVEC<T> * BVEC_MGR<T>::create()
{
	IS_TRUE(m_is_init, ("not yet init"));
	BVEC<T> * p = m_free_list.remove_head();
	if (p == NULL) {
		p = (BVEC<T>*)::malloc(sizeof(BVEC<T>));
		::memset(p, 0, sizeof(BVEC<T>));
		p->init();
		m_bs_list.append_tail(p);
	}
	return p;
}
//END BVEC_MGR


//
//Sparse BITSET
//

//#define DEBUG_SEG
//Segment
class SEG {
public:
	#ifdef DEBUG_SEG
	int id;
	#endif
	UINT start;
	BITSET bs;

	SEG()
	{
		#ifdef DEBUG_SEG
		id = 0;
		#endif
		start = 0;
	}
	SEG(SEG const& src) { copy(src); }

	inline void copy(SEG const& src)
	{
		start = src.start;
		bs.copy(src.bs);
	}

	inline UINT count_mem() const
	{
		return sizeof(start) + bs.count_mem();
	}

	inline void clean()
	{
		start = 0;
		bs.clean();
	}

	inline bool is_contain(UINT elem)
	{
		if (elem < start) { return false; }
		UINT last = start + MAX(bs.get_byte_size(), BYTES_PER_UINT) *
					BITS_PER_BYTE - 1;
		if (elem <= last) {
			return true;
		}
		return false;
	}

	//Return the start position of current segment.
	inline UINT get_start() const { return start; }
	//Return the end position of current segment.
	inline UINT get_end() const { return start + BITS_PER_SEG - 1; }
};


//Segment Manager.
//This class is responsible to allocate and destroy SEG object.
class SEG_MGR {
#ifdef _DEBUG_
public:
	UINT seg_count;
#endif
	SLIST<SEG*> m_free_list;
public:
	SEG_MGR()
	{
		#ifdef _DEBUG_
		seg_count = 0;
		#endif
		SMEM_POOL * p = smpool_create_handle(sizeof(SC<SEG*>) * 4,
											 MEM_CONST_SIZE);
		m_free_list.set_pool(p);
	}

	~SEG_MGR()
	{
		#ifdef _DEBUG_
		UINT n = m_free_list.get_elem_count();
		IS_TRUE(seg_count == n, ("MemLeak! There still have SEG not be freed"));
		#endif
		SC<SEG*> * sc;
		for (SEG * s = m_free_list.get_head(&sc);
			 s != NULL; s = m_free_list.get_next(&sc)) {
			delete s;
		}
		SMEM_POOL * p = m_free_list.get_pool();
		IS_TRUE(p, ("miss pool"));
		smpool_free_handle(p);
	}

	inline void free(SEG * s)
	{
		s->clean();
		m_free_list.append_head(s);
	}

	SEG * new_seg()
	{
		SEG * s = m_free_list.remove_head();
		if (s != NULL) { return s; }

		#ifdef _DEBUG_
		seg_count++;
		#endif

		s = new SEG();

		#ifdef DEBUG_SEG
		s->id = seg_count;
		#endif

		return s;
	}

	UINT count_mem() const
	{
		UINT count = 0;
		SC<SEG*> * sc;
		for (SEG * s = m_free_list.get_head(&sc);
			 s != NULL; s = m_free_list.get_next(&sc)) {
			count += s->count_mem();
		}

		count += m_free_list.count_mem();
		return count;
	}

	#ifdef _DEBUG_
	UINT get_seg_count() const { return seg_count; }

	//Decrease seg_count.
	void dec_seg_count() { seg_count--; }
	#endif

	SLIST<SEG*> const* get_free_list() const { return &m_free_list; }
};


class SDBITSET_MGR;


//Sparse BITSET Core
class SBITSETC {
protected:
	SLISTC<SEG*> segs;

	void * realloc(IN void * src, size_t orgsize, size_t newsize);
public:
	SBITSETC() {}
	SBITSETC(SBITSETC const& src); //Do not allow copy-constructor.
	~SBITSETC() { /*should call clean() before destruction.*/ }

	void bunion(SBITSETC const& src, SEG_MGR * sm,
				SC<SEG*> ** free_list, SMEM_POOL * pool);

	void bunion(UINT elem, SEG_MGR * sm,
				SC<SEG*> ** free_list, SMEM_POOL * pool);
	inline void bunion(UINT elem, SDBITSET_MGR & m);
	inline void bunion(SBITSETC const& src, SDBITSET_MGR &m);

	inline void clean(SDBITSET_MGR & m);
	void clean(SEG_MGR * sm, SC<SEG*> ** free_list);
	void copy(SBITSETC const& src, SEG_MGR * sm,
			  SC<SEG*> ** free_list, SMEM_POOL * pool);
	inline void copy(SBITSETC const& src, SDBITSET_MGR & m);
	UINT count_mem() const;

	void destroy_seg_and_clean(SEG_MGR * sm, SC<SEG*> ** free_list);
	void diff(UINT elem, SEG_MGR * sm, SC<SEG*> ** free_list);
	inline void diff(UINT elem, SDBITSET_MGR & m);
	void diff(SBITSETC const& src, SEG_MGR * sm, SC<SEG*> ** free_list);
	inline void diff(SBITSETC const& src, SDBITSET_MGR & m);
	void dump(FILE * h) const;
	void dump2(FILE * h) const;

	UINT get_elem_count() const;
	INT get_first(SC<SEG*> ** cur) const;
	INT get_last(SC<SEG*> ** cur) const;
	INT get_next(UINT elem, SC<SEG*> ** cur) const;

	void intersect(SBITSETC const& src, SEG_MGR * sm, SC<SEG*> ** free_list);
	inline void intersect(SBITSETC const& src, SDBITSET_MGR & m);
	bool is_equal(SBITSETC const& src) const;
	bool is_contain(UINT elem) const;
	bool is_intersect(SBITSETC const& src) const;
	bool is_empty() const;
};



//Sparse BITSET
class SBITSET : public SBITSETC {
protected:
	SMEM_POOL * m_pool;
	SC<SEG*> * m_flst; //free list
	SEG_MGR * m_sm;
public:
	SBITSET(SEG_MGR * sm, UINT sz = sizeof(SC<SEG*>))
	{
		m_pool = NULL;
		init(sm, sz);
	}

	SBITSET(SBITSET const& bs); //Do not allow copy-constructor.
	~SBITSET() { destroy(); }

	void init(SEG_MGR * sm, UINT sz = sizeof(SC<SEG*>))
	{
		IS_TRUE(sm, ("need SEG_MGR"));
		IS_TRUE(sz % sizeof(SC<SEG*>) == 0, ("pool size must be mulitple."));
		IS_TRUE(m_pool == NULL, ("already initialized"));
		m_pool = smpool_create_handle(sz, MEM_CONST_SIZE);
		m_sm = sm;
		m_flst = NULL;
	}

	void destroy()
	{
		IS_TRUE(m_pool != NULL, ("already destroy"));
		SC<SEG*> * st;
		for (SEG * s = segs.get_head(&st); s != NULL; s = segs.get_next(&st)) {
			m_sm->free(s);
		}

		//Unnecessary call clean(), since free pool will free all
		//SC<SEG*> object.
		//SBITSETC::clean(m_sm, &m_flst);
		smpool_free_handle(m_pool);
		m_pool = NULL;
	}

	void bunion(SBITSET const& src)
	{ SBITSETC::bunion(src, m_sm, &m_flst, m_pool);	}

	void bunion(UINT elem)
	{ SBITSETC::bunion(elem, m_sm, &m_flst, m_pool); }

	void copy(SBITSET const& src)
	{
		//Do NOT change current m_sm.
		SBITSETC::copy(src, m_sm, &m_flst, m_pool);
	}

	void clean()
	{ SBITSETC::clean(m_sm, &m_flst); }

	UINT count_mem() const
	{
		SC<SEG*> * st;
		UINT c = 0;
		for (SEG * s = segs.get_head(&st); s != NULL; s = segs.get_next(&st)) {
			c += s->count_mem();
		}
		c += sizeof(m_pool);
		c += sizeof(m_sm);
		c += segs.count_mem();
		c += smpool_get_pool_size_handle(m_pool);
		return c;
	}

	void diff(UINT elem)
	{ SBITSETC::diff(elem, m_sm, &m_flst); }

	//Difference between current bitset and 'src', current bitset
	//will be modified.
	void diff(SBITSET const& src)
	{ SBITSETC::diff(src, m_sm, &m_flst); }

	//Do intersection for current bitset and 'src', current bitset
	//will be modified.
	void intersect(SBITSET const& src)
	{ SBITSETC::intersect(src, m_sm, &m_flst); }
};


//
//START DBITSETC, Dual BITSET Core
//
class DBITSETC : public SBITSETC {
protected:
	BYTE m_is_sparse:1; //true if bitset is sparse.

	//Only read BITSET.
	BITSET const* read_bs() const
	{
		IS_TRUE(!m_is_sparse, ("only used by dense bitset"));
		SC<SEG*> * sc;
		SEG * t = segs.get_head(&sc);
		if (t == NULL) {
			return NULL;
		}
		return &t->bs;
	}

	//Get BITSET, alloc BITSET if it not exist.
	BITSET * alloc_bs(SEG_MGR * sm, SC<SEG*> ** flst, SMEM_POOL * pool)
	{
		IS_TRUE(!m_is_sparse, ("only used by dense bitset"));
		SC<SEG*> * sc;
		SEG * t = segs.get_head(&sc);
		if (t == NULL) {
			t = sm->new_seg();
			segs.append_tail(t, flst, pool);
		}
		return &t->bs;
	}

	//Get BITSET and modify BITSET, do not alloc.
	BITSET * get_bs()
	{
		IS_TRUE(!m_is_sparse, ("only used by dense bitset"));
		SC<SEG*> * sc;
		SEG * t = segs.get_head(&sc);
		if (t == NULL) {
			return NULL;
		}
		return &t->bs;
	}
public:
	DBITSETC() { m_is_sparse = true; }
	DBITSETC(DBITSETC const& src); //Do not allow copy-constructor.
	~DBITSETC() {}

	void bunion(DBITSETC const& src, SEG_MGR * sm, SC<SEG*> ** free_list,
				SMEM_POOL * pool)
	{
		IS_TRUE(this != &src, ("operate on same set"));
		IS_TRUE(m_is_sparse == src.m_is_sparse, ("diff set type"));
		if (m_is_sparse) {
			SBITSETC::bunion(src, sm, free_list, pool);
		} else {
			BITSET const* srcbs = src.read_bs();
			if (srcbs == NULL) { return; }
			BITSET * tgtbs = alloc_bs(sm, free_list, pool);
			tgtbs->bunion(*srcbs);
		}
	}
	inline void bunion(DBITSETC const& src, SDBITSET_MGR & m);

	void bunion(UINT elem, SEG_MGR * sm, SC<SEG*> ** free_list,
				SMEM_POOL * pool)
	{
		if (m_is_sparse) {
			SBITSETC::bunion(elem, sm, free_list, pool);
		} else {
			BITSET * tgtbs = alloc_bs(sm, free_list, pool);
			tgtbs->bunion(elem);
		}
	}
	inline void bunion(UINT elem, SDBITSET_MGR & m);

	void copy(DBITSETC const& src, SEG_MGR * sm, SC<SEG*> ** free_list,
			  SMEM_POOL * pool)
	{
		IS_TRUE(this != &src, ("operate on same set"));
		IS_TRUE(m_is_sparse == src.m_is_sparse, ("diff set type"));
		if (m_is_sparse) {
			SBITSETC::copy(src, sm, free_list, pool);
		} else {
			BITSET const* srcbs = src.read_bs();
			if (srcbs == NULL) {
				clean(sm, free_list);
				return;
			}
			BITSET * tgtbs = alloc_bs(sm, free_list, pool);
			tgtbs->copy(*srcbs);
		}
	}
	inline void copy(DBITSETC const& src, SDBITSET_MGR & m);

	UINT count_mem() const { return SBITSETC::count_mem() + 1; }

	void diff(UINT elem, SEG_MGR * sm, SC<SEG*> ** free_list)
	{
		if (m_is_sparse) {
			SBITSETC::diff(elem, sm, free_list);
		} else {
			BITSET * tgtbs = get_bs();
			if (tgtbs == NULL) { return; }
			tgtbs->diff(elem);
		}
	}
	inline void diff(UINT elem, SDBITSET_MGR & m);

	void diff(DBITSETC const& src, SEG_MGR * sm, SC<SEG*> ** free_list)
	{
		IS_TRUE(this != &src, ("operate on same set"));
		IS_TRUE(m_is_sparse == src.m_is_sparse, ("diff set type"));
		if (m_is_sparse) {
			SBITSETC::diff(src, sm, free_list);
		} else {
			BITSET const* srcbs = src.read_bs();
			if (srcbs == NULL) { return; }
			BITSET * tgtbs = get_bs();
			if (tgtbs == NULL) { return; }
			tgtbs->diff(*srcbs);
		}
	}
	inline void diff(DBITSETC const& src, SDBITSET_MGR & m);

	void intersect(DBITSETC const& src, SEG_MGR * sm, SC<SEG*> ** free_list)
	{
		IS_TRUE(this != &src, ("operate on same set"));
		IS_TRUE(m_is_sparse == src.m_is_sparse, ("diff set type"));
		if (m_is_sparse) {
			SBITSETC::intersect(src, sm, free_list);
		} else {
			BITSET const* srcbs = src.read_bs();
			if (srcbs == NULL) {
				clean(sm, free_list);
				return;
			}
			BITSET * tgtbs = get_bs();
			if (tgtbs == NULL) { return; }
			tgtbs->intersect(*srcbs);
		}
	}
	inline void intersect(DBITSETC const& src, SDBITSET_MGR & m);

	bool is_contain(UINT elem) const
	{
		if (m_is_sparse) {
			return SBITSETC::is_contain(elem);
		}
		BITSET const* tgtbs = read_bs();
		if (tgtbs == NULL) { return false; }
		return tgtbs->is_contain(elem);
	}

	bool is_equal(DBITSETC const& src) const
	{
		IS_TRUE(this != &src, ("operate on same set"));
		IS_TRUE(m_is_sparse == src.m_is_sparse, ("diff set type"));
		if (m_is_sparse) {
			return SBITSETC::is_equal(src);
		}
		BITSET const* srcbs = src.read_bs();
		BITSET const* tgtbs = read_bs();
		if (srcbs == NULL) {
			if (tgtbs == NULL) { return true; }
			if (tgtbs->is_empty()) { return true; }
			return false;
		}
		if (tgtbs == NULL) {
			if (srcbs->is_empty()) { return true; }
			return false;
		}
		return tgtbs->is_equal(*srcbs);
	}

	INT get_first(SC<SEG*> ** cur) const;
	INT get_last(SC<SEG*> ** cur) const;

	void set_sparse(bool is_sparse) { m_is_sparse = is_sparse; }
};



//
//START DBITSET, Dual BITSET
//
class DBITSET : public DBITSETC {
protected:
	SMEM_POOL * m_pool;
	SC<SEG*> * m_flst;
	SEG_MGR * m_sm;
public:
	DBITSET(SEG_MGR * sm, UINT sz = sizeof(SC<SEG*>))
	{
		IS_TRUE(sm, ("need SEG_MGR"));
		IS_TRUE(sz % sizeof(SC<SEG*>) == 0, ("pool size must be mulitple."));
		m_is_sparse = true;
		m_pool = smpool_create_handle(sz, MEM_CONST_SIZE);
		m_sm = sm;
		m_flst = NULL;
	}
	DBITSET(DBITSET const& src);  //Do not allow copy-constructor.
	~DBITSET()
	{
		SC<SEG*> * st;
		for (SEG * s = segs.get_head(&st); s != NULL; s = segs.get_next(&st)) {
			m_sm->free(s);
		}

		//Unnecessary call clean(), since free pool will free all
		//SC<SEG*> object.
		//DBITSETC::clean(m_sm, &m_flst);
		smpool_free_handle(m_pool);
	}

	void bunion(DBITSET const& src)
	{ DBITSETC::bunion(src, m_sm, &m_flst, m_pool);	}

	void bunion(UINT elem)
	{ DBITSETC::bunion(elem, m_sm, &m_flst, m_pool);	}

	void copy(DBITSET const& src)
	{ DBITSETC::copy(src, m_sm, &m_flst, m_pool); }

	UINT count_mem() const
	{
		UINT count = sizeof(m_pool);
		count += sizeof(m_flst);
		count += sizeof(m_sm);
		count += smpool_get_pool_size_handle(m_pool);
		count += DBITSETC::count_mem();
		return count;
	}

	void clean()
	{ DBITSETC::clean(m_sm, &m_flst); }

	void diff(UINT elem)
	{ DBITSETC::diff(elem, m_sm, &m_flst); }

	void diff(DBITSET const& src)
	{ DBITSETC::diff(src, m_sm, &m_flst); }

	void intersect(DBITSET const& src)
	{ DBITSETC::intersect(src, m_sm, &m_flst); }
};


#define SDBITSET_MGR_sc_free_list(sbs)		((sbs)->scflst)
class SDBITSET_MGR
{
protected:
	SLIST<SBITSET*> m_sbitset_list;
	SLIST<DBITSET*> m_dbitset_list;
	SLIST<DBITSETC*> m_dbitsetc_list;
	SLIST<SBITSETC*> m_free_sbitsetc_list;
	SLIST<SBITSET*> m_free_sbitset_list;
	SLIST<DBITSET*> m_free_dbitset_list;
	SLIST<DBITSETC*> m_free_dbitsetc_list;

	SMEM_POOL * m_sbitsetc_pool;
	SMEM_POOL * m_dbitsetc_pool;

protected:
	SBITSETC * xmalloc_sbitsetc()
	{
		IS_TRUE(m_sbitsetc_pool, ("not yet initialized."));
		SBITSETC * p = (SBITSETC*)smpool_malloc_h_const_size(sizeof(SBITSETC),
															m_sbitsetc_pool);
		IS_TRUE(p != NULL, ("malloc failed"));
		memset(p, 0, sizeof(SBITSETC));
		return p;
	}

	DBITSETC * xmalloc_dbitsetc()
	{
		IS_TRUE(m_dbitsetc_pool, ("not yet initialized."));
		DBITSETC * p = (DBITSETC*)smpool_malloc_h_const_size(sizeof(DBITSETC),
															m_dbitsetc_pool);
		IS_TRUE(p != NULL, ("malloc failed"));
		memset(p, 0, sizeof(DBITSETC));
		return p;
	}
public:
	//SEG manager.
	SEG_MGR sm;

	//Free list of SC<SEG*> container. It will be allocated in ptr_pool.
	SC<SEG*> * scflst;

	SMEM_POOL * ptr_pool; //only used to allocate SC<SEG*>.

public:
	SDBITSET_MGR() { ptr_pool = NULL; init(); }
	~SDBITSET_MGR() { destroy(); }

	void init()
	{
		if (ptr_pool != NULL) { return; }
		ptr_pool = smpool_create_handle(sizeof(SC<SEG*>) * 10,
										MEM_CONST_SIZE);
		m_sbitsetc_pool = smpool_create_handle(sizeof(SBITSETC) * 10,
										 MEM_CONST_SIZE);
		m_dbitsetc_pool = smpool_create_handle(sizeof(DBITSETC) * 10,
										 MEM_CONST_SIZE);

		m_sbitset_list.set_pool(ptr_pool);
		m_dbitset_list.set_pool(ptr_pool);
		m_dbitsetc_list.set_pool(ptr_pool);

		m_free_sbitsetc_list.set_pool(ptr_pool);
		m_free_sbitset_list.set_pool(ptr_pool);
		m_free_dbitset_list.set_pool(ptr_pool);
		m_free_dbitsetc_list.set_pool(ptr_pool);

		scflst = NULL;
	}

	void destroy()
	{
		if (ptr_pool == NULL) { return; }
		SC<SBITSET*> * st;
		for (SBITSET * s = m_sbitset_list.get_head(&st);
			 s != NULL; s = m_sbitset_list.get_next(&st)) {
			delete s;
		}

		SC<DBITSET*> * dt;
		for (DBITSET * d = m_dbitset_list.get_head(&dt);
			 d != NULL; d = m_dbitset_list.get_next(&dt)) {
			delete d;
		}

		//All DBITSETC and SBITSETC are allocated in the pool.
		//It is not necessary to destroy it specially.
		//SC<DBITSETC*> * dct;
		//for (DBITSETC * d = m_dbitsetc_list.get_head(&dct);
		//	 d != NULL; d = m_dbitsetc_list.get_next(&dct)) {
		//	delete d;
		//}

		m_sbitset_list.destroy();
		m_dbitset_list.destroy();
		m_dbitsetc_list.destroy();

		m_free_sbitsetc_list.destroy();
		m_free_sbitset_list.destroy();
		m_free_dbitset_list.destroy();
		m_free_dbitsetc_list.destroy();

		smpool_free_handle(m_sbitsetc_pool);
		smpool_free_handle(m_dbitsetc_pool);
		smpool_free_handle(ptr_pool);

		ptr_pool = NULL;
		m_sbitsetc_pool = NULL;
		m_dbitsetc_pool = NULL;
		scflst = NULL;
	}

	inline SBITSET * create_sbitset()
	{
		SBITSET * p = m_free_sbitset_list.remove_head();
		if (p == NULL) {
			p = new SBITSET(&sm);
			m_sbitset_list.append_tail(p);
		}
		return p;
	}

	inline SBITSETC * create_sbitsetc()
	{
		SBITSETC * p = m_free_sbitsetc_list.remove_head();
		if (p == NULL) {
			p = xmalloc_sbitsetc();
		}
		return p;
	}

	inline DBITSET * create_dbitset()
	{
		DBITSET * p = m_free_dbitset_list.remove_head();
		if (p == NULL) {
			p = new DBITSET(&sm);
			m_dbitset_list.append_tail(p);
		}
		return p;
	}

	inline DBITSETC * create_dbitsetc()
	{
		DBITSETC * p = m_free_dbitsetc_list.remove_head();
		if (p == NULL) {
			p = xmalloc_dbitsetc();
			p->set_sparse(true);
			m_dbitsetc_list.append_tail(p);
		}
		return p;
	}

	//Note that this function does not add up the memory allocated by
	//create_sbitsetc() and create_dbitsetc(). You should count these
	//objects additionally.
	UINT count_mem(FILE * h = NULL) const;

	//free bs for next use.
	inline void free_sbitset(SBITSET * bs)
	{
		if (bs == NULL) return;
		bs->clean();
		m_free_sbitset_list.append_head(bs);
	}

	//free bs for next use.
	inline void free_sbitsetc(SBITSETC * bs)
	{
		if (bs == NULL) return;
		bs->clean(*this);
		m_free_sbitsetc_list.append_head(bs);
	}

	//free bs for next use.
	inline void free_dbitset(DBITSET * bs)
	{
		if (bs == NULL) return;
		bs->clean();
		m_free_dbitset_list.append_head(bs);
	}

	//free bs for next use.
	inline void free_dbitsetc(DBITSETC * bs)
	{
		if (bs == NULL) return;
		bs->clean(&sm, &scflst);
		m_free_dbitsetc_list.append_head(bs);
	}

	//This function destroy SEG objects and free containers back to
	//SDBITSET_MGR for next use.
	inline void destroy_seg_and_freedc(DBITSETC * bs)
	{
		if (bs == NULL) { return; }
		bs->destroy_seg_and_clean(&sm, &scflst);

		//Recycle bs.
		m_free_dbitsetc_list.append_head(bs);
	}

	//Get SEG_MGR.
	inline SEG_MGR * get_seg_mgr() { return &sm; }
};
//END SDBITSET_MGR



//
//START SBITSETC
//
void SBITSETC::bunion(UINT elem, SDBITSET_MGR & m)
{
	bunion(elem, &m.sm, &m.scflst, m.ptr_pool);
}


void SBITSETC::bunion(SBITSETC const& src, SDBITSET_MGR &m)
{
	bunion(src, &m.sm, &m.scflst, m.ptr_pool);
}


void SBITSETC::copy(SBITSETC const& src, SDBITSET_MGR & m)
{
	copy(src, &m.sm, &m.scflst, m.ptr_pool);
}


void SBITSETC::clean(SDBITSET_MGR & m)
{
	clean(&m.sm, &m.scflst);
}


void SBITSETC::diff(UINT elem, SDBITSET_MGR & m)
{
	diff(elem, &m.sm, &m.scflst);
}


void SBITSETC::diff(SBITSETC const& src, SDBITSET_MGR & m)
{
	diff(src, &m.sm, &m.scflst);
}


void SBITSETC::intersect(SBITSETC const& src, SDBITSET_MGR & m)
{
	intersect(src, &m.sm, &m.scflst);
}
//END SBITSETC


//
//START DBITSETC
//
void DBITSETC::bunion(DBITSETC const& src, SDBITSET_MGR & m)
{
	bunion(src, &m.sm, &m.scflst, m.ptr_pool);
}


void DBITSETC::bunion(UINT elem, SDBITSET_MGR & m)
{
	bunion(elem, &m.sm, &m.scflst, m.ptr_pool);
}


void DBITSETC::copy(DBITSETC const& src, SDBITSET_MGR & m)
{
	copy(src, &m.sm, &m.scflst, m.ptr_pool);
}


void DBITSETC::diff(UINT elem, SDBITSET_MGR & m)
{
	diff(elem, &m.sm, &m.scflst);
}


void DBITSETC::diff(DBITSETC const& src, SDBITSET_MGR & m)
{
	diff(src, &m.sm, &m.scflst);
}


void DBITSETC::intersect(DBITSETC const& src, SDBITSET_MGR & m)
{
	intersect(src, &m.sm, &m.scflst);
}
//END DBITSETC

extern BYTE const g_bit_count[];
extern inline BITSET * bs_create(BITSET_MGR & bs_mgr)
{
	return bs_mgr.create();
}
extern BITSET * bs_union(IN BITSET const& set1, IN BITSET const& set2,
						OUT BITSET & res);
extern BITSET * bs_diff(IN BITSET const& set1, IN BITSET const& set2,
						OUT BITSET & res);
extern BITSET * bs_intersect(IN BITSET const& set1, IN BITSET const& set2,
						OUT BITSET & res);
#endif
