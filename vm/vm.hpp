namespace factor
{
  struct heap; 

  struct factorvm {
    /* GC is off during heap walking */
    bool gc_off;
    /* Set by the -securegc command line argument */
    bool secure_gc;

    datacollector datagc;

    heap *code;
    unordered_map<heap_block *,char *> forwarding;  // should be in 'code'?


    cell ds_size, rs_size;
    context *unused_contexts;


    factorvm();

  };

  extern factorvm *vm;
}
