namespace factor
{

/* Runtime errors */
enum vm_error_type
{
	ERROR_EXPIRED = 0,
	ERROR_IO,
	ERROR_NOT_IMPLEMENTED,
	ERROR_TYPE,
	ERROR_DIVIDE_BY_ZERO,
	ERROR_SIGNAL,
	ERROR_ARRAY_SIZE,
	ERROR_C_STRING,
	ERROR_FFI,
	ERROR_HEAP_SCAN,
	ERROR_UNDEFINED_SYMBOL,
	ERROR_DS_UNDERFLOW,
	ERROR_DS_OVERFLOW,
	ERROR_RS_UNDERFLOW,
	ERROR_RS_OVERFLOW,
	ERROR_MEMORY,
};

PRIMITIVE(die);
PRIMITIVE(call_clear);
PRIMITIVE(unimplemented);

/* Global variables used to pass fault handler state from signal handler to
user-space */
extern cell signal_number;
extern cell signal_fault_addr;
extern stack_frame *signal_callstack_top;

void memory_signal_handler_impl();
void misc_signal_handler_impl();

}
