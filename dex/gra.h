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
#ifndef __GRA_H__
#define __GRA_H__

#define LT_FIRST_POS	0
#define RG_PAIR_SZ 		2
#define FIRST_PHY_REG	0

class GLT;
class LTM;
class LTG;
class RA;
class GLTM;
class PRDF;
class RSC;
class RG;

//Local Life Time.
#define LT_uid(lt)				((lt)->id)
#define LT_range(lt)			((lt)->range)
#define LT_occ(lt)				((lt)->occ)
#define LT_usable(lt)			((lt)->usable)
#define LT_prno(lt)				((lt)->prno)
#define LT_priority(lt)			((lt)->priority)
#define LT_phy(lt)				((lt)->phy)
#define LT_prefer_reg(lt)		((lt)->prefer_reg)
#define LT_is_global(lt)		((lt)->is_global_pr)
#define LT_ltg(lt)				((lt)->lt_group)
#define LT_ltg_pos(lt)			((lt)->lt_group_pos)
#define LT_rg(lt)				((lt)->reg_group)
#define LT_rg_sz(lt)			((lt)->reg_group_size)
class LT {
public:
	UINT prno;
	UINT id;
	BITSET * range;
	BITSET * occ;
	BITSET const* usable;
	LTG * lt_group;
	RG * reg_group;	//register group
	float priority;
	USHORT phy;	//physical register.
	USHORT prefer_reg;
	USHORT reg_group_size; //the size of register group.
	BYTE lt_group_pos:5;
	BYTE is_global_pr:1; //true if pr is global.

	void clean();
	inline bool is_intersect(LT const* lt) const
	{
		IS_TRUE0(range);
		return LT_range(this)->is_intersect(*LT_range(lt));
	}
	inline bool is_def(UINT pos) const
	{ return (pos & 1) == 0; }
	inline bool is_reg_equal(LT const* l) const
	{ return prno == LT_prno(l) || phy == LT_phy(l); }

	/*
	Get forward searching occurrence corresponds to 'pos',
	or return -1 if nothing find.
	e.g:
		lowest_pos....pos...forward_occ...highest_pos
	*/
	inline INT get_forward_occ(INT pos, OUT bool * is_def, INT firstpos)
	{
		IS_TRUE(pos >= firstpos, ("Illegal position"));
		IS_TRUE0(occ);
		pos = occ->get_next(pos);
		*is_def = this->is_def(pos);
		return pos;

		//for (INT i = range->get_next(pos);
		//	 i >= 0; i = range->get_next(i)) {
		//	IR const* p = range->get(i);
		//	if (p != NULL) {
		//		*is_def = this->is_def(i);
		//		return i;
		//	}
		//}
		//*is_def = false;
		//return -1;
	}

	/*
	Get forward searching def corresponds to 'pos',
	or return -1 if nothing find.
	e.g:
		lowest_pos....pos...forward_def...highest_pos
	*/
	inline INT get_forward_def_occ(INT pos, INT firstpos)
	{
		IS_TRUE(pos >= firstpos, ("Illegal position"));
		IS_TRUE0(occ);
		for (pos = occ->get_next(pos); pos >= 0; pos = occ->get_next(pos)) {
			if (is_def(pos)) {
				return pos;
			}
		}
		return -1;
	}

	/*
	Get backward occurrences of 'pos'
	e.g:
		Lowest_Pos...Backward_Occ....Pos.....Highest_Pos
	*/
	inline INT get_backward_occ(INT pos, OUT bool * is_d, INT firstpos)
	{
		IS_TRUE(pos >= firstpos, ("Illegal position"));
		if (pos == firstpos && (occ == NULL || occ->is_empty())) { return -1; }

		INT backwpos = -1;
		INT start = LT_range(this)->get_first();
		for (INT i = pos - 1; i >= start; i--) {
			if (!occ->is_contain(i)) { continue; }
			backwpos = i;
			if (is_def(i)) {
				*is_d = true;
			} else {
				*is_d = false;
			}
			break;
		}
		return backwpos;
	}

