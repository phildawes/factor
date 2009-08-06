namespace factor
{

template <typename T> bool reallot_array_in_place_p(T *array, cell capacity)
{
  return in_zone(getnursery(),array) && capacity <= array_capacity(array);
}

template <typename TYPE> TYPE *factorvm::reallot_array(TYPE *array_, cell capacity)
{
	gc_root<TYPE> array(array_,this);

	if(reallot_array_in_place_p(array.untagged(),capacity))
	{
		array->capacity = tag_fixnum(capacity);
		return array.untagged();
	}
	else
	{
		cell to_copy = array_capacity(array.untagged());
		if(capacity < to_copy)
			to_copy = capacity;

		TYPE *new_array = allot_array_internal<TYPE>(capacity);
	
		memcpy(new_array + 1,array.untagged() + 1,to_copy * TYPE::element_size);
		memset((char *)(new_array + 1) + to_copy * TYPE::element_size,
			0,(capacity - to_copy) * TYPE::element_size);

		return new_array;
	}
}

}
