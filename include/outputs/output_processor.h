#pragma once

//#include "Config.h"

#include "output.h"
#include "envelopes.h"

// holds individual output nodes and processes them (eg queries them for the pitch and sends note on/offs)
class BaseOutputProcessor : public ISaveableSettingHost {

    bool enabled = true;

    public:
        virtual void process() = 0;

        virtual bool is_enabled() {
            return this->enabled;
        }
        virtual void set_enabled(bool v = true) {
            this->enabled = v;
        }

};

//#include "envelopes.h"

// handles MIDI output; 
// possibly todo: move MIDIOutputWrapper stuff out of usb_midi_clocker and into a library and use that here?
class MIDIOutputProcessor : public BaseOutputProcessor {
    public:

    LinkedList<BaseOutput*> *nodes = new LinkedList<BaseOutput*>();
    IMIDINoteAndCCTarget *output_target = nullptr;

    #ifdef ENABLE_SCALES
        bool    global_quantise_on = false, global_quantise_chord_on = false;
        scale_identity_t global_scale_identity = {SCALE_MAJOR, SCALE_ROOT_C};
        chord_identity_t global_chord_identity = {CHORD::TRIAD, -1, 0};
    #endif

    MIDIOutputProcessor(IMIDINoteAndCCTarget *output_target) : BaseOutputProcessor(), output_target(output_target) {
        #ifdef ENABLE_SCALES
            set_global_scale_identity_target(&this->global_scale_identity);
            set_global_chord_identity_target(&this->global_chord_identity);
        #endif
    }
    virtual void addNode(BaseOutput* node) {
        this->nodes->add(node);
    }
    virtual void addDrumNode(const char *label, byte note_number) {
        this->addNode(new MIDIDrumOutput(label, this->output_target, note_number));
    }

    virtual BaseOutput *get_output_for_label(const char *label) {
        const uint_fast8_t size = this->nodes->size();
        for (uint_fast8_t i = 0 ; i < size ; i++) {
            BaseOutput *o = this->nodes->get(i);
            if (o->matches_label(label))
                return o;
        }
        return nullptr;
    }

    //virtual void on_tick(uint32_t ticks) {
        //if (is_bpm_on_sixteenth(ticks)) {

    // ask all the nodes to do their thing; send the results out to our output device
    virtual void process() {
        Debug_println("process-->");
        //static int count = 0;
        //midi->sendNoteOff(35 + count, 0, 1);
        /*for (int i = 0 ; i < this->nodes.size() ; i++) {
            BaseOutput *n = this->nodes.get(i);
            n->stop();
        }*/
        //count = 0;
        const uint_fast8_t size = this->nodes->size();
        for (uint_fast8_t i = 0 ; i < size ; i++) {
            BaseOutput *o = this->nodes->get(i);
            Debug_printf("\tnode %i\n", i);
            o->process();
            Debug_println();
        }
        /*if (count>0) {
            Serial.printf("sending combo note %i\n", count);
            midi->sendNoteOn(35 + count, 127, 1);
            //count = 35;
        }*/

        for (uint_fast8_t i = 0 ; i < size ; i++) {
            this->nodes->get(i)->reset();
        }

        Debug_println(".end.");
    }

    virtual void loop() {
        const uint_fast8_t size = this->nodes->size();
        for (uint_fast8_t i = 0 ; i < size ; i++) {
            BaseOutput *o = this->nodes->get(i);
            Debug_printf("\tnode %i\n", i);
            if (o!=nullptr)
                o->loop();
            Debug_println();
        }
    }

    // configure target sequencer to use the output nodes held by this OutputProcessor
    // requires patterns and nodes to have already been created?
    FLASHMEM
    virtual void configure_sequencer(BaseSequencer *sequencer) {
        #ifdef DEBUG_ENVELOPES
            sequencer->configure_pattern_output(11, this->nodes->get(11));
        #else
            const uint_fast8_t size = this->nodes->size();
            for (uint_fast8_t i = 0 ; i < size ; i++) {
                sequencer->configure_pattern_output(i, this->nodes->get(i));
            }
        #endif
    }

    #ifdef ENABLE_PARAMETERS
        FLASHMEM
        virtual void setup_parameters() {
            for (unsigned int i = 0 ; i < this->nodes->size() ; i++) {
                //Serial.printf("MIDIOutputProcessor#setup_parameters processing item [%i/%i]\n", i+1, this->nodes->size());
                //Serial_flush();
                parameter_manager->addParameters(this->nodes->get(i)->get_parameters());
            }
        }
    #endif

