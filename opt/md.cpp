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

//
//START MDID
//
void MDID2MD::dump() const
{
	if (g_tfile == NULL) { return; }
	for (INT i = 0; i <= get_last_idx(); i++) {
		MD * md = SVECTOR<MD*>::get(i);
		if (md == NULL) { continue; }
		IS_TRUE0(MD_id(md) == (UINT)i);
		fprintf(g_tfile, "%d,", i);
	}
	fflush(g_tfile);
}
//END MDID


//
//START MD
//
//Check if 'this' cover 'm'.
bool MD::is_cover(MD const* m) const
{
	IS_TRUE0(m && this != m);
	if (MD_base(this) != MD_base(m)) {
		return false;
	}
	if (MD_ty(this) == MD_UNBOUND) {
		return true;
	}
	if (MD_ty(m) == MD_UNBOUND ||
		MD_ty(this) == MD_RANGE ||
		MD_ty(m) == MD_RANGE) {
		return false;
	}
	if ((MD_ofst(this) <= MD_ofst(m)) &&
		(MD_ofst(this) + MD_size(this) >= MD_ofst(m) + MD_size(m))) {
		return true;
	}
	return false;
}


//Check overlapping.
bool MD::is_overlap(MD const* m) const
{
	IS_TRUE0(m && this != m);
	if (MD_id(m) == MD_ALL_MEM || MD_id(this) == MD_ALL_MEM) {
		return true;
	}

	//if (MD_id(m) == MD_HEAP_MEM && MD_id(this) == MD_ALL_MEM)
	//{	return true; }

	if (MD_base(m) != MD_base(this)) { return false; }

	if (MD_ty(m) == MD_UNBOUND || MD_ty(this) == MD_UNBOUND) {
		return true;
	}

	return !(((MD_ofst(m) + MD_size(m)) <= MD_ofst(this)) ||
			 ((MD_ofst(this) + MD_size(this)) <= MD_ofst(m)));
}


//Caller should make sure 'buf' is large enough.
CHAR * MD::dump(CHAR * buf, UINT bufl) const
{
	IS_TRUE0(buf);
	CHAR * tb = buf;
	sprintf(buf, "MD%d -- base:", MD_id(this));
	buf += strlen(buf);
	IS_TRUE0(MD_base(this) != NULL);
	MD_base(this)->dump(buf);
	buf += strlen(buf);
	INT ofst = MD_ofst(this);
	if (MD_ty(this) == MD_EXACT) {
		sprintf(buf, " -- ofst:%d -- size:%u", ofst, MD_size(this));
	} else if (MD_ty(this) == MD_RANGE) {
		sprintf(buf, " -- start:%d -- end:%u", ofst, ofst + MD_size(this));
		strcat(buf, " -- range");
	} else {
		sprintf(buf, " -- ofst:unbound");
	}
	IS_TRUE(strlen(tb) < bufl, ("dump buf overflow!"));
	return tb;
}


void MD::dump() const
{
	if (g_tfile == NULL) return;
	//CHAR buf[MAX_BUF_LEN];
	CHAR buf[4096];
	buf[0] = 0;
	fprintf(g_tfile, "\n%s", dump(buf, 4096));
	fflush(g_tfile);
}
//END MD


//
//START MD_SET
//
//Get unique MD that is not fake memory object,
//but its offset might be invalid.
MD * MD_SET::get_effect_md(MD_SYS * ms) const
{
	IS_TRUE0(ms);
	if (get_elem_count() != 1) {
		return NULL;
	}
	MD * md = ms->get_md(get_first());
	IS_TRUE0(md != NULL);
	if (VAR_is_fake(MD_base(md))) {
		return NULL;
	}
	return md;
}


void MD_SET::bunion(UINT mdid)
{
	if (mdid == MD_ALL_MEM) {
		clean();
		BITSET::bunion(MD_ALL_MEM);
		return;
	}
	if (BITSET::is_contain(MD_ALL_MEM)) {
		IS_TRUE0(BITSET::get_elem_count() == 1);
		return;
	}
	BITSET::bunion(mdid);
}


