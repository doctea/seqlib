#include "Drums.h"
#include "outputs/output.h"
#include "outputs/output_processor.h"

#include "clock.h"

uint32_t external_cv_ticks_per_pulse_values[] = { 1, 2, 3, 4, 6, 8, 12, 16, 24 };
#define NUM_EXTERNAL_CV_TICKS_VALUES (sizeof(external_cv_ticks_per_pulse_values)/sizeof(uint32_t))
#ifdef ENABLE_CLOCK_INPUT_CV
    void set_external_cv_ticks_per_pulse_values(uint32_t new_value) {
        external_cv_ticks_per_pulse = new_value;
        //reset_clock();
        ticks = 0;
    }
    uint32_t get_external_cv_ticks_per_pulse_values() {
        return external_cv_ticks_per_pulse;
    }
#endif

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
        get_muso_note_for_drum(pitch),
        velocity,
        channel==(int8_t)GM_CHANNEL_DRUMS ? (int8_t)MUSO_TRIGGER_CHANNEL : channel
    };
}

//IMIDINoteAndCCTarget *output_wrapper = nullptr;
MIDIOutputProcessor *output_processor = nullptr;

void setup_output(IMIDINoteAndCCTarget *output_target) {
    Serial.println("setup_output.."); Serial_flush();
    output_processor = new MIDIOutputProcessor(output_target);     // todo: set this up dynamically, probably reading from a config file
    Serial.println("exiting setup_output"); Serial_flush();
}

void setup_output_parameters() {
    output_processor->setup_parameters();
}

#ifdef ENABLE_SCREEN
    #include "mymenu.h"
    #include "menuitems_object_multitoggle.h"

    void setup_output_menu() {
        //output_wrapper->create_menu_items();
        output_processor->create_menu_items();
    }

    void MIDIOutputProcessor::create_menu_items() {
        for (unsigned int i = 0 ; i < this->nodes->size() ; i++) {
            BaseOutput *node = this->nodes->get(i);
            node->make_menu_items(menu, i);
            node->make_parameter_menu_items(menu, i);
        }

        menu->add_page("Outputs");

        ObjectMultiToggleColumnControl *toggle = new ObjectMultiToggleColumnControl("Enable outputs", true);
        for (unsigned int i = 0 ; i < this->nodes->size() ; i++) {
            BaseOutput *output = this->nodes->get(i);

            MultiToggleItemClass<BaseOutput> *option = new MultiToggleItemClass<BaseOutput> (
                output->label,
                output,
                &BaseOutput::set_enabled,
                &BaseOutput::is_enabled
            );

            toggle->addItem(option);
        }
        menu->add(toggle);
    }

    #include "submenuitem_bar.h"
    #include "menuitems_object.h"
    #include "menuitems_lambda.h"
    #include "menuitems.h"

    #include "mymenu/menuitems_scale.h"
    #include "mymenu/menuitems_harmony.h"


    void MIDINoteTriggerCountOutput::make_menu_items(Menu *menu, int index) {
        //#ifdef ENABLE_ENVELOPE_MENUS
            char label[40];
            snprintf(label, 40, "MIDINoteOutput %i: %s", index, this->label);
            menu->add_page(label);

            SubMenuItemColumns *sub_menu_item_columns = new SubMenuItemColumns("Options", 2);
            sub_menu_item_columns->add(new ObjectToggleControl<MIDINoteTriggerCountOutput>("Quantise", this, &MIDINoteTriggerCountOutput::set_quantise, &MIDINoteTriggerCountOutput::is_quantise));
            sub_menu_item_columns->add(new DirectNumberControl<int_fast8_t>("Octave", &this->octave, this->octave, (int_fast8_t)0, (int_fast8_t)10));

            menu->add(sub_menu_item_columns);

            menu->add(new HarmonyDisplay("Output", &this->scale_number, &this->scale_root, &this->last_note_number, &this->quantise));

            menu->add(new LambdaScaleMenuItemBar(
                "Scale / Key", 
                [=](SCALE scale) -> void { this->set_scale_number(scale); }, 
                [=]() -> SCALE { return this->get_scale_number(); },
                [=](int_fast8_t scale_root) -> void { this->set_scale_root(scale_root); },
                [=]() -> int_fast8_t { return this->get_scale_root(); },
                true,
                true,
                false                
            ));

            menu->add(new HarmonyStatus("Output", &this->last_note_number, &this->note_number, false));
        //#endif
    }

    #include "mymenu_items/ParameterMenuItems_lowmemory.h"
    void BaseOutput::make_parameter_menu_items(Menu *menu, int index, uint16_t colour) {
        // don't make a menu page if no parameters to use
        LinkedList<FloatParameter*> *parameters = this->get_parameters();
        if (parameters==nullptr || parameters->size()==0)
            return;

        // create page
        char label[40];
        snprintf(label, 40, "Modulation %i: %s", index, this->label);
        menu->add_page(label, C_WHITE, false);

        // create lowmemory parameter controls
        create_low_memory_parameter_controls(label, parameters, colour);
    }

#endif