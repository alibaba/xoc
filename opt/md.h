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
#ifndef _MEM_DESC_H_
#define _MEM_DESC_H_

class MD_SYS;
class REGION;

typedef enum _MD_TYPE {
	MD_UNBOUND = 0,
	MD_EXACT,
	MD_RANGE,
} MD_TYPE;

#define MD_UNDEF			0 //Undefined.
#define MD_ALL_MEM			1 //All program memory.
#define MD_GLOBAL_MEM		2 //Allocate at static data section
							  //BYTE explicit definition.
#define MD_HEAP_MEM			3 //Allocate in heap.
#define MD_STACK_MEM		4 //Allocate at stack.
//The first id which is allocable.
#define MD_FIRST_ALLOCABLE (MD_GLOBAL_MEM + 1)


/*
Memory Descriptor.
MD is an appealing property to represent exact or inexact memory object.
By using MD, we can model miscellaneous memory accessing, and perform
memory analysis and optimizations.

MD is used to represent different memory object with
same base. Attributes of MD may be Id, Base Variable, Size, Offset,
Effect, Exact, Range, Unbound.
¡ð	Id
	Unique id of abstract memory object.

¡ð	Base Variable
	Since MD is the abstract version of VAR, it is closely related to
	individual variable. This variable may be the base of several MD.

¡ð Type
	This attribute represent abstract memory object type.
	* MD_UNBOUND
		The object is unbound if we have no knowledge about MD size or
		MD offset. The Def-Use relation to the object is inexact. The
		store to object is nonkilling definition.

	* MD_EXACT
		The object is exact indicate the memory address and size is
		determinate when load or store to the object. The Def-Use
		relation to the object is exact. In general, the memory load
		or store will be exact if its data type is primitive.
		The store to object is killing definition.

	* MD_RANGE
		The object is range if we both know the MD offset of base
		variable and MD size, but the precise address and byte size
		may be uncertain when load or store to the object. The Def-Use
		relation to the object is inexact. The store to object is
		nonkilling definition.

¡ð Size
	This attribute represents byte size of the abstract memory object.

¡ð Offset
	This attribute represents byte size offset to the base variable.

¡ð Effect
	This attribute refers to variables which are definitely declared
	by user or compiler and existed in the concrete. In contrast to
	effect MD, ALL_MEM memory object is ineffect.

¡ð	Exact
	This attribute represent abstract memory object with type is
	MD_EXACT. An exact MD is also effect.

¡ð Range
	This attribute represent abstract memory object with type is
	MD_RANGE. An range MD is also effect, but is not exact.

¡ð Unbound
		This attribute represent abstract memory object with type is
		MD_UNBOUND. An unbound MD may be effect, but is definitly inexact.
*/
//Unique id of memory object.
#define MD_id(md)				((md)->id)

//Each MD has a base, it is corresponding to an unique variable.
#define MD_base(md)				((md)->base)

//Record the offset from base if MD is exact or range, or it is not available.
#define MD_ofst(md)				((md)->ofst)

//Record the byte size of memory object if MD is exact or range,
//or it is not available.
#define MD_size(md)				((md)->size)

/* Memory object type. If it is MD_EXACT, there will be exact use/def.
If it is MD_RANGE, there will be inexact use/def, but the accessing restricted
into a computable range. If it is MD_UNBOUND, there will be inexact use/def,
and we do not know where to be access. */
#define MD_ty(md)				((md)->u2.s1.type)

//The memory object has been taken address.
#define MD_is_addr_taken(md)	((md)->u2.s1.is_addr_taken)

//The memory object is a PR.
#define MD_is_pr(md)			(VAR_is_pr(MD_base(md)))

//The memory object is a string.
#define MD_is_str(md)			(VAR_is_str(MD_base(md)))

//Indicate the MD can not be effect MD, namely, the MD can not be
//mustdef or mustuse.
#define MD_is_may(md)			((md)->u2.s1.is_may_reference)