void MD_SET::bunion(MD const* md)
{
	IS_TRUE0(md && (MD_id(md) != 0));
	if (MD_id(md) == MD_ALL_MEM) {
		clean();
		BITSET::bunion(MD_ALL_MEM);
		return;
	}
	if (BITSET::is_contain(MD_ALL_MEM)) {
		IS_TRUE0(BITSET::get_elem_count() == 1);
		return;
	}
	BITSET::bunion(MD_id(md));
}


//Return true current set is equivalent to mds, and every element
//in set is exact.
bool MD_SET::is_exact_equal(IN MD_SET const& mds, MD_SYS * ms) const
{
	IS_TRUE0(ms);
	UINT count = 0;
	INT md1 = -1;
	for (INT i = get_first(); i != -1; i = get_next(i)) {
		if (!ms->get_md(i)->is_exact()) {
			return false;
		}
		md1 = i;
		count++;
		if (count > 1) {
			//If the number of MD more than one, the Alias analysis
			//might compute the consevative solution.
			return false;
		}
	}

	count = 0;
	INT md2 = -1;
	for (INT i = mds.get_first(); i != -1; i = get_next(i)) {
		if (!ms->get_md(i)->is_exact()) {
			return false;
		}
		md2 = i;
		count++;
		if (count > 1) {
			//If the number of MD more than one,
			//the Alias analysis might compute the consevative solution.
			return false;
		}
	}
	return md1 == md2;
}


bool MD_SET::is_contain_only_exact_and_str(MD_SYS * ms) const
{
	IS_TRUE0(ms);
	for (INT i = get_first(); i != -1; i = get_next(i)) {
		MD * tmd = ms->get_md(i);
		IS_TRUE0(tmd != NULL);
		if (!tmd->is_exact() && !VAR_is_str(MD_base(tmd))) {
			return false;
		}
	}
	return true;
}


bool MD_SET::is_contain_inexact(MD_SYS * ms) const
{
	IS_TRUE0(ms);
	for (INT i = get_first(); i != -1; i = get_next(i)) {
		MD * tmd = ms->get_md(i);
		IS_TRUE0(tmd != NULL);
		if (MD_id(tmd) == MD_ALL_MEM) {
			return true;
		}
		if (!tmd->is_exact()) {
			return true;
		}
	}
	return false;
}


void MD_SET::bunion(MD_SET const& mds)
{
	if (this == &mds) { return; }
	IS_TRUE0(!((BITSET&)mds).is_contain(0));
	if (BITSET::is_contain(MD_ALL_MEM)) {
		return;
	}
	if (((BITSET const&)mds).is_contain(MD_ALL_MEM)) {
		clean();
		BITSET::bunion(MD_ALL_MEM);
		return;
	}
	BITSET::bunion((BITSET&)mds);
}


void MD_SET::dump(MD_SYS * ms, bool detail) const
{
	if (g_tfile == NULL) return;
	IS_TRUE0(ms);
	for (INT i = get_first(); i >= 0;) {
		fprintf(g_tfile, "MD%d", i);
		i = get_next(i);
		if (i >= 0) {
			fprintf(g_tfile, ",");
		}
	}
	if (detail) {
		for (INT i = get_first(); i != -1; i = get_next(i)) {
			MD const* md = ms->get_md(i);
			md->dump();
		}
	}
	fflush(g_tfile);
}
//END MD_SET


//
//START MD_SET_MGR
//
UINT MD_SET_MGR::count_mem()
{
	UINT count = 0;
	SC<MD_SET*> * sct;
	for (MD_SET const* mds = m_md_set_list.get_head(&sct);
		 mds != NULL; mds = m_md_set_list.get_next(&sct)) {
		count += mds->count_mem();
	}
	return count;
}


