namespace factor
{

/* compiled code */


PRIMITIVE(modify_code_heap);

PRIMITIVE(code_room);


inline static void check_code_pointer(cell ptr)
{
#ifdef FACTOR_DEBUG
	assert(in_code_heap_p(ptr));
#endif
}

}
