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

    void Init(float sr)
    {
        Reset();
        looplen_   = max_size;
        idx_       = 0;
        sr_        = sr;
        eol_ = false;
    }

    void Reset()
    {
        idx_ = 0;
        for(size_t i = 0; i < max_size; i++)
        {
            buff[i] = 0.f;
        }
    }

    T Read()
    {
        idx_++;
        if(idx_ >= looplen_){
            idx_ = 0;
            eol_ = true; // set the flag
        }
        return buff[idx_];
    }

    void Write(T input) { buff[idx_] = input; }

    void Setlooplen_gthMs(float length) { looplen_ = length * .001f * sr_; }

    bool EndOfLoop(){
        if(eol_){
            eol_ = false; //unset the flag
            return true;
        }

        return false;
    }

  private:
    T  buff[max_size];
    size_t looplen_;
    size_t idx_;
    float  sr_;
    bool eol_;
};
} // namespace daisysp
#endif