class MD {
public:
	UINT id; //unique id.
	UINT ofst; //byte offsets relative to 'base'
	UINT size; //byte size of the memory block
	VAR * base;
	union {
		struct {
			BYTE type:2;
			BYTE is_addr_taken:1;
			BYTE is_may_reference:1;
		} s1;
		BYTE s1v;
	} u2;

	MD() { clean(); }
	MD(MD const& md)
	{
		//Do not copy id.
		MD_id(this) = 0;
		copy(&md);
	}

	inline void copy(IN MD const* md)
	{
		IS_TRUE0(md && this != md);
		MD_base(this) = MD_base(md);
		MD_ofst(this) = MD_ofst(md);
		MD_size(this) = MD_size(md);
		u2.s1v = md->u2.s1v;
	}

	/* Return true if current md exactly cover 'm', such as:
	current md: |-------|
	m:            |----|
	*/
	bool is_cover(MD const* m) const;

	/* Return true if current md intersect but may be not cover 'm', such as:
	current md: |-------|
	m:            |-------|
	*/
	bool is_overlap(MD const* m) const;

	/* Return true if md represent real object that would be emitted to
	target machine. Fake object is not effect object.
	NOTE: Effect MD inexact represent the memory object which may or may
	not occur at run time. If stmt modified effect but inexact MD,
	it is non-killing definition. */
	bool is_effect() const { return !VAR_is_fake(MD_base(this)); }

	//Return true if md is exact object.
	//Exact MD represent must and killing-DEF or USE.
	bool is_exact() const { return MD_ty(this) == MD_EXACT; }

	//Return true if md is unbound.
	bool is_unbound() const { return MD_ty(this) == MD_UNBOUND; }

	//Return true if md is global variable.
	bool is_global() const { return VAR_is_global(MD_base(this)); }

	//Return true if md is volatile memory.
	bool is_volatile() const { return VAR_is_volatile(MD_base(this)); }

	//If MD is range, MD_base + MD_ofst indicate the start address,
	//MD_size indicate the range.
	bool is_range() const { return MD_ty(this) == MD_RANGE; }

	//Return true if md indicate PR.
	bool is_pr() const { return MD_is_pr(this); }

	//Return true if src is definitly equal to current md.
	bool is_equ(MD const& src) const
	{
		IS_TRUE0(this != &src);
		return *this == src;
	}

	inline bool operator == (MD const& src) const
	{
		IS_TRUE0(this != &src);
		if (MD_base(this) != MD_base(&src)) { return false; }
		if (is_unbound() && src.is_unbound()) { return true; }
		return ofst == src.ofst && size == src.size && u2.s1v == src.u2.s1v;
	}

	//Dump md into 'buf', 'bufl' indicate the byte length of the buffer.
	CHAR * dump(IN OUT CHAR * buf, UINT bufl) const;

	//Dump md to file.
	void dump() const;

	inline void clean()
	{
		MD_id(this) = 0;
		MD_ofst(this) = 0;
		MD_size(this) = 0;
		MD_base(this) = NULL;
		u2.s1v = 0;
	}
};


typedef TMAP_ITER<MD*, MD*> MD_ITER;


class MD_HF {
public:
	UINT get_hash_value(MD * md, UINT bucket_size) const
	{ return MD_ofst(md) % bucket_size; }

	bool compare(MD * md1, MD * md2) const
	{
		IS_TRUE0(MD_base(md1) == MD_base(md2));
		if (MD_ofst(md1) != MD_ofst(md2)) { return false; }
		if (MD_size(md1) != MD_size(md2)) { return false; }
		return true;
	}
};


class CMD_HF {
public:
	UINT get_hash_value(MD const* md, UINT bucket_size) const
	{ return MD_ofst(md) % bucket_size; }

	bool compare(MD const* md1, MD const* md2) const
	{
		IS_TRUE0(MD_base(md1) == MD_base(md2));
		if (MD_ofst(md1) != MD_ofst(md2)) { return false; }
		if (MD_size(md1) != MD_size(md2)) { return false; }
		return true;
	}
};


