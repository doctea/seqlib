#pragma once

//#include "Config.h"

#include "output.h"
#include "envelopes.h"

#ifdef ENABLE_SCALES
    #include "scales.h"
#endif

// holds individual output nodes and processes them (eg queries them for the pitch and sends note on/offs)
class BaseOutputProcessor 
    #ifdef ENABLE_STORAGE
        : virtual public SHDynamic<4, 6>  // 1-2 output children; 8+ settings
    #endif
    {
    bool enabled = true;

    public:
        virtual void process() = 0;

        virtual bool is_enabled() {
            return this->enabled;
        }
        virtual void set_enabled(bool v = true) {
            this->enabled = v;
        }

        #ifdef ENABLE_STORAGE
            virtual void setup_saveable_settings() {
                register_setting(new LSaveableSetting<bool>(
                        "Output enabled",
                        "BaseOutputProcessor",
                        &this->enabled,
                        [=](bool v) { this->set_enabled(v); },
                        [=]() -> bool { return this->is_enabled(); }
                    ), SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow enabled state to be saved at scene or project level, since it's more of a preference setting than a performance setting
                );
            }
        #endif

        #ifdef ENABLE_SCALES
        virtual void notify_harmony_changed(const scale_identity_t&scale, const chord_identity_t& chord) = 0;
        #endif

        virtual BaseOutput *get_output_for_label(const char *label) = 0;
        virtual LinkedList<BaseOutput*> *get_available_outputs() = 0;

        virtual void loop() = 0;
        virtual void on_bar(uint32_t bar_number) {};
        virtual void on_phrase(uint32_t phrase_number) {};
        virtual void on_restart() {}

         // configure target sequencer to use the output nodes held by this OutputProcessor
};

//#include "envelopes.h"

