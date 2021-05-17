#include "daisy.h"
#include "daisysp.h"
#include "resources.h"

using namespace daisy;
using namespace daisysp;

#define MAKE_INTEGRAL_FRACTIONAL(x)                   \
    int32_t x##_integral   = static_cast<int32_t>(x); \
    float   x##_fractional = x - static_cast<float>(x##_integral);

#define SLOPE(out, in, positive, negative)                \
    {                                                     \
        float error = (in)-out;                           \
        out += (error > 0 ? positive : negative) * error; \
    }

#define ONE_POLE(out, in, coefficient) out += (coefficient) * ((in)-out);

#define CONSTRAIN(var, min, max) \
    if(var < (min))              \
    {                            \
        var = (min);             \
    }                            \
    else if(var > (max))         \
    {                            \
        var = (max);             \
    }

#define JOIN(lhs, rhs) JOIN_1(lhs, rhs)
#define JOIN_1(lhs, rhs) JOIN_2(lhs, rhs)
#define JOIN_2(lhs, rhs) lhs##rhs

#define STATIC_ASSERT(expression, message)                                  \
    struct JOIN(__static_assertion_at_line_, __LINE__)                      \
    {                                                                       \
        impl::StaticAssertion<static_cast<bool>((expression))>              \
            JOIN(JOIN(JOIN(STATIC_ASSERTION_FAILED_AT_LINE_, __LINE__), _), \
                 message);                                                  \
    };

namespace impl
{
template <bool>
struct StaticAssertion;

template <>
struct StaticAssertion<true>
{
}; // StaticAssertion<true>

template <int i>
struct StaticAssertionTest
{
}; // StaticAssertionTest<int>

} // namespace impl

template <uint32_t a, uint32_t b, uint32_t c, uint32_t d>
struct FourCC
{
    static const uint32_t value = (((((d << 8) | c) << 8) | b) << 8) | a;
};

class CosineOscillator
{
  public:
    CosineOscillator() {}
    ~CosineOscillator() {}

    void Init(float freq)
    {
        float sample_rate = 48000; //hardcoded for now
        phs_inc_          = freq / sample_rate;
    }

    float Next()
    {
        phs_ += phs_inc_;

        if(phs_ >= 1.f)
        {
            phs_ -= 1.f;
        }

        value_ = cos(phs_);

        return value_;
    }

    float value() { return value_; }

  private:
    float value_ = 0.f;
    float sample_rate_;
    float phs_ = 0.f;
    float phs_inc_;
};

inline int32_t Clip16(int32_t x)
{
    if(x < -32768)
    {
        return -32768;
    }
    else if(x > 32767)
    {
        return 32767;
    }
    else
    {
        return x;
    }
}

inline float Interpolate(const float* table, float index, float size)
{
    index *= size;
    MAKE_INTEGRAL_FRACTIONAL(index)
    float a = table[index_integral];
    float b = table[index_integral + 1];
    return a + (b - a) * index_fractional;
}

inline float SemitonesToRatio(float semitones)
{
    float pitch = semitones + 128.0f;
    MAKE_INTEGRAL_FRACTIONAL(pitch)

    return lut_pitch_ratio_high[pitch_integral]
           * lut_pitch_ratio_low[static_cast<int32_t>(pitch_fractional
                                                      * 256.0f)];
}

inline float Crossfade(float a, float b, float fade)
{
    return a + (b - a) * fade;
}

template <typename To, typename From>
struct unsafe_bit_cast_t
{
    union
    {
        From from;
        To   to;
    };
};

template <typename To, typename From>
To unsafe_bit_cast(From from)
{
    unsafe_bit_cast_t<To, From> u;
    u.from = from;
    return u.to;
}

static inline float fast_rsqrt_carmack(float x)
{
    uint32_t    i;
    float       x2, y;
    const float threehalfs = 1.5f;
    y                      = x;
    i                      = unsafe_bit_cast<uint32_t, float>(y);
    i                      = 0x5f3759df - (i >> 1);
    y                      = unsafe_bit_cast<float, uint32_t>(i);
    x2                     = x * 0.5f;
    y                      = y * (threehalfs - (x2 * y * y));
    return y;
}

inline int16_t SoftConvert(float x)
{
    return Clip16(static_cast<int32_t>(SoftLimit(x * 0.5f) * 32768.0f));
}

static inline uint16_t fast_atan2r(float y, float x, float* r)
{
    float squared_magnitude = x * x + y * y;
    if(squared_magnitude == 0.0f)
    {
        *r = 0.0f;
        return 0.0f;
    }
    float rinv = fast_rsqrt_carmack(squared_magnitude);
    *r         = rinv * squared_magnitude;

    static const uint32_t sign_mask = 0x80000000;
    uint32_t ux_s     = sign_mask & unsafe_bit_cast<uint32_t, float>(x);
    uint32_t uy_s     = sign_mask & unsafe_bit_cast<uint32_t, float>(y);
    uint32_t quadrant = ((~ux_s & uy_s) >> 29 | ux_s >> 30);
    uint16_t angle    = 0;
    x                 = fabs(x);
    y                 = fabs(y);
    if(y > x)
    {
        angle
            = 16384 - atan_lut[static_cast<uint32_t>(x * rinv * 512.0f + 0.5f)];
    }
    else
    {
        angle = atan_lut[static_cast<uint32_t>(y * rinv * 512.0f + 0.5f)];
    }
    if(ux_s ^ uy_s)
    {
        angle = -angle;
    }
    return angle + (quadrant << 14);
}