class COMPARE_OFST {
public:
	bool is_less(MD * t1, MD *  t2) const
	{
		IS_TRUE0(MD_base(t1) == MD_base(t2));
		return (((ULONGLONG)MD_ofst(t1)) << 32 | ((ULONGLONG)MD_size(t1))) <
			   (((ULONGLONG)MD_ofst(t2)) << 32 | ((ULONGLONG)MD_size(t2)));
	}

	bool is_equ(MD * t1, MD * t2) const
	{
		IS_TRUE0(MD_base(t1) == MD_base(t2));
		return (((ULONGLONG)MD_ofst(t1)) << 32 | ((ULONGLONG)MD_size(t1))) ==
			   (((ULONGLONG)MD_ofst(t2)) << 32 | ((ULONGLONG)MD_size(t2)));
	}
};


//MD hashed by MD_ofst.
class OFST_TAB : public TMAP<MD*, MD*, COMPARE_OFST> {
public:
	//Return the entry.
	MD * find(MD * md)
	{ return TMAP<MD*, MD*, COMPARE_OFST>::get(md, NULL); }

	void append(MD * md)
	{ TMAP<MD*, MD*, COMPARE_OFST>::set(md, md); }
};


//Each VAR corresponds to an unqiue MD_TAB.
class MD_TAB {
protected:
	OFST_TAB m_ofst_tab;
	MD * m_invalid_ofst_md; //record MD with invalid ofst
public:
	MD_TAB() { m_invalid_ofst_md = NULL; }

	void init(UINT hash_bucket_size);
	void clean()
	{
		m_invalid_ofst_md = NULL;
		m_ofst_tab.clean();
	}

	UINT count_mem() const
	{ return m_ofst_tab.count_mem() + sizeof(m_invalid_ofst_md); }

	MD * find(MD * md)
	{
		if (md->is_exact()) {
			return m_ofst_tab.find(md);
		}
		return m_invalid_ofst_md;
	}

	void append(MD * md)
	{
		if (md->is_exact()) {
			m_ofst_tab.append(md);
			return;
		}
		IS_TRUE0(m_invalid_ofst_md == NULL);
		m_invalid_ofst_md = md;
	}

	UINT get_elem_count()
	{
		UINT elems = 0;
		if (m_invalid_ofst_md != NULL) {
			elems++;
		}
		elems += m_ofst_tab.get_elem_count();
		return elems;
	}

	OFST_TAB * get_ofst_tab() {  return &m_ofst_tab; }
	MD * get_effect_md() { return m_invalid_ofst_md; }
	void get_elems(OUT SVECTOR<MD*> & mdv, MD_ITER & iter)
	{
		UINT idx = 0;
		if (m_invalid_ofst_md != NULL) {
			mdv.set(idx++, m_invalid_ofst_md);
		}
		for (MD * md = m_ofst_tab.get_first(iter, NULL);
			 md != NULL; md = m_ofst_tab.get_next(iter, NULL)) {
			mdv.set(idx++, md);
		}
	}
};


//MD_SET, a set of MD.
class MD_SET : public BITSET {
public:
	void bunion(IN MD_SET const& pt);
	void bunion(IN MD const* md);
	void bunion(UINT mdid);
	void bunion_pure(UINT mdid) { BITSET::bunion(mdid); }
	void bunion_pure(MD_SET const& mds) { BITSET::bunion((BITSET const&)mds); }

	bool is_contain_pure(UINT mdid) const { return BITSET::is_contain(mdid); }

	//Return true if set contain global variable.
	bool is_contain_global() const
	{
		return BITSET::is_contain(MD_GLOBAL_MEM) ||
			   BITSET::is_contain(MD_ALL_MEM);
	}

	//Return true if set contain all memory variable.
	bool is_contain_all() const { return BITSET::is_contain(MD_ALL_MEM); }

	//Return true if set contain md.
	inline bool is_contain(MD const* md) const
	{
		if (BITSET::is_contain(MD_GLOBAL_MEM) && md->is_global()) {
			return true;
		}
		if (BITSET::is_contain(MD_ALL_MEM)) {
			return true;
		}
		return BITSET::is_contain(MD_id(md));
	}

