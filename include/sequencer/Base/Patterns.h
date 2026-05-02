#pragma once

#include <Arduino.h>

//#include "Config.h"

#include <clock.h>
#include <midi_helpers.h>

#ifndef CALLOC_FUNC
    #define CALLOC_FUNC calloc
#endif
#ifndef MALLOC_FUNC
    #define MALLOC_FUNC malloc
#endif

#ifdef ENABLE_SCREEN
    #include "menu.h"
#endif

#ifdef ENABLE_PARAMETERS
    #include "parameters/Parameter.h"
#endif

#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
    #include "outputs/SeqlibSaveableSettings.h"
#endif

#ifdef ENABLE_ACCENTS
    #include "accent/IAccentSource.h"
#endif

#define DEFAULT_VELOCITY    MIDI_MAX_VELOCITY

#define MAX_STEPS TIME_SIG_MAX_STEPS_PER_BAR

class BaseOutput;
class BaseSequencer;

class BasePattern 
    #ifdef ENABLE_STORAGE
        : virtual public SHDynamic<4, 4> // parameter children; steps/locked/output/shuffle settings
    #endif
    {
    public:

    uint8_t steps = MAX_STEPS;
    bool note_held = false;
    bool locked = false;

    #ifdef ENABLE_SHUFFLE
        uint8_t shuffle_track = 0;
    #endif

    BaseOutput *output = nullptr;
    LinkedList<BaseOutput*> *available_outputs = nullptr;

    bool debug = false;

    virtual void set_available_outputs(LinkedList<BaseOutput*> *available_outputs) {
        this->available_outputs = available_outputs;
    }

    BasePattern(LinkedList<BaseOutput*> *available_outputs) {
        this->set_available_outputs(available_outputs);
    }

    virtual void set_output_by_name(const char *output_name);
    virtual void set_output(BaseOutput *output) {
        this->output = output;
    }
    virtual BaseOutput *get_output() {
        return this->output;
    }

    #ifdef ENABLE_SCREEN
        uint16_t colour = C_WHITE;
        inline uint16_t get_colour() {
            return colour;
        }
        inline void set_colour(uint16_t c) {
            this->colour = c;
        }
    #endif

    virtual const char *get_summary() {
        return "??";
    }
    virtual const char *get_output_label();

    #ifdef ENABLE_ACCENTS
        // ---------------------------------------------------------------------------
        // Accent source — controls per-step velocity scaling.
        // nullptr means "use global_accent_source"; if that is also nullptr, velocity
        // is returned unmodified (equivalent to a ConstantAccentSource(1.0)).
        // ---------------------------------------------------------------------------
        IAccentSource* accent_source = nullptr;

        IAccentSource* get_effective_accent_source() const {
            return accent_source ? accent_source : global_accent_source;
        }

        void set_accent_source(IAccentSource* src) {
            accent_source = src;
        }
    #endif

    // get the velocity for the note about to be played (current step)
    // 127 by default, but can be overridden by patterns with velocity control
    virtual int8_t get_velocity() {
        return DEFAULT_VELOCITY;
    }

    // todo: ability to pass in step, offset, and bar number, like we have for the current euclidian...?
    //          or, tbf, we can derive this from the 'tick'
    virtual bool query_note_on_for_tick(unsigned int tick) = 0;
    virtual bool query_note_off_for_tick(unsigned int tick) = 0;

    virtual void set_event_for_tick(unsigned int tick, short note = 0, short velocity = 127, short channel = 0) = 0;
    virtual void unset_event_for_tick(unsigned int tick) = 0;

    virtual void process_step(int step) = 0;
    virtual void process_step_end(int step) = 0;
    virtual void process_tick(int tick) = 0;

    //virtual bool query_note_on_for_step(int step) = 0;
    //virtual bool query_note_off_for_step(int step) = 0;

    virtual bool query_note_on_for_step(int step) {
        return this->query_note_on_for_tick(step * TICKS_PER_STEP);
    }
    virtual bool query_note_off_for_step(int step) {
        return this->query_note_off_for_tick(step * TICKS_PER_STEP);
    }

    // duration of the note about to be played
    virtual int8_t get_tick_duration() {
        return PPQN;
    }

    virtual void set_steps(int8_t steps) {
        this->steps = steps;
    }
    virtual int8_t get_steps() {
        return this->steps;
    }
    virtual int8_t get_effective_steps() {
        return this->get_steps();
    }

    virtual void set_locked(bool v = true) {
        this->locked = v;
    }
    virtual bool is_locked() {
        return this->locked;
    }
    virtual bool toggle_locked() {
        set_locked(!is_locked());
        return is_locked();
    }

    #ifdef ENABLE_SHUFFLE
        virtual void set_shuffle_track(uint8_t v) {
            this->shuffle_track = v;
        }
        virtual bool is_shuffled() {
            return uClock.isShuffled(this->get_shuffle_track());
        }
        virtual uint8_t get_shuffle_track() {
            return this->shuffle_track;
        }
        virtual int8_t get_shuffle_length() {
            if (this->is_shuffled())
                return uClock.getShuffleLength(this->get_shuffle_track());
            else
                return 0;
        }
    #endif

    #ifdef ENABLE_PARAMETERS
        LinkedList<FloatParameter*> *parameters = nullptr;
        // instantiate parameters list if it doesn't exist, add it to parameter_manager, and return it
        virtual LinkedList<FloatParameter*> *getParameters(unsigned int i);
    #endif

    #ifdef ENABLE_SCREEN
        virtual void create_menu_items(Menu *menu, int index, BaseSequencer *target_sequencer, int combine_pages = 0);
    #endif

    #ifdef ENABLE_STORAGE
        virtual void add_saveable_settings(int pattern_index) {

            register_setting(
                new LSaveableSetting<uint8_t>("steps", "BasePattern", &this->steps), 
                SL_SCOPE_SCENE  // allow pattern length to be saved at scene level, since it's more of a performance setting than a preference setting
            );
            register_setting(
                new LSaveableSetting<bool>("locked", "BasePattern", &this->locked), 
                SL_SCOPE_SCENE  // allow locked state to be saved at scene level, since it's more of a performance setting than a preference setting
            );

            #ifdef ENABLE_SHUFFLE
                register_setting(
                    new LSaveableSetting<uint8_t>("shuffle_track", "BasePattern", &this->shuffle_track), 
                    SL_SCOPE_SCENE  // allow shuffle track to be saved at scene level, since it's more of a performance setting than a preference setting
                );
            #endif

            register_setting(
                new LOutputSaveableSetting(
                    "output",
                    "BasePattern",
                    [this](const char *output_label) -> void { this->set_output_by_name(output_label); },
                    [this]() -> const char * { return this->get_output_label(); }
                ),
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow pattern output to be saved at scene level, since it's more of a performance setting than a preference setting
            );
            
            // register parameters for this pattern
            LinkedList<FloatParameter*> *parameters = this->getParameters(pattern_index);
            if (parameters!=nullptr) {
                for (size_t i = 0 ; i < parameters->size() ; i++) {
                    FloatParameter *param = parameters->get(i);
                    register_child(param);
                }
            }
        }
    #endif
};