	inline INT get_backward_def_occ(INT pos, INT firstpos)
	{
		IS_TRUE(pos >= firstpos, ("Illegal position"));
		if (pos == firstpos && (occ == NULL || occ->is_empty())) { return -1; }

		INT start = LT_range(this)->get_first();
		for (INT i = pos - 1; i >= start; i--) {
			if (!occ->is_contain(i)) { continue; }
			if (is_def(i)) {
				return i;
			}
		}
		return -1;
	}

	bool has_allocated() const { return LT_phy(this) != REG_UNDEF; }
	bool has_branch(LTM * ltm) const;

	GLT * set_global(GLTM & gltm);
};


class PR2LT_MAP : public HMAP<UINT, LT*> {
public:
	PR2LT_MAP(UINT bsize = 0) :
		HMAP<UINT, LT*>(bsize) {}
};


typedef enum _LTG_TYPE {
	LTG_UNDEF = 0,
	LTG_RANGE_PARAM,
	LTG_REG_PAIR,
} LTG_TYPE;

//Lifetime Group.
class LTG : public SVECTOR<LT*> {
public:
	LTG_TYPE ty;
	LTG()
	{ ty = LTG_UNDEF; }

	//Return the number of lt in group.
	UINT get_lt_count() const { return get_last_idx() + 1; }

	//Return register group size.
	UINT get_rg_sz() const
	{
		UINT rgsz = 0;
		for (INT i = 0; i <= get_last_idx(); i++) {
			LT const* l = get(i);
			IS_TRUE0(l);
			rgsz += LT_rg_sz(l);
		}
		return rgsz;
	}

	//Return true if the lt that corresponded to 'prno' is in group.
	bool is_member(UINT prno)
	{
		for (INT i = 0; i <= get_last_idx(); i++) {
			LT * l = get(i);
			IS_TRUE0(l);
			if (LT_prno(l) == prno) {
				return true;
			}
		}
		return false;
	}
};


//Register Group.
class RG {
public:
	BYTE rnum; //the number of group member
	USHORT * rvec; //buffer to hold group member.

	void set(UINT i, UINT phy)
	{
		IS_TRUE0(i < rnum && rvec && phy != REG_UNDEF);
		rvec[i] = (USHORT)phy;
	}

	UINT get(UINT i)
	{
		IS_TRUE0(i < rnum && rvec);
		return (UINT)rvec[i];
	}
};


//Register Group Mgr
class RG_MGR {
	SMEM_POOL * m_pool;
public:
	RG_MGR()
	{ m_pool = smpool_create_handle(0, MEM_COMM); }
	~RG_MGR()
	{ smpool_free_handle(m_pool); }

	RG * create(UINT n)
	{
		RG * rg = (RG*)smpool_malloc_h(sizeof(RG), m_pool);
		rg->rvec = (USHORT*)smpool_malloc_h(sizeof(USHORT) * n, m_pool);
		rg->rnum = n;
		return rg;
	}
};


//Map from IR to INT.
typedef TMAP<IR*, INT> IR2INT;


//Lifetime Group Mgr
class LTG_MGR : public TMAP<UINT, LTG*> {
	LIST<LTG*> m_ltgs;
public:
	virtual ~LTG_MGR()
	{
		for (LTG * ltg = m_ltgs.get_head();
			 ltg != NULL; ltg = m_ltgs.get_next()) {
			delete ltg;
		}
	}

	LTG * map_ir2ltg(UINT irid)
	{ return get(irid); }

	void set_map_ir2ltg(UINT irid, LTG * ltg)
	{ set(irid, ltg); }

	LTG * create()
	{
		LTG * ltg = new LTG();
		m_ltgs.append_tail(ltg);
		return ltg;
	}
};


class IG : public GRAPH {
	LTM * m_ltm;
public:
	IG() {}
	void set_ltm(LTM * ltm) { IS_TRUE0(ltm); m_ltm = ltm; }
	bool is_interf(LT const* lt1, LT const* lt2) const;
	void build();
	void dump_vcg(CHAR const* name = NULL);
	void get_neighbor(OUT LIST<LT*> & nis, LT * lt) const;
};


