namespace factor
{
struct code_heap;
struct data_heap;
struct datacollector;
struct zone;
struct vm_parameters;

struct factorvm {
	/* GC is off during heap walking */
	bool gc_off;
	/* Set by the -securegc command line argument */
	bool secure_gc;

	// code heap
	code_heap *code;
	/* the heap */
	data_heap *data;

	unordered_map<heap_block *,char *> forwarding; 



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

	void relocate_data();
	void load_image(vm_parameters *p);
	bool save_image(const vm_char *file);


	// from local_roots.cpp
	segment *gc_locals_region;
	cell gc_locals;
	segment *gc_bignums_region;
	cell gc_bignums;

	// bignum
		
	void bignum_divide(bignum * numerator, bignum * denominator,
					   bignum * * quotient, bignum * * remainder);
	bignum * bignum_quotient(bignum *, bignum *);
	bignum * bignum_remainder(bignum *, bignum *);


	// data_gc ----------------------------------------------------------------------------------


	/* used during garbage collection only */
	zone *newspace;
	bool performing_gc;
	bool performing_compaction;
	cell collecting_gen;

	/* if true, we collecting aging space for the second time, so if it is still
	   full, we go on to collect tenured */
	bool collecting_aging_again;

	/* in case a generation fills up in the middle of a gc, we jump back
	   up to try collecting the next generation. */
	jmp_buf gc_jmp;
  
	gc_stats stats[max_gen_count];
	u64 cards_scanned;
	u64 decks_scanned;
	u64 card_scan_time;
	cell code_heap_scans;


	/* What generation was being collected when copy_code_heap_roots() was last
	   called? Until the next call to add_code_block(), future
	   collections of younger generations don't have to touch the code
	   heap. */
	cell last_code_heap_scan;

	/* sometimes we grow the heap */
	bool growing_data_heap;
	data_heap *old_data_heap;


	void init_data_gc();
	void gc();
	object *copy_untagged_object_impl(object *pointer, cell size);
	object *copy_object_impl(object *untagged);
	bool should_copy_p(object *untagged);
	object *resolve_forwarding(object *untagged);
	cell copy_object(cell pointer);
	void copy_handle(cell *handle);
	void copy_card(card *ptr, cell gen, cell here);
	void copy_card_deck(card_deck *deck, cell gen, card mask, card unmask);
	void copy_gen_cards(cell gen);
	void copy_cards();
	void copy_stack_elements(segment *region, cell top);
	void copy_registered_locals();
	void copy_registered_bignums();
	void copy_roots();
	cell copy_next_from_nursery(cell scan);
	cell copy_next_from_aging(cell scan);
	cell copy_next_from_tenured(cell scan);
	void copy_reachable_objects(cell scan, cell *end);
	void begin_gc(cell requested_bytes);
	void end_gc(cell gc_elapsed);
	void garbage_collection(cell gen,
							bool growing_data_heap_,
							cell requested_bytes);
	void clear_gc_stats();
	template <typename TYPE> TYPE *copy_untagged_object(TYPE *untagged);


	inline bool collecting_accumulation_gen_p()
	{
		return ((data->have_aging_p()
				 && collecting_gen == data->aging()
				 && !collecting_aging_again)
				|| collecting_gen == data->tenured());
	}


	inline object *allot_zone(zone *z, cell a)
	{
		cell h = z->here;
		z->here = h + align8(a);
		object *obj = (object *)h;
		allot_barrier(obj);
		return obj;
	}

	inline void check_data_pointer(object *pointer)
	{
#ifdef FACTOR_DEBUG
		if(!growing_data_heap)
			{
				assert((cell)pointer >= data->seg->start
					   && (cell)pointer < data->seg->end);
			}
#endif
	}

	inline void check_tagged_pointer(cell tagged)
	{
#ifdef FACTOR_DEBUG
		if(!immediate_p(tagged))
			{
				object *obj = untag<object>(tagged);
				check_data_pointer(obj);
				obj->h.hi_tag();
			}
#endif
	}

	/*
	 * It is up to the caller to fill in the object's fields in a meaningful
	 * fashion!
	 */
	inline object *allot_object(header header, cell size)
	{
#ifdef GC_DEBUG
		if(!gc_off)
			gc();
#endif

		object *obj;
		factor::zone *nursery = getnursery();
		if(nursery->size - allot_buffer_zone > size)
			{
				/* If there is insufficient room, collect the nursery */
				if(nursery->here + allot_buffer_zone + size > nursery->end)
					garbage_collection(data->nursery(),false,0);

				cell h = nursery->here;
				nursery->here = h + align8(size);
				obj = (object *)h;
			}
		/* If the object is bigger than the nursery, allocate it in
		   tenured space */
		else
			{
				zone *tenured = &data->generations[data->tenured()];

				/* If tenured space does not have enough room, collect */
				if(tenured->here + size > tenured->end)
					{
						gc();
						tenured = &data->generations[data->tenured()];
					}

				/* If it still won't fit, grow the heap */
				if(tenured->here + size > tenured->end)
					{
						garbage_collection(data->tenured(),true,size);
						tenured = &data->generations[data->tenured()];
					}

				obj = allot_zone(tenured,size);

				/* Allows initialization code to store old->new pointers
				   without hitting the write barrier in the common case of
				   a nursery allocation */
				write_barrier(obj);
			}

		obj->h = header;
		return obj;
	}

	template<typename T> T *allot(cell size)
	{
		return (T *)allot_object(header(T::type_number),size);
	}





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


	inline card *addr_to_allot_marker(object *a)
	{
		return (card *)(((cell)a >> card_bits) + allot_markers_offset);
	}

	/* we need to remember the first object allocated in the card */
	inline void allot_barrier(object *address)
	{
		card *ptr = addr_to_allot_marker(address);
		if(*ptr == invalid_allot_marker)
			*ptr = ((cell)address & addr_card_mask);
	}


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
