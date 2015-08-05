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
#ifndef __PRDF_H__
#define __PRDF_H__

class VAR2PR : public TMAP<UINT, UINT> {
};

class PRDF : public IR_OPT {
	REGION * m_ru;
	MD_SYS * m_md_sys;
	VAR2PR * m_var2pr;
	IR_CFG * m_cfg;
	SDBITSET_MGR m_sbs_mgr;
	SVECTOR<SBITSETC*> m_def;
	SVECTOR<SBITSETC*> m_use;
	SVECTOR<SBITSETC*> m_livein;
	SVECTOR<SBITSETC*> m_liveout;
	BYTE m_handle_may:1; //true if consider maydef/mayuse info.

protected:
	SBITSETC * get_def(UINT bbid)
	{
		SBITSETC * set = m_def.get(bbid);
		if (set == NULL) {
			set = m_sbs_mgr.create_sbitsetc();
			m_def.set(bbid, set);
		}
		return set;
	}

	SBITSETC * get_use(UINT bbid)
	{
		SBITSETC * set = m_use.get(bbid);
		if (set == NULL) {
			set = m_sbs_mgr.create_sbitsetc();
			m_use.set(bbid, set);
		}
		return set;
	}

	inline void process_opnd(IR const* exp, LIST<IR const*> & lst,
							SBITSETC * use, SBITSETC * gen);
	inline void process_may(IR const* pr, SBITSETC * gen,
							SBITSETC * use, bool is_lhs);

public:
	PRDF(REGION * ru)
	{
		m_ru = ru;
		m_md_sys = ru->get_md_sys();
		m_cfg = ru->get_cfg();
		m_var2pr = NULL;
		m_handle_may = false;
	}

	~PRDF()
	{
		for (INT i = 0; i <= m_def.get_last_idx(); i++) {
			SBITSETC * bs = m_def.get(i);
			if (bs != NULL) {
				m_sbs_mgr.free_sbitsetc(bs);
			}
		}

		for (INT i = 0; i <= m_use.get_last_idx(); i++) {
			SBITSETC * bs = m_use.get(i);
			if (bs != NULL) {
				m_sbs_mgr.free_sbitsetc(bs);
			}
		}

		for (INT i = 0; i <= m_livein.get_last_idx(); i++) {
			SBITSETC * bs = m_livein.get(i);
			if (bs != NULL) {
				m_sbs_mgr.free_sbitsetc(bs);
			}
		}

		for (INT i = 0; i <= m_liveout.get_last_idx(); i++) {
			SBITSETC * bs = m_liveout.get(i);
			if (bs != NULL) {
				m_sbs_mgr.free_sbitsetc(bs);
			}
		}
	}

	void compute_local(IR_BB * bb, LIST<IR const*> & lst);
	void compute_global();

	UINT count_mem() const
	{
		UINT count = m_sbs_mgr.count_mem();
		count += m_def.count_mem();
		count += m_use.count_mem();
		count += m_livein.count_mem();
		count += m_liveout.count_mem();

		for (INT i = 0; i <= m_def.get_last_idx(); i++) {
			SBITSETC * bs = m_def.get(i);
			if (bs != NULL) {
				count += bs->count_mem();
			}
		}

		for (INT i = 0; i <= m_use.get_last_idx(); i++) {
			SBITSETC * bs = m_use.get(i);
			if (bs != NULL) {
				count += bs->count_mem();
			}
		}

		for (INT i = 0; i <= m_livein.get_last_idx(); i++) {
			SBITSETC * bs = m_livein.get(i);
			if (bs != NULL) {
				count += bs->count_mem();
			}
		}

		for (INT i = 0; i <= m_liveout.get_last_idx(); i++) {
			SBITSETC * bs = m_liveout.get(i);
			if (bs != NULL) {
				count += bs->count_mem();
			}
		}

		return count;
	}

	void dump();

	virtual CHAR const* get_opt_name() const { return "PRDF"; }
	OPT_TYPE get_opt_type() const { return OPT_PRDF; }

	SDBITSET_MGR & get_sbs_mgr() { return m_sbs_mgr; }

	SBITSETC const* get_livein_c(UINT bbid) const
	{ return m_livein.get(bbid); }

	SBITSETC * get_livein(UINT bbid)
	{
		SBITSETC * x = m_livein.get(bbid);
		if (x == NULL) {
			x = m_sbs_mgr.create_sbitsetc();
			m_livein.set(bbid, x);
		}
		return x;
	}

	SBITSETC const* get_liveout_c(UINT bbid) const
	{ return m_liveout.get(bbid); }

	SBITSETC * get_liveout(UINT bbid)
	{
		SBITSETC * x = m_liveout.get(bbid);
		if (x == NULL) {
			x = m_sbs_mgr.create_sbitsetc();
			m_liveout.set(bbid, x);
		}
		return x;
	}

	void set_handle_may(bool is_handle_may) { m_handle_may = is_handle_may; }
	void set_var2pr(VAR2PR * v2p) { m_var2pr = v2p; }

	inline void set_pr_to_liveout(IR_BB * bb, UINT prno)
	{ get_liveout(IR_BB_id(bb))->bunion(prno, m_sbs_mgr); }

	virtual bool perform(OPT_CTX & oc);
};
#endif
