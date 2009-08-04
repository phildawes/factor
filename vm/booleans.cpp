#include "master.hpp"

namespace factor
{

VM_C_API void box_boolean(bool value)
{
	dpush(value ? vm->T : F);
}

VM_C_API bool to_boolean(cell value)
{
	return value != F;
}

}