    #ifdef ENABLE_SCREEN
        //FLASHMEM
        virtual void create_menu_items(bool combine_pages = false);
    #endif

    virtual void setup_saveable_settings() override {
        // inherit parent's settings
        BaseOutputProcessor::setup_saveable_settings();
        const uint_fast8_t size = this->nodes->size();
        for (uint_fast8_t i = 0 ; i < size ; i++) {
            BaseOutput *o = this->nodes->get(i);
            if (o==nullptr) continue;

            register_child(o);
            o->setup_saveable_settings();
        }
    }
};


class HalfDrumKitMIDIOutputProcessor : public MIDIOutputProcessor {
    public:
        HalfDrumKitMIDIOutputProcessor(IMIDINoteAndCCTarget *output_target) : MIDIOutputProcessor(output_target) {
            this->addDrumNode("Kick",          GM_NOTE_ELECTRIC_BASS_DRUM);
            //this->addDrumNode("HiTom",         GM_NOTE_HIGH_TOM);
            //this->addDrumNode("LoTom",         GM_NOTE_LOW_TOM);
            this->addDrumNode("Stick",         GM_NOTE_SIDE_STICK);
            this->addDrumNode("Snare",         GM_NOTE_ELECTRIC_SNARE);
            this->addDrumNode("Cymbal 1",      GM_NOTE_CRASH_CYMBAL_1);
            //this->addDrumNode("Tamb",          GM_NOTE_TAMBOURINE);
            //this->addDrumNode("HiTom",         GM_NOTE_HIGH_TOM);
            //this->addDrumNode("LoTom",         GM_NOTE_LOW_TOM);
            this->addDrumNode("Clap",          GM_NOTE_HAND_CLAP);

            this->addDrumNode("PHH",           GM_NOTE_PEDAL_HI_HAT);
            this->addDrumNode("OHH",           GM_NOTE_OPEN_HI_HAT);
            this->addDrumNode("CHH",           GM_NOTE_CLOSED_HI_HAT);
            #ifdef ENABLE_ENVELOPES
                this->addNode(new EnvelopeOutput("Cymbal 2", output_target,    GM_NOTE_CRASH_CYMBAL_2, MUSO_CV_CHANNEL, MUSO_CC_CV_1));
                //this->addNode(new EnvelopeOutput("Splash", output_target,    GM_NOTE_SPLASH_CYMBAL,  MUSO_CV_CHANNEL, MUSO_CC_CV_2));
                //this->addNode(new EnvelopeOutput("Vibra", output_target,     GM_NOTE_VIBRA_SLAP,     MUSO_CV_CHANNEL, MUSO_CC_CV_3));
                //this->addNode(new EnvelopeOutput("Ride Bell", output_target, GM_NOTE_RIDE_BELL,      MUSO_CV_CHANNEL, MUSO_CC_CV_4));
                //this->addNode(new EnvelopeOutput("Ride Cymbal", output_target, GM_NOTE_RIDE_CYMBAL_1,MUSO_CV_CHANNEL, MUSO_CC_CV_5));*/
            #else
                this->addDrumNode("Cymbal 2",      GM_NOTE_CRASH_CYMBAL_2); // todo: turn these into something like an EnvelopeOutput?
                //this->addDrumNode("Splash",        GM_NOTE_SPLASH_CYMBAL);  // todo: turn these into something like an EnvelopeOutput?
                //this->addDrumNode("Vibra",         GM_NOTE_VIBRA_SLAP);     // todo: turn these into something like an EnvelopeOutput?
                //this->addDrumNode("Ride Bell",     GM_NOTE_RIDE_BELL);      // todo: turn these into something like an EnvelopeOutput?
                //this->addDrumNode("Ride Cymbal",   GM_NOTE_RIDE_CYMBAL_1);  // todo: turn these into something like an EnvelopeOutput?
            #endif
    
            #ifdef ENABLE_SCALES
                //this->addNode(new MIDINoteTriggerCountOutput("Bass", this->nodes, output_target));
                //this->nodes->get(this->nodes->size()-1)->disabled = false;
                //this->nodes->get(0)->is_ = false;
            #endif
        }
};


