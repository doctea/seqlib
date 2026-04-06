// Emulate the Music Thing Turing Machine as a sequencer pattern. 

// todo: also have this act as a ParameterInput, so that it can be used as a sort of random modulation source for other patterns/parameters?

#pragma once

#ifndef FLASHMEM
    #define FLASHMEM
#endif

#include "debug.h"
#include <LinkedList.h>
#include "../Base/Patterns.h"
#include "../Base/Sequencer.h"
#include <bpm.h>
#include <midi_helpers.h>

namespace TuringMachine {    
    enum CombinePageOption {
        COMBINE_NONE = 0,
        COMBINE_MODULATION_WITH_MAIN = 1,
        // COMBINE_LOCKS_WITH_CIRCLE = 1,
        // COMBINE_MUTATION_WITH_LOCKS = 2,
        // COMBINE_MODULATION_WITH_MUTATION = 4,
        // COMBINE_PATTERN_MODULATION_WITH_PATTERN = 8,
        COMBINE_ALL = 15
    };
}


using namespace TuringMachine;

class TuringMachinePattern : public SimplePattern {
    uint8_t effective_steps = MAX_STEPS;
    int default_duration = PPQN/6;
    int effective_duration = default_duration;
    int8_t current_note_number = NOTE_OFF;

    float probability = 0.1f;
    float effective_probability = probability;

    // for random note generation, we can specify a range of notes to choose from
    // todo: crib the way this works from Nexus6
    int8_t lowest_note = MIDI_MIN_NOTE;
    int8_t highest_note = MIDI_MAX_NOTE;
    int8_t effective_lowest_note = MIDI_MIN_NOTE;
    int8_t effective_highest_note = MIDI_MAX_NOTE;

    public:

    TuringMachinePattern(LinkedList<BaseOutput*> *available_outputs) : SimplePattern(available_outputs) {
    }

    virtual const char *get_summary() override {
        return "Turing Machine";
    }

    virtual void trigger_on_for_step(int step) override;
    virtual void trigger_off_for_step(int step) override;

    // duration of the note about to be played
    virtual int8_t get_tick_duration() override {
        return effective_duration;
    }

    virtual int8_t get_effective_steps() {
        return this->effective_steps;
    }

    virtual bool flipacoin() {
        float r = random(10000) / 10000.0f;
        return r < effective_probability;
    }

    virtual void process_step(int step) override {

        SimplePattern::process_step(step);
        
        bool current_state = this->query_note_on_for_step(step);
        
        // each time a step has played, flip its state (or not), according to our probability value
        // and potentially change its note value

        if (!locked && flipacoin()) {
            // flip the state
            if (current_state) {
                this->unset_event_for_tick(step * TICKS_PER_STEP);
            } else {
                //int8_t new_note = random(effective_lowest_note, effective_highest_note);
                int8_t new_note = random(128); // choose from the full range of MIDI notes, then transpose into getLowestNote/getHighestNote range if necessary -- this seems to produce more interesting results than just choosing from within the range to begin with
                this->set_event_for_tick(step * TICKS_PER_STEP, new_note, MIDI_MAX_VELOCITY, 1);
            }
        }
    }

    #if defined(ENABLE_SCREEN) 
        virtual void create_menu_items(Menu *menu, int index, BaseSequencer *target_sequencer, int combine_settings = (TuringMachine::CombinePageOption)TuringMachine::COMBINE_NONE) override;
    #endif

    #if defined(ENABLE_PARAMETERS)
        virtual LinkedList<FloatParameter*> *getParameters(unsigned int i) override;
    #endif

    virtual void add_saveable_settings(int pattern_index) override {

        register_setting(new LSaveableSetting<int16_t>("duration", "TuringMachinePattern", &this->current_duration), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
        register_setting(new LSaveableSetting<float>("probability", "TuringMachinePattern", &this->probability), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
        register_setting(new LSaveableSetting<int8_t>("lowest_note", "TuringMachinePattern", &this->lowest_note), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
        register_setting(new LSaveableSetting<int8_t>("highest_note", "TuringMachinePattern", &this->highest_note), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
    }

    virtual void setLowestNote(int8_t note) {
        // don't allow highest note to be set higher than highest note
        if (note > this->getHighestNote())
            note = this->getHighestNote();
        if (!is_valid_note(note)) 
            note = MIDI_MIN_NOTE;
        this->lowest_note = note;
    }
    virtual int8_t getLowestNote() {
        return this->lowest_note;
    }

    virtual void setHighestNote(int8_t note) {
        // don't allow highest note to be set lower than lowest note
        if (note < this->getLowestNote())
            note = this->getLowestNote();
        if (!is_valid_note(note)) 
            note = MIDI_MAX_NOTE;
        this->highest_note = note;
    }
    virtual int8_t getHighestNote() {
        return this->highest_note;
    }
};