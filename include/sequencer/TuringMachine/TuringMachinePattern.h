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

#ifdef ENABLE_PARAMETERS
    #include "parameter_inputs/AnalogParameterInputBase.h"
#endif

#ifdef ENABLE_STORAGE
    #include "outputs/SeqlibSaveableSettings.h"
#endif

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

class TuringMachinePattern : public SimplePattern 
    #ifdef ENABLE_PARAMETERS
        , public AnalogParameterInputBase<float>
    #endif
    {
    uint8_t effective_steps = STEPS_PER_BAR; //TIME_SIG_MAX_STEPS_PER_BAR;
    int default_duration = PPQN/6;
    int effective_duration = default_duration;
    int8_t current_note_number = NOTE_OFF;

    float probability = 0.1f;
    float effective_probability = probability;

    uint8_t mutation_locks[TIME_SIG_MAX_STEPS_PER_BAR];
    int mutation_lock_count = 4; // how many times a step should be played before it can mutate again
    bool mutation_lock_active = true;

    public:

    TuringMachinePattern(LinkedList<BaseOutput*> *available_outputs) 
        : SimplePattern(available_outputs) 
        #ifdef ENABLE_PARAMETERS
            , AnalogParameterInputBase<float>((char*)"TM", "Seqs", 0.005, UNIPOLAR)
        #endif
    {}

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

    inline virtual bool is_mutation_lock_active() {
        return this->mutation_lock_active;
    }
    inline virtual void set_mutation_lock_active(bool active) {
        this->mutation_lock_active = active;
    }
    inline virtual int get_mutation_lock_count() {
        return this->mutation_lock_count;
    }
    inline virtual void set_mutation_lock_count(int count) {
        this->mutation_lock_count = count;
    }
    inline virtual bool is_step_locked(int step) {
        return this->is_locked() || (mutation_locks[step % this->get_effective_steps()] > 0);
    }
    inline virtual void lock_step(int step, int number_locks_to_add = -1) {
        if (number_locks_to_add < 0) {
            number_locks_to_add = this->mutation_lock_count;
        }
        mutation_locks[step % this->get_effective_steps()] = number_locks_to_add;
    }
    inline virtual void unlock_step(int step, int number_locks_to_remove = 1) {
        if (this->is_locked()) return; // if the pattern is locked, dont allow unlocking individual steps
        if (mutation_locks[step % this->get_effective_steps()] <= number_locks_to_remove) {
            mutation_locks[step % this->get_effective_steps()] = 0;
        } else {
            mutation_locks[step % this->get_effective_steps()] -= number_locks_to_remove;
        }
    }
    inline virtual void unlock_step_completely(int step) {
        if (this->is_locked()) return; // if the pattern is locked, dont allow unlocking individual steps
        mutation_locks[step % this->get_effective_steps()] = 0;
    }

    virtual bool flipacoin() {
        float r = random(10000) / 10000.0f;
        return r < effective_probability;
    }

    virtual void process_step(int step) override {

        SimplePattern::process_step(step);
        
        bool current_state = this->query_note_on_for_step(step);

        if (is_mutation_lock_active() && is_step_locked(step)) {
            // if mutation lock is active and this step is locked, skip mutation
            unlock_step(step, 1); // reduce the lock count by 1, so that it will eventually unlock after being played a few times
            return;
        }
        
        // each time a step has played, flip its state (or not), according to our probability value
        // and potentially change its note value

        if (!locked && flipacoin()) {
            // flip the state
            if (current_state) {
                this->unset_event_for_tick(step * TICKS_PER_STEP);
            } else {
                int8_t new_note = random(128);
                this->set_event_for_tick(step * TICKS_PER_STEP, new_note, MIDI_MAX_VELOCITY, 1);
            }
            if (is_mutation_lock_active()) this->lock_step(step);
        }
    }

    #if defined(ENABLE_SCREEN) 
        virtual void create_menu_items(Menu *menu, int index, BaseSequencer *target_sequencer, int combine_settings = (TuringMachine::CombinePageOption)TuringMachine::COMBINE_NONE) override;
    #endif

    #if defined(ENABLE_PARAMETERS)
        virtual LinkedList<FloatParameter*> *getParameters(unsigned int i) override;
    #endif

    #ifdef ENABLE_STORAGE
        virtual void add_saveable_settings(int pattern_index) override {
            SimplePattern::add_saveable_settings(pattern_index);
            
            register_setting(new LSaveableSetting<int16_t>("duration", "TuringMachinePattern", &this->current_duration), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
            register_setting(new LSaveableSetting<float>("probability", "TuringMachinePattern", &this->probability), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);

            register_setting(new LSaveableSetting<bool>("mutation_lock_active", "TuringMachinePattern", &this->mutation_lock_active), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);
            register_setting(new LSaveableSetting<int>("mutation_lock_count", "TuringMachinePattern", &this->mutation_lock_count), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);

            register_setting(new SaveableByteArraySetting<uint8_t>(
                "mutation_locks", 
                "TuringMachinePattern", 
                this->mutation_locks, 
                TIME_SIG_MAX_STEPS_PER_BAR
            ), SL_SCOPE_SNAPSHOT);
            //register_setting(new SaveableMIDINoteArraySetting("sequencer_pattern", "TuringMachinePattern", &this->events, TIME_SIG_MAX_STEPS_PER_BAR), SL_SCOPE_SCENE | SL_SCOPE_PROJECT);

            register_setting(new SaveableMIDINoteArraySetting(
                "events", 
                "TuringMachinePattern", 
                this->events, 
                TIME_SIG_MAX_STEPS_PER_BAR
            ), SL_SCOPE_SNAPSHOT);
        }
    #endif

    #ifdef ENABLE_PARAMETERS
        virtual const char* getInputInfo() override {
            return "TM";
        }

        virtual void read() override {
            this->currentValue = (float)this->current_note_number / 127.0f;

            #ifdef PARAMETER_INPUTS_USE_CALLBACKS
                float normal = get_normal_value(currentValue, UNIPOLAR);
                this->on_value_read(currentValue);
                if (callback != NULL) {
                Debug_print(this->name);
                Debug_print(F(": calling callback("));
                Debug_print(normal);
                Debug_println(')');
                callback(normal);
                }
            #endif
        }

        virtual const char *getExtra() override {
            if (this->supports_pitch()) {
                static char extra_output[40];
                snprintf(
                    extra_output, 
                    40,
                    "MIDI pitch for %3.3f is %s\n", 
                    this->currentValue, 
                    //get_note_name(get_voltage_pitch()).c_str()
                    get_note_name_c(this->get_voltage_pitch())
                );
                return extra_output;
            }
            return "";
        }

        virtual bool supports_pitch() override {
            return true;
        }
        
        virtual int8_t get_voltage_pitch() override {
            // current_note_number should only change when a step actually fires
            return this->current_note_number;
        }

        // int8_t last_note_number = NOTE_OFF;
        // virtual int8_t get_voltage_pitch() override {
        //     // only report a pitch while a step is actively held; otherwise the chord player
        //     // would keep playing a stale (possibly zero-initialised) note between steps
        //     if (!this->note_held) 
        //         return this->last_note_number;
        //     this->last_note_number = this->current_note_number;
        //     return this->current_note_number;
        // }

    #endif

};