// Emulate the Music Thing Turing Machine as a sequencer pattern. 

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
    int default_duration = PPQN/3;
    int effective_duration = PPQN/2;
    int8_t current_note_number = NOTE_OFF;
    float probability = 0.1f;

    // for random note generation, we can specify a range of notes to choose from
    // todo: crib the way this wotks from Nexus6
    int8_t lowest_note = MIDI_MIN_NOTE;
    int8_t highest_note = MIDI_MAX_NOTE;

    public:

    TuringMachinePattern(LinkedList<BaseOutput*> *available_outputs) : SimplePattern(available_outputs) {
        current_duration = PPQN/2;
    }

    virtual const char *get_summary() override {
        return "Turing Machine";
    }

    virtual void trigger_on_for_step(int step) override;
    virtual void trigger_off_for_step(int step) override;

    // duration of the note about to be played
    virtual int8_t get_tick_duration() {
        return effective_duration;
    }

    virtual void process_step(int step) override {

        SimplePattern::process_step(step);
        
        bool current_state = this->query_note_on_for_step(step);
        
        // each time a step has played, flip its state (or not), according to our probability value
        // and potentially change its note value

        float r = random(10000) / 10000.0f;
        if (r < probability) {
            // flip the state
            if (current_state) {
                this->unset_event_for_tick(step * ticks_per_step);
            } else {
                int8_t new_note = random(lowest_note, highest_note);
                this->set_event_for_tick(step * ticks_per_step, new_note, MIDI_MAX_VELOCITY, 1);
            }
        }

    }

    #if defined(ENABLE_SCREEN) 
        virtual void create_menu_items(Menu *menu, int index, BaseSequencer *target_sequencer, int combine_settings = (TuringMachine::CombinePageOption)TuringMachine::COMBINE_NONE) override;
    #endif

    #if defined(ENABLE_PARAMETERS)
        virtual LinkedList<FloatParameter*> *getParameters(unsigned int i) override;
    #endif

    virtual void add_saveable_parameters(int pattern_index, LinkedList<SaveableParameterBase*> *target) override {
        SimplePattern::add_saveable_parameters(pattern_index, target);
        char prefix[40];
        snprintf(prefix, 40, "track_%i_", pattern_index);

        // need to remove the parent's 'steps' parameter and replace it with the one for this pattern 
        // which uses arguments.steps instead of BasePattern::steps
        for (unsigned int i = 0 ; i < target->size() ; i++) {
            if (strcmp(target->get(i)->label, "steps")==0) {
                delete target->get(i);
                target->remove(i);
                break;
            }
        }

        target->add(new LSaveableParameter<uint8_t>((String(prefix) + String("steps")).c_str(), "TuringMachinePattern", &this->steps));
        target->add(new LSaveableParameter<int>((String(prefix) + String("duration")).c_str(), "TuringMachinePattern", &this->current_duration));
        target->add(new LSaveableParameter<float>((String(prefix) + String("probability")).c_str(), "TuringMachinePattern", &this->probability));
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