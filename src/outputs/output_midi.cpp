#ifdef ENABLE_SCREEN
    #include "outputs/output.h"

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
            menu->add_page(label, C_WHITE, false);

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

#endif