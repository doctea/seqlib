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

#include "SaveableParameters.h"

#define DEFAULT_VELOCITY    MIDI_MAX_VELOCITY

#define MAX_STEPS 32

class BaseOutput;
class BaseSequencer;

class BasePattern {
    public:

    uint8_t steps = MAX_STEPS;
    uint8_t steps_per_beat = STEPS_PER_BEAT;
    bool note_held = false;
    bool locked = false;
    uint32_t ticks_per_step = PPQN / steps_per_beat;            // todo: calculate this from desired pattern length in bars, PPQN and steps

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

    #ifdef ENABLE_SCREEN
        uint16_t colour = C_WHITE;
        inline uint16_t get_colour() {
            return colour;
        }
    #endif

    virtual const char *get_summary() {
        return "??";
    }
    virtual const char *get_output_label();

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
        return this->query_note_on_for_tick(step * ticks_per_step);
    }
    virtual bool query_note_off_for_step(int step) {
        return this->query_note_off_for_tick(step * ticks_per_step);
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

    #ifdef ENABLE_SHUFFLE
        virtual void set_shuffle_track(uint8_t v) {
            this->shuffle_track = v;
        }
        virtual bool is_shuffled() {
            #ifdef SHUFFLE_MULTIPLE_TRACKS
                return uClock.isTrackShuffled(this->shuffle_track);
            #else
                return uClock.isShuffled();
            #endif
        }
        virtual uint8_t get_shuffle_track() {
            return this->shuffle_track;
        }
        virtual int8_t get_shuffle_length() {
            #ifdef ENABLE_SHUFFLE
                if (this->is_shuffled())
                    #ifdef SHUFFLE_MULTIPLE_TRACKS
                        return uClock.getTrackShuffleLength(this->get_shuffle_track());
                    #else
                        return uClock.getShuffleLength();
                    #endif
                else
            #endif
            return 0;
        }
    #endif

    #ifdef ENABLE_PARAMETERS
        LinkedList<FloatParameter*> *parameters = nullptr;
        virtual LinkedList<FloatParameter*> *getParameters(unsigned int i);
    #endif

    #ifdef ENABLE_SCREEN
        virtual void create_menu_items(Menu *menu, int index, BaseSequencer *target_sequencer, bool combine_pages = false);
    #endif

    virtual void add_saveable_parameters(int pattern_index, LinkedList<SaveableParameterBase*> *target) {
        char prefix[40];
        snprintf(prefix, 40, "track_%i_", pattern_index);
        target->add(new LSaveableParameter<uint8_t>((String(prefix) + String("steps")).c_str(), "BasePattern", &this->steps));
        target->add(new LSaveableParameter<bool>((String(prefix) + String("locked")).c_str(), "BasePattern", &this->locked));

        //target->add(new LSaveableParameter<bool>((String(prefix) + String("active_status")).c_str(), "EuclidianPattern", &this->active_status));
        #ifdef ENABLE_SHUFFLE
            target->add(new LSaveableParameter<uint8_t>((String(prefix) + String("shuffle_track")).c_str(), "EuclidianPattern", &this->shuffle_track));
        #endif

        // todo: add the rest of the params...?
    }
};

class SimplePattern : public BasePattern {
    public:

    struct event {
        byte note = NOTE_OFF;
        byte velocity = DEFAULT_VELOCITY;
        byte channel = 0;
    };

    int triggered_on_step = -1;
    int32_t triggered_on_tick = -1;
    int current_duration = PPQN;

    event *events = nullptr;

    SimplePattern(LinkedList<BaseOutput*> *available_outputs) : BasePattern(available_outputs) {
        this->events = (event*)CALLOC_FUNC(sizeof(event), steps);
    }

    virtual void set_output(BaseOutput *output) {
        this->output = output;
    }
    virtual BaseOutput *get_output() {
        return this->output;
    }

    virtual unsigned int get_step_for_tick(unsigned int tick) {
        return (tick / this->ticks_per_step) % this->get_effective_steps();
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
        /*bool debug = false;
        if (strcmp(this->get_output_label(),"Clap")==0 || strcmp(this->get_output_label(),"Kick")==0) {
            debug = true;
        }
        if (debug) Serial.printf("%s:\tprocess_step(%4i)\t", this->get_output_label(), step);
        */
        if (this->query_note_on_for_step(step)) {
            //if (debug) Serial.printf("note on for step! (ticks=%6u)", ticks);
            if (!this->note_held)
                this->trigger_on_for_step(step);
        }
        //if (debug) Serial.println();
        //debug = false;
    };
    virtual void process_step_end(int step) override {
        if (this->query_note_off_for_step((step+1) % this->get_effective_steps()) && this->note_held) {
        //if (this->note_held) {
            //Serial.printf("%i: note off for step %i!", step);
            this->trigger_off_for_step(step);
        }
    }
    virtual void process_tick(int ticks) override { 
        // check if note is held and duration has passed...
        int step = (ticks / ticks_per_step); // % steps;
        //ticks = ticks % (ticks_per_step * steps);

        if (
            this->note_held && 
            (ticks >= triggered_on_tick + this->current_duration  || ticks < triggered_on_tick)
        ) {
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