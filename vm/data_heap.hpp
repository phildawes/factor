namespace factor
{
/* generational copying GC divides memory into zones */
struct zone {
	/* allocation pointer is 'here'; its offset is hardcoded in the
	   compiler backends */
	cell start;
	cell here;
	cell size;
	cell end;
};

struct data_heap {
	segment *seg;

	cell young_size;
	cell aging_size;
	cell tenured_size;

	cell gen_count;

	zone *generations;
	zone *semispaces;

	cell *allot_markers;
	cell *allot_markers_end;

	cell *cards;
	cell *cards_end;

	cell *decks;
	cell *decks_end;
	
	/* the 0th generation is where new objects are allocated. */
	cell nursery() { return 0; }
	
	/* where objects hang around */
	cell aging() { return gen_count - 2; }
	
	/* the oldest generation */
	cell tenured() { return gen_count - 1; }
	
	bool have_aging_p() { return gen_count > 2; }

};

static const cell max_gen_count = 3;

inline static bool in_zone(zone *z, object *pointer)
{
	return (cell)pointer >= z->start && (cell)pointer < z->end;
}

cell init_zone(zone *z, cell size, cell base);

void dealloc_data_heap(data_heap *data);


/* set up guard pages to check for under/overflow.
   size must be a multiple of the page size */
segment *alloc_segment(cell size);
void dealloc_segment(segment *block);




PRIMITIVE(data_room);
PRIMITIVE(size);

PRIMITIVE(begin_scan);
PRIMITIVE(next_object);
PRIMITIVE(end_scan);


cell find_all_words();




}


/* new objects are allocated here */
VM_C_API factor::zone nursery;

/* TODO: failed attempt to make thread-local - remove me */
VM_C_API factor::zone *getnursery();
