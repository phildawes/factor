#include "master.hpp"

namespace factor
{

word *allot_word(cell vocab_, cell name_)
{
	gc_root<object> vocab(vocab_,vm);
	gc_root<object> name(name_,vm);

	gc_root<word> new_word(vm->allot<word>(sizeof(word)),vm);

	new_word->hashcode = tag_fixnum((rand() << 16) ^ rand());
	new_word->vocabulary = vocab.value();
	new_word->name = name.value();
	new_word->def = userenv[UNDEFINED_ENV];
	new_word->props = F;
	new_word->counter = tag_fixnum(0);
	new_word->pic_def = F;
	new_word->pic_tail_def = F;
	new_word->subprimitive = F;
	new_word->profiling = NULL;
	new_word->code = NULL;

	jit_compile_word(new_word.value(),new_word->def,true);
	update_word_xt(new_word.value());

	if(vm->profiling_p)
		relocate_code_block(new_word->profiling,vm);

	return new_word.untagged();
}

/* <word> ( name vocabulary -- word ) */
PRIMITIVE(word)
{
	cell vocab = dpop();
	cell name = dpop();
	dpush(tag<word>(allot_word(vocab,name)));
}

/* word-xt ( word -- start end ) */
PRIMITIVE(word_xt)
{
	word *w = untag_check<word>(dpop(),vm);
	code_block *code = (vm->profiling_p ? w->profiling : w->code);
	dpush(allot_cell((cell)code->xt()));
	dpush(allot_cell((cell)code + code->size));
}

/* Allocates memory */
void update_word_xt(cell w_)
{
	gc_root<word> w(w_,vm);

	if(vm->profiling_p)
	{
		if(!w->profiling)
			w->profiling = vm->compile_profiling_stub(w.value());

		w->xt = w->profiling->xt();
	}
	else
		w->xt = w->code->xt();
}

PRIMITIVE(optimized_p)
{
	drepl(tag_boolean(word_optimized_p(untag_check<word>(dpeek(),vm))));
}

PRIMITIVE(wrapper)
{
	wrapper *new_wrapper = vm->allot<wrapper>(sizeof(wrapper));
	new_wrapper->object = dpeek();
	drepl(tag<wrapper>(new_wrapper));
}

}
