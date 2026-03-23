#pragma once

#include <Arduino.h>

#if defined(ENABLE_PARAMETERS)
    #include <LinkedList.h>
#endif

#include "SaveableParameters.h"

#ifdef ENABLE_SHUFFLE
    #include "shuffle.h"
#endif

#include <bpm.h>

class BasePattern;
class SimplePattern;
class BaseOutput;
class FloatParameter;
class Menu;

class BaseSequencer : virtual public ISaveableParameterHost {
    public:

    BaseSequencer() {}
    virtual ~BaseSequencer() = default;

    bool running = true;
    uint_fast8_t number_patterns = 20;
    bool debug = false;
    bool shuffle_enabled = true;

    virtual SimplePattern *get_pattern(unsigned int pattern) = 0;

    virtual bool is_running() {
        return this->running;
    }
    virtual void set_playing(bool state = true) {
        this->running = state;
    }

    // called every tick, call the appropriate callbacks for the current tick, step, beat, bar, and phrase
    //virtual void on_tick(int tick) = 0;
    virtual void on_tick(int tick) {
        if (is_bpm_on_phrase(tick)) {
            this->on_phrase(BPM_CURRENT_PHRASE);
        }
        if (is_bpm_on_bar(tick)) {
            this->on_bar(BPM_CURRENT_BAR_OF_PHRASE);
        }
        if (is_bpm_on_beat(tick)) {
            this->on_beat(BPM_CURRENT_BEAT_OF_BAR);
        }
        if (is_bpm_on_sixteenth(tick)) {
            this->on_step(tick / TICKS_PER_STEP);
        } else if (is_bpm_on_sixteenth(tick,1)) {
            // this re-enabled 2025-05-14 for Compulidean -- if usb_teensy_clocker/Microlidian start playing up then this might be the reason?
            this->on_step_end(tick / TICKS_PER_STEP); //(PPQN/STEPS_PER_BEAT));
        }
    }

    virtual void on_loop(int tick) = 0;
    virtual void on_beat(int beat) = 0;
    virtual void on_bar(int bar) = 0;
    virtual void on_phrase(int phrase) = 0;

    virtual void on_step(int step) = 0;
    virtual void on_step_end(int step) = 0;
    
    #ifdef ENABLE_SHUFFLE
        virtual void on_step_shuffled(int8_t track, int step) = 0;
        virtual void on_step_end_shuffled(int8_t track, int step) = 0;
        
        virtual bool is_shuffle_enabled() {
            return this->shuffle_enabled;
        }
        virtual void set_shuffle_enabled(bool state = true) {
            this->shuffle_enabled = state;
        }
    #endif

    virtual void configure_pattern_output(int index, BaseOutput *output);
    
    #if defined(ENABLE_PARAMETERS)
        virtual LinkedList<FloatParameter*> *getParameters();
        virtual FloatParameter* getParameterByName(const char *name);   // UNTESTED!!
    #endif

    #if defined(ENABLE_SCREEN)
        virtual void make_menu_items(Menu *menu);
    #endif

    // save/load stuff
    // todo: ideally, all this can be dealt with by inheriting from a file_manager "ISaveableParameterHost" class, or something like this
    virtual LinkedList<String> *save_pattern_add_lines(LinkedList<String> *lines) {
        ISaveableParameterHost::add_save_lines_saveable_parameters(lines);
        return lines;
    }
    virtual bool load_parse_key_value(String key, String value) {
        if (ISaveableParameterHost::load_parse_key_value_saveable_parameters(key, value))
            return true;
        return false;
    }

    virtual void setup_saveable_parameters() override;
};