void MD_SET_MGR::dump()
{
	if (g_tfile == NULL) { return; }

	FILE * h = g_tfile;
	UINT count = 0;
	SC<MD_SET*> * sct;
	for (MD_SET const* mds = m_md_set_list.get_head(&sct);
		 mds != NULL; mds = m_md_set_list.get_next(&sct)) {
		count += mds->count_mem();
	}

	//Dump mem usage into file.
	LIST<UINT> lst;
	for (MD_SET const* bs = m_md_set_list.get_head(&sct);
		 bs != NULL; bs = m_md_set_list.get_next(&sct)) {
		UINT c = bs->count_mem();

		bool inserted = false;

		if (m_md_set_list.get_elem_count() < 10000) {
			//Inserting sort complexity is quadratic.
			C<UINT> * ct;
			UINT n = lst.get_elem_count();
			lst.get_head(&ct);
			UINT i;
			for (i = 0; i < n; i++, lst.get_next(&ct)) {
				if (c >= C_val(ct)) {
					inserted = true;
					lst.insert_before(c, ct);
					break;
				}
			}
		}

		if (!inserted) {
			lst.append_head(c);
		}
	}

	UINT v = lst.get_head();

	fprintf(h,
		"\n==---- DUMP MD_SET_MGR: total %d MD_SETs, "
		"%d MD_SET are in free-list, mem usage are:\n",
		m_md_set_list.get_elem_count(), m_free_md_set.get_elem_count());

	UINT b = 0;
	UINT n = lst.get_elem_count();
	for (UINT i = 0; i < n; i++, v = lst.get_next(), b++) {
		if (b == 20) {
			fprintf(h, "\n");
			b = 0;
		}
		if (v < 1024) {
			fprintf(h, "%dB,", v);
		} else if (v < 1024 * 1024) {
			fprintf(h, "%dKB,", v/1024);
		} else {
			fprintf(h, "%dMB,", v/1024/1024);
		}
	}
	fflush(h);
}

//END MD_SET_MGR


//
//START MD2MD_SET_MAP
//
//Dump all relations between MD, and MD_SET.
//'md2mds': mapping from 'md' to an md-set it pointed to.
void MD2MDS::dump(REGION * ru)
{
	CHAR buf[MAX_BUF_LEN];
	if (g_tfile == NULL) { return; }

	fprintf(g_tfile, "\n*** Dump MD POINT-TO list ***");

	//Dump all MDs.
	MD_SYS * ms = ru->get_md_sys();
	ms->get_id2md_map()->dump();
	INT i;
	for (i = get_first(); i >= 0; i = get_next(i)) {
		MD * md = ms->get_md(i);
		IS_TRUE0(md);
		buf[0] = 0;
		fprintf(g_tfile, "\n\t%s", md->dump(buf, MAX_BUF_LEN));

		//Dumps MD_SET related to 'md'.
		MD_SET const* pts = get(MD_id(md));
		IS_TRUE0(pts);
		fprintf(g_tfile, "\n\t\tPOINT TO:\n");
		for (INT j = pts->get_first(); j >= 0; j = pts->get_next(j)) {
			MD * mmd = ms->get_md(j);
			IS_TRUE0(mmd);
			buf[0] = 0;
			fprintf(g_tfile, "\t\t\t%s\n", mmd->dump(buf, MAX_BUF_LEN));
		}
	}

	//Dump set of MD that corresponding to an individual VAR.
	fprintf(g_tfile, "\n*** Dump the mapping from VAR to set of MD ***");
	ID2VAR * var_tab = ru->get_var_mgr()->get_var_vec();
	SVECTOR<MD*> mdv;
	MD_ITER iter;
	for (INT i = 0; i <= var_tab->get_last_idx(); i++) {
		VAR * v = var_tab->get(i);
		if (v == NULL) { continue; }

		MD_TAB * mdtab = ms->get_md_tab(v);

		buf[0] = 0;

		fprintf(g_tfile, "\n\t%s", v->dump(buf));

		if (mdtab && mdtab->get_elem_count() > 0) {
			mdv.clean();
			iter.clean();
			mdtab->get_elems(mdv, iter);

			for (INT i = 0; i <= mdv.get_last_idx(); i++) {
				MD * md = mdv.get(i);
				buf[0] = 0;
				fprintf(g_tfile, "\n\t\t%s", md->dump(buf, MAX_BUF_LEN));
			}
		} //end if
	} //end for

	fprintf(g_tfile, "\n");
	fflush(g_tfile);
}
//END MD2MD_SET_MAP


