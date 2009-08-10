#include "master.hpp"

namespace factor
{

PRIMITIVE(bignum_to_fixnum)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	drepl(tag_fixnum(myvm->bignum_to_fixnum(untag<bignum>(dpeek()))));
}

PRIMITIVE(float_to_fixnum)
{
	drepl(tag_fixnum(float_to_fixnum(dpeek())));
}

/* Division can only overflow when we are dividing the most negative fixnum
by -1. */
PRIMITIVE(fixnum_divint)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	fixnum y = untag_fixnum(dpop()); \
	fixnum x = untag_fixnum(dpeek());
	fixnum result = x / y;
	if(result == -fixnum_min)
		drepl(myvm->allot_integer(-fixnum_min));
	else
		drepl(tag_fixnum(result));
}

PRIMITIVE(fixnum_divmod)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	cell y = ((cell *)ds)[0];
	cell x = ((cell *)ds)[-1];
	if(y == tag_fixnum(-1) && x == tag_fixnum(fixnum_min))
	{
		((cell *)ds)[-1] = myvm->allot_integer(-fixnum_min);
		((cell *)ds)[0] = tag_fixnum(0);
	}
	else
	{
		((cell *)ds)[-1] = tag_fixnum(untag_fixnum(x) / untag_fixnum(y));
		((cell *)ds)[0] = (fixnum)x % (fixnum)y;
	}
}

/*
 * If we're shifting right by n bits, we won't overflow as long as none of the
 * high WORD_SIZE-TAG_BITS-n bits are set.
 */
static inline fixnum sign_mask(fixnum x)
{
	return x >> (WORD_SIZE - 1);
}

static inline fixnum branchless_max(fixnum x, fixnum y)
{
	return (x - ((x - y) & sign_mask(x - y)));
}

static inline fixnum branchless_abs(fixnum x)
{
	return (x ^ sign_mask(x)) - sign_mask(x);
}

PRIMITIVE(fixnum_shift)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	fixnum y = untag_fixnum(dpop());
	fixnum x = untag_fixnum(dpeek());

	if(x == 0)
		return;
	else if(y < 0)
	{
		y = branchless_max(y,-WORD_SIZE + 1);
		drepl(tag_fixnum(x >> -y));
		return;
	}
	else if(y < WORD_SIZE - TAG_BITS)
	{
		fixnum mask = -((fixnum)1 << (WORD_SIZE - 1 - TAG_BITS - y));
		if(!(branchless_abs(x) & mask))
		{
			drepl(tag_fixnum(x << y));
			return;
		}
	}

	drepl(tag<bignum>(myvm->bignum_arithmetic_shift(
		myvm->fixnum_to_bignum(x),y)));
}

PRIMITIVE(fixnum_to_bignum)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	drepl(tag<bignum>(myvm->fixnum_to_bignum(untag_fixnum(dpeek()))));
}

PRIMITIVE(float_to_bignum)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	drepl(tag<bignum>(myvm->float_to_bignum(dpeek())));
}

#define POP_BIGNUMS(x,y) \
	bignum * y = untag<bignum>(dpop()); \
	bignum * x = untag<bignum>(dpop());

PRIMITIVE(bignum_eq)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	box_boolean(myvm->bignum_equal_p(x,y));
}

PRIMITIVE(bignum_add)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	dpush(tag<bignum>(myvm->bignum_add(x,y)));
}

PRIMITIVE(bignum_subtract)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	dpush(tag<bignum>(myvm->bignum_subtract(x,y)));
}

PRIMITIVE(bignum_multiply)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	dpush(tag<bignum>(myvm->bignum_multiply(x,y)));
}

PRIMITIVE(bignum_divint)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	dpush(tag<bignum>(myvm->bignum_quotient(x,y)));
}

PRIMITIVE(bignum_divmod)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	bignum *q, *r;
	POP_BIGNUMS(x,y);
	myvm->bignum_divide(x,y,&q,&r);
	dpush(tag<bignum>(q));
	dpush(tag<bignum>(r));
}

PRIMITIVE(bignum_mod)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	dpush(tag<bignum>(myvm->bignum_remainder(x,y)));
}

PRIMITIVE(bignum_and)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	dpush(tag<bignum>(myvm->bignum_bitwise_and(x,y)));
}

PRIMITIVE(bignum_or)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	dpush(tag<bignum>(myvm->bignum_bitwise_ior(x,y)));
}

PRIMITIVE(bignum_xor)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	dpush(tag<bignum>(myvm->bignum_bitwise_xor(x,y)));
}

