namespace factor
{

struct factorvm {

	// contexts
	void reset_datastack();
	void reset_retainstack();
	void fix_stacks();
	void save_stacks();
	context *alloc_context();
	void dealloc_context(context *old_context);
	void nest_stacks();
	void unnest_stacks();
	void init_stacks(cell ds_size_, cell rs_size_);
	bool stack_to_array(cell bottom, cell top);
	cell array_to_stack(array *array, cell bottom);
	inline void vmprim_datastack();
	inline void vmprim_retainstack();
	inline void vmprim_set_datastack();
	inline void vmprim_set_retainstack();
	inline void vmprim_check_datastack();

	// run
	inline void vmprim_getenv();
	inline void vmprim_setenv();
	inline void vmprim_exit();
	inline void vmprim_micros();
	inline void vmprim_sleep();
	inline void vmprim_set_slot();
	inline void vmprim_load_locals();
	cell clone_object(cell obj_);
	inline void vmprim_clone();

	// profiler
	void init_profiler();
	code_block *compile_profiling_stub(cell word_);
	void set_profiling(bool profiling);
	inline void vmprim_profiling();

	// errors
	void out_of_memory();
	void fatal_error(const char* msg, cell tagged);
	void critical_error(const char* msg, cell tagged);
	void throw_error(cell error, stack_frame *callstack_top);
	void not_implemented_error();
	bool in_page(cell fault, cell area, cell area_size, int offset);
	void memory_protection_error(cell addr, stack_frame *native_stack);
	void signal_error(int signal, stack_frame *native_stack);
	void divide_by_zero_error();
	inline void vmprim_call_clear();
	inline void vmprim_unimplemented();
	void memory_signal_handler_impl();
	void misc_signal_handler_impl();
	void type_error(cell type, cell tagged);
	void general_error(vm_error_type error, cell arg1, cell arg2, stack_frame *callstack_top);

	// bignum
	int bignum_equal_p(bignum * x, bignum * y);
	enum bignum_comparison bignum_compare(bignum * x, bignum * y);
	bignum *bignum_add(bignum * x, bignum * y);
	bignum *bignum_subtract(bignum * x, bignum * y);
	bignum *bignum_multiply(bignum * x, bignum * y);
	void bignum_divide(bignum * numerator, bignum * denominator, bignum * * quotient, bignum * * remainder);
	bignum *bignum_quotient(bignum * numerator, bignum * denominator);
	bignum *bignum_remainder(bignum * numerator, bignum * denominator);
	double bignum_to_double(bignum * bignum);
	bignum *double_to_bignum(double x);
	int bignum_equal_p_unsigned(bignum * x, bignum * y);
	enum bignum_comparison bignum_compare_unsigned(bignum * x, bignum * y);
	bignum *bignum_add_unsigned(bignum * x, bignum * y, int negative_p);
	bignum *bignum_subtract_unsigned(bignum * x, bignum * y);
	bignum *bignum_multiply_unsigned(bignum * x, bignum * y, int negative_p);
	bignum *bignum_multiply_unsigned_small_factor(bignum * x, bignum_digit_type y,int negative_p);
	void bignum_destructive_add(bignum * bignum, bignum_digit_type n);
	void bignum_destructive_scale_up(bignum * bignum, bignum_digit_type factor);
	void bignum_divide_unsigned_large_denominator(bignum * numerator, bignum * denominator, 
												  bignum * * quotient, bignum * * remainder, int q_negative_p, int r_negative_p);
	void bignum_divide_unsigned_normalized(bignum * u, bignum * v, bignum * q);
	bignum_digit_type bignum_divide_subtract(bignum_digit_type * v_start, bignum_digit_type * v_end, 
											 bignum_digit_type guess, bignum_digit_type * u_start);
	void bignum_divide_unsigned_medium_denominator(bignum * numerator,bignum_digit_type denominator, 
												   bignum * * quotient, bignum * * remainder,int q_negative_p, int r_negative_p);
	void bignum_destructive_normalization(bignum * source, bignum * target, int shift_left);
	void bignum_destructive_unnormalization(bignum * bignum, int shift_right);
	bignum_digit_type bignum_digit_divide(bignum_digit_type uh, bignum_digit_type ul, 
										  bignum_digit_type v, bignum_digit_type * q) /* return value */;
	bignum_digit_type bignum_digit_divide_subtract(bignum_digit_type v1, bignum_digit_type v2, 
												   bignum_digit_type guess, bignum_digit_type * u);
	void bignum_divide_unsigned_small_denominator(bignum * numerator, bignum_digit_type denominator, 
												  bignum * * quotient, bignum * * remainder,int q_negative_p, int r_negative_p);
	bignum_digit_type bignum_destructive_scale_down(bignum * bignum, bignum_digit_type denominator);
	bignum * bignum_remainder_unsigned_small_denominator(bignum * n, bignum_digit_type d, int negative_p);
	bignum *bignum_digit_to_bignum(bignum_digit_type digit, int negative_p);
	bignum *allot_bignum(bignum_length_type length, int negative_p);
	bignum * allot_bignum_zeroed(bignum_length_type length, int negative_p);
	bignum *bignum_shorten_length(bignum * bignum, bignum_length_type length);
	bignum *bignum_trim(bignum * bignum);
	bignum *bignum_new_sign(bignum * x, int negative_p);
	bignum *bignum_maybe_new_sign(bignum * x, int negative_p);
	void bignum_destructive_copy(bignum * source, bignum * target);
	bignum *bignum_bitwise_not(bignum * x);
	bignum *bignum_arithmetic_shift(bignum * arg1, fixnum n);
	bignum *bignum_bitwise_and(bignum * arg1, bignum * arg2);
	bignum *bignum_bitwise_ior(bignum * arg1, bignum * arg2);
	bignum *bignum_bitwise_xor(bignum * arg1, bignum * arg2);
	bignum *bignum_magnitude_ash(bignum * arg1, fixnum n);
	bignum *bignum_pospos_bitwise_op(int op, bignum * arg1, bignum * arg2);
	bignum *bignum_posneg_bitwise_op(int op, bignum * arg1, bignum * arg2);
	bignum *bignum_negneg_bitwise_op(int op, bignum * arg1, bignum * arg2);
	void bignum_negate_magnitude(bignum * arg);
	bignum *bignum_integer_length(bignum * x);
	int bignum_logbitp(int shift, bignum * arg);
	int bignum_unsigned_logbitp(int shift, bignum * bignum);
	bignum *digit_stream_to_bignum(unsigned int n_digits, unsigned int (*producer)(unsigned int), unsigned int radix, int negative_p);

