#include "master.hpp"

namespace factor
{

void *start_thread(void *(*start_routine)(void *),void *args){
    return CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, args, 0, 0); 
}


s64 factorvm::current_micros()
{
	FILETIME t;
	GetSystemTimeAsFileTime(&t);
	return (((s64)t.dwLowDateTime | (s64)t.dwHighDateTime<<32)
		- EPOCH_OFFSET) / 10;
}

FACTOR_STDCALL LONG exception_handler(PEXCEPTION_POINTERS pe)
{
	factorvm *myvm = lookup_vm(GetCurrentThreadId());
	PEXCEPTION_RECORD e = (PEXCEPTION_RECORD)pe->ExceptionRecord;
	CONTEXT *c = (CONTEXT*)pe->ContextRecord;

	if(myvm->in_code_heap_p(c->EIP))
		myvm->signal_callstack_top = (stack_frame *)c->ESP;
	else
		myvm->signal_callstack_top = NULL;

	if(e->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		myvm->signal_fault_addr = e->ExceptionInformation[1];
		c->EIP = (cell)memory_signal_handler_impl;
	}
	/* If the Widcomm bluetooth stack is installed, the BTTray.exe process
	injects code into running programs. For some reason this results in
	random SEH exceptions with this (undocumented) exception code being
	raised. The workaround seems to be ignoring this altogether, since that
	is what happens if SEH is not enabled. Don't really have any idea what
	this exception means. */
	else if(e->ExceptionCode != 0x40010006)
	{
		myvm->signal_number = e->ExceptionCode;
		c->EIP = (cell)misc_signal_handler_impl;
	}

	return EXCEPTION_CONTINUE_EXECUTION;
}

bool handler_added = 0;

void factorvm::c_to_factor_toplevel(cell quot)
{
	if(!handler_added){
		if(!AddVectoredExceptionHandler(0, (PVECTORED_EXCEPTION_HANDLER)exception_handler))
			fatal_error("AddVectoredExceptionHandler failed", 0);
		handler_added = 1;
	}
	c_to_factor(quot,this);
 	RemoveVectoredExceptionHandler((void *)exception_handler);
}

void factorvm::open_console()
{
}

}
