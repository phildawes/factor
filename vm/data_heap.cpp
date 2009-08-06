#include "master.hpp"

factor::zone nursery;

factor::zone *getnursery(){
  return &nursery;
}

namespace factor
{

/* new objects are allocated here */
VM_C_API zone nursery;


cell init_zone(zone *z, cell size, cell start)
{
	z->size = size;
	z->start = z->here = start;
	z->end = start + size;
	return z->end;
}

void data_heap::init_card_decks()
{
	cell start = align(seg->start,deck_size);
	vm->allot_markers_offset = (cell)allot_markers - (start >> card_bits);
	cards_offset = (cell)cards - (start >> card_bits);
	decks_offset = (cell)decks - (start >> deck_bits);
}

data_heap *data_heap::initial_setup(cell gens_,
			 cell young_size_,
			 cell aging_size_,
			 cell tenured_size_){

	young_size = young_size_;
	aging_size = aging_size_;
	tenured_size = tenured_size_;
	gen_count = gens_;

	cell total_size;
	if(gen_count == 2)
		total_size = young_size + 2 * tenured_size;
	else if(gen_count == 3)
		total_size = young_size + 2 * aging_size + 2 * tenured_size;
	else
	{
		vm->fatal_error("Invalid number of generations",gen_count);
		return NULL; /* can't happen */
	}

	total_size += deck_size;

	seg = alloc_segment(total_size);

	generations = (zone *)safe_malloc(sizeof(zone) * gen_count);
	semispaces = (zone *)safe_malloc(sizeof(zone) * gen_count);

	cell cards_size = total_size >> card_bits;
	allot_markers = (cell *)safe_malloc(cards_size);
	allot_markers_end = allot_markers + cards_size;

	cards = (cell *)safe_malloc(cards_size);
	cards_end = cards + cards_size;

	cell decks_size = total_size >> deck_bits;
	decks = (cell *)safe_malloc(decks_size);
	decks_end = decks + decks_size;

	cell alloter = align(seg->start,deck_size);

	alloter = init_zone(&generations[tenured()],tenured_size,alloter);
	alloter = init_zone(&semispaces[tenured()],tenured_size,alloter);

	if(gen_count == 3)
	{
		alloter = init_zone(&generations[aging()],aging_size,alloter);
		alloter = init_zone(&semispaces[aging()],aging_size,alloter);
	}

	if(gen_count >= 2)
	{
		alloter = init_zone(&generations[nursery()],young_size,alloter);
		alloter = init_zone(&semispaces[nursery()],0,alloter);
	}

	if(seg->end - alloter > deck_size)
		vm->critical_error("Bug in alloc_data_heap",alloter);

	return this;
}

data_heap *alloc_data_heap(cell gens,
	cell young_size,
	cell aging_size,
	cell tenured_size)
{
	young_size = align(young_size,deck_size);
	aging_size = align(aging_size,deck_size);
	tenured_size = align(tenured_size,deck_size);

	data_heap *data = (data_heap *)safe_malloc(sizeof(data_heap));
	return data->initial_setup(gens,young_size,aging_size,tenured_size);
}

data_heap *grow_data_heap(data_heap *data, cell requested_bytes)
{
	cell new_tenured_size = (data->tenured_size * 2) + requested_bytes;

	return alloc_data_heap(data->gen_count,
		data->young_size,
		data->aging_size,
		new_tenured_size);
}

void dealloc_data_heap(data_heap *data)
{
	dealloc_segment(data->seg);
	free(data->generations);
	free(data->semispaces);
	free(data->allot_markers);
	free(data->cards);
	free(data->decks);
	free(data);
}

void data_heap::clear_cards(cell from, cell to)
{
	/* NOTE: reverse order due to heap layout. */
	card *first_card = addr_to_card(generations[to].start);
	card *last_card = addr_to_card(generations[from].end);
	memset(first_card,0,last_card - first_card);
}

void data_heap::clear_decks(cell from, cell to)
{
	/* NOTE: reverse order due to heap layout. */
	card_deck *first_deck = addr_to_deck(generations[to].start);
	card_deck *last_deck = addr_to_deck(generations[from].end);
	memset(first_deck,0,last_deck - first_deck);
}

void data_heap::clear_allot_markers(cell from, cell to)
{
	/* NOTE: reverse order due to heap layout. */
	card *first_card = vm->addr_to_allot_marker((object *)generations[to].start);
	card *last_card = vm->addr_to_allot_marker((object *)generations[from].end);
	memset(first_card,invalid_allot_marker,last_card - first_card);
}

void data_heap::reset_generation(cell i)
{
  zone *z = (i == nursery() ? &factor::nursery : &generations[i]);

	z->here = z->start;
	if(vm->secure_gc)
		memset((void*)z->start,69,z->size);
}

/* After garbage collection, any generations which are now empty need to have
their allocation pointers and cards reset. */
void data_heap::reset_generations(cell from, cell to)
{
	cell i;
	for(i = from; i <= to; i++)
		reset_generation(i);

	clear_cards(from,to);
	clear_decks(from,to);
	clear_allot_markers(from,to);
}

void set_data_heap(data_heap *data_)
{
	vm->data = data_;
	nursery = vm->data->generations[vm->data->nursery()];
	vm->data->init_card_decks();
	vm->data->clear_cards(vm->data->nursery(),vm->data->tenured());
	vm->data->clear_decks(vm->data->nursery(),vm->data->tenured());
	vm->data->clear_allot_markers(vm->data->nursery(),vm->data->tenured());
}

void init_data_heap(cell gens,
	cell young_size,
	cell aging_size,
	cell tenured_size,
	bool secure_gc_)
{
	set_data_heap(alloc_data_heap(gens,young_size,aging_size,tenured_size));

	vm->gc_locals_region = alloc_segment(getpagesize());
	vm->gc_locals = vm->gc_locals_region->start - sizeof(cell);

	vm->gc_bignums_region = alloc_segment(getpagesize());
	vm->gc_bignums = vm->gc_bignums_region->start - sizeof(cell);

	vm->secure_gc = secure_gc_;

	vm->init_data_gc();
}

/* Size of the object pointed to by a tagged pointer */
cell object_size(cell tagged)
{
	if(immediate_p(tagged))
		return 0;
	else
		return untagged_object_size(untag<object>(tagged));
}

/* Size of the object pointed to by an untagged pointer */
cell untagged_object_size(object *pointer)
{
	return align8(unaligned_object_size(pointer));
}

/* Size of the data area of an object pointed to by an untagged pointer */
cell unaligned_object_size(object *pointer)
{
	switch(pointer->h.hi_tag())
	{
	case ARRAY_TYPE:
		return array_size((array*)pointer);
	case BIGNUM_TYPE:
		return array_size((bignum*)pointer);
	case BYTE_ARRAY_TYPE:
		return array_size((byte_array*)pointer);
	case STRING_TYPE:
		return string_size(string_capacity((string*)pointer));
	case TUPLE_TYPE:
		return tuple_size(untag<tuple_layout>(((tuple *)pointer)->layout));
	case QUOTATION_TYPE:
		return sizeof(quotation);
	case WORD_TYPE:
		return sizeof(word);
	case FLOAT_TYPE:
		return sizeof(boxed_float);
	case DLL_TYPE:
		return sizeof(dll);
	case ALIEN_TYPE:
		return sizeof(alien);
	case WRAPPER_TYPE:
		return sizeof(wrapper);
	case CALLSTACK_TYPE:
		return callstack_size(untag_fixnum(((callstack *)pointer)->length));
	default:
		vm->critical_error("Invalid header",(cell)pointer);
		return 0; /* can't happen */
	}
}

PRIMITIVE(size)
{
	box_unsigned_cell(object_size(dpop()));
}

/* The number of cells from the start of the object which should be scanned by
the GC. Some types have a binary payload at the end (string, word, DLL) which
we ignore. */
cell binary_payload_start(object *pointer)
{
	switch(pointer->h.hi_tag())
	{
	/* these objects do not refer to other objects at all */
	case FLOAT_TYPE:
	case BYTE_ARRAY_TYPE:
	case BIGNUM_TYPE:
	case CALLSTACK_TYPE:
		return 0;
	/* these objects have some binary data at the end */
	case WORD_TYPE:
		return sizeof(word) - sizeof(cell) * 3;
	case ALIEN_TYPE:
		return sizeof(cell) * 3;
	case DLL_TYPE:
		return sizeof(cell) * 2;
	case QUOTATION_TYPE:
		return sizeof(quotation) - sizeof(cell) * 2;
	case STRING_TYPE:
		return sizeof(string);
	/* everything else consists entirely of pointers */
	case ARRAY_TYPE:
		return array_size<array>(array_capacity((array*)pointer));
	case TUPLE_TYPE:
		return tuple_size(untag<tuple_layout>(((tuple *)pointer)->layout));
	case WRAPPER_TYPE:
		return sizeof(wrapper);
	default:
		vm->critical_error("Invalid header",(cell)pointer);
                return 0; /* can't happen */
	}
}

/* Push memory usage statistics in data heap */
PRIMITIVE(data_room)
{
	dpush(tag_fixnum((vm->data->cards_end - vm->data->cards) >> 10));
	dpush(tag_fixnum((vm->data->decks_end - vm->data->decks) >> 10));

	growable_array a;

	cell gen;
	for(gen = 0; gen < vm->data->gen_count; gen++)
	{
		zone *z = (gen == vm->data->nursery() ? &nursery : &vm->data->generations[gen]);
		a.add(tag_fixnum((z->end - z->here) >> 10));
		a.add(tag_fixnum((z->size) >> 10));
	}

	a.trim();
	dpush(a.elements.value());
}


/* Disables GC and activates next-object ( -- obj ) primitive */
void data_heap::begin_scan()
{
	heap_scan_ptr = vm->data->generations[vm->data->tenured()].start;
	vm->gc_off = true;
}

void data_heap::end_scan()
{
	vm->gc_off = false;
}

PRIMITIVE(begin_scan)
{
	vm->data->begin_scan();
}

cell data_heap::next_object()
{
	if(!vm->gc_off)
		vm->general_error(ERROR_HEAP_SCAN,F,F,NULL);

	if(heap_scan_ptr >= vm->data->generations[vm->data->tenured()].here)
		return F;

	object *obj = (object *)heap_scan_ptr;
	heap_scan_ptr += untagged_object_size(obj);
	return tag_dynamic(obj);
}

/* Push object at heap scan cursor and advance; pushes f when done */
PRIMITIVE(next_object)
{
	dpush(vm->data->next_object());
}

/* Re-enables GC */
PRIMITIVE(end_scan)
{
	vm->gc_off = false;
}

template<typename T> void each_object(T &functor)
{
	vm->data->begin_scan();
	cell obj;
	while((obj = vm->data->next_object()) != F)
		functor(tagged<object>(obj));
	vm->data->end_scan();
}

namespace
{

struct word_counter {
	cell count;
	word_counter() : count(0) {}
	void operator()(tagged<object> obj) { if(obj.type_p(WORD_TYPE)) count++; }
};

struct word_accumulator {
	growable_array words;
	word_accumulator(int count) : words(count) {}
	void operator()(tagged<object> obj) { if(obj.type_p(WORD_TYPE)) words.add(obj.value()); }
};

}

cell find_all_words()
{
	word_counter counter;
	each_object(counter);
	word_accumulator accum(counter.count);
	each_object(accum);
	accum.words.trim();
	return accum.words.elements.value();
}

}
