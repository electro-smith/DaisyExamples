/* Edge Impulse inferencing library
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __EI_ALLOC__H__
#define __EI_ALLOC__H__

#include "memory.hpp"

#if EIDSP_TRACK_ALLOCATIONS
#include <map>
#endif

namespace ei {

template <class T>
struct EiAlloc
{
    typedef T value_type;
    EiAlloc() = default;
    template <class U>
    constexpr EiAlloc(const EiAlloc<U> &) noexcept {}

    T *allocate(size_t n)
    {
        auto bytes = n * sizeof(T);
        auto ptr = ei_dsp_malloc(bytes);
#if EIDSP_TRACK_ALLOCATIONS
        get_allocs()[ptr] = bytes;
#endif
        return (T *)ptr;
    }

    void deallocate(T *p, size_t n) noexcept
    {
#if EIDSP_TRACK_ALLOCATIONS
        auto size_p = get_allocs().find(p);
        ei_dsp_free(p,size_p->second);
        get_allocs().erase(size_p);
#else
        ei_dsp_free(p,0);
#endif
    }
#if EIDSP_TRACK_ALLOCATIONS
    private:
    // [address] -> size requested
    typedef std::map<void*,size_t> map_t;
    static map_t& get_allocs() {
        static map_t allocs;
        return allocs;
    }
#endif
};

template <class T, class U>
bool operator==(const EiAlloc<T> &, const EiAlloc<U> &) { return true; }
template <class T, class U>
bool operator!=(const EiAlloc<T> &, const EiAlloc<U> &) { return false; }
}

#endif //!__EI_ALLOC__H__
