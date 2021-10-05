#pragma once
#ifndef DSY_LOOPER_H
#define DSY_LOOPER_H
#include <stdlib.h>
#include <stdint.h>

namespace daisysp
{
/** Simple Looper.
October 2021
By: beserge
*/
template <typename T, size_t max_size>
class Looper
{
    public:
        Looper(){};
        ~Looper(){};

        void Init(float sr){
            Reset();
            looplen_ = max_size;
            idx_ = 0;
            increment_ = 1;
            sr_ = sr;
        }

        void Reset(){
            idx_ = 0;
            for (size_t i = 0; i < max_size; i++){
                buff[i] = 0.f;
            }
        }

        float Read(){
            idx_ += increment_;
            idx_ %= looplen_;
            return buff[idx_];
        }

        void Write(float input){
            buff[idx_] = input;
        }

        void Setlooplen_gthMs(float length){
            looplen_ = length * .001f * sr_;
        }

    private:
        float buff[max_size];
        size_t looplen_;
        size_t idx_;
        size_t increment_;
        float sr_;
};
} // namespace daisysp
#endif