PRIMITIVE(bignum_shift)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	fixnum y = untag_fixnum(dpop());
        bignum* x = untag<bignum>(dpop());
	dpush(tag<bignum>(myvm->bignum_arithmetic_shift(x,y)));
}

PRIMITIVE(bignum_less)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	box_boolean(myvm->bignum_compare(x,y) == bignum_comparison_less);
}

PRIMITIVE(bignum_lesseq)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	box_boolean(myvm->bignum_compare(x,y) != bignum_comparison_greater);
}

PRIMITIVE(bignum_greater)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	box_boolean(myvm->bignum_compare(x,y) == bignum_comparison_greater);
}

PRIMITIVE(bignum_greatereq)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	POP_BIGNUMS(x,y);
	box_boolean(myvm->bignum_compare(x,y) != bignum_comparison_less);
}

PRIMITIVE(bignum_not)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	drepl(tag<bignum>(myvm->bignum_bitwise_not(untag<bignum>(dpeek()))));
}

PRIMITIVE(bignum_bitp)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	fixnum bit = to_fixnum(dpop());
	bignum *x = untag<bignum>(dpop());
	box_boolean(myvm->bignum_logbitp(bit,x));
}

PRIMITIVE(bignum_log2)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	drepl(tag<bignum>(myvm->bignum_integer_length(untag<bignum>(dpeek()))));
}

unsigned int bignum_producer(unsigned int digit)
{
	unsigned char *ptr = (unsigned char *)alien_offset(dpeek());
	return *(ptr + digit);
}

PRIMITIVE(byte_array_to_bignum)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	cell n_digits = array_capacity(untag_check<byte_array>(dpeek(),myvm));
	bignum * result = myvm->digit_stream_to_bignum(n_digits,bignum_producer,0x100,0);
	drepl(tag<bignum>(result));
}

cell factorvm::unbox_array_size()
{
	switch(tagged<object>(dpeek()).type())
	{
	case FIXNUM_TYPE:
		{
			fixnum n = untag_fixnum(dpeek());
			if(n >= 0 && n < (fixnum)array_size_max)
			{
				dpop();
				return n;
			}
			break;
		}
	case BIGNUM_TYPE:
		{
			bignum * zero = untag<bignum>(bignum_zero);
			bignum * max = cell_to_bignum(array_size_max);
			bignum * n = untag<bignum>(dpeek());
			if(bignum_compare(n,zero) != bignum_comparison_less
				&& bignum_compare(n,max) == bignum_comparison_less)
			{
				dpop();
				return bignum_to_cell(n);
			}
			break;
		}
	}

	vm->general_error(ERROR_ARRAY_SIZE,dpop(),tag_fixnum(array_size_max),NULL);
	return 0; /* can't happen */
}

PRIMITIVE(fixnum_to_float)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	drepl(myvm->allot_float(fixnum_to_float(dpeek())));
}

PRIMITIVE(bignum_to_float)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	drepl(myvm->allot_float(myvm->bignum_to_float(dpeek())));
}

PRIMITIVE(str_to_float)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	byte_array *bytes = untag_check<byte_array>(dpeek(),myvm);
	cell capacity = array_capacity(bytes);

	char *c_str = (char *)(bytes + 1);
	char *end = c_str;
	double f = strtod(c_str,&end);
	if(end == c_str + capacity - 1)
		drepl(myvm->allot_float(f));
	else
		drepl(F);
}

PRIMITIVE(float_to_str)
{
	factorvm *myvm = PRIMITIVE_GETVM();
	byte_array *array = myvm->allot_byte_array(33);
	snprintf((char *)(array + 1),32,"%.16g",untag_float_check(dpop()));
	dpush(tag<byte_array>(array));
}

#define POP_FLOATS(x,y) \
	double y = untag_float(dpop()); \
	double x = untag_float(dpop());

PRIMITIVE(float_eq)
{
	POP_FLOATS(x,y);
	box_boolean(x == y);
}

PRIMITIVE(float_add)
{
	POP_FLOATS(x,y);
	box_double(x + y);
}

PRIMITIVE(float_subtract)
{
	POP_FLOATS(x,y);
	box_double(x - y);
}

PRIMITIVE(float_multiply)
{
	POP_FLOATS(x,y);
	box_double(x * y);
}

PRIMITIVE(float_divfloat)
{
	POP_FLOATS(x,y);
	box_double(x / y);
}

PRIMITIVE(float_mod)
{
	POP_FLOATS(x,y);
	box_double(fmod(x,y));
}

PRIMITIVE(float_less)
{
	POP_FLOATS(x,y);
	box_boolean(x < y);
}

PRIMITIVE(float_lesseq)
{
	POP_FLOATS(x,y);
	box_boolean(x <= y);
}

