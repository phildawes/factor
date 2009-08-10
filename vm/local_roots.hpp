namespace factor
{

/* If a runtime function needs to call another function which potentially
allocates memory, it must wrap any local variable references to Factor
objects in gc_root instances */

template <typename T>
struct gc_root : public tagged<T>
{
	factorvm *myvm;

	DEFPUSHPOP(gc_local_,myvm->gc_locals)

	T *untag_check() const {   // suckage - couldn't work out how to get compiler to delegate to tagged::untag_check(myvm)
		return tagged<T>::untag_check(myvm);
	}

	void push() { myvm->check_tagged_pointer(tagged<T>::value()); gc_local_push((cell)this); }
	
	explicit gc_root(cell value_,factorvm *vm) : tagged<T>(value_), myvm(vm) { push(); }
	explicit gc_root(T *value_,factorvm *vm) : tagged<T>(value_), myvm(vm) { push(); }

	const gc_root<T>& operator=(const T *x) { tagged<T>::operator=(x); return *this; }
	const gc_root<T>& operator=(const cell &x) { tagged<T>::operator=(x); return *this; }

	~gc_root() {
#ifdef FACTOR_DEBUG
		cell old = gc_local_pop();
		assert(old == (cell)this);
#else
		gc_local_pop();
#endif
	}
};




struct gc_bignum
{
	factorvm *myvm;
	bignum **addr;

	DEFPUSHPOP(gc_bignum_,myvm->gc_bignums)

	gc_bignum(bignum **addr_,factorvm *vm) : myvm(vm), addr(addr_) {
		if(*addr_)
			myvm->check_data_pointer(*addr_);
		gc_bignum_push((cell)addr);
	}

	~gc_bignum() { assert((cell)addr == gc_bignum_pop()); }
};

#define GC_BIGNUM(x,vm) gc_bignum x##__gc_root(&x,vm)

}
