namespace factor
{
struct code_heap;
struct data_heap;
struct datacollector;
struct zone;
struct vm_parameters;
struct image_header;


typedef void (*heap_iterator)(heap_block *compiled);

VM_C_API cell userenv[USER_ENV];


template<typename T> cell array_capacity(T *array)
{
#ifdef FACTOR_DEBUG
	assert(array->h.hi_tag() == T::type_number);
#endif
	return array->capacity >> TAG_BITS;
}

template <typename T> cell array_size(cell capacity)
{
	return sizeof(T) + capacity * T::element_size;
}

template <typename T> cell array_size(T *array)
{
	return array_size<T>(array_capacity(array));
}


struct factorvm {
	/* GC is off during heap walking */
	bool gc_off;
	/* Set by the -securegc command line argument */
	bool secure_gc;

	// code heap
	code_heap *code;
	/* the heap */
	data_heap *data;

	/* A heap walk allows useful things to be done, like finding all
	   references to an object for debugging purposes. */
	cell heap_scan_ptr;


	// data_heap  ----------------------------------------------------------------------------

	void init_card_decks();
	void clear_cards(cell from, cell to);
	void clear_decks(cell from, cell to);
	void clear_allot_markers(cell from, cell to);
	void reset_generation(cell i);
	void reset_generations(cell from, cell to);

	data_heap *alloc_data_heap(cell gens,
							   cell young_size,
							   cell aging_size,
							   cell tenured_size);
	void init_data_heap(cell gens,
					cell young_size,
					cell aging_size,
					cell tenured_size,
						bool secure_gc_);
	data_heap *grow_data_heap(data_heap *data, cell requested_bytes);
	void set_data_heap(data_heap *data_);
	void begin_scan();
	void end_scan();
	cell next_object();
	cell untagged_object_size(object *pointer);
	cell unaligned_object_size(object *pointer);
	cell object_size(cell tagged);
	cell binary_payload_start(object *pointer);
/* Every object has a regular representation in the runtime, which makes GC
   much simpler. Every slot of the object until binary_payload_start is a pointer
   to some other object. */
	inline void do_slots(cell obj, void (* iter)(cell *,factorvm *))
	{
		cell scan = obj;
		cell payload_start = binary_payload_start((object *)obj);
		cell end = obj + payload_start;

		scan += sizeof(cell);

		while(scan < end)
			{
				iter((cell *)scan,this);
				scan += sizeof(cell);
			}
	}

	template<typename TYPE> void each_object(TYPE &functor);
	cell find_all_words();

	unordered_map<heap_block *,char *> forwarding; 

	// context     ----------------------------------------------------------------------------
	cell ds_size, rs_size;
	context *unused_contexts;

	void reset_datastack();
	void reset_retainstack();
	void fix_stacks();
	void init_stacks(cell ds_size, cell rs_size);



	//debug  ----------------------------------------------------------------------------
	bool fep_disabled;
	bool full_output;
	cell look_for;
	cell obj;
    
	//dispatch  ----------------------------------------------------------------------------
	cell megamorphic_cache_hits;
	cell megamorphic_cache_misses;


	// errors  ----------------------------------------------------------------------------
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
		

	// image  ----------------------------------------------------------------------------
	cell data_relocation_base;
	cell code_relocation_base;

	void relocate_data();
	void load_image(vm_parameters *p);
	bool save_image(const vm_char *file);
	void load_data_heap(FILE *file, image_header *h, vm_parameters *p);
	void load_code_heap(FILE *file, image_header *h, vm_parameters *p);
	void relocate_object(object *object);
	void relocate_code();

	void init_objects(image_header *h);
	template <typename TYPE> void code_fixup(TYPE **handle);
	void fixup_word(word *word);
	void fixup_quotation(quotation *quot);
	void fixup_alien(alien *d);
	void fixup_callstack_object(callstack *stack);

	// from local_roots ------------------------------------------------------------------
	segment *gc_locals_region;
	cell gc_locals;
	segment *gc_bignums_region;
	cell gc_bignums;


	// generic arrays ----------------------------------------------------------------------------