// handles MIDI output; 
// possibly todo: move MIDIOutputWrapper stuff out of usb_midi_clocker and into a library and use that here?
class MIDIOutputProcessor : public BaseOutputProcessor 
    #ifdef ENABLE_STORAGE
        , virtual public SHDynamic<26, 12>  // 26 output children; 8+ settings
    #endif
    {
    public:

    LinkedList<BaseOutput*> *nodes = new LinkedList<BaseOutput*>();
    IMIDINoteAndCCTarget *output_target = nullptr;

    MIDIOutputProcessor(IMIDINoteAndCCTarget *output_target) : BaseOutputProcessor(), output_target(output_target) {
        #ifdef ENABLE_STORAGE
            set_path_segment("MIDIOutputProcessor");
        #endif
    }
    virtual void addNode(BaseOutput* node) {
        this->nodes->add(node);
    }
    virtual void addDrumNode(const char *label, byte note_number) {
        this->addNode(new MIDIDrumOutput(label, this->output_target, note_number));
    }

    virtual BaseOutput *get_output_for_label(const char *label) override {
        for (auto* o : *this->nodes) {
            if (o->matches_label(label))
                return o;
        }
        return nullptr;
    }

    virtual LinkedList<BaseOutput*> *get_available_outputs() override {
        return this->nodes;
    }

    // ask all the nodes to do their thing; send the results out to our output device
    virtual void process() {
        Debug_println("process-->");

        // experiment: choke groups / singleton-hit drum mode
        // so for now, let's assume that all the drum outputs are in a single choke group, and we want to only allow one of them to be on at a time.  
        // so we will first query all the nodes to see which ones want to go on, and if more than one does, then:-
        // - the one that has been allowed to play the fewest times so far will be the one that is allowed to go on, and the others will be told not to go on.
        // - if there is a tie, then the lowest-numbered (first) ones will win.
        int output_to_allow_index = -1;
        BaseOutput *current_winner = nullptr;
        for (int i = 0 ; i < this->nodes->size(); ++i) {
            BaseOutput *node = this->nodes->get(i);
            if (node->choke_group != 1) continue; // only consider nodes in choke group 1 for now
            if (node==nullptr) continue;
            if (node->should_go_on()) {
                if (output_to_allow_index<0) {
                    output_to_allow_index = i;
                } else {
                    // we have a tie; check which one has been allowed to go on the fewest times so far
                    current_winner = this->nodes->get(output_to_allow_index);
                    if (node->event_on_count < current_winner->event_on_count) {
                        output_to_allow_index = i;
                    }
                }
            }
        }

        if (current_winner != nullptr)
            current_winner->process();

        for (auto *node : *this->nodes) {
            if (node == current_winner) continue; // already processed the winner above

            if (node->choke_group == 1)  {
                node->cancel_event_value_1(); // tell all the choke group nodes that they are not allowed to go on, so they will not send note on messages
            }
            node->process();
        }

        // unsigned _i = 0;
        // for (auto* o : *this->nodes) {
        //     Debug_printf("\tnode %i\n", _i);
        //     o->process();
        //     Debug_println();
        //     ++_i;
        // }

        for (auto* node : *this->nodes) {
            node->reset();
        }

        Debug_println(".end.");
    }

    virtual void loop() {
        for (auto* o : *this->nodes) {
            Debug_printf("\tnode\n");
            if (o!=nullptr)
                o->loop();
            Debug_println();
        }
    }

    virtual void on_bar(uint32_t bar_number) {
        for (auto* o : *this->nodes) {
            if (o!=nullptr)
                o->on_bar(bar_number);
        }
    };

    virtual void on_phrase(uint32_t phrase_number) {
        for (auto* o : *this->nodes) {
            if (o!=nullptr)
                o->on_phrase(phrase_number);
        }
    };

    virtual void on_restart() {
        for (auto* o : *this->nodes) {
            if (o!=nullptr)
                o->on_restart();
        }
    }

    #ifdef ENABLE_SCALES
        virtual void notify_harmony_changed(const scale_identity_t&scale, const chord_identity_t& chord) {
            for (auto* o : *this->nodes) {
                if (o!=nullptr)
                    o->notify_harmony_changed(scale, chord);
            }
        }
    #endif

    // configure target sequencer to use the output nodes held by this OutputProcessor
    // requires patterns and nodes to have already been created?
    FLASHMEM
    virtual void configure_sequencer(BaseSequencer *sequencer) {
        #ifdef DEBUG_ENVELOPES
            sequencer->configure_pattern_output(11, this->nodes->get(11));
        #else
            unsigned _i = 0;
            for (auto* node : *this->nodes) {
                sequencer->configure_pattern_output(_i, node);
                ++_i;
            }
        #endif
    }

    #ifdef ENABLE_PARAMETERS
        FLASHMEM
        virtual void setup_parameters() {
            for (auto* node : *this->nodes) {
                //Serial.printf("MIDIOutputProcessor#setup_parameters processing item...\n");
                //Serial_flush();
                parameter_manager->addParameters(node->get_parameters());
            }
        }
    #endif

    #ifdef ENABLE_SCREEN
        //FLASHMEM
        virtual void create_menu_items(bool combine_pages = false);
    #endif

    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override {
            // inherit parent's settings
            BaseOutputProcessor::setup_saveable_settings();

            // register all of the output nodes
            for (auto* o : *this->nodes) {
                if (o==nullptr) continue;
                register_child(o);
            }
        }
    #endif
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
                        3,      // midi channel
                        0,      // start_count_at 
                        -1,     // finish_count_at 
                        1       // octave shift of 1
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
                this->addNode(
                    new MIDIChordGeneratorOutput(
                        "Chords",
                        output_target,
                        2
                    )
                );
            }
    };

    class BassAndChordsAndMelodyMIDIOutputProcessor : public MIDIOutputProcessor {
        public:
            BassAndChordsAndMelodyMIDIOutputProcessor(IMIDINoteAndCCTarget *output_target) : MIDIOutputProcessor(output_target) {
                this->addNode(
                    new MIDINoteTriggerCountOutput(
                        "Bass", 
                        output_target, 
                        this->nodes,
                        1
                    )
                );
                this->addNode(
                    new MIDINoteOutput(
                        "Melody",
                        output_target,
                        2
                    )
                );
                this->addNode(
                    new MIDIChordGeneratorOutput(
                        "Chords",
                        output_target,
                        3
                    )
                );
            }
    };

#endif


void setup_output(IMIDINoteAndCCTarget *output_target, MIDIOutputProcessor *processor = nullptr);

//extern MIDIOutputWrapper *output_wrapper;
extern MIDIOutputProcessor *output_processor;

