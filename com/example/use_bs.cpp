#include "stdio.h"
#include "ltype.h"
#include "comf.h"
#include "smempool.h"
#include "sstl.h"
#include "bs.h"
#include "sbs.h"
using namespace xcom;
void bs()
{
<<<<<<< HEAD
	BitSet bs;
	bs.bunion(1);
	bs.bunion(2);
	bs.bunion(3);

	printf("Iter element:\n");
	for (int elem = bs.get_first(); elem != -1; elem = bs.get_next(elem)) {
		printf("%d\n", elem);
	}

	printf("\n");
	printf("%d\n", bs.get_elem_count());
=======
    BitSet bs;
    bs.bunion(1);
    bs.bunion(2);
    bs.bunion(3);

    printf("Iter element:\n");
    for (int elem = bs.get_first(); elem != -1; elem = bs.get_next(elem)) {
        printf("%d\n", elem);
    }

    printf("\n");
    printf("%d\n", bs.get_elem_count());
>>>>>>> dfa247d68c664b4147d8f39632c66fd093ca9d64
}

void sparse_bs()
{
<<<<<<< HEAD
	MiscBitSetMgr<100> mbsm;
	SBitSetCore<100> sbs;
	sbs.bunion(1, mbsm);
	sbs.bunion(99, mbsm);
	sbs.bunion(99, mbsm);
	sbs.bunion(99999, mbsm);

	printf("Iter element:\n");
	SC<SEG<100>*> * iter;
	for (int elem = sbs.get_first(&iter);
		 elem != -1; elem = sbs.get_next(elem, &iter)) {
		printf("%d\n", elem);
	}

	printf("\n");
	printf("%d\n", sbs.get_elem_count());

	sbs.clean(mbsm);
=======
    MiscBitSetMgr<100> mbsm;
    SBitSetCore<100> sbs;
    sbs.bunion(1, mbsm);
    sbs.bunion(99, mbsm);
    sbs.bunion(99, mbsm);
    sbs.bunion(99999, mbsm);

    printf("Iter element:\n");
    SC<SEG<100>*> * iter;
    for (int elem = sbs.get_first(&iter);
         elem != -1; elem = sbs.get_next(elem, &iter)) {
        printf("%d\n", elem);
    }

    printf("\n");
    printf("%d\n", sbs.get_elem_count());

    sbs.clean(mbsm);
>>>>>>> dfa247d68c664b4147d8f39632c66fd093ca9d64
}


int main()
{
<<<<<<< HEAD
	bs();
	sparse_bs();
	return 0;
=======
    bs();
    sparse_bs();
    return 0;
>>>>>>> dfa247d68c664b4147d8f39632c66fd093ca9d64
}
