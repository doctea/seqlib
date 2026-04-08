#pragma once

#ifndef DEFAULT_OUTPUT_TYPE
    #define DEFAULT_OUTPUT_TYPE OUTPUT_TYPE::DRUMS_MIDIMUSO
#endif

#include <Arduino.h>
#include <LinkedList.h>

#include "debug.h"

#ifdef USE_TINYUSB
    #include <Adafruit_TinyUSB.h>
#endif
#include "MIDI.h"
#include "Drums.h"
#include "bpm.h"

#include "sequencer/Base/Sequencer.h"
#include "outputs/base_outputs.h"

#include "midi_helpers.h"

#include "ParameterManager.h"
#include "parameters/MIDICCParameter.h"

#include <atomic>

int8_t get_muso_note_for_drum(int8_t drum_note);

struct note_message_t {
    int8_t pitch;
    int8_t velocity;
    int8_t channel;
};
note_message_t convert_note_for_muso_drum(int8_t pitch, int8_t velocity, int8_t channel);

enum OUTPUT_TYPE : uint8_t {
    DRUMS,
    DRUMS_MIDIMUSO,
    PITCHED,
    MIN_OUTPUT_TYPE = DRUMS,
    MAX_OUTPUT_TYPE = PITCHED
};
OUTPUT_TYPE operator ++( OUTPUT_TYPE &id, int );
OUTPUT_TYPE operator --( OUTPUT_TYPE &id, int );

struct output_type_t {
    OUTPUT_TYPE type_id;
    const char *label = "n/a";
    note_message_t(*converter_func)(int8_t,int8_t,int8_t) = nullptr;
};
const output_type_t available_output_types[] = {
    { OUTPUT_TYPE::DRUMS,           "Normal",           nullptr },
    { OUTPUT_TYPE::DRUMS_MIDIMUSO,  "Drums->MidiMuso",  &convert_note_for_muso_drum },
    { OUTPUT_TYPE::PITCHED,         "Pitched",          nullptr }
};


#if defined(ARDUINO_ARCH_RP2040)
    #include "midi_usb/midi_usb_rp2040.h"
#endif

// track basic monophonic MIDI output
class MIDIBaseOutput : public BaseOutput {
    public:
    
    int8_t note_number = NOTE_OFF, last_note_number = NOTE_OFF;
    int_fast8_t channel = 1;
    int_fast8_t event_value_1, event_value_2, event_value_3, event_value_4;

    IMIDINoteAndCCTarget *output_wrapper; // = nullptr;

    MIDIBaseOutput(const char *label, IMIDINoteAndCCTarget *output_wrapper, int_fast8_t note_number, int_fast8_t channel = 1) 
        : BaseOutput(label), output_wrapper(output_wrapper), note_number(note_number), channel(channel) {
        }

    virtual int_fast8_t get_note_number() {
        return this->note_number;
    }
    virtual int_fast8_t get_last_note_number() {
        return this->last_note_number;
    }
    virtual void set_last_note_number(int_fast8_t note_number) {
        this->last_note_number = note_number;
    }
    virtual int_fast8_t get_channel() {
        return this->channel;
    }

    virtual void stop() override {
        if(is_valid_note(last_note_number)) {
            output_wrapper->sendNoteOff(last_note_number, 0, this->get_channel());
            this->set_last_note_number(NOTE_OFF);
        }
    }
    virtual void process() override {
        BaseOutput::process();

        if (should_go_off()) {
            int8_t note_number = get_last_note_number();
            Debug_printf("\t\tgoes off note\t%i\t(%s), ", note_number, get_note_name_c(note_number));
            //Serial.printf("Sending note off for node %i on note_number %i chan %i\n", i, o->get_note_number(), o->get_channel());
            if (is_enabled() && is_valid_note(note_number)) {
                output_wrapper->sendNoteOff(note_number, 0, get_channel());
                set_last_note_number(NOTE_OFF);
                //this->went_off();
            }
        }
        if (should_go_on()) {
            this->stop();

            int8_t note_number = get_note_number();
            //Serial.printf("Sending note on  for node %i on note_number %i chan %i\n", i, o->get_note_number(), o->get_channel());
            if (is_enabled() && is_valid_note(note_number)) {
                Debug_printf("\t\tMIDIBaseOutput#process: goes on note\t%i\t(%s) \n", note_number, get_note_name_c(note_number));
                set_last_note_number(note_number);
                output_wrapper->sendNoteOn(note_number, this->event_value_4, get_channel());
                this->has_gone_on = true;
            }
            //count += i;
        }
        this->reset();
    }

    virtual bool should_go_on() {
        if (this->event_value_1>0)
            return true;
        return false;
    }
    virtual bool should_go_off() {
        if (this->event_value_2>0)
            return true;
        return false;
    }
    virtual void went_on() {
        this->event_value_1 -= 1;
    }
    virtual void went_off() {
        this->event_value_2 -= 1;
    }

    // receive an event from a sequencer
    virtual void receive_event(int_fast8_t event_value_1, int_fast8_t event_value_2, int_fast8_t event_value_3, int_fast8_t event_value_4) override {
        this->event_value_1 += event_value_1;   // note ons?
        this->event_value_2 += event_value_2;   // note offs?
        this->event_value_3 = event_value_3;    // used for carrying note value
        this->event_value_4 = event_value_4;    // used for carrying velocity value
    }

    // forget the last message
    virtual void reset() {
        this->event_value_1 = this->event_value_2 = this->event_value_3 = this->event_value_4 = 0;
    }

    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override {
            // inherit parent's settings
            BaseOutput::setup_saveable_settings();

            register_setting(
                new LSaveableSetting<int_fast8_t>(
                    "MIDI Channel",
                    "MIDIBaseOutput",
                    &this->channel,
                    [=](int_fast8_t value) -> void {
                        this->channel = value;
                    }, 
                    [=](void) -> int_fast8_t {
                        return this->channel;
                    }
                ), SL_SCOPE_PROJECT  // allow MIDI channel to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
        }
    #endif
};

#ifdef ENABLE_SCALES
    #include "scales.h"
    
