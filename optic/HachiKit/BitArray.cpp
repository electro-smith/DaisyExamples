#include "BitArray.h"

static const uint16_t mask = 1;
static const uint64_t nibMask = 15;

/***** bit array functions ******************************/

bool BitArray16_Get(bit_array_16 array, u8 index) {
  return (array & (mask << index)) != 0;
}

bit_array_16 BitArray16_Set(bit_array_16 array, u8 index, bool value) {
  if (value) {
    return array | (mask << index);
  } else {
    return array & ~(mask << index);
  }
}

bit_array_16 BitArray16_Toggle(bit_array_16 array, u8 index) {
  return array ^ (mask << index);
}


/***** nibble array functions ******************************/

u8 NibArray16_Get(nib_array_16& array, u8 index) {
  if (index < 8) {
    return (array.lo & (nibMask << (index*4))) >> (index*4);
  }
  index -= 8;
  return (array.hi & (nibMask << (index*4))) >> (index*4);
}

void NibArray16_Set(nib_array_16& array, u8 index, u8 value) {
  if (index < 8) {
    array.lo = (array.lo & ~(nibMask << (index*4))) | (value << (index*4));
  }
  index -= 8;
  array.hi = (array.hi & ~(nibMask << (index*4))) | (value << (index*4));
}

bool NibArray16_GetFlag(nib_array_16& array, u8 index) {
  return (NibArray16_Get(array, index) & 8) != 0;
}

u8 NibArray16_GetValue(nib_array_16& array, u8 index) {
  return NibArray16_Get(array, index) & 7;
}
void NibArray16_SetFlag(nib_array_16& array, u8 index, bool flag) {
  u8 value = NibArray16_GetValue(array, index);
  NibArray16_Set(array, index, value | (flag<<3));
}

void NibArray16_ToggleFlag(nib_array_16& array, u8 index) {
  bool toggleFlag = !NibArray16_GetFlag(array, index);
  u8 value = NibArray16_GetValue(array, index);
  NibArray16_Set(array, index, value | (toggleFlag<<3));
}

void NibArray16_SetValue(nib_array_16& array, u8 index, u8 value) {
  bool flag = NibArray16_GetFlag(array, index);
  return NibArray16_Set(array, index, value | (flag<<3));
}

// /***** nibble array functions ******************************/

// u8 NibArray16_Get(nib_array_16& array, u8 index) {
//   // SERIALPRINTLN("NA16_Get: idx=" + String(index));
//   return (array & (nibMask << (index*4))) >> (index*4);
// }

// void NibArray16_Set(nib_array_16& array, u8 index, u8 value) {
//   // SERIALPRINTLN("NA16_Set: idx=" + String(index));
//   array = (array & ~(nibMask << (index*4))) | (value << (index*4));
// }

// // s8 SignedNib(u8 value) {
// //   return value < 8 ? value : value - 16;
// // }

// // u8 UnsignedNib(s8 value) {
// //   return value >= 0 ? value : value + 16;
// // }

// // s8 NibArray16_GetSigned(nib_array_16& array, u8 index) {
// //   u8 value = NibArray16_Get(array, index);
// //   s8 signedValue = SignedNib(value);
// //   return signedValue;
// // }

// // void NibArray16_SetSigned(nib_array_16& array, u8 index, s8 value) {
// //   u8 unsignedValue = UnsignedNib(value);
// //   NibArray16_Set(array, index, unsignedValue);
// // }

// bool NibArray16_GetFlag(nib_array_16& array, u8 index) {
//   // SERIALPRINTLN("NA16_GetFlag: idx=" + String(index) + ", nib=" + String(NibArray16_Get(array, index)) + ", flag=" + String((NibArray16_Get(array, index) & 8) != 0));
//   return (NibArray16_Get(array, index) & 8) != 0;
// }

// u8 NibArray16_GetValue(nib_array_16& array, u8 index) {
//   // SERIALPRINTLN("NA16_GetValue: idx=" + String(index) +  ", nib=" + String(NibArray16_Get(array, index)) + ", flag=" + String(NibArray16_Get(array, index) & 7));
//   return NibArray16_Get(array, index) & 7;
// }
// void NibArray16_SetFlag(nib_array_16& array, u8 index, bool flag) {
//   u8 value = NibArray16_GetValue(array, index);
//   // SERIALPRINTLN("NA16_SetFlag: idx=" + String(index) + ", value=" + String(value) + ", flag=" + String(flag));
//   NibArray16_Set(array, index, value | (flag<<3));
// }

// void NibArray16_ToggleFlag(nib_array_16& array, u8 index) {
//   bool toggleFlag = !NibArray16_GetFlag(array, index);
//   u8 value = NibArray16_GetValue(array, index);
//   // SERIALPRINTLN("NA16_ToggleFlag: idx=" + String(index) + ", value=" + String(value) + ", tflag=" + String(toggleFlag));
//   NibArray16_Set(array, index, value | (toggleFlag<<3));
// }

// void NibArray16_SetValue(nib_array_16& array, u8 index, u8 value) {
//   bool flag = NibArray16_GetFlag(array, index);
//   return NibArray16_Set(array, index, value | (flag<<3));
// }

