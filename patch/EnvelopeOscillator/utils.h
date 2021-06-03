#pragma once
#include <stdint.h>
#include <vector>
#ifdef __cplusplus
namespace EnvelopeOscillator
{
struct Utils
{
    static inline float Lerp(const float a, const float b, const float perc)
    {
        return (1 - perc) * a + perc * b;
    }

    static inline float GetValue(const std::vector<float>& values,
                                 const float               percentage)
    {
        const float  scaledValue = percentage * values.size();
        float        integer;
        const float  remainder = modf(scaledValue, &integer);
        const size_t index     = static_cast<size_t>(integer);
        if(index >= values.size() - 1)
        {
            return values.back();
        }
        else
        {
            return Lerp(values.at(index), values.at(index + 1), remainder);
        }
    }
};
} // namespace EnvelopeOscillator
#endif