//
//START MD_SYS
//
/* Register MD and generating unique id for it, with the followed method:
1. Generating MD hash table for any unique VAR.
2. Entering 'md' into MD hash table, the hash-value comes
	from an evaluating binary-TREE that the branch of
	tree-node indicate determination data related with MD fields.
Return the registered element.

NOTICE:
1. DO NOT free the registered element!
2. If you want to register an new MD, keep the id is 0. */
MD * MD_SYS::register_md(MD & m)
{
	IS_TRUE0(MD_base(&m));
	if (MD_id(&m) > 0) {
		//Find the entry accroding to m.
		MD_TAB * mdtab = get_md_tab(MD_base(&m));
		IS_TRUE(mdtab != NULL, ("md has registered"));
		MD * entry = mdtab->find(&m);
		IS_TRUE(entry != NULL, ("md has registered"));

		MD_is_addr_taken(entry) =
			MAX(MD_is_addr_taken(entry), MD_is_addr_taken(&m));
		return entry;
	}

	IS_TRUE0(MD_base(&m) != NULL);

	//Check if MD has been registerd.
	MD_TAB * mdtab = get_md_tab(MD_base(&m));
	if (mdtab != NULL) {
		//VAR-base has been registered, then check md by
		//offset in md-table.
		MD * hash_entry = mdtab->find(&m);
		if (hash_entry != NULL) {
			//find MD via MD_ofst.
			MD_id(&m) = MD_id(hash_entry);
			MD_is_addr_taken(hash_entry) =
					MAX(MD_is_addr_taken(hash_entry), MD_is_addr_taken(&m));
			return hash_entry;
		}

		if (MD_base(&m) == m_all_mem) {
			MD_id(&m) = MD_ALL_MEM;
			return get_md(MD_ALL_MEM);
		}

		//TODO: remove HEAP, STACK id. I consider they are useless.
		//if (MD_base(md) == g_heap_mem) {
		//	MD_id(md) = MD_HEAP_MEM;
		//	return ::get_md(MD_HEAP_MEM);
		//}
		//if (MD_base(md) == g_stack_mem) {
		//	MD_id(md) = MD_STACK_MEM;
		//	return ::get_md(MD_STACK_MEM);
		//}
	}

	//Generate a new MD and record it in md-table accroding to its id.
	MD * entry = new_md();
	UINT id = m_free_mdid_list.remove_head();
	if (id != 0) {
		MD_id(entry) = id;
	} else {
		MD_id(entry) = m_md_count++;
	}
	entry->copy(&m);
	if (mdtab == NULL) {
		mdtab = new_md_tab();

		//MD_TAB indexed by VAR's id.
		m_varid2mdtab.set(VAR_id(MD_base(entry)), mdtab);
	}

	//Insert into MD-table.
	mdtab->append(entry);
	m_id2md_map.set(MD_id(entry), entry);
	return entry;
}


//MD for global memory.
void MD_SYS::init_global_mem(VAR_MGR * vm)
{
	m_global_mem = NULL;
	if (vm == NULL) { return; }

	MD x;
	m_global_mem = vm->register_var((CHAR*)".global_mem",
								D_MC,
								D_UNDEF,
								0,
								0,
								1,
								VAR_GLOBAL|VAR_FAKE);
	VAR_allocable(m_global_mem) = false;

	//MD_id(&x) = MD_GLOBAL_MEM;
	MD_base(&x) = m_global_mem;
	MD_size(&x) = 0;
	MD_ty(&x) = MD_UNBOUND;
	MD_is_may(&x) = true; //MD_GLOBAL_MEM can only be May reference.
	MD * e = register_md(x);
	IS_TRUE0(MD_id(e) == MD_GLOBAL_MEM);
}


//MD for total memory.
void MD_SYS::init_all_mem(VAR_MGR * vm)
{
	m_all_mem = NULL;
	if (vm == NULL) { return; }

	m_all_mem = vm->register_var((CHAR*)".all_mem",
								D_MC,
								D_UNDEF,
								0,
								0,
								1,
								VAR_GLOBAL|VAR_FAKE);
	VAR_allocable(m_all_mem) = false;

	MD x;
	//MD_id(&x) = MD_ALL_MEM;
	MD_base(&x) = m_all_mem;
	MD_is_may(&x) = true;  //MD_ALL_MEM can only be May reference.
	MD_size(&x) = 0;
	MD_ty(&x) = MD_UNBOUND;
	MD * e = register_md(x);
	IS_TRUE0(MD_id(e) == MD_ALL_MEM);
}


