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

TODO:
- Add fractional speed control
- Allow for reverse playback
- Allow user to read / write wherever they like in the buffer
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
        max_size_ms_ = max_size / sr * 1000.f;
        
        idx_       = 0;
        sr_        = sr;

        rec_arm_ = false;
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
        if(idx_ >= looplen_ || idx_ >= max_size){
            if(rec_arm_){
                EndLoop();
            }

            idx_ = 0;
        }
        return buff[idx_];
    }

    void Write(T input) { buff[idx_] = input; }

    float Process(float input){
        if(rec_arm_){
            Write(input);
        }
        return Read();
    }

    void SetLoopLengthMs(float length) { 
        length = fclamp(length, 1.f, max_size_ms_);
        looplen_ = length * .001f * sr_; 
    }

    void StartLoop(){
        Reset();
        rec_arm_ = true;
        looplen_ = max_size;
    }

    void EndLoop(){
        rec_arm_ = false;
        looplen_ = std::min(max_size, idx_);
    }

    void ToggleLoop(){
        if(rec_arm_){
            EndLoop();
        }
        else{   
            StartLoop();
        }
    }

  private:
    T  buff[max_size];
    size_t looplen_;
    float max_size_ms_;
    size_t idx_;
    float  sr_;
    bool _;
    bool rec_arm_;
};
} // namespace daisysp
#endif