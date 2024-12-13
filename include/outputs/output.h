#pragma once

//#include "Config.h"

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

#include "sequencer/Sequencer.h"
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
    
    int_fast8_t note_number = -1, last_note_number = -1;
    int_fast8_t channel = GM_CHANNEL_DRUMS;
    int_fast8_t event_value_1, event_value_2, event_value_3;

    IMIDINoteAndCCTarget *output_wrapper = nullptr;

    MIDIBaseOutput(const char *label, int_fast8_t note_number, IMIDINoteAndCCTarget *output_wrapper) 
        : BaseOutput(label), note_number(note_number), output_wrapper(output_wrapper) {}

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
            Debug_printf("\t\tgoes on note\t%i\t(%s), ", note_number, get_note_name_c(note_number));
            //Serial.printf("Sending note on  for node %i on note_number %i chan %i\n", i, o->get_note_number(), o->get_channel());
            if (is_enabled() && is_valid_note(note_number)) {
                set_last_note_number(note_number);
                output_wrapper->sendNoteOn(note_number, MIDI_MAX_VELOCITY, get_channel());
                //this->went_on();
            }
            //count += i;
        }
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
    virtual void receive_event(int_fast8_t event_value_1, int_fast8_t event_value_2, int_fast8_t event_value_3) override {
        this->event_value_1 += event_value_1;
        this->event_value_2 += event_value_2;
        this->event_value_3 += event_value_3;
    }

    // forget the last message
    virtual void reset() {
        this->event_value_1 = this->event_value_2 = this->event_value_3 = 0;
    }
};

// an output that tracks MIDI drum triggers
class MIDIDrumOutput : public MIDIBaseOutput {
    public:
    MIDIDrumOutput(const char *label, int_fast8_t note_number, IMIDINoteAndCCTarget *output_wrapper) 
        : MIDIBaseOutput(label, note_number, output_wrapper) {
        this->channel = GM_CHANNEL_DRUMS;
    }
    MIDIDrumOutput(const char *label, int_fast8_t note_number, int_fast8_t channel, IMIDINoteAndCCTarget *output_wrapper) 
        : MIDIBaseOutput(label, note_number, output_wrapper) {
        this->channel = channel;
    }
};

#ifdef ENABLE_SCALES
    #include "scales.h"
    
    // class that counts up all active triggers from passed-in nodes, and calculates a note from that, for eg monophonic basslines
    class MIDINoteTriggerCountOutput : public MIDIBaseOutput {
        public:
            LinkedList<BaseOutput*> *nodes = nullptr;   // output nodes that will count towards the note calculation

            int_fast8_t octave = 3;
            int_fast8_t scale_root = SCALE_ROOT_A;
            SCALE scale_number = SCALE::MAJOR;
            //int base_note = scale_root * octave;

            MIDINoteTriggerCountOutput(const char *label, LinkedList<BaseOutput*> *nodes, IMIDINoteAndCCTarget *output_wrapper, int_fast8_t channel = 1, int_fast8_t scale_root = SCALE_ROOT_A, SCALE scale_number = SCALE::MAJOR, int_fast8_t octave = 3) 
                : MIDIBaseOutput(label, 0, output_wrapper) {
                this->channel = channel;
                this->nodes = nodes;

                this->octave = octave;
                this->scale_root = scale_root;
                this->scale_number = scale_number;
                //this->base_note = scale_root * octave;
            }

            //int note_mode = 0;
            bool quantise = false;
            virtual int_fast8_t get_note_number() override {
                if (!this->is_quantise())
                    return get_base_note() + get_note_number_count();
                else
                    return quantise_pitch(get_base_note() + get_note_number_count()/*+ BPM_CURRENT_BEAT_OF_PHRASE*/, scale_root, scale_number);
            }

            // get the number of outputs that are also triggering this step
            virtual int_fast8_t get_note_number_count() {
                // count all the triggering notes and add that value ot the root note
                // then quantise according to selected scale to get final note number
                int count = 0;
                uint_fast16_t size = this->nodes->size();
                for (uint_fast16_t i = 0 ; i < size ; i++) {
                    BaseOutput *o = this->nodes->get(i);
                    if (o==this) continue;
                    count += o->should_go_on() ? (i%12) : 0;
                }
                Debug_printf("get_note_number in MIDINoteTriggerCountOutput is %i\n", count);
                //return base_note + quantise_pitch(count);

                // test mode, increment over 2 octaves to test scale quantisation
                // best used with pulses = 6 so that it loops round
                /*static int count = 0;
                count++;
                count %= 24;*/

                return count;
            }

            virtual int_fast8_t get_base_note() {
                //return this->scale_root * octave;
                return (octave * 12) + scale_root;
            }

            SCALE get_scale_number() {
                return scale_number;
            }
            void set_scale_number(SCALE scale_number) {
                this->scale_number = scale_number;
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

    };
#endif

void setup_output_parameters();