void MD_SYS::init(VAR_MGR * vm)
{
	m_pool = smpool_create_handle(sizeof(MD) * 5, MEM_CONST_SIZE);
	m_sc_mdptr_pool = smpool_create_handle(sizeof(SC<MD*>) * 10,
										   MEM_CONST_SIZE);
	m_sc_mdid_pool = smpool_create_handle(sizeof(SC<UINT>) * 10,
										  MEM_CONST_SIZE);
	m_free_md_list.set_pool(m_sc_mdptr_pool);
	m_free_mdid_list.set_pool(m_sc_mdid_pool);
	m_md_count = 1;
	init_all_mem(vm);
	init_global_mem(vm);
	m_md_count = MD_FIRST_ALLOCABLE;
}


void MD_SYS::destroy()
{
	for (INT i = 0; i <= m_varid2mdtab.get_last_idx(); i++) {
		MD_TAB * mdtab = m_varid2mdtab.get(i);
		if (mdtab == NULL) { continue; }
		delete mdtab;
	}
	smpool_free_handle(m_pool);
	smpool_free_handle(m_sc_mdptr_pool);
	smpool_free_handle(m_sc_mdid_pool);
}


/* Compute all other md which are overlapped with 'md', but the output
does not include md itself.
e.g: md is md1, and md1 overlapped with md2, md3,
then output set is {md2, md3}.

'md': input to compute the overlapped md-set.
'tmpvec': for local use.
'tabiter': for local use.
'strictly': set to true to compute if md may be overlapped with global memory.

Note output need to be clean before invoke this function. */
void MD_SYS::compute_overlap(MD const* md, MD_SET & output,
							 TMAP_ITER<MD*, MD*> & tabiter,
							 bool strictly)
{
	IS_TRUE0(md);
	IS_TRUE0(MD_id(md) != MD_ALL_MEM);

	if (strictly && md->is_global()) {
		output.bunion(get_md(MD_GLOBAL_MEM));
	}

	MD_TAB * mdt = get_md_tab(MD_base(md));
	IS_TRUE0(mdt != NULL);

	MD * effect_md = mdt->get_effect_md();
	if (effect_md != NULL && effect_md != md) {
		IS_TRUE0(MD_base(md) == MD_base(effect_md));
		output.bunion(effect_md);
	}

	OFST_TAB * ofsttab = mdt->get_ofst_tab();
	IS_TRUE0(ofsttab);
	tabiter.clean();
	for (MD * tmd = ofsttab->get_first(tabiter, NULL);
		 tmd != NULL; tmd = ofsttab->get_next(tabiter, NULL)) {
		IS_TRUE0(MD_base(md) == MD_base(tmd));
		if (tmd == md) { continue; }
		if (md->is_overlap(tmd)) {
			output.bunion(tmd);
		}
	}
}


/* Compute all other md which are overlapped with MD in set 'mds'.
e.g: mds contains {md1}, and md1 overlapped with md2, md3,
then output set is {md1, md2, md3}.

'mds': it is not only input but also output buffer.
'tmpvec': for local use.
'tabiter': for local use.
'strictly': set to true to compute if md may be overlapped with global memory.
*/
void MD_SYS::compute_overlap(IN OUT MD_SET & mds, SVECTOR<MD*> & tmpvec,
							 TMAP_ITER<MD*, MD*> & tabiter, bool strictly)
{
	if (((BITSET&)mds).is_contain(MD_ALL_MEM)) { return; }

	UINT count = 0;
	tmpvec.clean();
	bool set_global = false;
	for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
		MD * md = get_md(i);
		IS_TRUE0(md);
		MD_TAB * mdt = get_md_tab(MD_base(md));
		IS_TRUE0(mdt != NULL);
		if (md->is_global()) { set_global = true; }

		MD * effect_md = mdt->get_effect_md();
		if (effect_md != NULL && !mds.is_contain(effect_md)) {
			IS_TRUE0(MD_base(md) == MD_base(effect_md));
			tmpvec.set(count, effect_md);
			count++;
		}

		OFST_TAB * ofsttab = mdt->get_ofst_tab();
		IS_TRUE0(ofsttab);
		tabiter.clean();
		for (MD* tmd = ofsttab->get_first(tabiter, NULL);
			 tmd != NULL; tmd = ofsttab->get_next(tabiter, NULL)) {
			if (((BITSET&)mds).is_contain(MD_id(tmd))) {
				continue;
			}
			IS_TRUE0(MD_base(md) == MD_base(tmd));
			if (md->is_overlap(tmd)) {
				tmpvec.set(count, tmd);
				count++;
			}
		}
	}

	if (strictly && set_global) { mds.bunion(get_md(MD_GLOBAL_MEM)); }

	for (INT i = 0; i <= tmpvec.get_last_idx(); i++) {
		MD const* t = tmpvec.get(i);
		IS_TRUE0(t && t->is_effect());
		mds.bunion(t);
	}
}


