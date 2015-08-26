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
#ifndef __AI_H__
#define __AI_H__

namespace xoc {

/* How to use AttachInfo?
1. Allocate AttachInfo from Region.
2. Construct your data structure to be attached.
3. Set the AttachInfo type and the data structure.

e.g:
	IR * ir = ...; Given IR.
	IR_ai(ir) = region->newAI();
	Dbx * dbx = getDbx();
	IR_ai(ir)->set(AI_DBX, (BaseAttachInfo*)dbx);
*/

//Attach Info Type.
typedef enum _AI_TYPE {
	AI_UNDEF = 0,
	AI_DBX, //Debug Info
	AI_PROF, //Profile Info
	AI_TBAA, //Type Based AA
	AI_EH_LABEL, //Record a list of Labels.
	AI_USER_DEF, //User Defined
} AI_TYPE;


class BaseAttachInfo {
public:
	AI_TYPE type;

	explicit BaseAttachInfo(AI_TYPE t) { init(t); }
	COPY_CONSTRUCTOR(BaseAttachInfo);

	void init(AI_TYPE t) { type = t; }
};


class AttachInfo {
public:
	SimpleVec<BaseAttachInfo*, 1> cont;

public:
	AttachInfo() { init(); }
	COPY_CONSTRUCTOR(AttachInfo);

	void init()
	{
		if (cont.is_init()) { return; }
		cont.init();
	}

	INT is_init() const { return cont.is_init(); }

	void destroy() { cont.destroy(); }
	void destroy_vec() { cont.destroy_vec(); }

	void copy(AttachInfo const* ai)
	{
		ASSERT0(ai);
		if (!ai->is_init()) { return; }
		cont.copy(ai->cont);
	}

	void set(AI_TYPE type, BaseAttachInfo * c)
	{
		INT emptyslot = -1;
		if (!cont.is_init()) {
			if (c == NULL) { return; }
			cont.init();
		}

		UINT i;
		for (i = 0; i < cont.get_capacity(); i++) {
			BaseAttachInfo * ac = cont.get(i);
			if (ac == NULL) { emptyslot = (INT)i; }
			if (ac->type != type) { continue; }
			cont.set(i, c);
			return;
		}

		if (c != NULL) {
			if (emptyslot != -1) {
				cont.set((UINT)emptyslot, c);
			} else {
				cont.set(i, c);
			}
		}
	}

	BaseAttachInfo const* get(AI_TYPE type) const
	{
		if (!cont.is_init()) { return NULL; }
		for (UINT i = 0; i < cont.get_capacity(); i++) {
			BaseAttachInfo * ac = cont.get(i);
			if (ac != NULL && ac->type == type) {
				return ac;
			}
		}
		return NULL;
	}
};


//Exception Handler Labels.
class EHLabelAttachInfo : public BaseAttachInfo {
public:
	SList<LabelInfo*> labels; //record a list of Labels.
	
public:
	EHLabelAttachInfo(SMemPool * pool = NULL) : BaseAttachInfo(AI_EH_LABEL)
	{ init(pool); }
	COPY_CONSTRUCTOR(EHLabelAttachInfo);

	void init(SMemPool * pool)
	{
		BaseAttachInfo::init(AI_EH_LABEL);
		labels.set_pool(pool);
	}

	SList<LabelInfo*> const& get_labels() const { return labels; }
};


class DbxAttachInfo : public BaseAttachInfo {
public:
	Dbx dbx; //record debug info.

	DbxAttachInfo() : BaseAttachInfo(AI_DBX) { init(); }
	COPY_CONSTRUCTOR(DbxAttachInfo);

	void init()
	{
		BaseAttachInfo::init(AI_DBX);
		dbx.clean();
	}
};


class ProfileAttachInfo : public BaseAttachInfo {
public:
	SYM const* tag;

	//truebr freq, falsebr freq.
	INT * data;

	ProfileAttachInfo() : BaseAttachInfo(AI_DBX) { init(); }
	COPY_CONSTRUCTOR(ProfileAttachInfo);

	void init()
	{
		BaseAttachInfo::init(AI_PROF);
		tag = NULL;
		data = NULL;
	}
};


class TbaaAttachInfo : public BaseAttachInfo {
public:
	Type const* type;

	TbaaAttachInfo() : BaseAttachInfo(AI_TBAA) {}
	COPY_CONSTRUCTOR(TbaaAttachInfo);
};

} //namespace xoc
#endif
