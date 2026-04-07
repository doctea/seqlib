#pragma once

#include <Arduino.h>

#if defined(ENABLE_PARAMETERS)
    #include <LinkedList.h>
#endif

#ifdef ENABLE_STORAGE
    #include "saveload_settings.h"
#endif

#ifdef ENABLE_SHUFFLE
    #include "../shuffle.h"
#endif

#include <bpm.h>

class BasePattern;
class SimplePattern;
class BaseOutput;
class FloatParameter;
class Menu;

class BaseSequencer 
    #ifdef ENABLE_STORAGE
        : virtual public SHStorage<20, 4>   // up to 16 pattern + parameter children; few own settings
    #endif
    {
    public:

    BaseSequencer() {
        #ifdef ENABLE_STORAGE
            this->set_path_segment("BaseSequencer");
        #endif
    }
    virtual ~BaseSequencer() = default;

    bool running = true;
    //uint_fast8_t number_patterns = 0; // = 20;
    bool debug = false;
    bool shuffle_enabled = true;

    virtual SimplePattern *get_pattern(unsigned int pattern) = 0;
    virtual void add_pattern(BasePattern *pattern) = 0;
    virtual uint16_t get_number_patterns() = 0;

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
            this->on_step_end(tick / TICKS_PER_STEP);
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
        virtual void make_menu_items(Menu *menu, int combine_pages);
    #endif

    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override;
    #endif
};

class SimpleSequencer : public BaseSequencer {

    LinkedList<BaseOutput*> *available_outputs = nullptr;
    LinkedList<BasePattern*> *patterns = new LinkedList<BasePattern*>();

    public:
    SimpleSequencer(LinkedList<BaseOutput*> *available_outputs) : BaseSequencer() {
        this->available_outputs = available_outputs;
    }

    virtual uint16_t get_number_patterns() override {
        return this->patterns->size();
    }

    virtual SimplePattern *get_pattern(unsigned int pattern) override {
        if (pattern >= this->patterns->size()) {
            Serial.printf("SimpleSequencer#get_pattern(%i) out of bounds (size=%i)\n", pattern, this->patterns->size());
            return nullptr;
        }
        return (SimplePattern*)this->patterns->get(pattern);
    }

    virtual void add_pattern(BasePattern *pattern) override {
        this->patterns->add(pattern);
    }

    virtual void on_tick(int tick) override;

    virtual void on_loop(int tick) override {};
    virtual void on_beat(int beat) override {};
    virtual void on_bar(int bar) override {};
    virtual void on_phrase(int phrase) override {};

    // todo: this on_step stuff copied from EuclidianSequencer, but maybe we want to move it back up to BaseSequencer at some point?
    virtual void on_step(int step) override;
    virtual void on_step_end(int step) override;

    #ifdef ENABLE_SHUFFLE
        virtual void on_step_shuffled(int8_t track, int step) override;
        virtual void on_step_end_shuffled(int8_t track, int step) override;
    #endif
   
};