class SimplePattern : public BasePattern {
    public:
    // Override get_velocity() to apply accent scaling.
    virtual int8_t get_velocity() override {
        #ifdef ENABLE_ACCENTS
            IAccentSource* src = get_effective_accent_source();
            if (src) {
                float accent = src->get_accent(BPM_CURRENT_STEP_OF_SONG, BPM_CURRENT_STEP_OF_PHRASE);
                return (int8_t)constrain((int)((float)DEFAULT_VELOCITY * accent), 0, MIDI_MAX_VELOCITY);
            }
        #endif
        return DEFAULT_VELOCITY;
    }

    uint32_t triggered_on_step = -1;
    uint32_t triggered_on_tick = -1;
    int16_t current_duration = PPQN;

    // todo: can probably save some RAM if we allow subclassed patterns to choose their storage format
    // eg EuclidianPatterns only actually need to store on/off for each step so we could easily reduce memory usage by a third
    // and we could go even further and just store it as a bitfield, which would reduce it by a factor of 8 i think? 
    // would need to benchmark whether the extra bit-twiddling adds too much CPU overhead though
    // and the saveloadlib integration would need to be different for each struct type too
    midi_note_event_t *events = nullptr;

    SimplePattern(LinkedList<BaseOutput*> *available_outputs) : BasePattern(available_outputs) {
        this->events = (midi_note_event_t*)CALLOC_FUNC(sizeof(midi_note_event_t), steps);
    }

    // get the step number for this pattern for given tick number
    virtual unsigned int get_step_for_tick(unsigned int tick) {
        // get global step number for this tick, then mod by pattern length in steps to get the step number within this pattern
        return (tick / TICKS_PER_STEP) % this->get_effective_steps();
    }

