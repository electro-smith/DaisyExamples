#include "calibrate.h"

namespace torus
{

bool Calibrate::ProcessCalibration(float& cv_in, bool& button_pressed)
{
    bool done{false};
    switch(cal_state_)
    {
        case PATCH_1V:
            if(button_pressed)
            {
                value_1v_  = cv_in;
                cal_state_ = PATCH_3V;
            }
            /** Waiting for 1V, slow blink */
            brightness_ = (System::GetNow() & 1023) > 511 ? 1.f : 0.f;
            break;
        case PATCH_3V:
            if(button_pressed)
            {
                if(cal.Record(value_1v_, cv_in))
                {
                    cal_state_ = PATCH_DONE_CALIBRATING;
                    // Calibration is complete.
                    done = true;
                }
            }
            /** Waiting for 3V, fast blink */
            brightness_ = (System::GetNow() & 255) > 127 ? 1.f : 0.f;
            break;
        case PATCH_DONE_CALIBRATING: break;
    }
    return done;
}

} // namespace torus