    // class that outputs full notes based on what's passed in
    // todo: make this and MIDINoteTriggerCountOutput inherit from each other so that they can share the same
    // quantisation and scale settings
    class MIDINoteOutput : public MIDIBaseOutput {

        protected:

        int_fast8_t octave = 3;
        int_fast8_t scale_root = SCALE_ROOT_A;
        scale_index_t scale_number = SCALE_FIRST;

        public:
            MIDINoteOutput(const char *label, IMIDINoteAndCCTarget *output_wrapper, int_fast8_t channel = 1, int_fast8_t scale_root = SCALE_ROOT_A, scale_index_t scale_number = SCALE_MAJOR, int_fast8_t octave = 3) 
                : MIDIBaseOutput(label, output_wrapper, NOTE_OFF, channel) {
                this->scale_root = scale_root;
                this->scale_number = scale_number;
                this->octave = octave;
            }

            virtual int8_t get_note_to_play() {
                return this->event_value_3;
            }

            bool quantise = false;
            virtual int_fast8_t get_note_number() override {

                int8_t note_to_play = this->get_note_to_play();

                if (!is_valid_note(note_to_play))
                    return NOTE_OFF;

                if (!this->is_quantise())
                    return note_to_play;
                else
                    return quantise_pitch_to_scale(
                        note_to_play/*+ BPM_CURRENT_BEAT_OF_PHRASE*/, 
                        this->get_effective_scale_root(), 
                        this->get_effective_scale_number()
                    );
            }

            scale_index_t get_scale_number() {
                return scale_number;
            }
            void set_scale_number(scale_index_t scale_number) {
                this->scale_number = scale_number;
            }

            virtual int8_t get_effective_scale_root() {
                int8_t effective_root = ::get_effective_scale_root(this->scale_root);

                return effective_root;
            }
            virtual scale_index_t get_effective_scale_number() {
                scale_index_t effective_scale = ::get_effective_scale_type(this->scale_number);

                return effective_scale;
            }

            int get_scale_root() {
                return this->scale_root;
            }
            void set_scale_root(int scale_root) {
                this->scale_root = scale_root;
                //base_note = scale_root * octave;
            }

            void set_note_mode(int8_t mode) {
                //this->note_mode = mode;
                this->quantise = mode == 1;
            }
            int get_note_mode() {
                //return this->note_mode;
                return this->quantise ? 1 : 0;
            }
            void set_quantise(bool v) {
                //this->note_mode = v ? 1 : 0;
                this->quantise = v;
            }
            bool is_quantise() {
                //return this->note_mode==1;
                return this->quantise;
            }

            #ifdef ENABLE_SCREEN
                //FLASHMEM
                virtual void make_menu_items(Menu *menu, int index) override;
            #endif

            #ifdef ENABLE_STORAGE
                virtual void setup_saveable_settings() override {
                    // inherit parent's settings
                    MIDIBaseOutput::setup_saveable_settings();

                    register_setting(
                        new LSaveableSetting<bool>(
                            "Quantise",
                            "MIDINoteOutput",
                            &this->quantise,
                            [=](bool value) -> void {
                                this->set_quantise(value);
                            },
                            [=](void) -> bool {
                                return this->is_quantise();
                            }
                        ), SL_SCOPE_SCENE  // allow quantise state to be saved at scene level, since it's more of a performance setting than a preference setting
                    );

                    register_setting(
                        new LSaveableSetting<int_fast8_t>(
                            "Octave",
                            "MIDINoteOutput",
                            &this->octave,
                            [=](int_fast8_t value) -> void {
                                this->octave = value;
                            },
                            [=](void) -> int_fast8_t {
                                return this->octave;
                            }
                        ), SL_SCOPE_SCENE  // allow octave to be saved at scene level, since it's more of a performance setting than a preference setting
                    );

                    register_setting(
                        new LSaveableSetting<int_fast8_t>(
                            "Scale Root",
                            "MIDINoteOutput",
                            &this->scale_root,
                            [=](int_fast8_t value) -> void {
                                this->set_scale_root(value);
                            },
                            [=](void) -> int_fast8_t {
                                return this->get_scale_root();
                            }
                        ), SL_SCOPE_SCENE  // allow scale root to be saved at scene level, since it's more of a performance setting than a preference setting
                    );

                    register_setting(
                        new LSaveableSetting<scale_index_t>(
                            "Scale",
                            "MIDINoteOutput",
                            &this->scale_number,
                            [=](scale_index_t value) -> void {
                                this->set_scale_number(value);
                            },
                            [=](void) -> scale_index_t {
                                return this->get_scale_number();
                            }
                        ), SL_SCOPE_SCENE  // allow scale to be saved at scene level, since it's more of a performance setting than a preference setting
                    );
                }
            #endif
    };

    // todo: class that can output scale degrees - for use multiple times, one output per scale degree; driven
    // individually from different step triggers, for eg polyphonic arpegiator-ish outputs

#endif


// an output that tracks MIDI drum triggers
class MIDIDrumOutput : public MIDIBaseOutput {
    public:
    MIDIDrumOutput(const char *label, IMIDINoteAndCCTarget *output_wrapper, int_fast8_t note_number, int_fast8_t channel = GM_CHANNEL_DRUMS) 
        : MIDIBaseOutput(label, output_wrapper, note_number, channel) {
    }
};

void setup_output_processor_parameters();

