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
#ifndef _PRSSAINFO_H_
#define _PRSSAINFO_H_

class IR_SET : public SBITSET {
public:
	IR_SET(SEG_MGR * sm) : SBITSET(sm) {}

	void append(IR const* v)
	{ SBITSET::bunion(IR_id(v)); }

	bool find(IR const* v) const
	{
		IS_TRUE0(v);
		return SBITSET::is_contain(IR_id(v));
	}

	void remove(IR const* v)
	{
		IS_TRUE0(v);
		SBITSET::diff(IR_id(v));
	}
};


//Verisoned Presentation.
//For each version of each prno, VP is unique.
typedef SC<SEG*> * SSAUSE_ITER;

#define SSA_id(ssainfo)			((ssainfo)->id)
#define SSA_def(ssainfo)		((ssainfo)->def_stmt)
#define SSA_uses(ssainfo)		((ssainfo)->use_exp_set)
class SSAINFO {
protected:
	void clean_member()
	{
		id = 0;
		def_stmt = NULL;
	}
public:
	UINT id;
	IR * def_stmt;
	IR_SET use_exp_set;

	SSAINFO(SEG_MGR * sm) : use_exp_set(sm) { clean_member(); }

	inline void clean_du()
	{
		SSA_def(this) = NULL;
		SSA_uses(this).clean();
	}

	inline void init(SEG_MGR * sm)
	{
		clean_member();
		use_exp_set.init(sm);
	}

	void init_no_clean(SEG_MGR * sm) { use_exp_set.init(sm); }

	void destroy() { use_exp_set.destroy(); }
};


//Version PR.
#define VP_prno(v)			((v)->prno)
#define VP_ver(v)			((v)->version)
class VP : public SSAINFO {
public:
	UINT version;
	UINT prno;

	VP(SEG_MGR * sm) : SSAINFO(sm) { clean_member(); }

	inline void clean_member()
	{
		SSAINFO::clean_member();
		prno = 0;
		version = 0;
	}

	void init(SEG_MGR * sm)
	{
		clean_member();
		SSAINFO::init(sm);
	}
};


//Mapping from PRNO to vector of VP.
typedef SVECTOR<SVECTOR<VP*>*> UINT2VPVEC;


//Mapping from PRNO to SSTACK of VP.
typedef SVECTOR<SSTACK<VP*>*> UINT2VPSTACK;
#endif
