namespace factor
{

/* If a runtime function needs to call another function which potentially
allocates memory, it must wrap any local variable references to Factor
objects in gc_root instances */

template <typename T>
struct gc_root : public tagged<T>
{
	factorvm *myvm;

	DEFPUSHPOP(gc_local2_,myvm->gc_locals)

	void push() { myvm->datagc->check_tagged_pointer(tagged<T>::value()); gc_local2_push((cell)this); }
	
	explicit gc_root(cell value_,factorvm *vm) : tagged<T>(value_) { myvm=vm; push(); }
	explicit gc_root(T *value_,factorvm *vm) : tagged<T>(value_) { myvm=vm; push(); }

	const gc_root<T>& operator=(const T *x) { tagged<T>::operator=(x); return *this; }
	const gc_root<T>& operator=(const cell &x) { tagged<T>::operator=(x); return *this; }

	~gc_root() {
#ifdef FACTOR_DEBUG
		cell old = gc_local2_pop();
		assert(old == (cell)this);
#else
		gc_local2_pop();
#endif
	}
};




struct gc_bignum
{
	bignum **addr;

	DEFPUSHPOP(gc_bignum_,vm->gc_bignums)

	gc_bignum(bignum **addr_) : addr(addr_) {
		if(*addr_)
			vm->datagc->check_data_pointer(*addr_);
		gc_bignum_push((cell)addr);
	}

	~gc_bignum() { assert((cell)addr == gc_bignum_pop()); }
};

#define GC_BIGNUM(x) gc_bignum x##__gc_root(&x)

}
