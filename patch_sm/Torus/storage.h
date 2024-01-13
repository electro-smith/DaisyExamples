#ifndef TORUS_STORAGE_H_
#define TORUS_STORAGE_H_


namespace torus
{
struct Settings
{
    float offset, scale;
    bool  strum_in, note_in, exciter_in, easter_egg_on;
    int   model_mode, poly_mode, egg_mode;

    bool operator==(const Settings& rhs)
    {
        return offset == rhs.offset && scale == rhs.scale
               && strum_in == rhs.strum_in && note_in == rhs.note_in
               && exciter_in == rhs.exciter_in
               && easter_egg_on == rhs.easter_egg_on
               && model_mode == rhs.model_mode && poly_mode == rhs.poly_mode
               && egg_mode == rhs.egg_mode;
    }
    bool operator!=(const Settings& rhs) { return !operator==(rhs); }
};


} // namespace torus

#endif // TORUS_STORAGE_H_