	//Return true if md is overlap with the elements in set.
	inline bool is_overlap(MD const* md) const
	{
		if (BITSET::is_contain(MD_GLOBAL_MEM) && md->is_global()) {
			return true;
		}
		if ((BITSET::is_contain(MD_ALL_MEM)) ||
			(MD_id(md) == MD_ALL_MEM && !BITSET::is_empty())) {
			return true;
		}
		return BITSET::is_contain(MD_id(md));
	}

	bool is_contain_inexact(MD_SYS * ms) const;
	bool is_contain_only_exact_and_str(MD_SYS * ms) const;
	bool is_exact_equal(IN MD_SET const& mds, MD_SYS * ms) const;

	//Return true if set intersect with 'mds'.
	inline bool is_intersect(MD_SET const& mds) const
	{
		if (this == &mds) { return true; }
		if (BITSET::is_contain(MD_GLOBAL_MEM) &&
			((BITSET&)mds).is_contain(MD_GLOBAL_MEM)) {
			return true;
		}

		if ((BITSET::is_contain(MD_ALL_MEM) && !mds.is_empty()) ||
			(((BITSET&)mds).is_contain(MD_ALL_MEM) && !BITSET::is_empty())) {
			return true;
		}
		return BITSET::is_intersect(mds);
	}

	//Return true if all MD in set are PR.
	inline bool is_pr_set(MD_SYS const* mdsys) const;

	void diff(MD const* md)
	{
		IS_TRUE0(md);
		BITSET::diff(MD_id(md));
	}

	void diff(UINT id) { BITSET::diff(id); }

	inline void diff(MD_SET const& mds)
	{
		IS_TRUE0(this != &mds);
		IS_TRUE(!BITSET::is_contain(MD_ALL_MEM), ("low performance"));
		if (((BITSET const&)mds).is_contain(MD_ALL_MEM)) {
			clean();
			return;
		}
		BITSET::diff((BITSET const&)mds);
	}
	void dump(MD_SYS * ms, bool detail = false) const;

	//Get unique MD that is effective, and offset must be valid.
	inline MD * get_exact_md(MD_SYS * ms) const
	{
		MD * md = get_effect_md(ms);
		if (md != NULL && md->is_exact()) {
			return md;
		}
		return NULL;
	}
	MD * get_effect_md(MD_SYS * ms) const;
};


//MD_SET_MGR
class MD_SET_MGR {
protected:
	SMEM_POOL * m_mds_pool;
	SMEM_POOL * m_sc_mds_pool;
	SLIST<MD_SET*> m_free_md_set;
	SLIST<MD_SET*> m_md_set_list;
public:
	MD_SET_MGR()
	{
		m_mds_pool = smpool_create_handle(sizeof(MD_SET) * 8, MEM_CONST_SIZE);
		m_sc_mds_pool = smpool_create_handle(sizeof(SC<MD_SET*>) * 8,
											 MEM_CONST_SIZE);
		m_md_set_list.set_pool(m_sc_mds_pool);
		m_free_md_set.set_pool(m_sc_mds_pool);
	}
	~MD_SET_MGR() { destroy(); }

	//Clean and give it back to md set manager.
	//Do not destroy mds.
	inline void free(MD_SET * mds)
	{
		if (mds == NULL) { return; }

		#if 0
		//#ifdef _DEBUG_
		//Caution: this verification is pretty slowly, even if in debug
		//mode, so be patient.

		SC<MD_SET*> * sct;
		for (MD_SET * x = m_free_md_set.get_head(&sct);
			 x != NULL; x = m_free_md_set.get_next(&sct)) {
			IS_TRUE(x != mds, ("Already have been freed."));
		}
		#endif

		mds->clean();
		m_free_md_set.append_head(mds);
	}

	//Give it back to md set manager.
	//This function will reset mds.
	inline void free_and_destroy(MD_SET * mds)
	{
		if (mds == NULL) { return; }
		mds->destroy();
		mds->init();
		m_free_md_set.append_head(mds);
	}

	inline MD_SET * get_free() { return m_free_md_set.remove_head(); }
	UINT get_mdset_count() const { return m_md_set_list.get_elem_count(); }
	UINT get_free_mdset_count() const { return m_free_md_set.get_elem_count(); }

