/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */

namespace factor
{
	struct heap;
	struct datacollector;

	struct factorvm {
		/* GC is off during heap walking */
		bool gc_off;
		/* Set by the -securegc command line argument */
		bool secure_gc;

		// data heap, data gc
		datacollector *datagc;

		// code heap
		heap *code;
		unordered_map<heap_block *,char *> forwarding;  // should be in 'code'?



		// context
		cell ds_size, rs_size;
		context *unused_contexts;

		void reset_datastack();
		void reset_retainstack();
		void fix_stacks();
		void init_stacks(cell ds_size, cell rs_size);



		//debug.cpp
		bool fep_disabled;
		bool full_output;
		cell look_for;
		cell obj;
    
		//dispatch.cpp
		cell megamorphic_cache_hits;
		cell megamorphic_cache_misses;


		// errors.cpp
		/* Global variables used to pass fault handler state from signal handler to
		   user-space */
		cell signal_number;
		cell signal_fault_addr;
		stack_frame *signal_callstack_top;

		void out_of_memory();
		void fatal_error(const char* msg, cell tagged);
		void critical_error(const char* msg, cell tagged);

		void throw_error(cell error, stack_frame *native_stack);
		void general_error(vm_error_type error, cell arg1, cell arg2, stack_frame *native_stack);
		void divide_by_zero_error();
		void memory_protection_error(cell addr, stack_frame *native_stack);
		void signal_error(int signal, stack_frame *native_stack);
		void type_error(cell type, cell tagged);
		void not_implemented_error();
		

		// image.cpp
		cell data_relocation_base;
		cell code_relocation_base;

	// from local_roots.cpp
	segment *gc_locals_region;
	cell gc_locals;
	segment *gc_bignums_region;
	cell gc_bignums;

		// inline_cache.cpp
		cell max_pic_size;	  
		cell cold_call_to_ic_transitions;
		cell ic_to_pic_transitions;
		cell pic_to_mega_transitions;
		/* PIC_TAG, PIC_HI_TAG, PIC_TUPLE, PIC_HI_TAG_TUPLE */
		cell pic_counts[4];



#ifdef __APPLE__
		// mach_signal.hpp
		/* The exception port on which our thread listens. */
		mach_port_t our_exception_port;
#endif

		// math.cpp
		cell bignum_zero;
		cell bignum_pos_one;
		cell bignum_neg_one;

#ifdef WINDOWS
		HMODULE hFactorDll;
#endif

		// profiler.cpp
		bool profiling_p;
		void init_profiler();
		code_block *compile_profiling_stub(cell word);
		void set_profiling(bool profiling);


		// run.cpp
		cell T;   

		// write_barrier.cpp
		cell allot_markers_offset;


		factorvm();
	};

	extern factorvm *vm;	 // Ideally we want to get rid of this singleton ptr completely



// globals to be removed (referenced in factor code)

	// contexts.cpp
	VM_C_API factor::context *stack_chain;  

	// run.cpp
	/* TAGGED user environment data; see getenv/setenv prims */
	VM_C_API factor::cell userenv[USER_ENV];

	// write_barrier.cpp
	VM_C_API factor::cell cards_offset;
	VM_C_API factor::cell decks_offset;


#ifndef WINDOWS

	/* On Unix, shared fds such as stdin cannot be set to non-blocking mode
	   (http://homepages.tesco.net/J.deBoynePollard/FGA/dont-set-shared-file-descriptors-to-non-blocking-mode.html)
	   so we kludge around this by spawning a thread, which waits on a control pipe
	   for a signal, upon receiving this signal it reads one block of data from stdin
	   and writes it to a data pipe. Upon completion, it writes a 4-byte integer to
	   the size pipe, indicating how much data was written to the data pipe.

	   The read end of the size pipe can be set to non-blocking. */
	extern "C" {
		extern int stdin_read;
		extern int stdin_write;

		extern int control_read;
		extern int control_write;

		extern int size_read;
		extern int size_write;
	}

#endif

}