class FullDrumKitMIDIOutputProcessor : public MIDIOutputProcessor {
    public:
        FullDrumKitMIDIOutputProcessor(IMIDINoteAndCCTarget *output_target) : MIDIOutputProcessor(output_target) {
            this->addDrumNode("Kick",          GM_NOTE_ELECTRIC_BASS_DRUM);
            this->addDrumNode("Stick",         GM_NOTE_SIDE_STICK);
            this->addDrumNode("Clap",          GM_NOTE_HAND_CLAP);
            this->addDrumNode("Snare",         GM_NOTE_ELECTRIC_SNARE);
            this->addDrumNode("Cymbal 1",      GM_NOTE_CRASH_CYMBAL_1);
            this->addDrumNode("Tamb",          GM_NOTE_TAMBOURINE);
            this->addDrumNode("HiTom",         GM_NOTE_HIGH_TOM);
            this->addDrumNode("LoTom",         GM_NOTE_LOW_TOM);
            this->addDrumNode("PHH",           GM_NOTE_PEDAL_HI_HAT);
            this->addDrumNode("OHH",           GM_NOTE_OPEN_HI_HAT);
            this->addDrumNode("CHH",           GM_NOTE_CLOSED_HI_HAT);
            #ifdef ENABLE_ENVELOPES
                this->addNode(new EnvelopeOutput("Cymbal 2",    output_target, GM_NOTE_CRASH_CYMBAL_2, MUSO_CV_CHANNEL, MUSO_CC_CV_1));
                this->addNode(new EnvelopeOutput("Splash",      output_target, GM_NOTE_SPLASH_CYMBAL,  MUSO_CV_CHANNEL, MUSO_CC_CV_2));
                this->addNode(new EnvelopeOutput("Vibra",       output_target, GM_NOTE_VIBRA_SLAP,     MUSO_CV_CHANNEL, MUSO_CC_CV_3));
                this->addNode(new EnvelopeOutput("Ride Bell",   output_target, GM_NOTE_RIDE_BELL,      MUSO_CV_CHANNEL, MUSO_CC_CV_4));
                this->addNode(new EnvelopeOutput("Ride Cymbal", output_target, GM_NOTE_RIDE_CYMBAL_1,  MUSO_CV_CHANNEL, MUSO_CC_CV_5));
            #else
                this->addDrumNode("Cymbal 2",      GM_NOTE_CRASH_CYMBAL_2); // todo: turn these into something like an EnvelopeOutput?
                this->addDrumNode("Splash",        GM_NOTE_SPLASH_CYMBAL);  // todo: turn these into something like an EnvelopeOutput?
                this->addDrumNode("Vibra",         GM_NOTE_VIBRA_SLAP);     // todo: turn these into something like an EnvelopeOutput?
                this->addDrumNode("Ride Bell",     GM_NOTE_RIDE_BELL);      // todo: turn these into something like an EnvelopeOutput?
                this->addDrumNode("Ride Cymbal",   GM_NOTE_RIDE_CYMBAL_1);  // todo: turn these into something like an EnvelopeOutput?
            #endif 
        }
};

#ifdef ENABLE_SCALES
    #include "scales.h"
    class FullDrumKitAndBassMIDIOutputProcessor : public FullDrumKitMIDIOutputProcessor {
        public:
            FullDrumKitAndBassMIDIOutputProcessor(IMIDINoteAndCCTarget *output_target) : FullDrumKitMIDIOutputProcessor(output_target) {
                this->addNode(
                    new MIDINoteTriggerCountOutput(
                        "Bass", 
                        output_target, 
                        this->nodes,
                        3
                    )
                );
            }
    };

    class FullDrumKitAndBassAndChordsMIDIOutputProcessor : public FullDrumKitAndBassMIDIOutputProcessor {
        public:
            FullDrumKitAndBassAndChordsMIDIOutputProcessor(IMIDINoteAndCCTarget *output_target) : FullDrumKitAndBassMIDIOutputProcessor(output_target) {
                this->addNode(
                    new MIDINoteOutput(
                        "Melody",
                        output_target,
                        1
                    )
                );
            }
    };
#endif


void setup_output(IMIDINoteAndCCTarget *output_target, MIDIOutputProcessor *processor = nullptr);

//extern MIDIOutputWrapper *output_wrapper;
extern MIDIOutputProcessor *output_processor;

