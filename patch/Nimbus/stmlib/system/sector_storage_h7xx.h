// Copyright 2019 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Helper functions for using a sector of flash for non-volatile storage.

#ifndef STMLIB_SYSTEM_SECTOR_STORAGE_H7XX_H_
#define STMLIB_SYSTEM_SECTOR_STORAGE_H7XX_H_

#include <stm32h7xx_hal.h>

#include <algorithm>
#include <cstring>



namespace stmlib {
  
template<uint32_t> struct Sector { };

template<> struct Sector<0> { enum { start = 0x08000000 }; };
template<> struct Sector<1> { enum { start = 0x08020000 }; };
template<> struct Sector<2> { enum { start = 0x08040000 }; };
template<> struct Sector<3> { enum { start = 0x08060000 }; };
template<> struct Sector<4> { enum { start = 0x08080000 }; };
template<> struct Sector<5> { enum { start = 0x080a0000 }; };
template<> struct Sector<6> { enum { start = 0x080c0000 }; };
template<> struct Sector<7> { enum { start = 0x080e0000 }; };
template<> struct Sector<8> { enum { start = 0x08100000 }; };
template<> struct Sector<9> { enum { start = 0x08120000 }; };
template<> struct Sector<10> { enum { start = 0x08140000 }; };
template<> struct Sector<11> { enum { start = 0x08160000 }; };
template<> struct Sector<12> { enum { start = 0x08180000 }; };
template<> struct Sector<13> { enum { start = 0x081a0000 }; };
template<> struct Sector<14> { enum { start = 0x081c0000 }; };
template<> struct Sector<15> { enum { start = 0x081e0000 }; };
template<> struct Sector<16> { enum { start = 0x08200000 }; };

// Class for storing calibration and state data in a same sector of flash
// without having to rewrite the calibration data every time the state is
// changed.
//
// Data is stored in a RIFF-ish format, with the peculiarity that the size
// field of the header is 16-bit instead of 32-bit - the remaining 16 bits being
// used to store a naive checksum of the chunk data.
// 
// +----+-----------------+----+------------+----+------------+----+--
// |HEAD|CALIBRATION CHUNK|HEAD|STATE CHUNK1|HEAD|STATE CHUNK2|HEAD|..
// +----+-----------------+----+------------+----+------------+----+--
//
// The first chunk stores the large, "slow" data like calibration or presets.
// Whenever this data needs to be saved, the entire sector of flash is erased.
//
// The subsequent chunks store successive revision of the "fast" data like
// module state. Whenever this data changes, a new chunk is appended to the list
// until the flash sector is filled - in which case the sector is erased, and
// the calibration data + first version of state is written.
template<
    uint32_t sector_index,
    typename PersistentData,
    typename StateData>
class ChunkStorage {
 private:
  struct ChunkHeader {
    uint32_t tag;
    uint16_t size;
    uint16_t checksum;
  };

  enum {
    FLASH_STORAGE_BASE = Sector<sector_index>::start,
    FLASH_STORAGE_LAST = Sector<sector_index + 1>::start,
  };
  
  enum {
    BLOCK_SIZE = 32,  // Must be a power of 2!
  };

 public:
  ChunkStorage() { }
  ~ChunkStorage() { }
  // Loads the latest saved data. In case the sector is blank/corrupted,
  // reinitializes the sector and returns false.
  bool Init(PersistentData* persistent_data, StateData* state_data) {
    persistent_data_ = persistent_data;
    state_data_ = state_data;
  
    if (ReadChunk(0, persistent_data)) {
      for (next_state_chunk_index_ = 1;
           chunk_address(next_state_chunk_index_ + 1) <= FLASH_STORAGE_LAST;
           ++next_state_chunk_index_) {
         if (!ReadChunk(next_state_chunk_index_, state_data)) {
           break;
         }
      }
      if (next_state_chunk_index_ != 1) {
        return true;
      }
    }
    RewriteSector();
    return false;
  }

  void SaveState() {
    if (chunk_address(next_state_chunk_index_ + 1) > FLASH_STORAGE_LAST) {
      RewriteSector();
    } else {
      HAL_FLASH_Unlock();
      WriteChunk(next_state_chunk_index_, state_data_);
      next_state_chunk_index_++;
      HAL_FLASH_Lock();
    }
  }
  
  void SavePersistentData() {
    RewriteSector();
  }

 private:
   void RewriteSector() {
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef erase_params;
    erase_params.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase_params.Banks = (sector_index < 8) ? FLASH_BANK_1 : FLASH_BANK_2;
    erase_params.Sector = sector_index;
    erase_params.NbSectors = 1;
    erase_params.VoltageRange = FLASH_VOLTAGE_RANGE_4;
    
    uint32_t sector_error;
    HAL_FLASHEx_Erase(&erase_params, &sector_error);

    WriteChunk(0, persistent_data_);
    WriteChunk(1, state_data_);
    next_state_chunk_index_ = 2;
    HAL_FLASH_Lock();
  }

  template<typename T>
  bool ReadChunk(size_t index, T* data) {
    const char* flash_ptr = (const char*)(chunk_address(index));
    ChunkHeader* h = (ChunkHeader*)(flash_ptr);
    if (h->tag == T::tag &&
        h->size == sizeof(T) &&
        Checksum(flash_ptr + sizeof(ChunkHeader), h->size) == h->checksum) {
      memcpy(data, flash_ptr + sizeof(ChunkHeader), h->size);
      return true;
    } else {
      return false;
    }
  }

  template<typename T>
  void WriteChunk(size_t index, const T* data) {
    ChunkHeader h;
    h.tag = T::tag;
    h.size = sizeof(T);
    h.checksum = Checksum(data, sizeof(T));
    
    size_t num_flash_words = ChunkSize<T>() / BLOCK_SIZE;
    uint32_t flash_address = chunk_address(index);

    size_t data_size = sizeof(T);
    const uint8_t* data_source = (const uint8_t*)(data);
    
    for (size_t i = 0; i < num_flash_words; ++i) {
      size_t header_size = i == 0 ? sizeof(ChunkHeader) : 0;
      size_t size = std::min(data_size, BLOCK_SIZE - header_size);
      
      uint8_t buffer[BLOCK_SIZE];
      memcpy(&buffer[0], &h, header_size);
      memcpy(&buffer[header_size], data_source, size);
      
      HAL_FLASH_Program(
          FLASH_TYPEPROGRAM_FLASHWORD,
          flash_address,
          (uint64_t)(&buffer[0]));
      
      data_size -= size;
      data_source += size;
      flash_address += BLOCK_SIZE;
    }
  }
 
  template<typename T>
  inline size_t ChunkSize() {
    return (sizeof(ChunkHeader) + sizeof(T) + BLOCK_SIZE - 1) & \
        ~(BLOCK_SIZE - 1);
  }

  uint32_t chunk_address(size_t index) {
    if (index == 0) {
      return FLASH_STORAGE_BASE;
    } else {
      return FLASH_STORAGE_BASE + ChunkSize<PersistentData>() + ChunkSize<StateData>() * (index - 1);
    }
  }

  uint16_t Checksum(const void* data, size_t size) const {
    uint16_t sum = 0;
    const char* bytes = static_cast<const char*>(data);
    while (size--) {
      sum += *bytes++;
    }
    return sum ^ 0xffff;
  }

 private:
  PersistentData* persistent_data_;
  StateData* state_data_;
  uint16_t next_state_chunk_index_;

  DISALLOW_COPY_AND_ASSIGN(ChunkStorage);
};

};  // namespace stmlib

#endif  // STMLIB_SYSTEM_SECTOR_STORAGE_H7XX_H_
