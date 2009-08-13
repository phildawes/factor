namespace factor
{

/* If a runtime function needs to call another function which potentially
allocates memory, it must wrap any local variable references to Factor
objects in gc_root instances */
extern segment *gc_locals_region;
extern cell gc_locals;

struct factorvm;

template <typename TYPE>
struct gc_root : public tagged<TYPE>
{
	DEFPUSHPOP(gc_local_,myvm->gc_locals)

	factorvm *myvm;
	void push() { check_tagged_pointer(tagged<TYPE>::value()); gc_local_push((cell)this); }
	
	//explicit gc_root(cell value_, factorvm *vm) : myvm(vm),tagged<TYPE>(value_) { push(); }
	explicit gc_root(cell value_,factorvm *vm) : tagged<TYPE>(value_),myvm(vm) { push(); }
	explicit gc_root(TYPE *value_, factorvm *vm) : tagged<TYPE>(value_),myvm(vm) { push(); }

	const gc_root<TYPE>& operator=(const TYPE *x) { tagged<TYPE>::operator=(x); return *this; }
	const gc_root<TYPE>& operator=(const cell &x) { tagged<TYPE>::operator=(x); return *this; }

	~gc_root() {
#ifdef FACTOR_DEBUG
		cell old = gc_local_pop();
		assert(old == (cell)this);
#else
		gc_local_pop();
#endif
	}
};

/* A similar hack for the bignum implementation */
extern segment *gc_bignums_region;
extern cell gc_bignums;


struct gc_bignum
{
	bignum **addr;
	factorvm *myvm;
	
	DEFPUSHPOP(gc_bignum_,vm->gc_bignums)

	//gc_bignum(bignum **addr_, factorvm *vm) : addr(addr_), myvm(vm) {
	gc_bignum(bignum **addr_) : addr(addr_), myvm(vm) {
		if(*addr_)
			check_data_pointer(*addr_);
		gc_bignum_push((cell)addr);
	}

	~gc_bignum() { assert((cell)addr == gc_bignum_pop()); }
};

#define GC_BIGNUM(x) gc_bignum x##__gc_root(&x)

}
