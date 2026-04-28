#pragma once

#include "output_midi.h"

// class that counts up all active triggers from passed-in nodes, and calculates a note from that, for eg monophonic basslines
// todo: make it so that it can also take into account the actual note values of the other nodes, not just count them, for more interesting melodic possibilities
// todo: make a base class for melodic outputs, and then have this and a more advanced version that takes into account the note values of other nodes both inherit from it
// todo: make it output chords, not just single notes

#ifdef ENABLE_SCALES

class MIDINoteTriggerCountOutput : public MIDINoteOutput {
    public:
        LinkedList<BaseOutput*> *nodes = nullptr;   // output nodes that will count towards the note calculation

        uint8_t start_count_at = 0;
        int finish_count_at = -1;

        MIDINoteTriggerCountOutput(const char *label, IMIDINoteAndCCTarget *output_wrapper, LinkedList<BaseOutput*> *nodes, int_fast8_t channel = 1, int start_count_at = 0, int finish_count_at = -1) 
            : MIDINoteOutput(label, output_wrapper, channel) {
            this->nodes = nodes;

            this->start_count_at = start_count_at;
            this->finish_count_at = finish_count_at;
            if (this->finish_count_at<0)
                this->finish_count_at = this->nodes->size() + finish_count_at;  // if finish_count_at is negative, count backwards from the end of the list
        }

        virtual int8_t get_note_to_play() override {
            int8_t note_number = get_base_note() + get_note_number_count();
            // Serial.printf(
            //     "MIDINoteTriggerCountOutput calculating note number: "
            //     "base_note is %i (%s), count is %i, result is %i (%s)\n", 
            //     get_base_note(), 
            //     get_note_name_c(get_base_note()), 
            //     get_note_number_count(), 
            //     note_number, 
            //     get_note_name_c(note_number)
            // );
            
            if (is_valid_note(note_number))
                return note_number;
            else
                return NOTE_OFF;
        }

        virtual int_fast8_t get_base_note() {
            //return this->scale_root * octave;
            // base note is determined by octave and scale root; 
            // the count of active nodes will be added to this before quantisation 
            // to get the final note number
            return (octave * 12) + this->get_effective_scale_root();   
        }

        // get the number of outputs that are also triggering this step
        virtual int_fast8_t get_note_number_count() {
            // count all the triggering notes and add that value to the root note
            // then quantise according to selected scale to get final note number
            int count = 0;
            int number_of_active_nodes = 0;
            int_fast16_t size = this->nodes->size();
            //Serial.printf("get_note_number_count in MIDINoteTriggerCountOutput: size is %i, start_count_at is %i, finish_count_at is %i\n", size, start_count_at, finish_count_at);
            for (int_fast16_t i = start_count_at ; i < finish_count_at+1 && i < size ; i++) {
                BaseOutput *o = this->nodes->get(i);
                if (o==nullptr) continue;
                if (o==this) continue;
                count += o->has_gone_on_this_time() ? (i%12) : 0;
                if (o->has_gone_on_this_time()) {
                    //Serial.printf("get_note_number_count in MIDINoteTriggerCountOutput: %i\t%s\tis active\n", i, o->label);
                    number_of_active_nodes++;
                } else {
                    //Serial.printf("get_note_number_count in MIDINoteTriggerCountOutput: %i\t%s\tnot active\n", i, o->label);
                }
            }
            Debug_printf("get_note_number_count in MIDINoteTriggerCountOutput is %i\n", count);
            //return base_note + quantise_pitch_to_scale(count);

            // test mode, increment over 2 octaves to test scale quantisation
            // best used with pulses = 6 so that it loops round
            /*static int count = 0;
            count++;
            count %= 24;*/

            return count;
        }

        #ifdef ENABLE_SCREEN
            virtual const char* get_menu_type_name() const { return "TriggerCountOutput"; }
        #endif


};

#endif