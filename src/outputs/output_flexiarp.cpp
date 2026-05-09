#ifdef ENABLE_SCALES

#include "outputs/output_flexiarp.h"
#include "outputs/output_processor.h"

#ifdef ENABLE_SCREEN
    // need to add a menuitem for the degree setting
    void FlexiArpOutput::make_menu_items(Menu *menu, int index) {
        MIDINoteOutput::make_menu_items(menu, index);

        // cheat a little bit and get the last added menu item, which should be the 
        // midi settings bar?


        LinkedList<MenuItem*> *items = menu->get_page(menu->get_number_pages()-1)->items;
        SubMenuItemBar *midi_settings_bar = nullptr;

        for (auto* item : *items) {
            if (strcmp(item->label, "MIDI settings")==0) {
                midi_settings_bar = static_cast<SubMenuItemBar*>(item);
                break;
            }
        }

        if (midi_settings_bar == nullptr) {
            Serial.println("Error: could not find MIDI settings submenu to add degree setting to!");
            midi_settings_bar = new SubMenuItemBar("MIDI settings");
        }

        LambdaSelectorControl<int8_t> *degree_item = new LambdaSelectorControl<int8_t>(
            "Degree", 
            [=](int8_t value) -> void {
                this->degree = value;
            }, 
            [=](void) -> int8_t {
                return this->degree;
            },
            nullptr, true, true
        );
        degree_item->add_available_value(-1, "None");
        degree_item->add_available_value(0, "Root");
        degree_item->add_available_value(1, "Second");
        degree_item->add_available_value(2, "Third");
        degree_item->add_available_value(3, "Fourth");
        degree_item->add_available_value(4, "Fifth");
        degree_item->add_available_value(5, "Sixth");
        degree_item->add_available_value(6, "Seventh");

        midi_settings_bar->add(degree_item);
        midi_settings_bar->on_add();
    }
#endif

void setup_flexiarp_outputs(MIDIOutputProcessor *output_processor, IMIDINoteAndCCTarget *target_wrapper, int num_outputs, int midi_channel) {
    for (int i = 0 ; i < num_outputs ; i++) {
        char label[MENU_C_MAX];
        snprintf(label, MENU_C_MAX, "FlexiArp %i", i+1);
        FlexiArpOutput *output = new FlexiArpOutput(
            label,
            target_wrapper,
            midi_channel,
            i+1
        );
        output_processor->addNode(output);
    }
}

#endif