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
#ifndef _IR_BB_H_
#define _IR_BB_H_

class IR_BB;

//
//START BBIR_LIST
//
//NOTE: Overload funtion when inserting or remving new IR.
class BBIR_LIST : public ELIST<IR*, IR2HOLDER> {
	IR_BB * m_bb;
public:
	BBIR_LIST() { m_bb = NULL; }

	inline C<IR*> * append_head(IR * ir)
	{
		if (ir == NULL) { return NULL; }
		IS_TRUE0(m_bb != NULL);
		ir->set_bb(m_bb);
		return ELIST<IR*, IR2HOLDER>::append_head(ir);
	}

	inline C<IR*> * append_tail(IR * ir)
	{
		if (ir == NULL) { return NULL; }
		IS_TRUE0(m_bb != NULL);
		ir->set_bb(m_bb);
		return ELIST<IR*, IR2HOLDER>::append_tail(ir);
	}

	//Insert ir prior to cond_br, uncond_br, call, return.
	C<IR*> * append_tail_ex(IR * ir)
	{
		if (ir == NULL) { return NULL; }
		C<IR*> * ct;
		for (IR * tir = LIST<IR*>::get_tail(&ct);
			 tir != NULL; tir = LIST<IR*>::get_prev(&ct)) {
			if (!tir->is_uncond_br() &&
				!tir->is_cond_br() &&
				!tir->is_multicond_br() &&
				!tir->is_return() &&
				!tir->is_call()) {
				break;
			}
		}
		IS_TRUE0(m_bb != NULL);
		ir->set_bb(m_bb);
		if (ct == NULL) {
			return ELIST<IR*, IR2HOLDER>::append_head(ir);
		} else {
			return ELIST<IR*, IR2HOLDER>::insert_after(ir, ct);
		}
	}

	//Count up memory size of BBIR_LIST
	UINT count_mem() const
	{
		return sizeof(m_bb) + ((ELIST<IR*, IR2HOLDER>*)this)->count_mem();
	}

	//Insert 'ir' before 'marker'.
	inline C<IR*> * insert_before(IN IR * ir, IN IR * marker)
	{
		if (ir == NULL) { return NULL; }
		IS_TRUE0(marker != NULL);
		IS_TRUE0(m_bb != NULL);
		ir->set_bb(m_bb);
		return ELIST<IR*, IR2HOLDER>::insert_before(ir, marker);
	}

	//Insert 'ir' before 'marker'. marker will be modified.
	inline C<IR*> * insert_before(IN IR * ir, IN C<IR*> * marker)
	{
		if (ir == NULL) { return NULL; }
		IS_TRUE0(marker != NULL);
		IS_TRUE0(m_bb != NULL);
		ir->set_bb(m_bb);
		return ELIST<IR*, IR2HOLDER>::insert_before(ir, marker);
	}

	//Insert 'ir' after 'marker'.
	inline C<IR*> * insert_after(IR * ir, IR * marker)
	{
		if (ir == NULL) { return NULL; }
		IS_TRUE0(marker != NULL);
		IS_TRUE0(m_bb != NULL);
		ir->set_bb(m_bb);
		return ELIST<IR*, IR2HOLDER>::insert_after(ir, marker);
	}

	//Insert 'ir' after 'marker'.
	inline C<IR*> * insert_after(IR * ir, IN C<IR*> * marker)
	{
		if (ir == NULL) { return NULL; }
		IS_TRUE0(marker != NULL);
		IS_TRUE0(m_bb != NULL);
		ir->set_bb(m_bb);
		return ELIST<IR*, IR2HOLDER>::insert_after(ir, marker);
	}

	//Remove ir that hold by 'holder'.
	inline IR * remove(IN C<IR*> * holder)
	{
		if (holder == NULL) return NULL;
		C_val(holder)->set_bb(NULL);
		return ELIST<IR*, IR2HOLDER>::remove(holder);
	}

	//Remove ir.
	inline IR * remove(IN IR * ir)
	{
		if (ir == NULL) return NULL;
		ir->set_bb(NULL);
		return ELIST<IR*, IR2HOLDER>::remove(ir);
	}

	inline void set_bb(IR_BB * bb) { m_bb = bb; }
};
//END BBIR_LIST


//
//START IR_BB
//
#define MAX_BB_KIDS_NUM 	2

