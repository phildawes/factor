#include "master.hpp"

namespace factor
{

void factorvm::check_frame(stack_frame *frame)
{
#ifdef FACTOR_DEBUG
	check_code_pointer((cell)frame->xt);
	assert(frame->size != 0);
#endif
}

callstack *factorvm::allot_callstack(cell size)
{
	callstack *stack = allot<callstack>(callstack_size(size));
	stack->length = tag_fixnum(size);
	return stack;
}

stack_frame *factorvm::fix_callstack_top(stack_frame *top, stack_frame *bottom)
{
	stack_frame *frame = bottom - 1;

	while(frame >= top)
		frame = frame_successor(frame);

	return frame + 1;
}

/* We ignore the topmost frame, the one calling 'callstack',
so that set-callstack doesn't get stuck in an infinite loop.

This means that if 'callstack' is called in tail position, we
will have popped a necessary frame... however this word is only
called by continuation implementation, and user code shouldn't
be calling it at all, so we leave it as it is for now. */
stack_frame *factorvm::capture_start()
{
	stack_frame *frame = stack_chain->callstack_bottom - 1;
	while(frame >= stack_chain->callstack_top
		&& frame_successor(frame) >= stack_chain->callstack_top)
	{
		frame = frame_successor(frame);
	}
	return frame + 1;
}

PRIMITIVE(callstack)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	stack_frame *top = myvm->capture_start();
	stack_frame *bottom = stack_chain->callstack_bottom;

	fixnum size = (cell)bottom - (cell)top;
	if(size < 0)
		size = 0;

	callstack *stack = myvm->allot_callstack(size);
	memcpy(stack->top(),top,size);
	dpush(tag<callstack>(stack));
}

PRIMITIVE(set_callstack)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	callstack *stack = untag_check<callstack>(dpop(),myvm);

	set_callstack(stack_chain->callstack_bottom,
		stack->top(),
		untag_fixnum(stack->length),
		memcpy);

	/* We cannot return here ... */
	myvm->critical_error("Bug in set_callstack()",0);
}

code_block *factorvm::frame_code(stack_frame *frame)
{
	check_frame(frame);
	return (code_block *)frame->xt - 1;
}

cell factorvm::frame_type(stack_frame *frame)
{
	return frame_code(frame)->type;
}

cell factorvm::frame_executing(stack_frame *frame)
{
	code_block *compiled = frame_code(frame);
	if(compiled->literals == F || !stack_traces_p())
		return F;
	else
	{
		array *literals = untag<array>(compiled->literals);
		cell executing = array_nth(literals,0);
		check_data_pointer((object *)executing);
		return executing;
	}
}

stack_frame *factorvm::frame_successor(stack_frame *frame)
{
	check_frame(frame);
	return (stack_frame *)((cell)frame - frame->size);
}

/* Allocates memory */
cell factorvm::frame_scan(stack_frame *frame)
{
	switch(frame_type(frame))
	{
	case QUOTATION_TYPE:
		{
			cell quot = frame_executing(frame);
			if(quot == F)
				return F;
			else
			{
				char *return_addr = (char *)FRAME_RETURN_ADDRESS(frame,this);
				char *quot_xt = (char *)(frame_code(frame) + 1);

				return tag_fixnum(quot_code_offset_to_scan(
					quot,(cell)(return_addr - quot_xt)));
			}
		}
	case WORD_TYPE:
		return F;
	default:
		critical_error("Bad frame type",frame_type(frame));
		return F;
	}
}

namespace
{

struct stack_frame_accumulator {
	growable_array frames;

	stack_frame_accumulator(factorvm *vm) : frames(vm) {} 

	void operator()(stack_frame *frame,factorvm* myvm)
	{
		gc_root<object> executing(myvm->frame_executing(frame),myvm);
		gc_root<object> scan(myvm->frame_scan(frame),myvm);

		frames.add(executing.value());
		frames.add(scan.value());
	}
};

}

PRIMITIVE(callstack_to_array)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	gc_root<callstack> callstack(dpop(),myvm);

	stack_frame_accumulator accum(vm);
	myvm->iterate_callstack_object(callstack.untagged(),accum);
	accum.frames.trim();

	dpush(accum.frames.elements.value());
}

stack_frame *factorvm::innermost_stack_frame(callstack *stack)
{
	stack_frame *top = stack->top();
	stack_frame *bottom = stack->bottom();
	stack_frame *frame = bottom - 1;

	while(frame >= top && frame_successor(frame) >= top)
		frame = frame_successor(frame);

	return frame;
}

stack_frame *factorvm::innermost_stack_frame_quot(callstack *callstack)
{
	stack_frame *inner = innermost_stack_frame(callstack);
	tagged<quotation>(frame_executing(inner)).untag_check(vm);
	return inner;
}

/* Some primitives implementing a limited form of callstack mutation.
Used by the single stepper. */
PRIMITIVE(innermost_stack_frame_executing)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	dpush(myvm->frame_executing(myvm->innermost_stack_frame(untag_check<callstack>(dpop(),myvm))));
}

PRIMITIVE(innermost_stack_frame_scan)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	dpush(myvm->frame_scan(myvm->innermost_stack_frame_quot(untag_check<callstack>(dpop(),myvm))));
}

PRIMITIVE(set_innermost_stack_frame_quot)
{
	factorvm *myvm = PRIMITIVE_GETVM();	
	gc_root<callstack> callstack(dpop(),myvm);
	gc_root<quotation> quot(dpop(),myvm);

	callstack.untag_check();
	quot.untag_check();

	myvm->jit_compile(quot.value(),true);

	stack_frame *inner = myvm->innermost_stack_frame_quot(callstack.untagged());
	cell offset = (char *)FRAME_RETURN_ADDRESS(inner,myvm) - (char *)inner->xt;
	inner->xt = quot->xt;
	FRAME_RETURN_ADDRESS(inner,myvm) = (char *)quot->xt + offset;
}

/* called before entry into Factor code. */
VM_ASM_API void save_callstack_bottom(stack_frame *callstack_bottom)
{
	stack_chain->callstack_bottom = callstack_bottom;
}

}