	template <typename TYPE> TYPE *allot_array_internal(cell capacity)
	{
        TYPE *array = allot<TYPE>(array_size<TYPE>(capacity));
		array->capacity = tag_fixnum(capacity);
		return array;
	}


	template <typename TYPE> TYPE *reallot_array(TYPE *array_, cell capacity);

	// arrays ----------------------------------------------------------------------------

	array *allot_array(cell capacity, cell fill);
	cell allot_array_1(cell obj);
	cell allot_array_2(cell v1, cell v2);
	cell allot_array_4(cell v1, cell v2, cell v3, cell v4);


	// strings ----------------------------------------------------------------------------

	string* allot_string_internal(cell capacity);
	string* allot_string(cell capacity, cell fill);
	string *reallot_string(string *string, cell capacity);
	void fill_string(string *str_, cell start, cell capacity, cell fill);
	void set_string_nth_slow(string *str_, cell index, cell ch);
	void set_string_nth(string *str, cell index, cell ch);
	
	// byte_array ----------------------------------------------------------------------------

	byte_array *allot_byte_array(cell size);

	// tuples ----------------------------------------------------------------------------

	tuple *allot_tuple(cell layout_);

	// words  ----------------------------------------------------------------------------

	word *allot_word(cell vocab, cell name);
	void update_word_xt(cell word);
	

	// code heap  ----------------------------------------------------------------------------

	void fixup_object_xts();


	// quotations  ----------------------------------------------------------------------------

	void compile_all_words();


	// bignum  ----------------------------------------------------------------------------
		
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





	// inline_cache  ----------------------------------------------------------------------------
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

	// math  ----------------------------------------------------------------------------
	cell bignum_zero;
	cell bignum_pos_one;
	cell bignum_neg_one;

	cell allot_float(double n);

	cell unbox_array_size();


#ifdef WINDOWS
	HMODULE hFactorDll;
#endif


	// io   ----------------------------------------------------------------------------

	void init_c_io();
	void io_error();
	


	// profiler  ----------------------------------------------------------------------------
	bool profiling_p;
	void init_profiler();
	code_block *compile_profiling_stub(cell word);
	void set_profiling(bool profiling);


	// debug   ----------------------------------------------------------------------------
	void print_array(array* array, cell nesting);
	void print_tuple(tuple *tuple, cell nesting);
	void print_word(word* word, cell nesting);
	void print_obj(cell obj);
	void print_nested_obj(cell obj, fixnum nesting);
	void print_objects(cell *start, cell *end);
	void print_datastack();
	void print_retainstack();
	void print_callstack();
	void find_data_references(cell look_for_);
	void dump_generations();
	void factorbug();
	//void dump_zone(zone *z);
	void dump_objects(cell type);
	void dump_code_heap();

	// run   ----------------------------------------------------------------------------
	cell T;   

	// write_barrier  ----------------------------------------------------------------------------

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

	// code_gc  ----------------------------------------------------------------------------

	void new_heap(code_heap *h, cell size);
	void build_free_list(code_heap *h, cell size);
	heap_block *heap_allot(code_heap *h, cell size);
	void heap_free(code_heap *h, heap_block *block);
	void mark_block(heap_block *block);
	void unmark_marked(code_heap *heap);
	void free_unmarked(code_heap *heap, heap_iterator iter);
	void heap_usage(code_heap *h, cell *used, cell *total_free, cell *max_free);
	cell heap_size(code_heap *h);
	cell compute_heap_forwarding(code_heap *h, unordered_map<heap_block *,char *> &forwarding);
	void compact_heap(code_heap *h, unordered_map<heap_block *,char *> &forwarding);
	void assert_free_block(free_heap_block *block);
	free_heap_block *find_free_block(code_heap *heap, cell size);

	// code_block  ----------------------------------------------------------------------------

	/* code relocation table consists of a table of entries for each fixup */
	typedef u32 relocation_entry;
	