#define IR_BB_rpo(b)				((b)->rpo)
#define IR_BB_id(b)					((b)->id)
#define IR_BB_lab_list(b)			((b)->lab_list)
#define IR_BB_ir_list(b)			((b)->ir_list)
#define IR_BB_first_ir(b)			((b)->ir_list.get_head())
#define IR_BB_next_ir(b)			((b)->ir_list.get_next())
#define IR_BB_prev_ir(b)			((b)->ir_list.get_prev())
#define IR_BB_last_ir(b)			((b)->ir_list.get_tail())
#define IR_BB_ir_num(b)				((b)->ir_list.get_elem_count())
#define IR_BB_is_entry(b)			((b)->u1.s1.is_entry)
#define IR_BB_is_exit(b)			((b)->u1.s1.is_exit)
#define IR_BB_is_fallthrough(b)		((b)->u1.s1.is_fallthrough)
#define IR_BB_is_target(b)			((b)->u1.s1.is_target)
#define IR_BB_is_catch_start(b)		((b)->u1.s1.is_catch_start)
#define IR_BB_is_unreachable(b)		((b)->u1.s1.is_unreachable)
#define IR_BB_ru(b)					((b)->m_ru)
class IR_BB {
public:
	UINT id; //BB's id
	INT rpo; //reverse post order
	BBIR_LIST ir_list; //IR list
	LIST<LABEL_INFO*> lab_list; //Record labels attached on BB
	REGION * m_ru;
	union {
		struct {
			BYTE is_entry:1; //bb is entry of the region.
			BYTE is_exit:1; //bb is exit of the region.
			BYTE is_fallthrough:1; //bb has a fall through successor.
			BYTE is_target:1; //bb is branch target.
			BYTE is_catch_start:1; //bb is catch start.
			BYTE is_unreachable:1; //bb is unreachable.
		} s1;
		BYTE u1b1;
	} u1;

	IR_BB()
	{
		ir_list.set_bb(this);
		id = 0;
		m_ru = NULL;
		u1.u1b1 = 0;
		rpo = -1;
	}

	~IR_BB()
	{
		/*
		If BB destructed in ~IR_BB_MGR(), then it is dispensable to free them.
		Or the ir_list must be clean before the delete BB.
		for (IR * ir = ir_list.get_head(); ir != NULL; ir = ir_list.get_next()) {
			m_ru->free_irs(ir);
		}
		*/
	}

	inline void add_label(LABEL_INFO * li)
	{
		IS_TRUE0(li != NULL);
		if (!lab_list.find(li)) {
			if (LABEL_INFO_is_catch_start(li)) {
				IR_BB_is_catch_start(this) = true;
			}
			if (LABEL_INFO_is_unreachable(li)) {
				IR_BB_is_unreachable(this) = true;

			}
			lab_list.append_tail(li);
		}
	}

	UINT count_mem() const;

	void dump();

	REGION * get_ru() { return m_ru; }

	/* For some aggressive optimized purposes, call node is not looked as
	boundary of basic block.
	So we must bottom-up go through whole bb to find call. */
	inline bool is_bb_has_call() const
	{
		BBIR_LIST * irlst = const_cast<BBIR_LIST*>(&IR_BB_ir_list(this));
		for (IR * ir = irlst->get_tail();
			 ir != NULL; ir = irlst->get_prev()) {
			if (ir->is_call()) {
				return true;
			}
		}
		return false;
	}

	inline bool is_bb_has_return() const
	{
		BBIR_LIST * irlst = const_cast<BBIR_LIST*>(&IR_BB_ir_list(this));
		for (IR * ir = irlst->get_tail();
			 ir != NULL; ir = irlst->get_prev()) {
			if (ir->is_return()) {
				return true;
			}
		}
		return false;
	}

	//Return true if BB is a CATCH block entry.
	inline bool is_exp_handling() const
	{
		bool r = IR_BB_is_catch_start(this);
		#ifdef _DEBUG_
		bool find = false;
		IR_BB * pthis = const_cast<IR_BB*>(this);
		for (LABEL_INFO const* li = IR_BB_lab_list(pthis).get_head();
			 li != NULL; li = IR_BB_lab_list(pthis).get_next()) {
			if (LABEL_INFO_is_catch_start(li)) {
				find = true;
				break;
			}
		}
		IS_TRUE0(r == find);
		#endif
		return r;
	}