//Local Life Time Manager.
//TODO: update lt incrementally.
class LTM {
protected:
	friend class GLTM;
	friend class BBRA;
	friend class RSC;
	friend class RA;
	IR_BB * m_bb;
	//PR2LT_MAP m_prno2lt_map;
	TMAP<UINT, LT*> m_prno2lt;
	SVECTOR<LT*> m_lt_vec;
	SVECTOR<IR*> m_pos2ir;
	IG m_ig;
	SMEM_POOL * m_pool;
	PRDF * m_prdf;
	PRNO2UINT * m_pr2v;
	UINT2PR * m_v2pr;
	GLTM * m_gltm;
	DT_MGR * m_dm;
	REGION * m_ru;
	RA * m_ra;
	UINT m_lt_count; //local lt count.
	UINT m_max_lt_len;

	void * xmalloc(UINT size)
	{
		IS_TRUE0(m_pool);
		void * p = smpool_malloc_h(size, m_pool);
		IS_TRUE0(p != NULL);
		memset(p, 0, size);
		return p;
	}

	void revise_lt_case_1(LT * lt);
	void revise_special_lt(LIST<LT*> * lts);
	void process_rg(LT * lt);
	void process_liveout(IN OUT BITSET & lived_lt, UINT pos,
						 bool always_consider_glt);
	void process_livein(IN OUT BITSET & lived_lt, UINT pos,
						bool always_consider_glt);
	void process_func_exit_bb(IN OUT LIST<LT*> * liveout_exitbb,
							  IN OUT BITSET & lived_lt,
							  IN BITSET const& retval_regset, UINT pos);
	void process_use(IN IR * ir, CIR_ITER & cii, INT pos,
					 OUT BITSET & lived_lt, bool group_part);
	void process_res(IN IR * ir, INT pos, OUT BITSET & lived_lt,
					 bool group_part);
	LT * process_use_pr(IR const* ir, UINT pos, OUT BITSET & lived_lt);
	LT * process_res_pr(UINT prno, UINT pos, OUT BITSET & lived_lt);
	void process_use_group_part(IR const* ir, UINT pos, OUT BITSET & lived_lt);
	void process_res_group_part(IR const* ir, UINT pos, OUT BITSET & lived_lt);

	void gen_range_call_group(IR const* ir);
	IR * gen_dedicate_pr(UINT phyrid);
	void record_phy_reg_occ(LT * lt, UINT pos, IN BITSET & lived_lt);
	void rename_use(IR * ir, LT * l, IR ** newpr);
	void dump_allocated(FILE * h, BITSET & visited);
	void dump_unallocated(FILE * h, BITSET & visited);
public:
	LTM(IR_BB * bb, PRDF * prdf, GLTM * gltm, SMEM_POOL * pool);
	~LTM() {}

	void build(bool consider_glt, LIST<LT*> * liveout_exitbb_lts,
			   CIR_ITER & iter);
	void build_group(CIR_ITER & cii);

	void clean();

	IR_BB * get_bb() { return m_bb; }
	LT * get_lt(UINT ltid) { return m_lt_vec.get(ltid); }
	SVECTOR<LT*> * get_lt_vec() { return &m_lt_vec; }
	UINT get_first_pos() const { return LT_FIRST_POS; }
	UINT get_last_pos() const { return m_max_lt_len - 1; }
	IR * get_ir(UINT pos) { return m_pos2ir.get(pos); }
	IR * gen_mapped_pr(UINT vid, UINT tyid);
	IG * get_ig() { return &m_ig; }
	void gen_group(LT * first, LT * second);
	IR * gen_spill(LT * lt, INT pos);
	IR * gen_spill(UINT prno, UINT dt, IR * marker, IR * spill_loc);
	IR * gen_spill_swap(IR * stmt, UINT prno, UINT prdt, IR * spill_loc);
	IR * gen_reload(LT * lt, INT pos, IR * spill_loc);
	IR * gen_reload(IR * newpr, IR * marker, IR * spill_loc);
	IR * gen_reload_swap(IR * newpr, IR * marker);

	bool has_pair_res(IR * ir);

