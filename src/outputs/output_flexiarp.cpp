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
            { NEVER,        "Never"     },
            { BOUNCE_UP,    "Up-Down"   },
            { BOUNCE_DOWN,  "Down-Up"   },
            { UP,           "Up"        },
            { DOWN,         "Down"      },
            { RANDOM_WALK,  "Rand walk" },
            { RANDOM_ANY,   "Rand any"  }
        }
    };

    labelled_value_list_t<int8_t> reset_on_mode_value_labels = {
        (labelled_value_t<int8_t>[]) {
            { NONE,         "None"      },
            { BAR,          "Bar"       },
            { PHRASE,       "Phrase"    },
            { TWO_PHRASE,   "2xPhrase"  },
            { FOUR_PHRASE,  "4xPhrase"  }
        }
    };

    // need to add a menuitem for the degree setting
    void FlexiArpOutput::make_menu_items(Menu *menu, int index, const char *group_name = "FlexiArp") {
        MIDINoteOutput::make_menu_items(menu, index, group_name);

        // modify the existing submenus to add/change our settings by getting the last page and looping to 
        // find the controls we want to modify
        MenuItemList *items = menu->get_page(menu->get_number_pages()-1)->items;

        // TODO: make a bar dedicated to the flexiarp rules; but for now add flexi arp
        // settings to the existing midi settings bar, rather than creating a new submenu for
        SubMenuItemBar *midi_settings_bar = nullptr;

        for (auto* item : *items) {
            if (strcmp(item->label, "MIDI settings")==0) {
                midi_settings_bar = static_cast<SubMenuItemBar*>(item);
                break;
            } else if (strcmp(item->label, "Scale / Key")==0) {
                // adjust the quantiser mode controls; it makes no sense to have zero quantising for flexiarps,
                // so we should remove the 'Off' setting.  then we should prefer this setting when choosing whether to
                // quantise by scale or chord, falling back to scale if no chord exists.
                
                // item should now be the LambdaScaleMenuitemBar, so we need to find the quantise mode control within it
                LambdaScaleMenuItemBar *scale_menu = static_cast<LambdaScaleMenuItemBar*>(item);
                for (auto* sub_item : *scale_menu->items) {
                    if (strcmp(sub_item->label, "Quant")==0) {
                        LambdaQuantiseModeControl *quant_mode_item = static_cast<LambdaQuantiseModeControl*>(sub_item);
                        // replace quantise mode control with one that has no 'Off' option, so that flexiarps always quantise to scale or chord
                        quant_mode_item->set_available_values(&quantise_mode_list_no_none);
                        break;
                    }
                }
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
        midi_settings_bar->add(degree_item);

        LambdaNumberControl<int8_t> *mod_item = new LambdaNumberControl<int8_t>(
            "Change", 
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
        next_mode_item->set_available_values(&next_mode_value_labels);
        midi_settings_bar->add(next_mode_item);

        LambdaSelectorControl<int8_t> *reset_on_mode_item = new LambdaSelectorControl<int8_t>(
            "Reset", 
            [=](int8_t value) -> void {
                this->reset_mode = static_cast<reset_on_mode_t>(value);
            }, 
            [=](void) -> int8_t {
                return this->reset_mode;
            },
            nullptr, true, true
        );
        reset_on_mode_item->set_available_values(&reset_on_mode_value_labels);
        midi_settings_bar->add(reset_on_mode_item);

        CallbackMenuItem *mod_degree_item = new CallbackMenuItem(
            "Mod", 
            [=]() -> const char * {
                static char buf[9];
                snprintf(buf, sizeof(buf), "%s", degree_item->get_label_for_value(this->get_degree_mod_result()));
                return buf;
            }
        );
        #if MENU_PERF_PARTIAL_UPDATES
            mod_degree_item->add_redraw_custom_policy([=](bool selected, bool opened) -> bool {
                static int8_t last_degree_mod_result = -1;
                if (this->get_degree_mod_result() != last_degree_mod_result) {
                    last_degree_mod_result = this->get_degree_mod_result();
                    return true;
                }
                return false;
            });
        #endif
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