	//Return true if BB is unreachable.
	inline bool is_unreachable() const
	{
		bool r = IR_BB_is_unreachable(this);
		#ifdef _DEBUG_
		bool find = false;
		IR_BB * pthis = const_cast<IR_BB*>(this);
		for (LABEL_INFO const* li = IR_BB_lab_list(pthis).get_head();
			 li != NULL; li = IR_BB_lab_list(pthis).get_next()) {
			if (LABEL_INFO_is_unreachable(li)) {
				find = true;
				break;
			}
		}
		IS_TRUE0(r == find);
		#endif
		return r;
	}

	//Could ir be looked as a boundary stmt of basic block?
	inline bool is_bb_boundary(IR * ir)
	{ return (is_bb_up_boundary(ir) || is_bb_down_boundary(ir)); }

	//Could ir be looked as a first stmt in basic block?
	inline bool is_bb_up_boundary(IR const* ir) const
	{
		IS_TRUE(ir->is_stmt_in_bb() || ir->is_lab(), ("illegal stmt in bb"));
		return ir->is_lab();
	}
	bool is_bb_down_boundary(IR * ir);

	//Is bb containing such label carried by 'lir'.
	inline bool is_bb_has_label(LABEL_INFO const* lab)
	{
		for (LABEL_INFO const* li = IR_BB_lab_list(this).get_head();
			 li != NULL; li = IR_BB_lab_list(this).get_next()) {
			if (is_same_label(li, lab)) {
				return true;
			}
		}
		return false;
	}

	inline bool is_attach_dedicated_lab()
	{
		for (LABEL_INFO * li = IR_BB_lab_list(this).get_head();
			 li != NULL; li = IR_BB_lab_list(this).get_next()) {
			if (LABEL_INFO_is_catch_start(li) ||
				LABEL_INFO_is_try_start(li) ||
				LABEL_INFO_is_try_end(li) ||
				LABEL_INFO_is_pragma(li)) {
				return true;
			}
		}
		return false;
	}

	//Return true if ir1 dominate ir2 in current bb.
	//Function will modify the IR container of bb.
	//'is_strict': true if ir1 should not equal to ir2.
	inline bool is_dom(IR const* ir1, IR const* ir2, bool is_strict)
	{
		IS_TRUE0(ir1->is_stmt() && ir2->is_stmt() &&
				 ir1->get_bb() == this && ir2->get_bb() == this);
		if (is_strict && ir1 == ir2) {
			return false;
		}
		IR * ir;
		for (ir = IR_BB_first_ir(this); ir != NULL; ir = IR_BB_next_ir(this)) {
			if (ir == ir1) {
				return true;
			}
			if (ir == ir2) {
				return false;
			}
		}
		IS_TRUE0(ir == NULL);
		return false;
	}

	//Union all labels of 'src' to current bb.
	inline void union_labs(IR_BB * src)
	{
		for (LABEL_INFO * li = IR_BB_lab_list(src).get_head();
			 li != NULL; li = IR_BB_lab_list(src).get_next()) {
			if (!lab_list.find(li)) {
				lab_list.append_head(li);
				if (LABEL_INFO_is_catch_start(li)) {
					IR_BB_is_catch_start(this) = true;
				}
				if (LABEL_INFO_is_unreachable(li)) {
					IR_BB_is_unreachable(this) = true;
				}
			}
		}
	}

	void verify();
};
//END IR_BB



//
//START IR_BB_MGR
//
class IR_BB_MGR {
protected:
	IR_BB_LIST m_bbs_list;
	UINT m_bb_count; //counter of IR_BB.

public:
	IR_BB_MGR() { m_bb_count = 1; }
	~IR_BB_MGR()
	{
		for (IR_BB * bb = m_bbs_list.get_head();
			 bb != NULL; bb = m_bbs_list.get_next()) {
			delete bb;
		}
	}

	inline IR_BB * new_bb()
	{
		IR_BB * bb = new IR_BB();
		IR_BB_id(bb) = m_bb_count++;
		m_bbs_list.append_tail(bb);
		return bb;
	}

	UINT count_mem()
	{
		UINT count = 0;
		for (IR_BB * bb = m_bbs_list.get_head();
			 bb != NULL; bb = m_bbs_list.get_next()) {
			count += bb->count_mem();
		}
		return count;
	}
};
//END IR_BB_MGR


//Exported Functions
extern void dump_bbs(IR_BB_LIST * bbl, CHAR const* name = NULL);
extern void dump_bbs_order(IR_BB_LIST * bbl);
#endif

