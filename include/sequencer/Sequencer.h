#pragma once

#include <Arduino.h>

#if defined(ENABLE_PARAMETERS)
    #include <LinkedList.h>
#endif

#include "SaveableParameters.h"

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
    virtual SimplePattern *get_pattern(unsigned int pattern) = 0;

    virtual bool is_running() {
        return this->running;
    }
    virtual void set_playing(bool state = true) {
        this->running = state;
    }

    //virtual void process_tick_internal(int tick) = 0;

    virtual void on_loop(int tick) = 0;
    virtual void on_tick(int tick) = 0;
    virtual void on_step(int step) = 0;
    virtual void on_step_end(int step) = 0;
    virtual void on_beat(int beat) = 0;
    virtual void on_bar(int bar) = 0;
    virtual void on_phrase(int phrase) = 0;

    virtual void configure_pattern_output(int index, BaseOutput *output);
    
    #if defined(ENABLE_PARAMETERS)
        virtual LinkedList<FloatParameter*> *getParameters();
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