/* Compute all other md which are overlapped with MD in set 'mds'.
e.g: mds contains {md1}, and md1 overlapped with md2, md3,
then output is {md1, md2, md3}.

'mds': it is not only input but also output buffer.
'output': output md set.
'tabiter': for local use.
'strictly': set to true to compute if md may be overlapped with global memory.

Note output do not need to clean before invoke this function. */
void MD_SYS::compute_overlap(MD_SET const& mds, OUT MD_SET & output,
							 TMAP_ITER<MD*, MD*> & tabiter, bool strictly)
{
	IS_TRUE0(&mds != &output);

	if (mds.is_contain_pure(MD_ALL_MEM)) { return; }

	bool set_global = false;
	for (INT i = mds.get_first(); i >= 0; i = mds.get_next(i)) {
		MD * md = get_md(i);
		IS_TRUE0(md);
		MD_TAB * mdt = get_md_tab(MD_base(md));
		IS_TRUE0(mdt != NULL);
		if (md->is_global()) { set_global = true; }

		MD const* effect_md = mdt->get_effect_md();
		if (effect_md != NULL && !mds.is_contain_pure(MD_id(effect_md))) {
			IS_TRUE0(MD_base(md) == MD_base(effect_md));
			output.bunion_pure(MD_id(effect_md));
		}

		OFST_TAB const* ofsttab = mdt->get_ofst_tab();
		IS_TRUE0(ofsttab);
		tabiter.clean();
		for (MD const* tmd = ofsttab->get_first(tabiter, NULL);
			 tmd != NULL; tmd = ofsttab->get_next(tabiter, NULL)) {
			if (mds.is_contain_pure(MD_id(tmd))) {
				continue;
			}
			IS_TRUE0(MD_base(md) == MD_base(tmd));
			if (md->is_overlap(tmd)) {
				output.bunion_pure(MD_id(tmd));
			}
		}
	}

	if (strictly && set_global) { output.bunion_pure(MD_GLOBAL_MEM); }
}


void MD_SYS::clean()
{
	for (INT i = 0; i <= m_varid2mdtab.get_last_idx(); i++) {
		MD_TAB * mdtab = m_varid2mdtab.get(i);
		if (mdtab == NULL) { continue; }
		mdtab->clean();
	}
	for (INT i = 0; i <= m_id2md_map.get_last_idx(); i++) {
		MD * md = m_id2md_map.get(i);
		if (md == NULL) { continue; }
		free_md(md);
	}
	m_md_count = 2; //Index 0 is reserved, index 1 is all-mem-id.
	m_free_mdid_list.clean();
}


void MD_SYS::dump_all_mds()
{
	if (g_tfile == NULL) return;
	fprintf(g_tfile, "\n==---- DUMP ALL MD ----==");
	for (INT i = 0; i <= m_id2md_map.get_last_idx(); i++) {
		MD * md = m_id2md_map.get(i);
		if (md == NULL) { continue; }
		IS_TRUE0(MD_id(md) == (UINT)i);
		md->dump();
	}
}


//Remove VAR and its related info.
void MD_SYS::remove_var(VAR * v)
{
	IS_TRUE0(v);
	TMAP_ITER<MD*, MD*> iter;
	MD_TAB * mdtab = get_md_tab(v);
	if (mdtab != NULL) {
		MD * x = mdtab->get_effect_md();
		if (x != NULL) {
			free_md(x);
		}

		OFST_TAB * ofstab = mdtab->get_ofst_tab();
		IS_TRUE0(ofstab);
		if (ofstab->get_elem_count() > 0) {
			iter.clean();
			for (MD * md = ofstab->get_first(iter, NULL);
				 md != NULL; md = ofstab->get_next(iter, NULL)) {
				free_md(md);
			}
		}
	}
	m_varid2mdtab.set(VAR_id(v), NULL);
	delete mdtab;
}
//END MD_SYS
