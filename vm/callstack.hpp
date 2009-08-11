namespace factor
{

inline static cell callstack_size(cell size)
{
	return sizeof(callstack) + size;
}

PRIMITIVE(callstack);
PRIMITIVE(set_callstack);
PRIMITIVE(callstack_to_array);
PRIMITIVE(innermost_stack_frame_executing);
PRIMITIVE(innermost_stack_frame_scan);
PRIMITIVE(set_innermost_stack_frame_quot);

VM_ASM_API void save_callstack_bottom(stack_frame *callstack_bottom, factorvm *vm);


/* This is a little tricky. The iterator may allocate memory, so we
   keep the callstack in a GC root and use relative offsets */
template<typename TYPE> void factorvm::iterate_callstack_object(callstack *stack_, TYPE &iterator)
{
	gc_root<callstack> stack(stack_,this);
	fixnum frame_offset = untag_fixnum(stack->length) - sizeof(stack_frame);

	while(frame_offset >= 0)
		{
			stack_frame *frame = stack->frame_at(frame_offset);
			frame_offset -= frame->size;
			iterator(frame,this);
		}
}


}
