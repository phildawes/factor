#include "master.hpp"

namespace factor
{

cell megamorphic_cache_hits;
cell megamorphic_cache_misses;

cell factorvm::search_lookup_alist(cell table, cell klass)
{
	array *elements = untag<array>(table);
	fixnum index = array_capacity(elements) - 2;
	while(index >= 0)
	{
		if(array_nth(elements,index) == klass)
			return array_nth(elements,index + 1);
		else
			index -= 2;
	}

	return F;
}

cell search_lookup_alist(cell table, cell klass)
{
	return vm->search_lookup_alist(table,klass);
}

cell factorvm::search_lookup_hash(cell table, cell klass, cell hashcode)
{
	array *buckets = untag<array>(table);
	cell bucket = array_nth(buckets,hashcode & (array_capacity(buckets) - 1));
	if(tagged<object>(bucket).type_p(WORD_TYPE) || bucket == F)
		return bucket;
	else
		return search_lookup_alist(bucket,klass);
}

cell search_lookup_hash(cell table, cell klass, cell hashcode)
{
	return vm->search_lookup_hash(table,klass,hashcode);
}

cell factorvm::nth_superclass(tuple_layout *layout, fixnum echelon)
{
	cell *ptr = (cell *)(layout + 1);
	return ptr[echelon * 2];
}

cell nth_superclass(tuple_layout *layout, fixnum echelon)
{
	return vm->nth_superclass(layout,echelon);
}

cell factorvm::nth_hashcode(tuple_layout *layout, fixnum echelon)
{
	cell *ptr = (cell *)(layout + 1);
	return ptr[echelon * 2 + 1];
}

cell nth_hashcode(tuple_layout *layout, fixnum echelon)
{
	return vm->nth_hashcode(layout,echelon);
}

cell factorvm::lookup_tuple_method(cell obj, cell methods)
{
	tuple_layout *layout = untag<tuple_layout>(untag<tuple>(obj)->layout);

	array *echelons = untag<array>(methods);

	fixnum echelon = untag_fixnum(layout->echelon);
	fixnum max_echelon = array_capacity(echelons) - 1;
	if(echelon > max_echelon) echelon = max_echelon;
       
	while(echelon >= 0)
	{
		cell echelon_methods = array_nth(echelons,echelon);

		if(tagged<object>(echelon_methods).type_p(WORD_TYPE))
			return echelon_methods;
		else if(echelon_methods != F)
		{
			cell klass = nth_superclass(layout,echelon);
			cell hashcode = untag_fixnum(nth_hashcode(layout,echelon));
			cell result = search_lookup_hash(echelon_methods,klass,hashcode);
			if(result != F)
				return result;
		}

		echelon--;
	}

	critical_error("Cannot find tuple method",methods);
	return F;
}

cell lookup_tuple_method(cell obj, cell methods)
{
	return vm->lookup_tuple_method(obj,methods);
}

cell factorvm::lookup_hi_tag_method(cell obj, cell methods)
{
	array *hi_tag_methods = untag<array>(methods);
	cell tag = untag<object>(obj)->h.hi_tag() - HEADER_TYPE;
#ifdef FACTOR_DEBUG
	assert(tag < TYPE_COUNT - HEADER_TYPE);
#endif
	return array_nth(hi_tag_methods,tag);
}

cell lookup_hi_tag_method(cell obj, cell methods)
{
	return vm->lookup_hi_tag_method(obj,methods);
}

cell factorvm::lookup_hairy_method(cell obj, cell methods)
{
	cell method = array_nth(untag<array>(methods),TAG(obj));
	if(tagged<object>(method).type_p(WORD_TYPE))
		return method;
	else
	{
		switch(TAG(obj))
		{
		case TUPLE_TYPE:
			return lookup_tuple_method(obj,method);
			break;
		case OBJECT_TYPE:
			return lookup_hi_tag_method(obj,method);
			break;
		default:
			critical_error("Bad methods array",methods);
			return 0;
		}
	}
}

cell lookup_hairy_method(cell obj, cell methods)
{
	return vm->lookup_hairy_method(obj,methods);
}

cell factorvm::lookup_method(cell obj, cell methods)
{
	cell tag = TAG(obj);
	if(tag == TUPLE_TYPE || tag == OBJECT_TYPE)
		return lookup_hairy_method(obj,methods);
	else
		return array_nth(untag<array>(methods),TAG(obj));
}

cell lookup_method(cell obj, cell methods)
{
	return vm->lookup_method(obj,methods);
}

inline void factorvm::vmprim_lookup_method()
{
	cell methods = dpop();
	cell obj = dpop();
	dpush(lookup_method(obj,methods));
}

PRIMITIVE(lookup_method)
{
	PRIMITIVE_GETVM()->vmprim_lookup_method();
}

cell factorvm::object_class(cell obj)
{
	switch(TAG(obj))
	{
	case TUPLE_TYPE:
		return untag<tuple>(obj)->layout;
	case OBJECT_TYPE:
		return untag<object>(obj)->h.value;
	default:
		return tag_fixnum(TAG(obj));
	}
}

cell object_class(cell obj)
{
	return vm->object_class(obj);
}

cell factorvm::method_cache_hashcode(cell klass, array *array)
{
	cell capacity = (array_capacity(array) >> 1) - 1;
	return ((klass >> TAG_BITS) & capacity) << 1;
}

cell method_cache_hashcode(cell klass, array *array)
{
	return vm->method_cache_hashcode(klass,array);
}

void factorvm::update_method_cache(cell cache, cell klass, cell method)
{
	array *cache_elements = untag<array>(cache);
	cell hashcode = method_cache_hashcode(klass,cache_elements);
	set_array_nth(cache_elements,hashcode,klass);
	set_array_nth(cache_elements,hashcode + 1,method);
}

void update_method_cache(cell cache, cell klass, cell method)
{
	return vm->update_method_cache(cache,klass,method);
}

inline void factorvm::vmprim_mega_cache_miss()
{
	megamorphic_cache_misses++;

	cell cache = dpop();
	fixnum index = untag_fixnum(dpop());
	cell methods = dpop();

	cell object = ((cell *)ds)[-index];
	cell klass = object_class(object);
	cell method = lookup_method(object,methods);

	update_method_cache(cache,klass,method);

	dpush(method);
}

PRIMITIVE(mega_cache_miss)
{
	PRIMITIVE_GETVM()->vmprim_mega_cache_miss();
}

inline void factorvm::vmprim_reset_dispatch_stats()
{
	megamorphic_cache_hits = megamorphic_cache_misses = 0;
}

PRIMITIVE(reset_dispatch_stats)
{
	PRIMITIVE_GETVM()->vmprim_reset_dispatch_stats();
}

inline void factorvm::vmprim_dispatch_stats()
{
	growable_array stats(this);
	stats.add(allot_cell(megamorphic_cache_hits));
	stats.add(allot_cell(megamorphic_cache_misses));
	stats.trim();
	dpush(stats.elements.value());
}

PRIMITIVE(dispatch_stats)
{
	PRIMITIVE_GETVM()->vmprim_dispatch_stats();
}

void quotation_jit::emit_mega_cache_lookup(cell methods_, fixnum index, cell cache_)
{
	gc_root<array> methods(methods_,myvm);
	gc_root<array> cache(cache_,myvm);

	/* Generate machine code to determine the object's class. */
	emit_class_lookup(index,PIC_HI_TAG_TUPLE);

	/* Do a cache lookup. */
	emit_with(userenv[MEGA_LOOKUP],cache.value());
	
	/* If we end up here, the cache missed. */
	emit(userenv[JIT_PROLOG]);

	/* Push index, method table and cache on the stack. */
	push(methods.value());
	push(tag_fixnum(index));
	push(cache.value());
	word_call(userenv[MEGA_MISS_WORD]);

	/* Now the new method has been stored into the cache, and its on
	   the stack. */
	emit(userenv[JIT_EPILOG]);
	emit(userenv[JIT_EXECUTE_JUMP]);
}

}
