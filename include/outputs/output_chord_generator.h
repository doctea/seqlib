#pragma once

#include "output_midi.h"
#include "chord_player.h"
#include "chord_modulation_parameters.h"

// Output that consumes note events and renders full chords.
#ifdef ENABLE_SCALES
class MIDIChordGeneratorOutput : public MIDINoteOutput {
    public:
    int8_t source_to_output_note[MIDI_NUM_NOTES];
    uint8_t output_note_refcount[MIDI_NUM_NOTES];
    bool chord_modulation_parameters_added = false;

    void reset_note_mappings() {
        for (size_t i = 0; i < MIDI_NUM_NOTES; i++) {
            this->source_to_output_note[i] = NOTE_OFF;
            this->output_note_refcount[i] = 0;
        }
    }

    int8_t map_output_note(int8_t source_note) {
        if (!is_valid_note(source_note)) {
            return NOTE_OFF;
        }

        int8_t mapped_note = apply_note_limits(
            source_note,
            this->lowest_note_mode,
            this->highest_note_mode,
            this->get_effective_lowest_note(),
            this->get_effective_highest_note()
        );

        return is_valid_note(mapped_note) ? mapped_note : NOTE_OFF;
    }

    void emit_limited_note_off(int8_t source_note, uint8_t velocity) {

        source_note += this->octave * 12;

        if (!is_valid_note(source_note)) {
            return;
        }

        int8_t mapped_note = this->source_to_output_note[source_note];
        if (!is_valid_note(mapped_note)) {
            return;
        }

        this->source_to_output_note[source_note] = NOTE_OFF;

        if (this->output_note_refcount[mapped_note] > 0) {
            this->output_note_refcount[mapped_note] -= 1;
        }

        if (this->output_note_refcount[mapped_note] == 0) {
            this->output_wrapper->sendNoteOff(mapped_note, velocity, this->get_channel());
        }
    }

    void emit_limited_note_on(int8_t source_note, uint8_t velocity) {

        source_note += this->octave * 12;

        if (!is_valid_note(source_note) || !this->enabled) {
            return;
        }

        // Ensure a re-trigger from the same source note cannot leak refcounts.
        this->emit_limited_note_off(source_note, 0);

        int8_t mapped_note = this->map_output_note(source_note);
        if (!is_valid_note(mapped_note)) {
            return;
        }

        this->source_to_output_note[source_note] = mapped_note;

        if (this->output_note_refcount[mapped_note] == 0) {
            this->output_wrapper->sendNoteOn(mapped_note, velocity, this->get_channel());
        }
        this->output_note_refcount[mapped_note] += 1;
    }

    ChordPlayer chord_player = ChordPlayer(
        [=](int8_t channel, int8_t note, int8_t velocity) -> void {
            this->emit_limited_note_on(note, velocity);
        },
        [=](int8_t channel, int8_t note, int8_t velocity) -> void {
            this->emit_limited_note_off(note, velocity);
        }
    );

    MIDIChordGeneratorOutput(const char *label, IMIDINoteAndCCTarget *output_wrapper, int_fast8_t channel = 1)
        : MIDINoteOutput(label, output_wrapper, channel) {
        this->reset_note_mappings();
        this->chord_player.set_play_chords(true);
        this->chord_player.set_quantise(true);
        this->chord_player.set_selected_chord(CHORD::TRIAD);
    }

    virtual void set_enabled(bool state) override {
        if (this->enabled && !state) {
            this->chord_player.stop_all();
            this->reset_note_mappings();
        }
        BaseOutput::set_enabled(state);
    }

    virtual const char* get_menu_type_name() const override { return "Chord"; }
    virtual bool is_menu_scrollable() const override { return true; }
    virtual void add_scale_menu_items(Menu *menu) override { (void)menu; }
    virtual void add_status_menu_items(Menu *menu) override { (void)menu; }

    virtual void process() override {
        BaseOutput::process();

        if (!this->enabled) {
            this->reset();
            return;
        }

        // todo: should probably move this logic into get_note_number?
        const int8_t note = is_valid_note(this->event_value_3) ? this->event_value_3 : conductor->get_chord_root();
        const int8_t velocity = constrain((int)this->event_value_4, 0, MIDI_MAX_VELOCITY);

        if (this->event_value_2 > 0) {
            this->chord_player.trigger_off_for_pitch_because_changed(note, velocity);
        }

        if (this->event_value_1 > 0 && is_valid_note(note)) {
            this->chord_player.trigger_on_for_pitch(note, velocity, this->chord_player.get_selected_chord(), this->chord_player.get_inversion());
        }

        this->reset();
    }

    #ifdef ENABLE_SCREEN
    virtual void make_menu_items(Menu *menu, int index) override {
        MIDINoteOutput::make_menu_items(menu, index);
        LinkedList<MenuItem*> *items = this->chord_player.make_menu_items();
        for (size_t i = 0; i < items->size(); i++) {
            menu->add(items->get(i));
        }
    }
    #endif

    #ifdef ENABLE_PARAMETERS
    virtual LinkedList<FloatParameter*> *get_parameters() override {
        LinkedList<FloatParameter*> *parameters = MIDINoteOutput::get_parameters();
        if (!this->chord_modulation_parameters_added && parameters != nullptr) {
            add_chord_modulation_parameters(parameters, &this->chord_player);
            this->chord_modulation_parameters_added = true;
        }
        return parameters;
    }
    #endif

    #ifdef ENABLE_STORAGE
    virtual void setup_saveable_settings() override {
        MIDINoteOutput::setup_saveable_settings();
        register_child(&chord_player);
    }
    #endif
};

using MIDIChordOutput = MIDIChordGeneratorOutput;
#endif
