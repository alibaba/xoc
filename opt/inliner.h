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
#ifndef _INLINER_H_
#define _INLINER_H_
#define INLINFO_need_el(i)		((i)->need_el)
#define INLINFO_has_ret(i)		((i)->has_ret)
class INLINE_INFO {
public:
	BYTE need_el:1;
	BYTE has_ret:1;
};


class INLINER {
protected:
	RegionMgr * m_ru_mgr;
	SMemPool * m_pool;
	CallGraph * m_callg;
	TMap<Region*, INLINE_INFO*> m_ru2inl;

	void ck_ru(IN Region * ru, OUT bool & need_el, OUT bool & has_ret) const;

	void * xmalloc(UINT size)
	{
		void * p = smpoolMalloc(size, m_pool);
		ASSERT0(p);
		memset(p, 0, size);
		return p;
	}

	INLINE_INFO * map_ru2ii(Region * ru, bool alloc)
	{
		INLINE_INFO * ii = m_ru2inl.get(ru);
		if (ii == NULL && alloc) {
			ii = (INLINE_INFO*)xmalloc(sizeof(INLINE_INFO));
			m_ru2inl.set(ru, ii);
		}
		return ii;
	}
public:
	INLINER(RegionMgr * ru_mgr)
	{
		m_ru_mgr = ru_mgr;
		m_callg = ru_mgr->get_callg();
		m_pool = smpoolCreate(16, MEM_COMM);
	}
	virtual ~INLINER() { smpoolDelete(m_pool); }

	bool can_be_cand(Region * ru);

	bool do_inline_c(Region * caller, Region * callee);
	void do_inline(Region * cand);

	inline bool is_call_site(IR * call, Region * ru);

	virtual CHAR const* get_pass_name() const { return "INLINER"; }

	IR * replace_return_c(Region * caller, IR * caller_call,
						  IR * new_irs, LabelInfo * el);
	IR * replace_return(Region * caller, IR * caller_call,
						IR * new_irs, INLINE_INFO * ii);
	virtual bool perform(OptCTX & oc);
};
#endif

