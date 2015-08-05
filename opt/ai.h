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

//IR Attach Info.
typedef enum _IRAI_TYPE {
	IRAI_UNDEF = 0,
	IRAI_DBX, //Debug Info
	IRAI_PROF, //Profiling Info
	IRAI_TBAA, //Type Based AA
	IRAI_USER_DEF, //User Defined
} IRAI_TYPE;


//AI content
class AI_BASE {
public:
	IRAI_TYPE type;

	AI_BASE(IRAI_TYPE t) { init(t); }
	void init(IRAI_TYPE t) { type = t; }
};


class IR_AI {
public:
	SSVEC<AI_BASE*, 1> cont;

	IR_AI() { init(); }

	void init()
	{
		if (cont.is_init()) { return; }
		cont.init();
	}

	INT is_init() const { return cont.is_init(); }

	void destroy() { cont.destroy(); }
	void destroy_vec() { cont.destroy_vec(); }

	void copy(IR_AI const* ai)
	{
		IS_TRUE0(ai);
		if (!ai->is_init()) { return; }
		cont.copy(ai->cont);
	}

	void set(IRAI_TYPE type, AI_BASE * c)
	{
		INT emptyslot = -1;
		if (!cont.is_init()) {
			if (c == NULL) { return; }
			cont.init();
		}
		UINT i;
		for (i = 0; i < cont.get_size(); i++) {
			AI_BASE * ac = cont.get(i);
			if (ac == NULL) { emptyslot = i; }
			if (ac->type != type) { continue; }
			cont.set(i, c);
			return;
		}

		if (c != NULL) {
			if (emptyslot != -1) {
				cont.set(emptyslot, c);
			} else {
				cont.set(i, c);
			}
		}
	}

	AI_BASE * get(IRAI_TYPE type)
	{
		if (!cont.is_init()) { return NULL; }
		for (UINT i = 0; i < cont.get_size(); i++) {
			AI_BASE * ac = cont.get(i);
			if (ac != NULL && ac->type == type) {
				return ac;
			}
		}
		return NULL;
	}
};


class DBX_AI : public AI_BASE {
public:
	DBX dbx; //record debug info.

	DBX_AI() : AI_BASE(IRAI_DBX) { init(); }
	void init()
	{
		AI_BASE::init(IRAI_DBX);
		dbx.clean();
	}
};


class PROF_AI : public AI_BASE {
public:
	SYM const* tag;

	//truebr freq, falsebr freq.
	INT * data;

	PROF_AI() : AI_BASE(IRAI_DBX) { init(); }

	void init()
	{
		AI_BASE::init(IRAI_PROF);
		tag = NULL;
		data = NULL;
	}
};


class TBAA_AI : public AI_BASE {
public:
	UINT tyid; //tyid start from 1.

	TBAA_AI() : AI_BASE(IRAI_TBAA) {}
};
#endif
