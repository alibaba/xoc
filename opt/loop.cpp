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
#include "cominc.h"

/* Find the bb that is the start of the unqiue backedge of loop.
   BB1: loop start bb
   BB2: body start bb
   BB3: goto loop start bb

   BB2 is the loop header fallthrough bb. */
bool find_loop_header_two_succ_bb(LI<IR_BB> const* li, IR_CFG * cfg,
								UINT * succ1, UINT * succ2)
{
	IS_TRUE0(li && cfg && succ1 && succ2);
	IR_BB * head = LI_loop_head(li);

	VERTEX * headvex = cfg->get_vertex(IR_BB_id(head));
	if (cfg->get_out_degree(headvex) != 2) {
		//Not natural loop.
		return false;
	}

	EDGE_C const* ec = VERTEX_out_list(headvex);
	IS_TRUE0(ec && EC_next(ec));

	*succ1 = VERTEX_id(EDGE_to(EC_edge(ec)));
	*succ2 = VERTEX_id(EDGE_to(EC_edge(EC_next(ec))));
	return true;
}


/* Find the bb that is the start of the unqiue backedge of loop.
   BB1: loop start bb
   BB2: body
   BB3: goto loop start bb

   BB3 is the backedge start bb. */
IR_BB * find_single_backedge_start_bb(LI<IR_BB> const* li, IR_CFG * cfg)
{
	IS_TRUE0(li && cfg);
	IR_BB * head = LI_loop_head(li);

	UINT backedgebbid = 0;
	UINT backedgecount = 0;
	EDGE_C const* ec = VERTEX_in_list(cfg->get_vertex(IR_BB_id(head)));
	while (ec != NULL) {
		backedgecount++;
		UINT pred = VERTEX_id(EDGE_from(EC_edge(ec)));
		if (li->inside_loop(pred)) {
			backedgebbid = pred;
		}
		ec = EC_next(ec);
	}
	IS_TRUE0(backedgebbid > 0 && cfg->get_bb(backedgebbid));
	if (backedgecount > 2) {
		//There are multiple backedges.
		return NULL;
	}
	return cfg->get_bb(backedgebbid);
}


/* Find preheader BB. If it does not exist, insert one before loop 'li'.

'insert_bb': return true if this function insert a new bb before loop,
	otherwise return false.

'force': force to insert preheader BB whatever it has exist.
	Return the new BB if insertion is successful.

Note if we find the preheader, the last IR of it may be call.
So if you are going to insert IR at the tail of preheader, the best is
force to insert a new bb. */
IR_BB * find_and_insert_prehead(LI<IR_BB> const* li, REGION * ru,
								OUT bool & insert_bb,
								bool force)
{
	IS_TRUE0(li && ru);
	insert_bb = false;
	IR_CFG * cfg = ru->get_cfg();
	IR_BB_LIST * bblst = ru->get_bb_list();
	IR_BB * head = LI_loop_head(li);

	C<IR_BB*> * bbholder = NULL;
	bblst->find(head, &bbholder);
	IS_TRUE0(bbholder);
	C<IR_BB*> * tt = bbholder;
	IR_BB * prev = bblst->get_prev(&tt);

	//Find appropriate BB to be prehead.
	bool find_appropriate_prev_bb = false;
	EDGE_C const* ec = VERTEX_in_list(cfg->get_vertex(IR_BB_id(head)));
	while (ec != NULL) {
		UINT pred = VERTEX_id(EDGE_from(EC_edge(ec)));
		if (pred == IR_BB_id(prev)) {
			find_appropriate_prev_bb = true;
			break;
		}
		ec = EC_next(ec);
	}

	if (!force && find_appropriate_prev_bb) { return prev; }

	LIST<IR_BB*> preds;
	cfg->get_preds(preds, head);
	insert_bb = true;
	IR_BB * newbb = ru->new_bb();
	bblst->insert_before(newbb, bbholder);
	BITSET * loop_body = LI_bb_set(li);
	for (IR_BB * p = preds.get_head(); p != NULL; p = preds.get_next()) {
		if (loop_body->is_contain(IR_BB_id(p))) {
			continue;
		}
		cfg->add_bb(newbb);
		cfg->insert_vertex_between(IR_BB_id(p), IR_BB_id(head),
								   IR_BB_id(newbb));
		IR_BB_is_fallthrough(newbb) = 1;
	}
	return newbb;
}
