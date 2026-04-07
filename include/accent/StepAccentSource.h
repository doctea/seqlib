#ifdef ENABLE_ACCENTS

// StepAccentSource.h
// Per-step accent source: holds a float multiplier for each step in a bar.
// Steps beyond the configured length wrap (modulo).
//
// The accent array is indexed by local_step % num_steps, so it works
// correctly regardless of pattern length.
//
// Default accent level for all steps is 1.0 except step 0 which starts
// at 1.0 (full velocity).  You can call set_accent(step, value) to set
// individual steps, or set_all(value) to reset everything.

#pragma once

#include "IAccentSource.h"

#ifndef CALLOC_FUNC
    #define CALLOC_FUNC calloc
#endif
#ifndef MALLOC_FUNC
    #define MALLOC_FUNC malloc
#endif


class StepAccentSource : public IAccentSource {
    float*    accent_levels = nullptr;
    uint16_t  num_steps;

public:
    explicit StepAccentSource(uint16_t steps = 16) : num_steps(steps) {
        accent_levels = (float*)CALLOC_FUNC(steps, sizeof(float));
        set_all(1.0f);

        #ifdef ENABLE_STORAGE
            this->set_path_segment("StepAccent");
        #endif
    }

    virtual ~StepAccentSource() {
        // note: we intentionally do NOT free — pattern objects are permanent on embedded targets
    }

    // RTTI-free downcast (see IAccentSource::as_step_source)
    virtual StepAccentSource* as_step_source() override { return this; }

    uint16_t get_num_steps() const { return num_steps; }

    void set_accent(int step, float value) {
        if (accent_levels && step >= 0 && step < (int)num_steps)
            accent_levels[step] = value;
    }

    float get_accent_raw(int step) const {
        if (!accent_levels || num_steps == 0) return 1.0f;
        return accent_levels[((unsigned)step) % num_steps];
    }

    void set_all(float value) {
        for (uint16_t i = 0; i < num_steps; ++i)
            accent_levels[i] = value;
    }

    // Convenience: set a typical strong/weak two-level pattern
    // strong_steps: array of step indices that get 'strong' level; rest get 'weak'
    void set_pattern(const uint8_t* strong_steps, uint8_t count,
                     float strong = 1.0f, float weak = 0.6f) {
        //set_all(weak);
        for (uint8_t i = 0; i < count; ++i)
            set_accent(strong_steps[i], strong);
    }

    // IAccentSource
    float get_accent(int local_step, int /*global_step*/) override {
        return get_accent_raw(local_step);
    }

    const char* get_label() const override { return "Step Accent"; }

    #ifdef ENABLE_STORAGE
        // Note: saving the full float array is non-trivial with LSaveableSetting.
        // For now we register a simplified "all steps" level as a placeholder.
        // A proper SaveableNibbleArraySetting or custom setting could encode the full array.
        // TODO: implement a proper array saveable setting for step accents
        virtual void setup_saveable_settings() override {
            IAccentSource::setup_saveable_settings();
            // placeholder: not yet saved per-step
        }
    #endif

    #ifdef ENABLE_SCREEN
        virtual void make_menu_items() override;
    #endif

};

#endif