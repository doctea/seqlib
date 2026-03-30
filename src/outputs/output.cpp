#include "Drums.h"
#include "outputs/output.h"
#include "outputs/output_processor.h"

#include "clock.h"


OUTPUT_TYPE operator ++( OUTPUT_TYPE &id, int )
{
   OUTPUT_TYPE currentID = id;

   if ( MAX_OUTPUT_TYPE < id + 1 ) id = MAX_OUTPUT_TYPE;
   else id = static_cast<OUTPUT_TYPE>( id + 1 );

   return ( currentID );
}
OUTPUT_TYPE operator --( OUTPUT_TYPE &id, int )
{
   OUTPUT_TYPE currentID = id;

   if ( MIN_OUTPUT_TYPE < id - 1 ) id = MIN_OUTPUT_TYPE;
   else id = static_cast<OUTPUT_TYPE>( id - 1 );

   return ( currentID );
}

// todo: different modes to correlate with the midimuso mode + output availability..
int8_t get_muso_note_for_drum(int8_t drum_note) {
    int8_t retval = 60;
    switch (drum_note) {
        case GM_NOTE_ACOUSTIC_BASS_DRUM:
        case GM_NOTE_ELECTRIC_BASS_DRUM:
            retval += TRIGGER_SIDESTICK; break;
        case GM_NOTE_HAND_CLAP:
            retval += TRIGGER_CLAP; break;
        case GM_NOTE_ACOUSTIC_SNARE:
        case GM_NOTE_ELECTRIC_SNARE:
            retval += TRIGGER_SNARE; break;
        case GM_NOTE_CRASH_CYMBAL_1:
            retval += TRIGGER_CRASH_1; break;
        case GM_NOTE_TAMBOURINE:
            retval += TRIGGER_TAMB; break;
        case GM_NOTE_LOW_TOM:
            retval += TRIGGER_LOTOM; break;
        case GM_NOTE_HIGH_TOM:
            retval += TRIGGER_HITOM; break;
        case GM_NOTE_PEDAL_HI_HAT:
            retval += TRIGGER_PEDALHAT; break;
        case GM_NOTE_OPEN_HI_HAT:
            retval += TRIGGER_OPENHAT; break;
        case GM_NOTE_CLOSED_HI_HAT:
            retval += TRIGGER_CLOSEDHAT; break;
    }     
    return retval;
}

note_message_t convert_note_for_muso_drum(int8_t pitch, int8_t velocity, int8_t channel) {
    return {
        channel==GM_CHANNEL_DRUMS ? get_muso_note_for_drum(pitch) : pitch,
        velocity,
        channel==(int8_t)GM_CHANNEL_DRUMS ? (int8_t)MUSO_TRIGGER_CHANNEL : channel
    };
}

MIDIOutputProcessor *output_processor = nullptr;

//FLASHMEM
void setup_output(IMIDINoteAndCCTarget *output_target, MIDIOutputProcessor *processor) {
    if (Serial) { Serial.println("setup_output.."); Serial_flush(); }
    if (processor==nullptr) {
        processor = new FullDrumKitMIDIOutputProcessor(output_target);
    }
    output_processor = processor;     // todo: set this up dynamically, probably reading from a config file
    if (Serial) { Serial.println("exiting setup_output"); Serial_flush(); }
}

#ifdef ENABLE_PARAMETERS
    void setup_output_processor_parameters() {
        output_processor->setup_parameters();
    }
#endif

#ifdef ENABLE_SCREEN

    #include "submenuitem_bar.h"
    #include "menuitems_object.h"
    #include "menuitems_lambda.h"
    #include "menuitems.h"

    #include "mymenu/menuitems_scale.h"
    #include "mymenu/menuitems_harmony.h"

    //FLASHMEM
    void MIDINoteOutput::make_menu_items(Menu *menu, int index) {
        //#ifdef ENABLE_ENVELOPE_MENUS
            char label[40];
            snprintf(label, 40, "MIDINoteOutput %i: %s", index, this->label);
            menu->add_page(label);

            SubMenuItemColumns *sub_menu_item_columns = new SubMenuItemColumns("Options", 3);
            // todo: convert all these ObjectNumberControls and ObjectToggleControls into LambdaNumberControls and LambdaToggleControls
            // todo: probably turn this Quantise toggle into a feature of LambdaScaleMenuItemBar to avoid having to make a custom menu item for it
            sub_menu_item_columns->add(new ObjectToggleControl<MIDINoteOutput>("Quantise", this, &MIDINoteOutput::set_quantise, &MIDINoteOutput::is_quantise));
            // todo: turn this into a lambda-based control, kill off any playing notes when the octave is changed to avoid stuck notes
            sub_menu_item_columns->add(new DirectNumberControl<int_fast8_t>("Octave", &this->octave, this->octave, (int_fast8_t)0, (int_fast8_t)10));
            // todo: turn this into a lambda-based control, kill off any playing notes when the channel is changed to avoid stuck notes
            sub_menu_item_columns->add(new DirectNumberControl<int_fast8_t>("Channel", &this->channel, this->channel, (int_fast8_t)1, (int_fast8_t)16));

            menu->add(sub_menu_item_columns);

            #ifdef ENABLE_SCALES
                menu->add(new HarmonyDisplay("Output", &this->scale_number, &this->scale_root, &this->last_note_number, &this->quantise));

                menu->add(new LambdaScaleMenuItemBar(
                    "Scale / Key",
                    [=](scale_index_t scale) -> void { this->set_scale_number(scale); },
                    [=]() -> scale_index_t { return this->get_scale_number(); },
                    [=](int_fast8_t scale_root) -> void { this->set_scale_root(scale_root); },
                    [=]() -> int_fast8_t { return this->get_scale_root(); },
                    true,
                    true,
                    false
                ));
            #endif
            
        //#endif
    }

    #ifdef ENABLE_PARAMETERS
        #include "mymenu_items/ParameterMenuItems_lowmemory.h"
        //FLASHMEM
        void BaseOutput::make_parameter_menu_items(Menu *menu, int index, uint16_t colour, bool combine_pages) {
            // don't make a menu page if no parameters to use
            LinkedList<FloatParameter*> *parameters = this->get_parameters();
            if (parameters==nullptr || parameters->size()==0)
                return;

            if (combine_pages) {
                menu->add(new SeparatorMenuItem("Modulation", colour));
            } else {
                // create page
                char label[40];
                snprintf(label, 40, "Modulation %i: %s", index, this->label);
                menu->add_page(label, C_WHITE, false);
            }

            // create lowmemory parameter controls
            create_low_memory_parameter_controls(label, parameters, colour);
        }
    #endif

#endif