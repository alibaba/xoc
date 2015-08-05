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
#ifndef __CFS_MGR_H__
#define __CFS_MGR_H__

//Abstract Tree Type to describe high level control flow struct.
typedef enum _ABS_TYPE {
	ABS_LOOP = 0,
	ABS_IF,
	ABS_BB,
} ABS_TYPE;


/*
SCOP
  |-SCOP_NODE:LOOP
     |-SCOP_NODE:BB
     |-SCOP_NODE:IF
        |-SCOP_NODE:BB
        |-SCOP_NODE:BB
     |-SCOP_NODE:LOOP
     |-...
  |-SCOP_NODE:IF
     |-SCOP_NODE:LOOP
        |-SCOP_NODE:BB
        |-SCOP_NODE:BB
     |-SCOP_NODE:BB
     |-...
   ...

*/
#define ABS_NODE_prev(sn)			((sn)->prev)
#define ABS_NODE_next(sn)			((sn)->next)
#define ABS_NODE_parent(sn)			((sn)->parent)
#define ABS_NODE_loop_body(sn)		((sn)->u2.loop_body)
#define ABS_NODE_true_body(sn)		((sn)->u2.u3.true_body)
#define ABS_NODE_false_body(sn)		((sn)->u2.u3.false_body)
#define ABS_NODE_type(sn)			((sn)->ty)
#define ABS_NODE_if_head(sn)		((sn)->u1.if_head)
#define ABS_NODE_loop_head(sn)		((sn)->u1.loop_head)
#define ABS_NODE_bb(sn)				((sn)->u1.bb)
class ABS_NODE {
public:
	ABS_NODE * prev;
	ABS_NODE * next;
	ABS_NODE * parent;
	ABS_TYPE ty; //TYPE of SCOP NODE
	union {
		ABS_NODE * loop_body;
		struct {
			ABS_NODE * true_body;
			ABS_NODE * false_body;
		} u3;
	} u2;

	union {
		IR_BB * if_head;
		IR_BB * loop_head;
		IR_BB * bb;
	} u1;
};


//
//CFS_INFO
//
#define CFS_INFO_cfs_type(ci)			((ci)->cfs_type)
#define CFS_INFO_ir(ci)					((ci)->u2.ir)
#define CFS_INFO_head(ci)				((ci)->u2.head)
#define CFS_INFO_true_body(ci)			((ci)->u1.if_info.true_body_ir_set)
#define CFS_INFO_false_body(ci)			((ci)->u1.if_info.false_body_ir_set)
#define CFS_INFO_loop_body(ci)			((ci)->u1.loop_info.loop_body_ir_set)
class CFS_INFO {
public:
	IR_TYPE cfs_type;
	union {
		struct {
			BITSET * true_body_ir_set; //TRUE BODY
			BITSET * false_body_ir_set; //FALSE BODY
		} if_info;
		struct {
			BITSET * loop_body_ir_set; //LOOP BODY
		} loop_info;
	} u1;
	union {
		IR * ir;
		IR_BB * head;
	} u2;
};



//
//CFS_MGR, record and rebuild the control flow structure information.
//
class CFS_MGR : public IR_OPT {
protected:
	BITSET_MGR m_bs_mgr; //BITSET manager.
	SMEM_POOL * m_pool;
	SVECTOR<CFS_INFO*> m_map_ir2cfsinfo;
	REGION * m_ru;
	SVECTOR<ABS_NODE*> m_map_bb2abs;

protected:
	void * xmalloc(size_t size)
	{
		void * p = smpool_malloc_h(size, m_pool);
		IS_TRUE0(p);
		memset(p, 0, size);
		return p;
	}
public:
	CFS_MGR(REGION * ru)
	{
		m_ru = ru;
		m_pool = smpool_create_handle(64, MEM_COMM);
	}

	~CFS_MGR() { smpool_free_handle(m_pool); }

	ABS_NODE * construct_abs_loop(
							IN IR_BB * entry,
							IN ABS_NODE * parent,
							IN BITSET * cur_region,
							IN GRAPH & cur_graph,
							IN OUT BITSET & visited);
	ABS_NODE * construct_abs_if(IN IR_BB * entry,
							IN ABS_NODE * parent,
							IN GRAPH & cur_graph,
							IN OUT BITSET & visited);
	ABS_NODE * construct_abs_bb(IN IR_BB * bb, IN ABS_NODE * parent);
	ABS_NODE * construct_abs_tree(
							IN IR_BB * entry,
							IN ABS_NODE * parent,
							IN BITSET * cur_region,
							IN GRAPH & cur_graph,
							IN OUT BITSET & visited);
	ABS_NODE * construct_abstract_cfs();

	void dump_indent(UINT indent);
	void dump_abs_tree(ABS_NODE * an);
	void dump_abs_tree(ABS_NODE * an, UINT indent);

	CFS_INFO * map_ir2cfsinfo(IR * ir);
	ABS_NODE * map_bb2abs(IR_BB const* bb);

	CFS_INFO * new_cfs_info(IR_TYPE irtype);
	ABS_NODE * new_abs_node(ABS_TYPE ty);

	void set_map_ir2cfsinfo(IR * ir, CFS_INFO * ci);
	void set_map_bb2abs(IR_BB const* bb, ABS_NODE * abs);

	void record_ir_stmt(IR * ir, BITSET & irset);

	virtual CHAR const* get_opt_name() const
	{ return "Control Flow Structure MGR"; }

	OPT_TYPE get_opt_type() const { return OPT_CFS_MGR; }
};
#endif
