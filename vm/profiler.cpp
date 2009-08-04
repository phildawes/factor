#include "master.hpp"

namespace factor
{


void init_profiler()
{
	vm->profiling_p = false;
}

/* Allocates memory */
code_block *compile_profiling_stub(cell word_)
{
	gc_root2<word> word(word_,vm);

	jit jit(WORD_TYPE,word.value());
	jit.emit_with(userenv[JIT_PROFILING],word.value());

	return jit.to_code_block();
}

/* Allocates memory */
static void set_profiling(bool profiling)
{
	if(profiling == vm->profiling_p)
		return;

	vm->profiling_p = profiling;

	/* Push everything to tenured space so that we can heap scan
	and allocate profiling blocks if necessary */
	vm->datagc->gc();

	gc_root2<array> words(find_all_words(),vm);

	cell i;
	cell length = array_capacity(words.untagged());
	for(i = 0; i < length; i++)
	{
		tagged<word> word(array_nth(words.untagged(),i));
		if(profiling)
			word->counter = tag_fixnum(0);
		update_word_xt(word.value());
	}

	/* Update XTs in code heap */
	iterate_code_heap(relocate_code_block);
}

PRIMITIVE(profiling)
{
	set_profiling(to_boolean(dpop()));
}

}
