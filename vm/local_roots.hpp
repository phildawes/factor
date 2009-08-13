namespace factor
{

/* If a runtime function needs to call another function which potentially
allocates memory, it must wrap any local variable references to Factor
objects in gc_root instances */
extern segment *gc_locals_region;
extern cell gc_locals;

/* A similar hack for the bignum implementation */
extern segment *gc_bignums_region;
extern cell gc_bignums;

}