	inline bool is_pair(IR const* ir) const
	{ return m_dm->get_dtd_bytesize(IR_dt(ir))== 8; }

	inline bool is_livein(UINT prno) const
	{ return m_prdf->get_livein(IR_BB_id(m_bb))->is_contain(prno); }
	inline bool is_livein(LT const* l) const
	{
		IS_TRUE0(LT_range(l));
		return LT_range(l)->is_contain(get_first_pos());
	}

	inline bool is_liveout(UINT prno) const
	{ return m_prdf->get_liveout(IR_BB_id(m_bb))->is_contain(prno); }
	inline bool is_liveout(LT const* l) const
	{
		IS_TRUE0(LT_range(l));
		return LT_range(l)->is_contain(get_last_pos());
	}

	inline LT * map_pr2lt(UINT prno) const
	{
		if (m_prno2lt.get_elem_count() == 0) {
			return NULL;
		}
		return m_prno2lt.get_c(prno);
	}

	LT * new_lt(UINT prno);

	void remove_lt(LT * lt);
	void rename_lt(LT * l, IR ** newpr);
	void rename(TMAP<UINT, LT*> & prno2lt, BITSET & met);

	void dump();
};


#define GLT_id(g)			((g)->id)
#define GLT_prno(g)			((g)->prno)
#define GLT_phy(g)			((g)->phy)
#define GLT_prefer_reg(g)	((g)->prefer_reg)
#define GLT_bbs(g)			((g)->livebbs)
#define GLT_prio(g)			((g)->prio)
#define GLT_freq(g)			((g)->freq)
#define GLT_usable(g)		((g)->usable)
#define GLT_rg(g)			((g)->reg_group)
#define GLT_rg_sz(g)		((g)->reg_group_size)
#define GLT_is_param(g)		((g)->is_param)
#define GLT_param_pos(g)	((g)->param_pos)
class GLT {
public:
	UINT id;
	UINT prno;
	float prio;
	float freq;
	DBITSETC * livebbs;
	RG * reg_group;
	BITSET const* usable;
	USHORT phy;
	USHORT prefer_reg;
	USHORT reg_group_size; //the size of register group.
	BYTE param_pos;
	BYTE is_param:1;

	UINT comp_num_of_occ(GLTM & gltm);
	bool has_allocated() const { return GLT_phy(this) != REG_UNDEF; }
	void set_local(GLTM & gltm);
	void set_local_usable(GLTM & gltm);
};


class GLT2REG : public HMAP<GLT*, UINT> {
public:
	GLT2REG() : HMAP<GLT*, UINT>(0) {}

	void set(GLT * g, UINT v)
	{
		IS_TRUE0(v != 0);
		HMAP<GLT*, UINT>::set(g, v);
	}
};


/*
Format of DEX instructions.
c: constant.
v: value, can be offset or idx.
h: high part value.
*/
typedef enum _FMT {
	FUNDEF = 0,
	F0,				//op
	FAB,			// op vA, vB
	FABcv,			// op vA, #+B
	FAA,			// op vAA
	FAAv,			// op +AA
	FAAAAv,			// op +AAAA
	FAABBBB,		// op vAA, vBBBB

	FAABBBBv,		// op vAA, +BBBB
					// op vAA, thing@BBBB
	FAABBBBcv,		// op vAA, #+BBBB
	FAABBBBcvh,		// op vAA, #+BBBB00000[00000000]
	FAABBCC,		// op vAA, vBB, vCC
	FAABBCCcv,		// op vAA, vBB, #+CC
	FABCCCCv,		// op vA, vB, +CCCC
	FABCCCCcv,		// op vA, vB, #+CCCC
					// op vA, vB, thing@CCCC
	FAAAABBBB,		// op vAAAA, vBBBB
	FAAAAAAAAv,		// op +AAAAAAAA
	FAABBBBBBBBv,	// op vAA, +BBBBBBBB
					// op vAA, thing@BBBBBBBB
	FAABBBBBBBBcv,	// op vAA, #+BBBBBBBB
	FACDEFGBBBBv,	// op {vC, vD, vE, vF, vG}, thing@BBBB (B: count, A: vG)
	FAACCCCBBBBv,	// op {vCCCC .. v(CCCC+AA-1)}, meth@BBBB
	FAABBBBBBBBBBBBBBBBcv,  // op vAA, #+BBBBBBBBBBBBBBBB
	FNUM,
} FMT;


