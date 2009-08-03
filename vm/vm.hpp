namespace factor
{
  struct factorvm {
    /* GC is off during heap walking */
    bool gc_off;
    /* Set by the -securegc command line argument */
    bool secure_gc;

    datacollector datagc;

  };

  extern factorvm *vm;
}