	//data_heap
	cell init_zone(zone *z, cell size, cell start);
	void init_card_decks();
	data_heap *alloc_data_heap(cell gens, cell young_size,cell aging_size,cell tenured_size);
	data_heap *grow_data_heap(data_heap *data, cell requested_bytes);
	void dealloc_data_heap(data_heap *data);
	void clear_cards(cell from, cell to);
	void clear_decks(cell from, cell to);
	void clear_allot_markers(cell from, cell to);
	void reset_generation(cell i);
	void reset_generations(cell from, cell to);
	void set_data_heap(data_heap *data_);
	void init_data_heap(cell gens,cell young_size,cell aging_size,cell tenured_size,bool secure_gc_);
	cell untagged_object_size(object *pointer);
	cell unaligned_object_size(object *pointer);
	inline void vmprim_size();
	cell binary_payload_start(object *pointer);
	inline void vmprim_data_room();
	void begin_scan();
	void end_scan();
	inline void vmprim_begin_scan();
	cell next_object();
	inline void vmprim_next_object();
	inline void vmprim_end_scan();
	template<typename T> void each_object(T &functor);
	cell find_all_words();

	//data_gc
	void init_data_gc();
	object *copy_untagged_object_impl(object *pointer, cell size);
	object *copy_object_impl(object *untagged);
	bool should_copy_p(object *untagged);
	object *resolve_forwarding(object *untagged);
	template <typename T> T *copy_untagged_object(T *untagged);
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
	void garbage_collection(cell gen,bool growing_data_heap_,cell requested_bytes);
	void gc();
	inline void vmprim_gc();
	inline void vmprim_gc_stats();
	void clear_gc_stats();
	inline void vmprim_become();
	void inline_gc(cell *gc_roots_base, cell gc_roots_size);

	//debug
	void print_chars(string* str);
	void print_word(word* word, cell nesting);
	void print_factor_string(string* str);
	void print_array(array* array, cell nesting);
	void print_tuple(tuple *tuple, cell nesting);
	void print_nested_obj(cell obj, fixnum nesting);
	void print_obj(cell obj);
	void print_objects(cell *start, cell *end);
	void print_datastack();
	void print_retainstack();
	void print_stack_frame(stack_frame *frame);
	void print_callstack();
	void dump_cell(cell x);
	void dump_memory(cell from, cell to);
	void dump_zone(zone *z);
	void dump_generations();
	void dump_objects(cell type);
	void find_data_references_step(cell *scan);
	void find_data_references(cell look_for_);
	void dump_code_heap();
	void factorbug();
	inline void vmprim_die();

	//arrays
	array *allot_array(cell capacity, cell fill_);
	inline void vmprim_array();
	cell allot_array_1(cell obj_);
	cell allot_array_2(cell v1_, cell v2_);
	cell allot_array_4(cell v1_, cell v2_, cell v3_, cell v4_);
	inline void vmprim_resize_array();

	//strings
	cell string_nth(string* str, cell index);
	void set_string_nth_fast(string *str, cell index, cell ch);
	void set_string_nth_slow(string *str_, cell index, cell ch);
	void set_string_nth(string *str, cell index, cell ch);
	string *allot_string_internal(cell capacity);
	void fill_string(string *str_, cell start, cell capacity, cell fill);
	string *allot_string(cell capacity, cell fill);
	inline void vmprim_string();
	bool reallot_string_in_place_p(string *str, cell capacity);
	string* reallot_string(string *str_, cell capacity);
	inline void vmprim_resize_string();
	inline void vmprim_string_nth();
	inline void vmprim_set_string_nth_fast();
	inline void vmprim_set_string_nth_slow();

