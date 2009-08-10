#include "master.hpp"

namespace factor
{

/* push a new tuple on the stack */
tuple *factorvm::allot_tuple(cell layout_)
{
	gc_root<tuple_layout> layout(layout_,this);
	gc_root<tuple> t(allot<tuple>(tuple_size(layout.untagged())),this);
	t->layout = layout.value();
	return t.untagged();
}

PRIMITIVE(tuple)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	gc_root<tuple_layout> layout(dpop(),myvm);
	tuple *t = myvm->allot_tuple(layout.value());
	fixnum i;
	for(i = tuple_size(layout.untagged()) - 1; i >= 0; i--)
		t->data()[i] = F;

	dpush(tag<tuple>(t));
}

/* push a new tuple on the stack, filling its slots from the stack */
PRIMITIVE(tuple_boa)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	gc_root<tuple_layout> layout(dpop(),myvm);
	gc_root<tuple> t(myvm->allot_tuple(layout.value()),myvm);
	cell size = untag_fixnum(layout.untagged()->size) * sizeof(cell);
	memcpy(t->data(),(cell *)(ds - (size - sizeof(cell))),size);
	ds -= size;
	dpush(t.value());
}

}
