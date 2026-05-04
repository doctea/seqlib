// okay, so this is our 'flexiarp' output.

// the idea is that it will expose N Output nodes, which each node corresponding to a degree in 
// the scale or chord; so when triggered, 
// the node for degree 1 will output the root note, degree 2 will output the second, etc.
// those results should then be output as MIDI notes.
#ifdef ENABLE_SCALES

#pragma once

#include "output_midi.h"
#include "conductor.h"

class MIDIOutputProcessor;

class FlexiArpOutput : public MIDINoteOutput 
    #ifdef ENABLE_STORAGE
        , virtual public SHDynamic<2, 12> // parameter children; own settings
    #endif
{

    int8_t degree = 1;

    public:
    FlexiArpOutput(
        const char *label, 
        IMIDINoteAndCCTarget *output_wrapper, 
        int_fast8_t channel = 1, 
        int_fast8_t degree = 0
    ) : MIDINoteOutput(label, output_wrapper, channel), 
        degree(degree) {
    }

    virtual int8_t get_note_to_play() override {

        if (this->degree <= 0)
            return NOTE_OFF;

        // get the this->degreeth note of the current scale
        chord_identity_t current_global_chord = get_global_chord_identity();

        // if its a valid chord, return the note
        if (current_global_chord.is_valid_chord()) {
            // get the pitch for the this->degreeth chord tone
            chord_instance_t current_chord_instance;
            current_chord_instance.set_from_chord_identity(
                current_global_chord, 
                get_effective_scale_root(), 
                get_effective_scale_number()
            );

            int8_t root_pitch = current_chord_instance.chord_root;

            int8_t note_to_play = get_quantise_pitch_chord_note(
                root_pitch,
                current_global_chord.type, 
                this->degree-1, 
                get_effective_scale_root(), 
                get_effective_scale_number(), 
                current_chord_instance.chord.inversion
            );

            note_to_play += octave * 12;

            note_to_play = apply_note_limits(
                note_to_play,
                this->lowest_note_mode,
                this->highest_note_mode,
                this->get_effective_lowest_note(),
                this->get_effective_highest_note()
            );

            return is_valid_note(note_to_play) ? note_to_play : NOTE_OFF;
        }

        // otherwise, return the this->degreeth note of the current scale
        int8_t note_to_play = quantise_get_root_pitch_for_degree(
            this->degree, 
            this->get_effective_scale_root(),
            this->get_effective_scale_number()
        );

        note_to_play += octave * 12;

        note_to_play = apply_note_limits(
            note_to_play,
            this->lowest_note_mode,
            this->highest_note_mode,
            this->get_effective_lowest_note(),
            this->get_effective_highest_note()
        );

        return is_valid_note(note_to_play) ? note_to_play : NOTE_OFF;
    }

    virtual int_fast8_t get_note_number() override {
        return this->get_note_to_play();
    }

    #ifdef ENABLE_SCREEN
        // need to add a menuitem for the degree setting
        virtual void make_menu_items(Menu *menu, int index) override;
        virtual const char* get_menu_type_name() const { return "FlexiArp"; }
    #endif

    // need to add a saveable setting for the degree setting
    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override {
            // inherit parent's settings
            MIDINoteOutput::setup_saveable_settings();

            register_setting(
                new VarSetting<int8_t>(
                    "Degree",
                    "FlexiArpOutput",
                    &this->degree
                ), SL_SCOPE_PROJECT | SL_SCOPE_SCENE // allow degree to be saved at scene or project level, since it's more of a performance setting than a preference setting
            );
        }
    #endif
};

void setup_flexiarp_outputs(MIDIOutputProcessor *output_processor, IMIDINoteAndCCTarget *target_wrapper, int num_outputs = 4, int midi_channel = 1);


#endif