namespace factor
{

/* statistics */
struct gc_stats {
	cell collections;
	u64 gc_time;
	u64 max_gc_time;
	cell object_count;
	u64 bytes_copied;
};

/* We leave this many bytes free at the top of the nursery so that inline
allocation (which does not call GC because of possible roots in volatile
registers) does not run out of memory */
static const cell allot_buffer_zone = 1024;

struct datacollector {

/* used during garbage collection only */
  zone *newspace;
  bool performing_gc;
  bool performing_compaction;
  cell collecting_gen;

/* if true, we collecting aging space for the second time, so if it is still
full, we go on to collect tenured */
  bool collecting_aging_again;

/* in case a generation fills up in the middle of a gc, we jump back
up to try collecting the next generation. */
  jmp_buf gc_jmp;
  
  gc_stats stats[max_gen_count];
  u64 cards_scanned;
  u64 decks_scanned;
  u64 card_scan_time;
  cell code_heap_scans;


/* What generation was being collected when copy_code_heap_roots() was last
called? Until the next call to add_code_block(), future
collections of younger generations don't have to touch the code
heap. */
  cell last_code_heap_scan;

/* sometimes we grow the heap */
  bool growing_data_heap;
  data_heap *old_data_heap;


  void init_data_gc();
  void gc();
  object *copy_untagged_object_impl(object *pointer, cell size);
  object *copy_object_impl(object *untagged);
  bool should_copy_p(object *untagged);
  object *resolve_forwarding(object *untagged);
  cell copy_object(cell pointer);
  void copy_handle(cell *handle);
  void copy_card(card *ptr, cell gen, cell here);
  void copy_card_deck(card_deck *deck, cell gen, card mask, card unmask);
  void copy_gen_cards(cell gen);
  void copy_cards();
  void copy_stack_elements(segment *region, cell top);
  void copy_registered_locals();
  void copy_registered_bignums();
  void copy_roots();
  cell copy_next_from_nursery(cell scan);
  cell copy_next_from_aging(cell scan);
  cell copy_next_from_tenured(cell scan);
  void copy_reachable_objects(cell scan, cell *end);
  void begin_gc(cell requested_bytes);
  void end_gc(cell gc_elapsed);
  void garbage_collection(cell gen,
			bool growing_data_heap_,
			cell requested_bytes);
  void clear_gc_stats();
  template <typename T> T *copy_untagged_object(T *untagged);


inline bool collecting_accumulation_gen_p()
{
	return ((vm->data->have_aging_p()
		&& collecting_gen == vm->data->aging()
		&& !collecting_aging_again)
		|| collecting_gen == vm->data->tenured());
}


inline object *allot_zone(zone *z, cell a)
{
	cell h = z->here;
	z->here = h + align8(a);
	object *obj = (object *)h;
	allot_barrier(obj);
	return obj;
}

inline void check_data_pointer(object *pointer)
{
#ifdef FACTOR_DEBUG
	if(!growing_data_heap)
	{
		assert((cell)pointer >= vm->data->seg->start
		       vm->&& (cell)pointer < data->seg->end);
	}
#endif
}

inline void check_tagged_pointer(cell tagged)
{
#ifdef FACTOR_DEBUG
	if(!immediate_p(tagged))
	{
		object *obj = untag<object>(tagged);
		check_data_pointer(obj);
		obj->h.hi_tag();
	}
#endif
}

/*
 * It is up to the caller to fill in the object's fields in a meaningful
 * fashion!
 */
inline object *allot_object(header header, cell size)
{
#ifdef GC_DEBUG
	if(!gc_off)
		gc();
#endif

	object *obj;
	factor::zone *nursery = getnursery();
	if(nursery->size - allot_buffer_zone > size)
	{
		/* If there is insufficient room, collect the nursery */
	  if(nursery->here + allot_buffer_zone + size > nursery->end)
			garbage_collection(vm->data->nursery(),false,0);

		cell h = nursery->here;
		nursery->here = h + align8(size);
		obj = (object *)h;
	}
	/* If the object is bigger than the nursery, allocate it in
	tenured space */
	else
	{
		zone *tenured = &vm->data->generations[vm->data->tenured()];

		/* If tenured space does not have enough room, collect */
		if(tenured->here + size > tenured->end)
		{
			gc();
			tenured = &vm->data->generations[vm->data->tenured()];
		}

		/* If it still won't fit, grow the heap */
		if(tenured->here + size > tenured->end)
		{
			garbage_collection(vm->data->tenured(),true,size);
			tenured = &vm->data->generations[vm->data->tenured()];
		}

		obj = allot_zone(tenured,size);

		/* Allows initialization code to store old->new pointers
		without hitting the write barrier in the common case of
		a nursery allocation */
		write_barrier(obj);
	}

	obj->h = header;
	return obj;
}

template<typename T> T *allot(cell size)
{
	return (T *)allot_object(header(T::type_number),size);
}

}; // end datacollector

extern datacollector *coll ; // nasty singleton


PRIMITIVE(gc);
PRIMITIVE(gc_stats);
PRIMITIVE(clear_gc_stats);
PRIMITIVE(become);

VM_ASM_API void inline_gc(cell *gc_roots_base, cell gc_roots_size);

}
