#pragma once

#include <Arduino.h>

#if defined(ENABLE_PARAMETERS)
    #include <LinkedList.h>
#endif

#include "SaveableParameters.h"

#include "uClock.h"
#define NUMBER_SHUFFLE_PATTERNS 16

class BasePattern;
class SimplePattern;
class BaseOutput;
class FloatParameter;
class Menu;

class ShufflePatternWrapper {
    public:
        int8_t track_number = 0;
        int8_t step[MAX_SHUFFLE_TEMPLATE_SIZE] = {0};
        int8_t size = 16; //MAX_SHUFFLE_TEMPLATE_SIZE;
        float amount = 1.0f;

        int8_t last_sent_step[MAX_SHUFFLE_TEMPLATE_SIZE] = {0};

        ShufflePatternWrapper(int8_t track_number) {
            this->track_number = track_number;
            for (int i = 0 ; i < MAX_SHUFFLE_TEMPLATE_SIZE ; i++) {
                set_step(i, 0);
            }
            update_target();
        }

        void set_active(bool active) {
            uClock.setTrackShuffle(this->track_number, active);
        }
        bool is_active() {
            return uClock.isTrackShuffled(this->track_number);
        }

        void set_steps(int8_t *steps, int8_t size) {
            for (int i = 0 ; i < size ; i++) {
                set_step(i, steps[i]);
            }
            update_target();
        }
        void set_step(int8_t step_number, int8_t value) {
            if (step_number >= 0 && step_number < MAX_SHUFFLE_TEMPLATE_SIZE) {
                step[step_number] = value;
            }
        }
        void set_step_and_update(int8_t step_number, int8_t value) {
            set_step(step_number, value);
            update_target();
        }

        int8_t get_step(int8_t step_number) {
            if (step_number >= 0 && step_number < MAX_SHUFFLE_TEMPLATE_SIZE) {
                return step[step_number];
            }
            return 0;
        }
        void set_track_number(int8_t track_number) {
            this->track_number = track_number;
        }

        void set_amount(float amount) {
            this->amount = amount;
        }
        float get_amount() {
            return amount;
        }

        void update_target() {
            uClock.setTrackShuffleSize(this->track_number, this->size);
            if (this->amount == 0.0f) {
                uClock.setTrackShuffle(this->track_number, false);
            } else {
                uClock.setTrackShuffle(this->track_number, true);
            }
            for (int i = 0 ; i < size ; i++) {
                int t = step[i] * this->amount;
                if (t!=last_sent_step[i]) {
                    uClock.setTrackShuffleData(this->track_number, i, t);
                    last_sent_step[i] = t;
                }                
            }
        }
};

extern ShufflePatternWrapper *shuffle_pattern_wrapper[NUMBER_SHUFFLE_PATTERNS];

class BaseSequencer : virtual public ISaveableParameterHost {
    public:

    BaseSequencer() {}
    virtual ~BaseSequencer() = default;

    bool running = true;
    uint_fast8_t number_patterns = 20;
    bool debug = false;
    bool shuffle_enabled = false;

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
    virtual void on_beat(int beat) = 0;
    virtual void on_bar(int bar) = 0;
    virtual void on_phrase(int phrase) = 0;

    virtual void on_step(int step) = 0;
    virtual void on_step_end(int step) = 0;
    virtual void on_step_shuffled(int8_t track, int step) = 0;
    virtual void on_step_end_shuffled(int8_t track, int step) = 0;

    virtual bool is_shuffle_enabled() {
        return this->shuffle_enabled;
    }
    virtual void set_shuffle_enabled(bool state = true) {
        this->shuffle_enabled = state;
    }

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