	//Create new MD_SET.
	MD_SET * create()
	{
		MD_SET * mds = get_free();
		if (mds == NULL) {
			mds = (MD_SET*)smpool_malloc_h_const_size(sizeof(MD_SET),
													  m_mds_pool);
			IS_TRUE0(mds);
			memset(mds, 0, sizeof(MD_SET));
			m_md_set_list.append_tail(mds);
			mds->init();
		}
		return mds;
	}

	//Destroy MD_SET manager.
	inline void destroy()
	{
		m_free_md_set.clean();
		SC<MD_SET*> * sc;
		for (MD_SET * mds = m_md_set_list.get_head(&sc);
			 mds != NULL; mds = m_md_set_list.get_next(&sc)) {
			mds->destroy();
		}
		m_md_set_list.clean();
		smpool_free_handle(m_mds_pool);
		smpool_free_handle(m_sc_mds_pool);
		m_mds_pool = NULL;
		m_sc_mds_pool = NULL;
	}

	void dump();

	UINT count_mem();
};


class MDID2MD : public SVECTOR<MD*> {
	UINT m_count;
public:
	MDID2MD() { m_count = 0; }

	void remove(UINT mdid)
	{
		IS_TRUE0(mdid != 0); //0 is illegal mdid.
		IS_TRUE0(get(mdid) != NULL);
		SVECTOR<MD*>::set(mdid, NULL);
		m_count--;
	}

	void set(UINT mdid, MD * md)
	{
		IS_TRUE(SVECTOR<MD*>::get(mdid) == NULL, ("already mapped"));
		SVECTOR<MD*>::set(mdid, md);
		m_count++;
	}

	UINT get_elem_count() const { return m_count; }
	void dump() const;
};


class MD2NODE2 {
public:
	MD_SET * mds;
	HMAP<UINT, MD2NODE2*, HASH_FUNC_BASE2<UINT> > next;

	MD2NODE2(UINT hash_tab_size = 16) : next(hash_tab_size) { mds = NULL; }
	~MD2NODE2() {}

	UINT count_mem() const { return next.count_mem() + sizeof(mds); }
};


class MD2NODE {
public:
	MD_SET * mds;
	TMAP<UINT, MD2NODE*> next;

	MD2NODE() { mds = NULL; }
	~MD2NODE() {}

	void init()
	{
		mds = NULL;
		next.init();
	}

	void destroy()
	{
		mds = NULL;
		next.destroy();
	}

	UINT count_mem() const { return next.count_mem() + sizeof(mds); }
};

#define _MD2NODE2_
#define MD2NODE2_INIT_SZ	8

class MD_SET_HASH {
protected:
	SMEM_POOL * m_pool;
	MD_SET_MGR * m_mds_mgr;

	#ifdef _DEBUG_
	UINT m_num_node; //record the number of MD2NODE in the tree.
	#endif

	#ifdef _MD2NODE2_
	typedef MD2NODE2 MNTY;
	#else
	typedef MD2NODE MNTY;
	#endif

	MNTY * m_md2node;

protected:
	inline MNTY * new_md2node()
	{
		#ifdef _MD2NODE2_
		MD2NODE2 * mn = new MD2NODE2(MD2NODE2_INIT_SZ);
		#else
		MD2NODE * mn =
			(MD2NODE*)smpool_malloc_h_const_size(sizeof(MD2NODE), m_pool);
		::memset(mn, 0, sizeof(MD2NODE));
		mn->init();
		#endif

		#ifdef _DEBUG_
		m_num_node++;
		#endif

		return mn;
	}

	void dump_helper_a(MD_SET * mds, UINT indent, UINT id)
	{
		fprintf(g_tfile, "\n");

		UINT i = 0;
		while (i < indent) { fprintf(g_tfile, " "); i++; }
		fprintf(g_tfile, "%d", id);

		if (mds != NULL) {
			fprintf(g_tfile, " {");
			for (INT i = mds->get_first(); i >= 0;) {
				fprintf(g_tfile, "%d", i);
				i = mds->get_next(i);
				if (i >= 0) {
					fprintf(g_tfile, ",");
				}
			}
			fprintf(g_tfile, "}");
		}
	}

