#ifdef ENABLE_SCREEN
    #include "outputs/output.h"

    #include "submenuitem_bar.h"
    #include "menuitems_object.h"
    #include "menuitems_lambda.h"
    #include "menuitems.h"

    #include "mymenu/menuitems_scale.h"
    #include "mymenu/menuitems_harmony.h"

    #ifdef ENABLE_SCALES
    void MIDINoteOutput::add_scale_menu_items(Menu *menu) {
        LambdaScaleMenuItemBar *scale_menu = new LambdaScaleMenuItemBar(
            "Scale / Key",
            [=](scale_index_t scale) -> void { this->set_scale_number(scale); },
            [=]() -> scale_index_t { return this->get_scale_number(); },
            [=](int_fast8_t scale_root) -> void { this->set_scale_root(scale_root); },
            [=]() -> int_fast8_t { return this->get_scale_root(); },
            true,
            true,
            false
        );

        LambdaQuantiseModeControl *quant_mode = new LambdaQuantiseModeControl(
            "Quant",
            [=](int8_t v) -> void {
                this->set_quantise_mode((quantise_mode_t)constrain((int)v, (int)quantise_mode_t::QUANTISE_MODE_NONE, (int)quantise_mode_t::QUANTISE_MODE_CHORD));
            },
            [=]() -> int8_t {
                return (int8_t)this->get_quantise_mode();
            }
        );
        scale_menu->add(quant_mode);

        menu->add(scale_menu);
    }

    void MIDINoteOutput::add_status_menu_items(Menu *menu) {
        menu->add(new HarmonyDisplay(
            "Output",
            &this->scale_number,
            &this->scale_root,
            &this->last_note_number,
            &this->quantise_mode,
            &conductor->global_chord_identity,
            false
        ));
    }
    #endif

    void MIDINoteOutput::add_note_limit_menu_items(Menu *menu) {
        SubMenuItemBar *transposition_bar = new SubMenuItemBar("Transpose", true, false);
        LambdaScaleNoteMenuItem<int8_t> *lowest_note_control = new LambdaScaleNoteMenuItem<int8_t>(
            "Low",
            [=](int8_t v) -> void { this->setLowestNote(v); },
            [=]() -> int8_t { return this->getLowestNote(); },
            nullptr,
            (int8_t)MIDI_MIN_NOTE,
            (int8_t)MIDI_MAX_NOTE,
            true,
            true
        );
        transposition_bar->add(lowest_note_control);

        LambdaScaleNoteMenuItem<int8_t> *highest_note_control = new LambdaScaleNoteMenuItem<int8_t>(
            "High",
            [=](int8_t v) -> void { this->setHighestNote(v); },
            [=]() -> int8_t { return this->getHighestNote(); },
            nullptr,
            (int8_t)MIDI_MIN_NOTE,
            (int8_t)MIDI_MAX_NOTE,
            true,
            true
        );
        transposition_bar->add(highest_note_control);

        LambdaSelectorControl<NOTE_LIMIT_MODE> *lowest_note_mode_control = new LambdaSelectorControl<NOTE_LIMIT_MODE>(
            "Lo Mode",
            [=](NOTE_LIMIT_MODE v) -> void { this->setLowestNoteMode(v); },
            [=]() -> NOTE_LIMIT_MODE { return this->getLowestNoteMode(); },
            nullptr,
            true
        );
        lowest_note_mode_control->add_available_value(NOTE_LIMIT_MODE::IGNORE, "Drop");
        lowest_note_mode_control->add_available_value(NOTE_LIMIT_MODE::TRANSPOSE, "Wrap");
        transposition_bar->add(lowest_note_mode_control);

        LambdaSelectorControl<NOTE_LIMIT_MODE> *highest_note_mode_control = new LambdaSelectorControl<NOTE_LIMIT_MODE>(
            "Hi Mode",
            [=](NOTE_LIMIT_MODE v) -> void { this->setHighestNoteMode(v); },
            [=]() -> NOTE_LIMIT_MODE { return this->getHighestNoteMode(); },
            nullptr,
            true
        );
        highest_note_mode_control->set_available_values(lowest_note_mode_control->available_values);
        transposition_bar->add(highest_note_mode_control);

        menu->add(transposition_bar);
    }

    void MIDINoteOutput::add_midi_settings_menu_items(Menu *menu) {
        SubMenuItemBar *midi_settings_bar = new SubMenuItemBar("MIDI settings", true, false);
        midi_settings_bar->add(new DirectNumberControl<int_fast8_t>("MIDI Channel", &this->channel, this->channel, (int_fast8_t)1, (int_fast8_t)16));
        midi_settings_bar->add(new DirectNumberControl<int_fast8_t>("Octave", &this->octave, this->octave, (int_fast8_t)0, (int_fast8_t)10));
        menu->add(midi_settings_bar);
    }

    //FLASHMEM
    void MIDINoteOutput::make_menu_items(Menu *menu, int index) {
        //#ifdef ENABLE_ENVELOPE_MENUS
            char label[40];
            snprintf(label, 40, "%s %i: %s", this->get_menu_type_name(), index, this->label);
            menu->add_page(label, C_WHITE, this->is_menu_scrollable());

            // SubMenuItemColumns *sub_menu_item_columns = new SubMenuItemColumns("Options", 3);
            // // todo: convert all these ObjectNumberControls and ObjectToggleControls into LambdaNumberControls and LambdaToggleControls
            // // todo: probably turn this Quantise toggle into a feature of LambdaScaleMenuItemBar to avoid having to make a custom menu item for it
            // //sub_menu_item_columns->add(new ObjectToggleControl<MIDINoteOutput>("Quantise", this, &MIDINoteOutput::set_quantise, &MIDINoteOutput::is_quantise));
            // // todo: turn this into a lambda-based control, kill off any playing notes when the octave is changed to avoid stuck notes
            // sub_menu_item_columns->add(new DirectNumberControl<int_fast8_t>("Octave", &this->octave, this->octave, (int_fast8_t)0, (int_fast8_t)10));
            // // todo: turn this into a lambda-based control, kill off any playing notes when the channel is changed to avoid stuck notes
            // sub_menu_item_columns->add(new DirectNumberControl<int_fast8_t>("Channel", &this->channel, this->channel, (int_fast8_t)1, (int_fast8_t)16));

            // menu->add(sub_menu_item_columns);

            #ifdef ENABLE_SCALES
                this->add_scale_menu_items(menu);
            #endif
            this->add_note_limit_menu_items(menu);
            this->add_midi_settings_menu_items(menu);
            #ifdef ENABLE_SCALES
                this->add_status_menu_items(menu);
            #endif
        //#endif
    }

#endif