    virtual void set_event_for_tick(unsigned int tick, short note = 0, short velocity = DEFAULT_VELOCITY, short channel = 0) override {
        short step = get_step_for_tick(tick);
        this->events[step].note = note;
        this->events[step].velocity = velocity;
        this->events[step].channel = channel;
    }
    virtual void unset_event_for_tick(unsigned int tick) override {
        short step = get_step_for_tick(tick);
        this->events[step].velocity = 0;
        //this->events[step].note = NOTE_OFF;
        //this->events[step].channel = channel;
    }

    virtual bool query_note_on_for_tick(unsigned int tick) override {
        short step = get_step_for_tick(tick);
        return (this->events[step].velocity>0);
    }
    virtual bool query_note_off_for_tick(unsigned int tick) override {
        short step = get_step_for_tick(tick);
        return (this->events[step].velocity==0);
    }

    virtual void process_step(int step) override {
        if (this->query_note_on_for_step(step)) {
            if (debug) Serial.printf("note on for step! (ticks=%6u)", ticks);
            if (!this->note_held)
                this->trigger_on_for_step(step);
        }
        //if (debug) Serial.println();
        //debug = false;
    };
    virtual void process_step_end(int step) override {
        if (this->query_note_off_for_step((step+1) % this->get_effective_steps()) && this->note_held) {
            // only turn off if the duration has passed, otherwise we might cut off a note early
            if ((ticks >= triggered_on_tick + this->current_duration || ticks < triggered_on_tick)) {
                //Serial.printf("%i: actually doing note off for step %i!\n", step % get_effective_steps(), step);
                this->trigger_off_for_step(step);
            } else {
                //Serial.printf("%i: would turn off for step %i, but duration hasn't passed yet (ticks=%6u, triggered_on_tick=%6u, current_duration=%u)\n", step % get_effective_steps(), step, ticks, triggered_on_tick, current_duration);
            }
        }
    }
    virtual void process_tick(int ticks) override { 
        // Early-out: most ticks have no note held — skip the modulo division entirely.
        if (!this->note_held) return;

        // check if note is held and duration has passed...
        int step = BPM_GLOBAL_STEP_FROM_TICKS(ticks) % steps;

        //Serial.printf("SimplePattern::process_tick: step_of_song=%i, step_of_pattern=%i, ticks=%6u, triggered_on_tick=%6u, current_duration=%u\n", BPM_CURRENT_STEP_OF_SONG, step, ticks, triggered_on_tick, current_duration); Serial.flush();
        //Serial.printf("SimplePattern::process_tick: ticks=%i, step_of_song=%i, step_of_pattern=%i\n", ticks, BPM_GLOBAL_STEP_FROM_TICKS(ticks), step);

        if ((uint32_t)ticks >= triggered_on_tick + this->current_duration || (uint32_t)ticks < triggered_on_tick) {
            this->trigger_off_for_step(step);
        }
    }

    virtual void trigger_on_for_step(int step);
    virtual void trigger_off_for_step(int step);

    virtual void restore_default_arguments() {}
    virtual void store_current_arguments_as_default() {}

    /*#ifdef ENABLE_SCREEN
        virtual void create_menu_items(Menu *menu, int index) override;
    #endif*/

};


#ifdef ENABLE_SCREEN
    #include "menuitems_object_multitoggle.h"
    
    class PatternMultiToggleItem : public MultiToggleColourItemClass<BasePattern> {
        public:
        PatternMultiToggleItem(const char *label, BasePattern *target, void(BasePattern::*setter)(bool), bool(BasePattern::*getter)(), bool invert_colours = false) 
            : MultiToggleColourItemClass<BasePattern>(label, target, setter, getter, invert_colours) 
            {}

        BasePattern *last_known_output = nullptr;
        char cached_label[MENU_C_MAX/2];
        virtual const char *get_label() override {
            if (last_known_output!=this->target) {
                snprintf(cached_label, MENU_C_MAX/2, "%s: %s", this->label, this->target->get_output_label());
                //snprintf(cached_label, MENU_C_MAX/2, "%s", this->target->get_output_label()); // todo: hmm this seemed to be significantly faster to render - maybe because its twice the text to render?
                last_known_output = this->target;
            }
            //static const char label[MENU_C_MAX];
            //snprintf(label, MENU_C_MAX, "%s: %s", )
            //return (String(this->label) + ": " + this->target->get_output_label()).c_str();
            return cached_label;
        }
    };
#endif