	void dump_helper(MD2NODE * mn, UINT indent, MD_SYS * mdsys)
	{
		MD2NODE * nextmn = NULL;
		TMAP_ITER<UINT, MD2NODE*> ti;
		for (UINT id = mn->next.get_first(ti, &nextmn);
			 id != 0; id = mn->next.get_next(ti, &nextmn)) {
			dump_helper_a(nextmn->mds, indent, id);
			IS_TRUE0(nextmn);
			dump_helper(nextmn, indent + 2, mdsys);
		}
	}

	void dump_helper(MD2NODE2 * mn, UINT indent, MD_SYS * mdsys)
	{
		INT pos;
		for (MD2NODE2 * nextmn = mn->next.get_first_elem(pos);
			 nextmn != NULL; nextmn = mn->next.get_next_elem(pos)) {
			dump_helper_a(nextmn->mds, indent, pos);
			dump_helper(nextmn, indent + 2, mdsys);
		}
	}

public:
	MD_SET_HASH(MD_SET_MGR * mds_mgr)
	{
		IS_TRUE0(mds_mgr);
		m_mds_mgr = mds_mgr;
		m_pool = smpool_create_handle(sizeof(MD2NODE) * 4, MEM_CONST_SIZE);

		#ifdef _DEBUG_
		m_num_node = 0;
		#endif

		#ifdef _MD2NODE2_
		m_md2node = new MD2NODE2(MD2NODE2_INIT_SZ);
		#else
		m_md2node = new MD2NODE();
		#endif
	}

	~MD_SET_HASH()
	{
		destroy_mn2node();

		#ifdef _MD2NODE2_
		//m_md2node already deleted in destroy_mn2node().
		#else
		delete m_md2node;
		#endif

		smpool_free_handle(m_pool);
	}

	inline void destroy_mn2node()
	{
		//Destroy the TMAP structure, here you do
		//NOT need invoke MD_SET's destroy() because they were allocated from
		//MD_SET_MGR and will be deleted by the destroy() of it.
		LIST<MNTY*> wl;

		#ifdef _MD2NODE2_
		//Do nothing.
		#else
		TMAP_ITER<UINT, MD2NODE*> ti;
		#endif

		wl.append_tail(get_root());

		while (wl.get_elem_count() != 0) {
			MNTY * mn = wl.remove_head();
			IS_TRUE0(mn);

			MNTY * nextmn = NULL;

			#ifdef _MD2NODE2_
			INT pos = 0;
			for (nextmn = mn->next.get_first_elem(pos);
				 nextmn != NULL; nextmn = mn->next.get_next_elem(pos)) {
				wl.append_tail(nextmn);
			}
			#else
			ti.clean();
			for (UINT id = mn->next.get_first(ti, &nextmn);
				 id != 0; id = mn->next.get_next(ti, &nextmn)) {
				IS_TRUE0(nextmn);
				wl.append_tail(nextmn);
			}
			#endif

			if (mn->mds != NULL) {
				m_mds_mgr->free(mn->mds);
			}

			mn->next.destroy();

			#ifdef _MD2NODE2_
			delete mn;
			#else
			//Do nothing.
			#endif
		}
	}

	inline void check_and_grow(MNTY * mn)
	{
		#ifdef _MD2NODE2_
		//This class use HMAP to hash MD_SET.
		//As the element is more and more appended,
		//the collisions become more frequently.
		//Extend HMAP if the number of element is twice of the hash
		//table size.
		if (mn->next.get_elem_count() >	mn->next.get_bucket_size() * 2) {
			mn->next.grow();
		}
		#else
		//Do nothing.
		#endif
	}