//GLT Manager
class GLTM {
	friend class LTM;
	friend class RA;
	friend class GIG;
protected:
	SVECTOR<BITSET*> m_glt2usable_regs; //Map GLT to its usable registers.
	SVECTOR<LTM*> m_bb2ltmgr; //Map from BB id to LTM.
	SVECTOR<GLT*> m_gltid2glt_map; //Map from id to GLT.
	SDBITSET_MGR m_sbs_mgr;
	RG_MGR m_rg_mgr;
	BITSET_MGR m_bs_mgr;
	LTG_MGR m_ltgmgr;
	SVECTOR<GLT*> m_pr2glt;
	SVECTOR<UINT> m_params;
	CIR_ITER m_cii; //const IR iter.
	REGION * m_ru;
	RA * m_ra;
	RSC * m_rsc;
	SMEM_POOL * m_pool;
	PRDF * m_prdf;
	DT_MGR * m_dm;
	UINT m_glt_count;
	bool m_is_consider_local_interf;

	void comp_st_usage(IR const* ir);
	void comp_ir_usage(IR const* ir);
	void * xmalloc(UINT size)
	{
		IS_TRUE(m_pool != NULL,("need graph pool!!"));
		void * p = smpool_malloc_h(size, m_pool);
		IS_TRUE0(p != NULL);
		memset(p, 0, size);
		return p;
	}
	bool verify();
public:
	GLTM(REGION * ru, PRDF * prdf, RA * ra);
	~GLTM()
	{
		for (INT i = 0; i <= m_bb2ltmgr.get_last_idx(); i++) {
			LTM * l = m_bb2ltmgr.get(i);
			if (l != NULL) {
				delete l;
			}
		}
		smpool_free_handle(m_pool);
	}

	void build(bool build_group_part);
	GLT * build_glt_like(IR * pr, GLT * cand);
	void dump();
	void dumpg();
	void dumpl(UINT bbid);

	//Get LTM via BB's id.
	LTM * get_ltm(UINT bbid) { return m_bb2ltmgr.get(bbid); }
	REGION * get_ru() { return m_ru; }
	BITSET_MGR * get_bs_mgr() { return &m_bs_mgr; }
	SVECTOR<GLT*> * get_pr2glt_map() { return &m_pr2glt; }
	UINT get_num_of_glt() const { return m_glt_count - 1; }
	SVECTOR<GLT*> * get_gltvec() { return &m_gltid2glt_map; }
	BITSET * get_usable_regs(GLT const* g, bool alloc)
	{
		BITSET * rs = m_glt2usable_regs.get(GLT_id(g));
		if (rs == NULL && alloc) {
			rs = m_bs_mgr.create();
			m_glt2usable_regs.set(GLT_id(g), rs);
		}
		return rs;
	}
	GLT * get_glt(UINT gltid) { return m_gltid2glt_map.get(gltid); }

	//Free all DBITSETC allocated.
	void free_gltbs()
	{
		SVECTOR<GLT*> * gltv = get_gltvec();
		for (INT i = 0; i <= gltv->get_last_idx(); i++) {
			GLT * g = gltv->get(i);
			if (g == NULL) { continue; }
			m_sbs_mgr.free_dbitsetc(GLT_bbs(g));
		}
	}

	RG * new_rg(UINT rnum) { return m_rg_mgr.create(rnum); }
	GLT * new_glt(UINT prno);

	LTM * map_bb2ltm(IR_BB * bb);

	GLT * map_pr2glt(UINT prno)
	{ return m_pr2glt.get(prno); }

	void rename_use(IR const* ir, LT * l, IR ** newpr);
	void rename_glt(GLT * g);
	void rename_local();
	void rename();
	void set_consider_local_interf(bool doit)
	{ m_is_consider_local_interf = doit; }

	void localize(GLT * g);
};