PRIMITIVE(float_greater)
{
	POP_FLOATS(x,y);
	box_boolean(x > y);
}

PRIMITIVE(float_greatereq)
{
	POP_FLOATS(x,y);
	box_boolean(x >= y);
}

PRIMITIVE(float_bits)
{
	box_unsigned_4(float_bits(untag_float_check(dpop())));
}

PRIMITIVE(bits_float)
{
	box_float(bits_float(to_cell(dpop())));
}

PRIMITIVE(double_bits)
{
	box_unsigned_8(double_bits(untag_float_check(dpop())));
}

PRIMITIVE(bits_double)
{
	box_double(bits_double(to_unsigned_8(dpop())));
}

VM_C_API fixnum to_fixnum(cell tagged)
{
	switch(TAG(tagged))
	{
	case FIXNUM_TYPE:
		return untag_fixnum(tagged);
	case BIGNUM_TYPE:
		return vm->bignum_to_fixnum(untag<bignum>(tagged));
	default:
		vm->type_error(FIXNUM_TYPE,tagged);
		return 0; /* can't happen */
	}
}

VM_C_API cell to_cell(cell tagged)
{
	return (cell)to_fixnum(tagged);
}

VM_C_API void box_signed_1(s8 n)
{
	dpush(tag_fixnum(n));
}

VM_C_API void box_unsigned_1(u8 n)
{
	dpush(tag_fixnum(n));
}

VM_C_API void box_signed_2(s16 n)
{
	dpush(tag_fixnum(n));
}

VM_C_API void box_unsigned_2(u16 n)
{
	dpush(tag_fixnum(n));
}

VM_C_API void box_signed_4(s32 n)
{
	dpush(vm->allot_integer(n));
}

VM_C_API void box_unsigned_4(u32 n)
{
	dpush(vm->allot_cell(n));
}

VM_C_API void box_signed_cell(fixnum integer)
{
	dpush(vm->allot_integer(integer));
}

VM_C_API void box_unsigned_cell(cell cell)
{
	dpush(vm->allot_cell(cell));
}

VM_C_API void box_signed_8(s64 n)
{
	if(n < fixnum_min || n > fixnum_max)
		dpush(tag<bignum>(vm->long_long_to_bignum(n)));
	else
		dpush(tag_fixnum(n));
}

VM_C_API s64 to_signed_8(cell obj)
{
	switch(tagged<object>(obj).type())
	{
	case FIXNUM_TYPE:
		return untag_fixnum(obj);
	case BIGNUM_TYPE:
		return vm->bignum_to_long_long(untag<bignum>(obj));
	default:
		vm->type_error(BIGNUM_TYPE,obj);
		return 0;
	}
}

VM_C_API void box_unsigned_8(u64 n)
{
	if(n > (u64)fixnum_max)
		dpush(tag<bignum>(vm->ulong_long_to_bignum(n)));
	else
		dpush(tag_fixnum(n));
}

VM_C_API u64 to_unsigned_8(cell obj)
{
	switch(tagged<object>(obj).type())
	{
	case FIXNUM_TYPE:
		return untag_fixnum(obj);
	case BIGNUM_TYPE:
		return vm->bignum_to_ulong_long(untag<bignum>(obj));
	default:
		vm->type_error(BIGNUM_TYPE,obj);
		return 0;
	}
}

VM_C_API void box_float(float flo)
{
        dpush(vm->allot_float(flo));
}

VM_C_API float to_float(cell value)
{
	return untag_float_check(value);
}

VM_C_API void box_double(double flo)
{
        dpush(vm->allot_float(flo));
}

VM_C_API double to_double(cell value)
{
	return untag_float_check(value);
}

/* The fixnum+, fixnum- and fixnum* primitives are defined in cpu_*.S. On
overflow, they call these functions. */
VM_ASM_API void overflow_fixnum_add(fixnum x, fixnum y)
{
	drepl(tag<bignum>(vm->fixnum_to_bignum(
		untag_fixnum(x) + untag_fixnum(y))));
}

VM_ASM_API void overflow_fixnum_subtract(fixnum x, fixnum y)
{
	drepl(tag<bignum>(vm->fixnum_to_bignum(
		untag_fixnum(x) - untag_fixnum(y))));
}

VM_ASM_API void overflow_fixnum_multiply(fixnum x, fixnum y)
{
	bignum *bx = vm->fixnum_to_bignum(x);
	GC_BIGNUM(bx,vm);
	bignum *by = vm->fixnum_to_bignum(y);
	GC_BIGNUM(by,vm);
	drepl(tag<bignum>(vm->bignum_multiply(bx,by)));
}

}