	void flush_icache_for(code_block *compiled);
	void iterate_relocations(code_block *compiled, relocation_iterator iter);
	void store_address_in_code_block(cell klass, cell offset, fixnum absolute_value);
	void mark_active_blocks(context *stacks);
	void mark_object_code_block(object *scan);
	int number_of_parameters(relocation_type type);
	void *object_xt(cell obj);
	void *xt_pic(word *w, cell tagged_quot);
	void *get_rel_symbol(array *literals, cell index);
	cell compute_relocation(relocation_entry rel, cell index, code_block *compiled);
	void store_address_masked(cell *ptr, fixnum value, cell mask, fixnum shift);
	void check_code_address(cell address);
	void mark_code_block(code_block *compiled);
	code_block *allot_code_block(cell size);
	code_block *add_code_block(cell type,
							   cell code_,
							   cell labels_,
							   cell relocation_,
							   cell literals_);
	void *word_xt_pic(word *w);
	void *word_xt_pic_tail(word *w);
	void update_literal_references(code_block *compiled);
	void fixup_labels(array *labels, code_block *compiled);


	// code_heap ----------------------------------------------------------------------------

	void init_code_heap(cell size);
	bool in_code_heap_p(cell ptr);
	void jit_compile_word(cell word, cell def, bool relocate);
	typedef void (*code_heap_iterator)(code_block *compiled, factorvm *vm);
	void iterate_code_heap(code_heap_iterator iter);
	void copy_code_heap_roots();
	void compact_code_heap();
	void update_code_heap_words();
	code_block *forward_xt(code_block *compiled);
	void forward_object_xts();

	inline void check_code_pointer(cell ptr)
	{
#ifdef FACTOR_DEBUG
		assert(in_code_heap_p(ptr));
#endif
	}

	// callstack ----------------------------------------------------------------------------

	void check_frame(stack_frame *frame);
	callstack *allot_callstack(cell size);
	stack_frame *fix_callstack_top(stack_frame *top, stack_frame *bottom);
	stack_frame *frame_successor(stack_frame *frame);
	code_block *frame_code(stack_frame *frame);
	cell frame_executing(stack_frame *frame);
	cell frame_scan(stack_frame *frame);
	cell frame_type(stack_frame *frame);
	stack_frame *capture_start();
	stack_frame *innermost_stack_frame(callstack *stack);
	stack_frame *innermost_stack_frame_quot(callstack *callstack);
	template<typename TYPE> void iterate_callstack(cell top, cell bottom, TYPE &iterator)
	{
		stack_frame *frame = (stack_frame *)bottom - 1;
		
		while((cell)frame >= top)
			{
				iterator(frame,this);
				frame = frame_successor(frame);
			}
	}

	template<typename TYPE> void iterate_callstack_object(callstack *stack_, TYPE &iterator);


	// alien  ----------------------------------------------------------------------------

	cell allot_alien(cell delegate, cell displacement);
	char *pinned_alien_offset(cell obj);
	

	// quotations   ------------------------------------------------------------------------
	void set_quot_xt(quotation *quot, code_block *code);
	void jit_compile(cell quot, bool relocate);
	fixnum quot_code_offset_to_scan(cell quot, cell offset);


	// dispatch   ------------------------------------------------------------------------

	cell lookup_hairy_method(cell obj, cell methods);
	cell lookup_tuple_method(cell obj, cell methods);
	cell lookup_method(cell object, cell methods);

	// inline cache ----------------------------------------------------------------------
	
	void init_inline_caching(int max_size);
	void deallocate_inline_cache(cell return_address);
	cell determine_inline_cache_type(array *cache_entries);
	void update_pic_count(cell type);
code_block *compile_inline_cache(fixnum index,
					cell generic_word_,
					cell methods_,
					cell cache_entries_,
								 bool tail_call_p);
	cell inline_cache_size(cell cache_entries);
	cell add_inline_cache_entry(cell cache_entries_, cell klass_, cell method_);
	void update_pic_transitions(cell pic_size);

	factorvm();
};

extern factorvm *vm;	 // Ideally we want to get rid of this singleton ptr completely



// globals to be removed (referenced in factor code)

// contexts.cpp
VM_C_API factor::context *stack_chain;  

// run.cpp
/* TAGGED user environment data; see getenv/setenv prims */

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