//GIG
#define GIG_ru(g)			((g)->m_ru)
#define GIG_glt_mgr(g)		((g)->m_glt_mgr)
class GIG : public GRAPH {
protected:
public:
	REGION * m_ru;
	IR_CFG * m_cfg;
	GLTM * m_gltm;

	//consider local life time interference during global allocation.
	bool m_is_consider_local_interf;

	void dump_vcg(CHAR const* name = NULL);
	GIG(REGION * ru, GLTM * glt_mgr)
	{
		IS_TRUE0(ru && glt_mgr);
		m_gltm = glt_mgr;
		m_ru = ru;
		m_cfg = m_ru->get_cfg();
		m_is_consider_local_interf = false;
		set_direction(false);
	}

	void add_glt(GLT * g) { add_vertex(GLT_id(g)); }
	void remove_glt(GLT * g) { remove_vertex(GLT_id(g)); }
	bool is_interf(IN GLT * glt1, IN GLT * glt2);
	bool is_interf_with_neighbour(GLT * g, SBITSET & nis, UINT phy);

	void set_consider_local_interf(bool doit)
	{ m_is_consider_local_interf = doit; }
	void set_interf_with(UINT gltid, LIST<UINT> & lst);

	void rebuild()
	{
		erase();
		build();
	}
	void build();
};


//Compute the Resource Constrains
class RSC {
	friend class RA;
protected:
	BITSET * m_4;
	BITSET * m_8;
	BITSET * m_16;
	REGION * m_ru;
	GLTM * m_gltm;
	DT_MGR * m_dm;
	BITSET_MGR * m_bsm;
	SVECTOR<FMT> m_ir2fmt;
	STR2INTRI m_str2intri;
	BITSET * m_usable[FNUM][2]; //1:def, 0:use

	void init_usable();
public:
	RSC(GLTM * gltm)
	{
		IS_TRUE0(gltm);
		m_gltm = gltm;
		m_ru = gltm->get_ru();
		m_dm = m_ru->get_dm();
		m_bsm = gltm->get_bs_mgr();
		m_4 = NULL;
		m_8 = NULL;
		m_16 = NULL;
		init_usable();
	}
	~RSC()
	{
		m_bsm->free(m_4);
		m_bsm->free(m_8);
		m_bsm->free(m_16);
	}

	//Return the usable regs for 'fmt'.
	BITSET * get_usable(FMT fmt, bool is_def)
	{
		IS_TRUE0(FUNDEF < fmt && fmt < FNUM);
		return m_usable[fmt][is_def];
	}

	BITSET * get_4();
	BITSET * get_8();
	BITSET * get_16();
	void comp_st_fmt(IR const* ir);
	void comp_ist_fmt(IR const* ir);
	void comp_call_fmt(IR const* ir);
	void comp_ir_fmt(IR const* ir);
	void comp_usable_regs(LT * lt, LTM * ltm);
	void comp_ir_constrain();
	void comp_lt_usable(LT * lt, LTM * ltm);
	void comp_local_usage(LTM * ltm, bool only_local, bool omit_constrain);

	void dump_ir_fmt();
	void dump_glt_usable();
	void dump_bb(UINT bbid);
	void dump();
	FMT get_fmt(IR const* ir) const { return m_ir2fmt.get(IR_id(ir)); }

	GLT * map_lt2glt(LT * lt) { return m_gltm->map_pr2glt(LT_prno(lt)); }
	bool verify_fmt();
	void perform(bool omit_constrain);
};


class BBRA {
	IR_BB * m_bb;
	RA * m_ra;
	GLTM * m_gltm;
	LTM * m_ltm;
	REGION * m_ru;
	RSC * m_rsc;
	IG * m_ig;

	//true if omit constrain when compute lt usable-regs set.
	BYTE m_omit_constrain:1;

	LIST<LT*> * m_tmp_lts; //for local tmp use.
	LIST<LT*> * m_tmp_lts2; //for local tmp use.
	LIST<UINT> * m_tmp_uints; //for local tmp use.
	CIR_ITER * m_tmp_cii; //for local tmp use.

