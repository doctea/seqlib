#ifdef ENABLE_SCALES

#include "outputs/output_flexiarp.h"
#include "outputs/output_processor.h"

#ifdef ENABLE_SCREEN

    // override -1 to show "None" for the density group selector in the EuclidianPatternControl
    override_label_t<int8_t> flexiarp_change_every_override_label = {
        .label = "-",
        .value = 0
    };

    labelled_value_list_t<int8_t> next_mode_value_labels = {
        (labelled_value_t<int8_t>[]) {
            { NEVER, "Never" },
            { BOUNCE, "Bounce" },
            { UP, "Up" },
            { DOWN, "Down" },
            { RANDOM_WALK, "Random walk" },
            { RANDOM_ANY, "Random any" }
        },
        6
    };

    // need to add a menuitem for the degree setting
    void FlexiArpOutput::make_menu_items(Menu *menu, int index) {
        MIDINoteOutput::make_menu_items(menu, index);

        // modify the existing "MIDI settings" submenu to add the degree setting to it, 
        // rather than creating a new submenu for it

        MenuItemList *items = menu->get_page(menu->get_number_pages()-1)->items;
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
            "Base", 
            [=](int8_t value) -> void {
                this->degree_base.source = value;
            }, 
            [=](void) -> int8_t {
                return this->degree_base.source;
            },
            nullptr, true, true
        );
        degree_item->set_available_values(&degree_value_label_list);
        // degree_item->add_available_value(0, "None");
        // degree_item->add_available_value(1, "Root");
        // degree_item->add_available_value(2, "Second");
        // degree_item->add_available_value(3, "Third");
        // degree_item->add_available_value(4, "Fourth");
        // degree_item->add_available_value(5, "Fifth");
        // degree_item->add_available_value(6, "Sixth");
        // degree_item->add_available_value(7, "Seventh");
        midi_settings_bar->add(degree_item);

        LambdaNumberControl<int8_t> *mod_item = new LambdaNumberControl<int8_t>(
            "Ch.on", 
            [=](int8_t value) -> void {
                this->change_every.source = value;
            }, 
            [=](void) -> int8_t {
                return this->change_every.source;
            },
            nullptr, 
            0, 16,  // min, max
            true, true, // go_back_on_select, direct
            &flexiarp_change_every_override_label
        );
        midi_settings_bar->add(mod_item);

        LambdaSelectorControl<int8_t> *next_mode_item = new LambdaSelectorControl<int8_t>(
            "Dir'n", 
            [=](int8_t value) -> void {
                this->next_mode.source = static_cast<flexiarp_next_mode_t>(value);
            }, 
            [=](void) -> int8_t {
                return this->next_mode.source;
            },
            nullptr, true, true
        );
        next_mode_item->add_available_value(NEVER, "None");
        next_mode_item->add_available_value(BOUNCE, "Bounce");
        next_mode_item->add_available_value(UP, "Up");
        next_mode_item->add_available_value(DOWN, "Down");
        next_mode_item->add_available_value(RANDOM_WALK, "Walky");
        next_mode_item->add_available_value(RANDOM_ANY, "Random");
        midi_settings_bar->add(next_mode_item);

        CallbackMenuItem *mod_degree_item = new CallbackMenuItem(
            "Mod", 
            [=]() -> const char * {
                static char buf[9];
                snprintf(buf, sizeof(buf), "%s", degree_item->get_label_for_value(this->get_degree_mod_result()));
                return buf;
            }
        );
        mod_degree_item->add_redraw_custom_policy([=](bool selected, bool opened) -> bool {
            static int8_t last_degree_mod_result = -1;
            if (this->get_degree_mod_result() != last_degree_mod_result) {
                last_degree_mod_result = this->get_degree_mod_result();
                return true;
            }
            return false;
        });
        midi_settings_bar->add(mod_degree_item);

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