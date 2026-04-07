// IAccentSource.cpp
// Defines the global default accent source pointer.
// Set global_accent_source to any IAccentSource* before patterns start playing.
// If left as nullptr, BasePattern::get_effective_accent_source() returns nullptr
// and get_velocity() returns DEFAULT_VELOCITY unchanged.

#ifdef ENABLE_ACCENTS

    #include "accent/IAccentSource.h"

    IAccentSource* global_accent_source = nullptr;

#endif