	LT * comp_split_cand(LT * lt, bool & has_hole,
						 LIST<LT*> * tmp, LIST<LT*> * tmp2);
	void compute_lt_residein_hole(OUT LIST<LT*> & reside_in, LT const* lt);
	bool can_be_split(LT const* lt) const;
	void collect_unalloc(LIST<LT*> & unalloc);

	void dump_prio(LIST<LT*> & prios);
	bool get_max_hole(OUT INT * startpos, OUT INT * endpos, LT const* lt);

	bool find_hole(OUT INT & startpos, OUT INT & endpos,
				   LT const* owner, LT const* inner);

	bool is_live_through(LT const* l) const;
	bool is_opnd_samewith_result(IR * ir);

	void select_reasonable_split_pos(OUT INT & pos1, OUT INT & pos2,
									 OUT bool & is_pos1_spill,
									 OUT bool & is_pos2_spill,
									 LT * lt);
	void split_lt_at(INT start, INT end, bool is_start_spill,
					 bool is_end_spill, LT * lt);
	bool split(LT * lt);
	bool satisfied_constrain(LT * lt, LT * cand);

	void rename_result(IR * ir, UINT old_prno, IR * newpr);
	void rename_opnd(IR * ir, UINT old_prno, IR * newpr);
	void rename_opnds_in_range(LT * lt, IR * newpr, INT start, INT end);
public:
	BBRA(IR_BB * bb, RA * ra);
	~BBRA() {}
	void build_prio_list(IN LIST<LT*> const& lts, OUT LIST<LT*> & prios);
	float comp_prio(LT const* lt);
	void rename();
	bool assign_reg(LT * l, LIST<UINT> & nis);
	void alloc_prio_list(OUT LIST<LT*> & prios, LIST<UINT> & nis);

	void set_omit_constrain(bool omit) { m_omit_constrain = omit; }
	void set_tmp_lts(LIST<LT*> * tmp) { IS_TRUE0(tmp); m_tmp_lts = tmp; }
	void set_tmp_lts2(LIST<LT*> * tmp) { IS_TRUE0(tmp); m_tmp_lts2 = tmp; }
	void set_tmp_uints(LIST<UINT> * tmp) { IS_TRUE0(tmp); m_tmp_uints = tmp; }
	void set_tmp_cii(CIR_ITER * tmp) { IS_TRUE0(tmp); m_tmp_cii = tmp; }

	bool solve(LIST<LT*> & prios);
	bool perform(LIST<LT*> & prios);
};


//RA
class RA {
protected:
	friend class BBRA;
	friend class LTM;
	friend class GLTM;

	PRDF m_prdf;
	GLTM m_gltm;
	GIG m_ig;
	RSC m_rsc;
	REGION * m_ru;
	IR_CFG * m_cfg;
	DT_MGR * m_dm;
	TYIDR * m_tr;
	UINT2PR * m_v2pr;
	PRNO2UINT * m_pr2v;
	VAR2PR * m_var2pr;
	UINT m_param_num; //record the number of param.
	UINT m_param_reg_start;
	UINT m_vregnum; //record the number of original vreg.
	UINT m_maxreg; //record the max vreg used. note it is not the number of vreg.
	IR_ITER m_ii; //for tmp used.
	CIR_ITER m_cii; //for tmp used.


	void assign_ltg(LTG * ltg, IR * ir);
	bool assign_reg(GLT * g, LIST<UINT> & nis, LIST<UINT> & nis2);
	void alloc_param();
	void alloc_group();
	void alloc_prio_list(OUT LIST<GLT*> & prios, OUT LIST<GLT*> & unalloc,
						  LIST<UINT> & nis, LIST<UINT> & nis2);
	void alloc_global(LIST<UINT> & nis, LIST<UINT> & nis2);
	void alloc_local(LIST<UINT> & nis, bool omit_constrain);
	void alloc_local_spec(LIST<UINT> & nis);

	void build_local_ig();
	void build_prio_list(OUT LIST<GLT*> & prios);