	MD_SET const* append(MD_SET const& mds)
	{
		INT id = mds.get_first();
		if (id < 0) { return NULL; }

		check_and_grow(m_md2node);
		MNTY * mn = m_md2node->next.get(id);
		if (mn == NULL) {
			mn = new_md2node();
			m_md2node->next.set(id, mn);
		}

		INT nextid = mds.get_next(id);
		for (; nextid >= 0; id = nextid, nextid = mds.get_next(nextid)) {
			check_and_grow(mn);
			MNTY * nextmn = mn->next.get(nextid);
			if (nextmn == NULL) {
				nextmn = new_md2node();
				mn->next.set(nextid, nextmn);
			}
			mn = nextmn;
		}

		IS_TRUE0(mn);
		if (mn->mds == NULL) {
			MD_SET * s = m_mds_mgr->create();
			IS_TRUE0(s);
			s->copy(mds);
			mn->mds = s;
		}
		IS_TRUE0(mn->mds == &mds || mn->mds->is_equal(mds));
		return mn->mds;
	}

	UINT count_mem() const
	{
		UINT count = smpool_get_pool_size_handle(m_pool);
		count += get_root()->count_mem();
		return count;
	}

	void dump(MD_SYS * mdsys)
	{
		if (g_tfile == NULL) { return; }

		fprintf(g_tfile, "\n==---- DUMP MD_SET_HASH ----==");

		#ifdef _DEBUG_
		fprintf(g_tfile, " NumOfNode:%d ----==",
				m_num_node + 1 /*The first node is m_md2node*/);
		#endif

		dump_helper(get_root(), 1, mdsys);

		fflush(g_tfile);
	}

	MNTY * get_root() const  { return m_md2node; }

	//Return the number of MD_SET recorded in the hash.
	UINT get_elem_count() const
	{
		UINT count = 0;

		//Destroy the TMAP structure, here you do
		//NOT need invoke MD_SET's destroy() because they were allocated from
		//MD_SET_MGR and will be deleted by the destroy() of it.

		LIST<MNTY const*> wl;

		#ifdef _MD2NODE2_
		//Do nothing.
		#else
		TMAP_ITER<UINT, MD2NODE*> ti;
		#endif

		wl.append_tail(get_root());
		while (wl.get_elem_count() != 0) {
			MNTY const* mn = wl.remove_head();
			IS_TRUE0(mn);

			MNTY * nextmn = NULL;
			#ifdef _MD2NODE2_
			INT pos = 0;
			for (nextmn = mn->next.get_first_elem(pos);
				 nextmn != NULL; nextmn = mn->next.get_next_elem(pos)) {
				wl.append_tail(nextmn);
			}
			#else
			ti.clean();
			for (UINT id = mn->next.get_first(ti, &nextmn);
				 id != 0; id = mn->next.get_next(ti, &nextmn)) {
				IS_TRUE0(nextmn);
				wl.append_tail(nextmn);
			}
			#endif

			if (mn->mds != NULL) {
				count++;
			}
		}
		return count;
	}

	//Return true if MD_SET pointer has been record in the hash.
	bool find(MD_SET const& mds)
	{
		INT id = mds.get_first();
		if (id < 0) { return false; }

		MNTY * mn = get_root()->next.get(id);
		if (mn == NULL) { return false; }

		INT nextid = mds.get_next(id);
		for (; nextid >= 0; id = nextid, nextid = mds.get_next(nextid)) {
			MNTY * nextmn = mn->next.get(nextid);
			if (nextmn == NULL) { return false; }
			mn = nextmn;
		}

		IS_TRUE0(mn);
		if (mn->mds == NULL) {
			return false;
		}
		return mn->mds == &mds;
	}
};


/* MD System.
Manage the memory allocation and free of MD, and
the mapping between MD_id and MD.
Manage the memory allocation and free of MD_TAB, and
the mapping between VAR_id and MD_TAB.
NOTE: each region manager has a single MD_SYS. */
class MD_SYS {
	SMEM_POOL * m_pool;
	SMEM_POOL * m_sc_mdptr_pool;
	SMEM_POOL * m_sc_mdid_pool;
	VAR * m_all_mem;
	VAR * m_global_mem;
	MDID2MD m_id2md_map; //Map MD id to MD.
	SLIST<MD*> m_free_md_list; //MD allocated in pool.
	SLIST<UINT> m_free_mdid_list;
	UINT m_md_count; //generate MD index, used by register_md().
	SVECTOR<MD_TAB*> m_varid2mdtab; //map VAR id to MD_TAB.