	//booleans
	void box_boolean(bool value);
	bool to_boolean(cell value);

	//byte arrays
	byte_array *allot_byte_array(cell size);
	inline void vmprim_byte_array();
	inline void vmprim_uninitialized_byte_array();
	inline void vmprim_resize_byte_array();

	//tuples
	tuple *allot_tuple(cell layout_);
	inline void vmprim_tuple();
	inline void vmprim_tuple_boa();

	//words
	word *allot_word(cell vocab_, cell name_);
	inline void vmprim_word();
	inline void vmprim_word_xt();
	void update_word_xt(cell w_);
	inline void vmprim_optimized_p();
	inline void vmprim_wrapper();

	//math
	inline void vmprim_bignum_to_fixnum();
	inline void vmprim_float_to_fixnum();
	inline void vmprim_fixnum_divint();
	inline void vmprim_fixnum_divmod();
	inline fixnum sign_mask(fixnum x);
	inline fixnum branchless_max(fixnum x, fixnum y);
	inline fixnum branchless_abs(fixnum x);
	inline void vmprim_fixnum_shift();
	inline void vmprim_fixnum_to_bignum();
	inline void vmprim_float_to_bignum();
	inline void vmprim_bignum_eq();
	inline void vmprim_bignum_add();
	inline void vmprim_bignum_subtract();
	inline void vmprim_bignum_multiply();
	inline void vmprim_bignum_divint();
	inline void vmprim_bignum_divmod();
	inline void vmprim_bignum_mod();
	inline void vmprim_bignum_and();
	inline void vmprim_bignum_or();
	inline void vmprim_bignum_xor();
	inline void vmprim_bignum_shift();
	inline void vmprim_bignum_less();
	inline void vmprim_bignum_lesseq();
	inline void vmprim_bignum_greater();
	inline void vmprim_bignum_greatereq();
	inline void vmprim_bignum_not();
	inline void vmprim_bignum_bitp();
	inline void vmprim_bignum_log2();
	unsigned int bignum_producer(unsigned int digit);
	inline void vmprim_byte_array_to_bignum();
	cell unbox_array_size();
	inline void vmprim_fixnum_to_float();
	inline void vmprim_bignum_to_float();
	inline void vmprim_str_to_float();
	inline void vmprim_float_to_str();
	inline void vmprim_float_eq();
	inline void vmprim_float_add();
	inline void vmprim_float_subtract();
	inline void vmprim_float_multiply();
	inline void vmprim_float_divfloat();
	inline void vmprim_float_mod();
	inline void vmprim_float_less();
	inline void vmprim_float_lesseq();
	inline void vmprim_float_greater();
	inline void vmprim_float_greatereq();
	inline void vmprim_float_bits();
	inline void vmprim_bits_float();
	inline void vmprim_double_bits();
	inline void vmprim_bits_double();
	fixnum to_fixnum(cell tagged);
	cell to_cell(cell tagged);
	void box_signed_1(s8 n);
	void box_unsigned_1(u8 n);
	void box_signed_2(s16 n);
	void box_unsigned_2(u16 n);
	void box_signed_4(s32 n);
	void box_unsigned_4(u32 n);
	void box_signed_cell(fixnum integer);
	void box_unsigned_cell(cell cell);
	void box_signed_8(s64 n);
	s64 to_signed_8(cell obj);
	void box_unsigned_8(u64 n);
	u64 to_unsigned_8(cell obj);
	void box_float(float flo);
	float to_float(cell value);
	void box_double(double flo);
	double to_double(cell value);
	void overflow_fixnum_add(fixnum x, fixnum y);
	void overflow_fixnum_subtract(fixnum x, fixnum y);
	void overflow_fixnum_multiply(fixnum x, fixnum y);
	
	//io
	void init_c_io();
	void io_error();
	inline void vmprim_fopen();
	inline void vmprim_fgetc();
	inline void vmprim_fread();
	inline void vmprim_fputc();
	inline void vmprim_fwrite();
	inline void vmprim_fseek();
	inline void vmprim_fflush();
	inline void vmprim_fclose();
	int err_no();
	void clear_err_no();

	//code_gc
	void clear_free_list(heap *heap);
	void new_heap(heap *heap, cell size);
	void add_to_free_list(heap *heap, free_heap_block *block);
	void build_free_list(heap *heap, cell size);
	void assert_free_block(free_heap_block *block);
	free_heap_block *find_free_block(heap *heap, cell size);
	free_heap_block *split_free_block(heap *heap, free_heap_block *block, cell size);
	heap_block *heap_allot(heap *heap, cell size);
	void heap_free(heap *heap, heap_block *block);
	void mark_block(heap_block *block);
	void unmark_marked(heap *heap);
	void free_unmarked(heap *heap, heap_iterator iter);
	void heap_usage(heap *heap, cell *used, cell *total_free, cell *max_free);
	cell heap_size(heap *heap);
	cell compute_heap_forwarding(heap *heap, unordered_map<heap_block *,char *> &forwarding);
	void compact_heap(heap *heap, unordered_map<heap_block *,char *> &forwarding);
	// next method here:




};

extern factorvm *vm;

}