	inline bool check_if_need_spill(UINT prno, FMT fmt, LTM const* ltm);
	UINT comp_reserve_reg(IR_ITER & ii, LIST<IR*> & resolve_list);
	UINT comp_regn(UINT rangestart, UINT rangeend, LTG const* ltg,
				   BITSET const& occupied, BITSET const& assigned,
				   BITSET const& liveout_phy);
	UINT comp_satisfied_regn(UINT rangestart, LTG const* ltg, UINT rgsz,
							 BITSET const& occupied, BITSET const& assigned,
							 BITSET const& liveout_phy);
	float compute_prio(GLT * g);

	void dump_ltg();
	void diff_local_neighbour_used(GLT * g, LIST<UINT> & nis,
								   BITSET * unusable);
	void free_gltbs();

	IR * insert_move_before(IR * stmt, IR * src);
	bool is_cross_param(UINT reg_start, UINT rg_sz) const
	{ return reg_start < m_param_num && (reg_start + rg_sz) > m_param_num; }

	bool is_cross_liveout_phy(UINT reg_start, UINT rgsz,
							  BITSET const& liveout_phy);

	void shift_reg(UINT ofst);
	IR * split(GLT * g);
	void solve_conflict(OUT LIST<GLT*> & unalloc, LIST<UINT> & nis);
	RA * self() { return this; }

	INT try_reuse_appeared(LTG const* ltg, BITSET const& occupied,
						   BITSET const& assigned, BITSET const& liveout_phy);
	INT try_extend(LTG const* ltg, BITSET const& occupied,
				   BITSET const& liveout_phy, BITSET const& assigned);

	void remedy_ltg(LTG * ltg, IR * ir, LTM * ltm,
					SBITSET & nis, BITSET & visited, UINT rangestart);
	void revise_rsc();
	void rotate_reg();
	void revise_param();
	bool overlap_param(LT const* l) const;
public:
	RA(REGION * ru, TYIDR * tr, UINT param_num, UINT vregnum,
	   UINT2PR * v2pr, PRNO2UINT * pr2v, VAR2PR * var2pr) :
		m_prdf(ru), m_gltm(ru, &m_prdf, self()), m_ig(ru, &m_gltm),
		m_rsc(&m_gltm)
	{
		IS_TRUE0(ru && tr);
		m_ru = ru;
		m_cfg = ru->get_cfg();
		m_dm = ru->get_dm();
		m_tr = tr;
		m_param_num = param_num;
		m_vregnum = vregnum;
		m_maxreg = 0;
		m_v2pr = v2pr;
		m_pr2v = pr2v;
		m_var2pr = var2pr;
	}
	~RA(){}

	LT * get_lt(UINT bbid, UINT prno)
	{
		LTM * ltm = m_gltm.get_ltm(bbid);
		if (ltm == NULL) { return NULL; }
		return ltm->map_pr2lt(prno);
	}
	GLT * get_glt(UINT prno) { return m_gltm.map_pr2glt(prno); }
	GLTM * get_gltm() { return &m_gltm; }
	RSC * get_rsc() { return &m_rsc; }
	UINT get_maxreg() const { return m_maxreg; }
	UINT get_paramnum() const { return m_param_num; }

	void update_local();
	void update_maxreg(UINT r) { m_maxreg = MAX(m_maxreg, r); }
	void update_glt_maxreg(GLT * g)
	{
		if (GLT_rg_sz(g) > 1) {
			IS_TRUE0(GLT_rg_sz(g) == RG_PAIR_SZ);
			update_maxreg(GLT_phy(g) + 1);
		} else {
			update_maxreg(GLT_phy(g));
		}
	}
	void update_lt_maxreg(LT * l)
	{
		if (!l->has_allocated()) { return; }
		if (LT_rg_sz(l) > 1) {
			IS_TRUE0(LT_rg_sz(l) == RG_PAIR_SZ);
			update_maxreg(LT_phy(l) + 1);
		} else {
			update_maxreg(LT_phy(l));
		}
	}

	bool verify_interf();
	bool verify_lt_occ();
	bool verify_usable();
	bool verify_rsc();
	bool verify_ltg();
	bool verify_reg(bool check_usable, bool check_alloc);
	bool verify_glt(bool check_alloc);
	bool perform(OPT_CTX & oc);
};
#endif