	inline MD * new_md()
	{
		MD * md = m_free_md_list.remove_head();
		if (md == NULL) {
			md = (MD*)smpool_malloc_h_const_size(sizeof(MD), m_pool);
			md->clean();
		}
		return md;
	}

	//Allocated object should be recorded in list.
	MD_TAB * new_md_tab() { return new MD_TAB(); }
	MD2NODE * new_md2node();
	void init_global_mem(VAR_MGR * vm);
	void init_all_mem(VAR_MGR * vm);
public:
	MD_SYS(VAR_MGR * vm) { init(vm); }
	~MD_SYS() { destroy(); }

	void init(VAR_MGR * vm);
	void clean();
	void compute_overlap(MD const* md, MD_SET & output,
						 TMAP_ITER<MD*, MD*> & tabiter, bool strictly);
	void compute_overlap(IN OUT MD_SET & mds, SVECTOR<MD*> & tmpvec,
						 TMAP_ITER<MD*, MD*> & tabiter, bool strictly);
	void compute_overlap(MD_SET const& mds, OUT MD_SET & output,
						 TMAP_ITER<MD*, MD*> & tabiter, bool strictly);

	//Dump all registered MDs.
	void dump_all_mds();
	void destroy();

	//Get registered MD.
	//NOTICE: DO NOT free the return value, because it is the registered one.
	MD * get_md(UINT id)
	{
		IS_TRUE0(id != 0);
		MD * md = m_id2md_map.get(id);
		IS_TRUE0(md == NULL || MD_id(md) == id);
		return md;
	}

	MD const* get_md_c(UINT id) const
	{
		IS_TRUE0(id != 0);
		MD * md = m_id2md_map.get(id);
		IS_TRUE0(md == NULL || MD_id(md) == id);
		return md;
	}

	//Get MD TAB that described mds which under same base VAR.
	inline MD_TAB * get_md_tab(VAR const* v)
	{
		IS_TRUE0(v);
		return m_varid2mdtab.get(VAR_id(v));
	}

	inline UINT get_num_of_md() const { return m_id2md_map.get_elem_count(); }
	inline MDID2MD const* get_id2md_map() const { return &m_id2md_map; }

	inline void free_md(MD * md)
	{
		if (md == NULL) { return; }
		m_id2md_map.remove(MD_id(md));
		m_free_mdid_list.append_head(MD_id(md));
		memset(md, 0, sizeof(MD));
		m_free_md_list.append_head(md);
	}

	MD * register_md(MD & m);
	MD_SET const* register_mds(MD_SET const& mds, MD_SET_MGR * mdsmgr);
	void remove_var(VAR * v);
};


//Return true if all MD in set are PR.
bool MD_SET::is_pr_set(MD_SYS const* mdsys) const
{
	for (INT i = get_first(); i >= 0; i = get_next(i)) {
		MD const* md = mdsys->get_md_c(i);
		IS_TRUE0(md);
		if (!md->is_pr()) { return false; }
	}
	return true;
}


//MD2MD_SET_MAP
//Record MD->MDS relations.
class MD2MDS : public BVEC<MD_SET const*> {
public:
	~MD2MDS()
	{
		IS_TRUE0(get_elem_count() == 0); //should call free first.
	}

	/* Clean each MD->MD_SET, but do not free MD_SET.
	This function might be invoked during each iter
	in computing PT_SET in general. */
	void clean() { BVEC<MD_SET const*>::clean(); }

	UINT count_mem()
	{
		UINT count = 0;
		for (INT i = get_first(); i >= 0; i = get_next(i)) {
			MD_SET const* mds = BVEC<MD_SET const*>::get(i);
			if (mds == NULL) { continue; }
			count += mds->count_mem();
		}
		count += BVEC<MD_SET const*>::count_mem();
		return count;
	}

	void dump(REGION * ru);
};
#endif
