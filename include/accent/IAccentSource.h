#ifdef ENABLE_ACCENTS

// IAccentSource.h
// Interface for per-step velocity accent sources.
//
// Each Pattern holds an optional IAccentSource* (accent_source).
// If null, it falls back to global_accent_source.
// If that is also null, DEFAULT_VELOCITY is used unmodified.
//
// Usage:
//   pattern->accent_source = new StepAccentSource(16);          // per-pattern override
//   global_accent_source = new StepAccentSource(16);            // global default
//
// The canonical call at trigger time (inside SimplePattern::get_velocity()):
//   float accent = get_effective_accent_source()->get_accent(local_step, global_step);
//   return (int8_t)constrain((int)(DEFAULT_VELOCITY * accent), 0, MIDI_MAX_VELOCITY);

#pragma once

#include <Arduino.h>
#include <bpm.h>

#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
#endif

// Forward declaration so IAccentSource can expose as_step_source() without RTTI.
class StepAccentSource;
class Menu;

// ---------------------------------------------------------------------------
// IAccentSource — pure interface
// ---------------------------------------------------------------------------
class IAccentSource
    #ifdef ENABLE_STORAGE
        : virtual public SHDynamic<0, 8>
    #endif
{
public:
    // Return accent multiplier in [0.0, 1.0] (values > 1.0 are legal for boost).
    // local_step  — step index within the pattern (0..pattern_steps-1)
    // global_step — absolute step counter (BPM_CURRENT_STEP_OF_SONG), for sync across patterns
    virtual float get_accent(int local_step, int global_step) = 0;

    virtual const char* get_label() const = 0;

    // RTTI-free downcast — overridden by StepAccentSource to return `this`.
    // All other subclasses return nullptr.
    virtual StepAccentSource* as_step_source() { return nullptr; }

    virtual ~IAccentSource() {}

    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override {
            // subclasses register their own settings
        }
    #endif

    #ifdef ENABLE_SCREEN
        virtual void make_menu_items() {
            // subclasses can add their own menu items if they want
        }
    #endif
};

// ---------------------------------------------------------------------------
// Global default — points to whatever source is active for all patterns that
// have no per-pattern override.  Set this before sl_setup_all() so that the
// source's saveable settings are registered in the tree.
//
// Defined in accent/IAccentSource.cpp (or any single .cpp that includes this header).
// ---------------------------------------------------------------------------
extern IAccentSource* global_accent_source;

// ---------------------------------------------------------------------------
// ConstantAccentSource — always returns the same multiplier (default: 1.0).
// Useful as a placeholder / "off" source.
// ---------------------------------------------------------------------------
class ConstantAccentSource : public IAccentSource {
    float value;
public:
    explicit ConstantAccentSource(float v = 1.0f) : value(v) {
        #ifdef ENABLE_STORAGE
            this->set_path_segment("ConstantAccent");
        #endif
    }

    void set_value(float v) { value = v; }
    float get_value() const { return value; }

    float get_accent(int /*local_step*/, int /*global_step*/) override {
        return value;
    }

    const char* get_label() const override { return "Constant"; }

    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override {
            IAccentSource::setup_saveable_settings();
            register_setting(
                new LSaveableSetting<float>(
                    "Accent Level", "ConstantAccent",
                    &this->value
                ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT
            );
        }
    #endif
};


#endif