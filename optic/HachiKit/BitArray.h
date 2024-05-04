#ifndef BITARRAY_H
#define BITARRAY_H


// typedef and convenient functions for treating a 16-bit uint as a bit array
// does not change the array in the func, but returns the modified form
typedef uint16_t bit_array_16;

bool BitArray16_Get(bit_array_16 array, u8 index);
bit_array_16 BitArray16_Set(bit_array_16 array, u8 index, bool value);
bit_array_16 BitArray16_Toggle(bit_array_16 array, u8 index);


// typedef and convenient functions for a 4-bit (nibble) array
// nib array is treated as unsigned
// even though value is set/returned as a u8, only lower 4 bits are used
// these functions change the array in place
// 
// typedef u64 nib_array_16;   // <-- this did not work properly on arduino
struct nib_array_16 {
  u32 lo = 0;
  u32 hi = 0;
};

u8 NibArray16_Get(nib_array_16& array, u8 index);
void NibArray16_Set(nib_array_16& array, u8 index, u8 value);

// these functions treat the nibble as a 4-bit signed 2s-complement value (i.e. where 1111 = -1)
// s8 SignedNib(u8 value);
// u8 UnsignedNib(s8 value);
// s8 NibArray16_GetSigned(nib_array_16& array, u8 index);
// void NibArray16_SetSigned(nib_array_16& array, u8 index, s8 value);

// these functions treat the nibble as a 1-bit flag and a 3-bit unsigned value
bool NibArray16_GetFlag(nib_array_16& array, u8 index);
u8 NibArray16_GetValue(nib_array_16& array, u8 index);
void NibArray16_SetFlag(nib_array_16& array, u8 index, bool flag);
void NibArray16_ToggleFlag(nib_array_16& array, u8 index);
void NibArray16_SetValue(nib_array_16& array, u8 index, u8 value);




#endif
