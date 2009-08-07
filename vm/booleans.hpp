namespace factor
{

inline cell factorvm::tag_boolean(cell untagged)
{
	return (untagged ? T : F);
}

VM_C_API void box_boolean(bool value);
VM_C_API bool to_boolean(cell value);

}
