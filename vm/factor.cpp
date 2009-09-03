#include "master.hpp"

namespace factor
{

factorvm *vm;

void init_globals()
{
	init_platform_globals();
}

void factorvm::default_parameters(vm_parameters *p)
{
	p->image_path = NULL;

	/* We make a wild guess here that if we're running on ARM, we don't
	have a lot of memory. */
#ifdef FACTOR_ARM
	p->ds_size = 8 * sizeof(cell);
	p->rs_size = 8 * sizeof(cell);

	p->gen_count = 2;
	p->code_size = 4;
	p->young_size = 1;
	p->aging_size = 1;
	p->tenured_size = 6;
#else
	p->ds_size = 32 * sizeof(cell);
	p->rs_size = 32 * sizeof(cell);

	p->gen_count = 3;
	p->code_size = 8 * sizeof(cell);
	p->young_size = sizeof(cell) / 4;
	p->aging_size = sizeof(cell) / 2;
	p->tenured_size = 4 * sizeof(cell);
#endif

	p->max_pic_size = 3;

	p->secure_gc = false;
	p->fep = false;

#ifdef WINDOWS
	p->console = false;
#else
	if (this == vm)
		p->console = true;
	else 		
		p->console = false;
	
#endif

	p->stack_traces = true;
}

bool factorvm::factor_arg(const vm_char* str, const vm_char* arg, cell* value)
{
	int val;
	if(SSCANF(str,arg,&val) > 0)
	{
		*value = val;
		return true;
	}
	else
		return false;
}

void factorvm::init_parameters_from_args(vm_parameters *p, int argc, vm_char **argv)
{
	default_parameters(p);
	p->executable_path = argv[0];

	int i = 0;

	for(i = 1; i < argc; i++)
	{
		if(factor_arg(argv[i],STRING_LITERAL("-datastack=%d"),&p->ds_size));
		else if(factor_arg(argv[i],STRING_LITERAL("-retainstack=%d"),&p->rs_size));
		else if(factor_arg(argv[i],STRING_LITERAL("-generations=%d"),&p->gen_count));
		else if(factor_arg(argv[i],STRING_LITERAL("-young=%d"),&p->young_size));
		else if(factor_arg(argv[i],STRING_LITERAL("-aging=%d"),&p->aging_size));
		else if(factor_arg(argv[i],STRING_LITERAL("-tenured=%d"),&p->tenured_size));
		else if(factor_arg(argv[i],STRING_LITERAL("-codeheap=%d"),&p->code_size));
		else if(factor_arg(argv[i],STRING_LITERAL("-pic=%d"),&p->max_pic_size));
		else if(STRCMP(argv[i],STRING_LITERAL("-securegc")) == 0) p->secure_gc = true;
		else if(STRCMP(argv[i],STRING_LITERAL("-fep")) == 0) p->fep = true;
		else if(STRNCMP(argv[i],STRING_LITERAL("-i="),3) == 0) p->image_path = argv[i] + 3;
		else if(STRCMP(argv[i],STRING_LITERAL("-console")) == 0) p->console = true;
		else if(STRCMP(argv[i],STRING_LITERAL("-no-stack-traces")) == 0) p->stack_traces = false;
	}
}

/* Do some initialization that we do once only */
void factorvm::do_stage1_init()
{
	print_string("*** Stage 2 early init... ");
	fflush(stdout);

	compile_all_words();
	userenv[STAGE2_ENV] = T;

	print_string("done\n");
	fflush(stdout);
}

void factorvm::init_factor(vm_parameters *p)
{
	/* Kilobytes */
	p->ds_size = align_page(p->ds_size << 10);
	p->rs_size = align_page(p->rs_size << 10);

	/* Megabytes */
	p->young_size <<= 20;
	p->aging_size <<= 20;
	p->tenured_size <<= 20;
	p->code_size <<= 20;

	/* Disable GC during init as a sanity check */
	gc_off = true;

	/* OS-specific initialization */
	early_init();

	const vm_char *executable_path = vm_executable_path();

	if(executable_path)
		p->executable_path = executable_path;

	if(p->image_path == NULL)
		p->image_path = default_image_path();

	srand(current_micros());
	init_ffi();
	init_stacks(p->ds_size,p->rs_size);
	load_image(p);
	init_c_io();
	init_inline_caching(p->max_pic_size);
	init_signals();

	if(p->console)
		open_console();

	init_profiler();

	userenv[CPU_ENV] = allot_alien(F,(cell)FACTOR_CPU_STRING);
	userenv[OS_ENV] = allot_alien(F,(cell)FACTOR_OS_STRING);
	userenv[CELL_SIZE_ENV] = tag_fixnum(sizeof(cell));
	userenv[EXECUTABLE_ENV] = allot_alien(F,(cell)p->executable_path);
	userenv[ARGS_ENV] = F;
	userenv[EMBEDDED_ENV] = F;

	/* We can GC now */
	gc_off = false;

	if(userenv[STAGE2_ENV] == F)
	{
		userenv[STACK_TRACES_ENV] = tag_boolean(p->stack_traces);
		do_stage1_init();
	}
}

/* May allocate memory */
void factorvm::pass_args_to_factor(int argc, vm_char **argv)
{
	growable_array args(this);
	int i;

	for(i = 1; i < argc; i++){
		args.add(allot_alien(F,(cell)argv[i]));
	}

	args.trim();
	userenv[ARGS_ENV] = args.elements.value();
}

void factorvm::start_factor(vm_parameters *p)
{
	if(p->fep) factorbug();

	nest_stacks();
	c_to_factor_toplevel(userenv[BOOT_ENV]);
	unnest_stacks();
}


char *factorvm::factor_eval_string(char *string)
{
	char *(*callback)(char *) = (char *(*)(char *))alien_offset(userenv[EVAL_CALLBACK_ENV]);
	return callback(string);
}

void factorvm::factor_eval_free(char *result)
{
	free(result);
}

void factorvm::factor_yield()
{
	void (*callback)() = (void (*)())alien_offset(userenv[YIELD_CALLBACK_ENV]);
	callback();
}

void factorvm::factor_sleep(long us)
{
	void (*callback)(long) = (void (*)(long))alien_offset(userenv[SLEEP_CALLBACK_ENV]);
	callback(us);
}

void factorvm::start_standalone_factor(int argc, vm_char **argv)
{
	vm_parameters p;
	default_parameters(&p);
	init_parameters_from_args(&p,argc,argv);
	init_factor(&p);
	pass_args_to_factor(argc,argv);
	start_factor(&p);
}

struct startargs {
	int argc;
	vm_char **argv;
};

void* start_standalone_factor_thread(void *arg) 
{
	factorvm *newvm = new factorvm;
	register_vm_with_thread(newvm);
	startargs *args = (startargs*) arg;
	newvm->start_standalone_factor(args->argc, args->argv);
	return 0;
}


VM_C_API void start_standalone_factor(int argc, vm_char **argv)
{
	factorvm *newvm = new factorvm;
	newvm->print_vm_data();
	printf("PHIL YEAH: %d %d %d %d\n",(void*)(newvm),(void*)(newvm+1),sizeof(newvm), sizeof(factorvm));
	vm = newvm;
	register_vm_with_thread(newvm);
	return newvm->start_standalone_factor(argc,argv);
}

VM_C_API THREADHANDLE start_standalone_factor_in_new_thread(int argc, vm_char **argv)
{
	startargs *args = new startargs;   // leaks startargs structure
	args->argc = argc; args->argv = argv;
	return start_thread(start_standalone_factor_thread,args);
}


void factorvm::print_vm_data() {
	printf("PHIL: stack_chain %d\n",&stack_chain);
	printf("PHIL: nursery %d\n",&nursery);
	printf("PHIL: cards_offset %d\n",&cards_offset);
	printf("PHIL: decks_offset %d\n",&decks_offset);
	printf("PHIL: userenv %d\n",&userenv);
	printf("PHIL: ds_size %d\n",&ds_size);
	printf("PHIL: rs_size %d\n",&rs_size);
	printf("PHIL: unused_contexts %d\n",&unused_contexts);
	printf("PHIL: T %d\n",&T);
	printf("PHIL: profiling_p %d\n",&profiling_p);
	printf("PHIL: signal_number %d\n",&signal_number);
	printf("PHIL: signal_fault_addr %d\n",&signal_fault_addr);
	printf("PHIL: signal_callstack_top %d\n",&signal_callstack_top);
	printf("PHIL: secure_gc %d\n",&secure_gc);
	printf("PHIL: gc_off %d\n",&gc_off);
	printf("PHIL: data %d\n",&data);
	printf("PHIL: heap_scan_ptr %d\n",&heap_scan_ptr);
	printf("PHIL: allot_markers_offset %d\n",&allot_markers_offset);
	printf("PHIL: newspace %d\n",&newspace);
	printf("PHIL: performing_gc %d\n",&performing_gc);
	printf("PHIL: performing_compaction %d\n",&performing_compaction);
	printf("PHIL: collecting_gen %d\n",&collecting_gen);
	printf("PHIL: collecting_aging_again %d\n",&collecting_aging_again);
	printf("PHIL: gc_jmp %d\n",&gc_jmp);
	printf("PHIL: stats %d\n",&stats);
	printf("PHIL: cards_scanned %d\n",&cards_scanned);
	printf("PHIL: decks_scanned %d\n",&decks_scanned);
	printf("PHIL: card_scan_time %d\n",&card_scan_time);
	printf("PHIL: code_heap_scans %d\n",&code_heap_scans);
	printf("PHIL: last_code_heap_scan %d\n",&last_code_heap_scan);
	printf("PHIL: growing_data_heap %d\n",&growing_data_heap);
	printf("PHIL: old_data_heap %d\n",&old_data_heap);
	printf("PHIL: gc_locals_region %d\n",&gc_locals_region);
	printf("PHIL: gc_locals %d\n",&gc_locals);
	printf("PHIL: gc_bignums_region %d\n",&gc_bignums_region);
	printf("PHIL: gc_bignums %d\n",&gc_bignums);
	printf("PHIL: fep_disabled %d\n",&fep_disabled);
	printf("PHIL: full_output %d\n",&full_output);
	printf("PHIL: look_for %d\n",&look_for);
	printf("PHIL: obj %d\n",&obj);
	printf("PHIL: bignum_zero %d\n",&bignum_zero);
	printf("PHIL: bignum_pos_one %d\n",&bignum_pos_one);
	printf("PHIL: bignum_neg_one %d\n",&bignum_neg_one);
	printf("PHIL: code %d\n",&code);
	printf("PHIL: forwarding %d\n",&forwarding);
	printf("PHIL: code_relocation_base %d\n",&code_relocation_base);
	printf("PHIL: data_relocation_base %d\n",&data_relocation_base);
	printf("PHIL: megamorphic_cache_hits %d\n",&megamorphic_cache_hits);
	printf("PHIL: megamorphic_cache_misses %d\n",&megamorphic_cache_misses);
	printf("PHIL: max_pic_size %d\n",&max_pic_size);
	printf("PHIL: cold_call_to_ic_transitions %d\n",&cold_call_to_ic_transitions);
	printf("PHIL: ic_to_pic_transitions %d\n",&ic_to_pic_transitions);
	printf("PHIL: pic_to_mega_transitions %d\n",&pic_to_mega_transitions);
	printf("PHIL: pic_counts %d\n",&pic_counts);
}

