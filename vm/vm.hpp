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


    factorvm();

  };

  extern factorvm *vm;   // Ideally we want to get rid of this singleton ptr completely
}

// globals to be replaced

VM_C_API factor::context *stack_chain;  
