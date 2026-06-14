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

enum flexiarp_next_mode_t : int8_t {
    NEVER,             // just stick on current note, never move to next
    BOUNCE, // after each event, note is moved to the next one, and when it reaches the end, it bounces back down
    UP, // after each event, note is moved to the next one
    DOWN, // after each event, note is moved to the previous one
    RANDOM_WALK,           // move +1 or -1 at random after each event, but don't go outside the range of the scale/chord
    RANDOM_ANY,            // random from any note in chord
    // what other modes would be musically interesting..? 
    // maybe a 'next note in scale' mode, which would be like a 'scale arpeggiator' mode, but not necessarily in order (ie could be randomised)
    // maybe a 'next note in chord' mode, which would be like a 'chord arpeggiator' mode, but not necessarily in order (ie could be randomised)
    // maybe a 'next note in chord' mode, which would be like a 'chord arpeggiator' mode, but not necessarily in order (ie could be randomised)
};

extern labelled_value_list_t<int8_t> next_mode_value_labels;

class FlexiArpOutput : public MIDINoteOutput 
    #ifdef ENABLE_STORAGE
        , virtual public SHDynamic<2, 12> // parameter children; own settings
    #endif
{

    // int8_t degree_base = 1;
    int8_t degree_mod = 0;

    ProxiedValue<int8_t> degree_base = { .source = 1, .effective = 1 };
    // ProxiedValue<int8_t> degree_mod = { .source = 0, .target = 0 };
    ProxiedValue<int8_t> next_mode = { .source = NEVER, .effective = NEVER };
    ProxiedValue<int8_t> change_every = { .source = 0, .effective = 0 }; // change degree every N triggers, 0 = never change

    public:
    FlexiArpOutput(
        const char *label, 
        IMIDINoteAndCCTarget *output_wrapper, 
        int_fast8_t channel = 1, 
        int_fast8_t degree_base = 1,
        int8_t next_mode = NEVER,
        int_fast8_t change_every = 0
    ) : MIDINoteOutput(label, output_wrapper, channel)//, degree_base(degree_base)
    {
        this->degree_base.source = degree_base;
        this->degree_base.effective = degree_base;
        this->next_mode.source = next_mode;
        this->next_mode.effective = next_mode;
        this->change_every.source = change_every;
        this->change_every.effective = change_every;
    }

    int trigger_count = 0;  // track how may triggers have been received since the last reset (new phrase)

    virtual void on_restart() override {
        MIDINoteOutput::on_restart();
        trigger_count = 0;
    }
    virtual void on_phrase(uint32_t phrase) override {
        MIDINoteOutput::on_phrase(phrase);
        trigger_count = 0;
    }
    // virtual void on_bar(uint32_t bar) {}

    virtual int8_t get_degree_mod_result() {
        int8_t max_degree = get_global_chord_identity().is_valid_chord() ? 
            (get_global_chord_identity().degree-1) : 
            PITCHES_PER_SCALE;
        return (this->degree_base.effective + this->degree_mod) % max_degree;
    }

    int8_t bounce_direction = 1;    // 1 = up, -1 = down
    virtual void calculate_next_degree_mod() {
        trigger_count++;

        if (change_every.effective > 0 && (trigger_count % change_every.effective) != 0) {
            // don't change degree this time
            return;
        }

        int8_t max_degree = get_global_chord_identity().is_valid_chord() ? 
            (get_global_chord_identity().degree-1) : 
            PITCHES_PER_SCALE;

        switch (next_mode.effective) {
            // case NEVER:
            //     // do nothing
            //     break;
            case BOUNCE:
                degree_mod += bounce_direction;
                if (degree_mod >= max_degree) {
                    degree_mod = max_degree - 2; // bounce back down
                    bounce_direction = -1;
                } else if (degree_mod < 0) {
                    degree_mod = 1; // bounce back up
                    bounce_direction = 1;
                }
                break;
            case UP:
                degree_mod++;
                degree_mod = degree_mod % max_degree;
                break;
            case DOWN:
                degree_mod--;
                degree_mod = (degree_mod + max_degree) % max_degree;
                break;
            case RANDOM_WALK:
                if (random(0, 2) == 0) {
                    degree_mod--;
                } else {
                    degree_mod++;
                }
                if (degree_mod < 0) degree_mod = max_degree - 1;
                if (degree_mod >= max_degree) degree_mod = 0;
                break;
            case RANDOM_ANY:
                degree_mod = random(0, max_degree);
                if (degree_mod < 0) degree_mod = max_degree - degree_mod;
                if (degree_mod >= max_degree) degree_mod = degree_mod - max_degree;
                break;
        }
    }

    virtual int8_t get_note_to_play() override {

        if (this->degree_base.effective <= 0)
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

            int8_t degree_to_use = this->get_degree_mod_result();

            (this->degree_base.effective + this->degree_mod) % current_chord_instance.get_num_notes();

            int8_t note_to_play = get_quantise_pitch_chord_note(
                root_pitch,
                current_global_chord.type, 
                degree_to_use-1, 
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

            if (!is_valid_note(note_to_play))
                return NOTE_OFF;

            this->calculate_next_degree_mod();

            return note_to_play;
        }

        int8_t degree_to_use = this->get_degree_mod_result();

        // otherwise, return the this->degreeth note of the current scale
        int8_t note_to_play = quantise_get_root_pitch_for_degree(
            degree_to_use, 
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

        if (!is_valid_note(note_to_play))
            return NOTE_OFF;

        this->calculate_next_degree_mod();

        return note_to_play;
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
                    &this->degree_base.source
                ), SL_SCOPE_PROJECT | SL_SCOPE_SCENE // allow degree to be saved at scene or project level, since it's more of a performance setting than a preference setting
            );
            
            register_setting(
                new VarSetting<int8_t>(
                    "Next mode",
                    "FlexiArpOutput",
                    &this->next_mode.source
                ), SL_SCOPE_PROJECT | SL_SCOPE_SCENE // allow next mode to be saved at scene or project level, since it's more of a performance setting than a preference setting
            );

            register_setting(
                new VarSetting<int8_t>(
                    "Change every",
                    "FlexiArpOutput",
                    &this->change_every.source
                ), SL_SCOPE_PROJECT | SL_SCOPE_SCENE // allow change every to be saved at scene or project level, since it's more of a performance setting than a preference setting
            );
        }
    #endif

    #ifdef ENABLE_PARAMETERS
        virtual ParameterList *get_parameters() override {
            if (this->parameters!=nullptr)
                return this->parameters;
            ParameterList *params = MIDINoteOutput::get_parameters();

            // add degree parameter
            // TODO: do we want to make this modulatable..? yeah i guess probably do
            params->add(new ProxyLabelledParameter<int8_t>(
                "Base Degree",
                &this->degree_base,
                // 0, PITCHES_PER_SCALE
                &degree_value_label_list
            ));

            // add "next mode" parameter
            params->add(new ProxyLabelledParameter<int8_t>(
                "Next mode",
                &this->next_mode,
                &next_mode_value_labels
            ));

            // add "change every" parameter
            // TODO: allow to override ProxyParameter value so that 0 is displayed as "-" or never; 
            // similar to how menuitem does it?  but maybe we just use the new labelled_value_list_t
            // with a new version that just has a "single override" version
            params->add(new ProxyParameter<int8_t>(
                "Change every",
                &this->change_every.source,
                &this->change_every.effective,
                0, 16
            ));

            return params;
        }
    #endif
};

void setup_flexiarp_outputs(MIDIOutputProcessor *output_processor, IMIDINoteAndCCTarget *target_wrapper, int num_outputs = 4, int midi_channel = 1);


#endif