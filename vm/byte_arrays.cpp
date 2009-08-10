#include "master.hpp"

namespace factor
{

byte_array *factorvm::allot_byte_array(cell size)
{
	byte_array *array = allot_array_internal<byte_array>(size);
	memset(array + 1,0,size);
	return array;
}

PRIMITIVE(byte_array)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	cell size = vm->unbox_array_size();
	dpush(tag<byte_array>(myvm->allot_byte_array(size)));
}

PRIMITIVE(uninitialized_byte_array)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	cell size = myvm->unbox_array_size();
	dpush(tag<byte_array>(myvm->allot_array_internal<byte_array>(size)));
}

PRIMITIVE(resize_byte_array)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	byte_array *array = untag_check<byte_array>(dpop(),myvm);
	cell capacity = myvm->unbox_array_size();
	dpush(tag<byte_array>(myvm->reallot_array(array,capacity)));
}

void growable_byte_array::append_bytes(void *elts, cell len)
{
	cell new_size = count + len;

	if(new_size >= array_capacity(elements.untagged())){
		factorvm *myvm = elements.myvm;
		elements = myvm->reallot_array(elements.untagged(),new_size * 2);
	}

	memcpy(&elements->data<u8>()[count],elts,len);

	count += len;
}

void growable_byte_array::append_byte_array(cell byte_array_)
{
	factorvm *myvm = elements.myvm;
	gc_root<byte_array> byte_array(byte_array_,myvm);

	cell len = array_capacity(byte_array.untagged());
	cell new_size = count + len;

	if(new_size >= array_capacity(elements.untagged()))
		elements = myvm->reallot_array(elements.untagged(),new_size * 2);

	memcpy(&elements->data<u8>()[count],byte_array->data<u8>(),len);

	count += len;
}

void growable_byte_array::trim()
{
	factorvm *myvm = elements.myvm;
	elements = myvm->reallot_array(elements.untagged(),count);
}

}
