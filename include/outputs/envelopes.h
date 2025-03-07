#pragma once

//#include "Config.h"
#ifndef DEFAULT_ENVELOPE_CLASS
    #define DEFAULT_ENVELOPE_CLASS Weirdolope
    //#define DEFAULT_ENVELOPE_CLASS RegularEnvelope
#endif

#include "bpm.h"
#include "output.h"

#include "SinTables.h"

#ifdef ENABLE_SCREEN
    #include <LinkedList.h>
#endif

#include "envelopes/envelopes.h"
#include "envelopes/borolope.h"

#ifdef ENABLE_PARAMETERS
    #include "parameter_inputs/EnvelopeParameterInput.h"
#endif

class EnvelopeOutput : public MIDIDrumOutput {
    public:

    byte midi_cc = -1;

    EnvelopeBase *envelope;

    EnvelopeOutput(const char *label, byte note_number, byte cc_number, byte channel, IMIDINoteAndCCTarget *output_wrapper) : 
        MIDIDrumOutput(label, note_number, channel, output_wrapper)
        ,midi_cc(cc_number)
        {
            // todo: allow to switch to different types of envelope..?
            this->envelope = new DEFAULT_ENVELOPE_CLASS(
                label, 
                [=](float level) -> void { 
                    output_wrapper->sendControlChange(
                        this->midi_cc, 
                        (int8_t)((float)level * 127.0), 
                        this->channel
                    ); 
                } 
            );

            #if defined(ENABLE_PARAMETERS) && defined(ENABLE_ENVELOPES) && defined(ENABLE_ENVELOPES_AS_PARAMETER_INPUTS)
                parameter_manager->addInput(
                    new EnvelopeParameterInput((char*)label, "Env", this->envelope)
                );
            #endif
        }

    virtual void process() override {
        if (!is_enabled()) return;

        bool x_should_go_off = should_go_off();
        bool x_should_go_on  = should_go_on();

        if (x_should_go_off) {
            this->envelope->update_state(0, false, ticks);
        }
        if (x_should_go_on) {
            this->envelope->update_state(127, true, ticks);
        }
    }

    virtual void loop() override {
        if (!is_enabled()) return;
        this->envelope->process_envelope(ticks);
    }

    #ifdef ENABLE_SCREEN
        //FLASHMEM
        virtual void make_menu_items(Menu *menu, int index) override {
            this->envelope->make_menu_items(menu, index);
        };
    #endif

    #ifdef ENABLE_PARAMETERS
        virtual LinkedList<FloatParameter*> *get_parameters() override {
            return this->envelope->get_parameters();
        }
    #endif
};