	// if you change this struct, also change vm.factor k--------
	context *stack_chain; 
	zone nursery; /* new objects are allocated here */
	cell cards_offset;
	cell decks_offset;
	cell userenv[USER_ENV]; /* TAGGED user environment data; see getenv/setenv prims */

	// -------------------------------

	// contexts
	cell ds_size, rs_size;
	context *unused_contexts;

	// run
	cell T;  /* Canonical T object. It's just a word */

	// profiler
	bool profiling_p;

	// errors
	/* Global variables used to pass fault handler state from signal handler to
	   user-space */
	cell signal_number;
	cell signal_fault_addr;
	stack_frame *signal_callstack_top;

	//data_heap
	bool secure_gc;  /* Set by the -securegc command line argument */
	bool gc_off; /* GC is off during heap walking */
	data_heap *data;
	/* A heap walk allows useful things to be done, like finding all
	   references to an object for debugging purposes. */
	cell heap_scan_ptr;
	//write barrier
	cell allot_markers_offset;
	//data_gc
	/* used during garbage collection only */
	zone *newspace;
	bool performing_gc;
	bool performing_compaction;
	cell collecting_gen;
	/* if true, we are collecting aging space for the second time, so if it is still
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

	// local roots
	/* If a runtime function needs to call another function which potentially
	   allocates memory, it must wrap any local variable references to Factor
	   objects in gc_root instances */
	segment *gc_locals_region;
	cell gc_locals;
	segment *gc_bignums_region;
	cell gc_bignums;

	//debug
	bool fep_disabled;
	bool full_output;
	cell look_for;
	cell obj;

	//math
	cell bignum_zero;
	cell bignum_pos_one;
	cell bignum_neg_one;	

	//code_heap
	heap code;
	unordered_map<heap_block *,char *> forwarding;

	//image
	cell code_relocation_base;
	cell data_relocation_base;

	//dispatch
	cell megamorphic_cache_hits;
	cell megamorphic_cache_misses;

	//inline cache
	cell max_pic_size;
	cell cold_call_to_ic_transitions;
	cell ic_to_pic_transitions;
	cell pic_to_mega_transitions;
	cell pic_counts[4];  /* PIC_TAG, PIC_HI_TAG, PIC_TUPLE, PIC_HI_TAG_TUPLE */
}
