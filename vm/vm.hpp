namespace factor
{
  struct heap; 
  

  struct factorvm {
    /* GC is off during heap walking */
    bool gc_off;
    /* Set by the -securegc command line argument */
    bool secure_gc;

    // data heap, data gc
    datacollector datagc;

    // code heap
    heap *code;
    unordered_map<heap_block *,char *> forwarding;  // should be in 'code'?

    // context
    cell ds_size, rs_size;
    context *unused_contexts;

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

    // image.cpp
    cell data_relocation_base;
    cell code_relocation_base;


    // inline_cache.cpp
    cell max_pic_size;    
    cell cold_call_to_ic_transitions;
    cell ic_to_pic_transitions;
    cell pic_to_mega_transitions;
    /* PIC_TAG, PIC_HI_TAG, PIC_TUPLE, PIC_HI_TAG_TUPLE */
    cell pic_counts[4];


    // local_roots.cpp
    segment *gc_locals_region;
    cell gc_locals;
    segment *gc_bignums_region;
    cell gc_bignums;

#ifdef __APPLE__
    // mach_signal.hpp
    /* The exception port on which our thread listens. */
    mach_port_t our_exception_port;
#endif

    // math.cpp
    cell bignum_zero;
    cell bignum_pos_one;
    cell bignum_neg_one;


    factorvm();

  };

  extern factorvm *vm;   // Ideally we want to get rid of this singleton ptr completely
}

// globals to be replaced

VM_C_API factor::context *stack_chain;  
