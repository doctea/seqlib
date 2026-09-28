#ifdef ENABLE_SHUFFLE

    #include <Arduino.h>

    #include "sequencer/shuffle.h"

    // callback wrapper that we can give to uClock and have it forward the call to our shuffle_pattern_wrapper instance
    void shuffled_callback(uint32_t step, uint8_t track_number) {
        shuffle_pattern_wrapper.shuffled_callback(step, track_number);
    }

    #ifdef ENABLE_SCREEN

        #include "mymenu.h"
        #include "submenuitem_bar.h"
        #include "menuitems_lambda.h"
        #include "mymenu/menuitems_shuffleeditor.h"

        #ifdef ENABLE_PARAMETERS
            #include "mymenu_items/ParameterMenuItems_lowmemory.h"
        #endif

        void setup_menu_shuffle() {
            menu->add_page("Shuffle patterns", C_WHITE, true, "Conductor");
            for (size_t i = 0 ; i < shuffle_pattern_wrapper.getCount() ; i++) {
                char label[MENU_C_MAX];
                snprintf(label, MENU_C_MAX, "Shuffle %i", i);
                SubMenuItemBar *submenu = new DualMenuItem(label, false, true, 48);
                submenu->add(new LambdaNumberControl<float>("Amount", [=](float v) -> void { shuffle_pattern_wrapper[i]->set_amount(v); shuffle_pattern_wrapper[i]->update_target(); }, [=]() -> float { return shuffle_pattern_wrapper[i]->get_amount(); }, nullptr, 0.0f, 1.0f, true, true));
                //submenu->add(new LambdaToggleControl("Active", [=](bool v) -> void { shuffle_pattern_wrapper[i]->set_active(v); shuffle_pattern_wrapper[i]->update_target(); }, [=]() -> bool { return shuffle_pattern_wrapper[i]->is_active(); }));
                submenu->add(new ShufflePatternEditorControl((const char*)label, shuffle_pattern_wrapper[i]));
                menu->add(submenu);
            }

            #ifdef ENABLE_PARAMETERS
                menu->add(new SeparatorMenuItem("Modulation"));
                ParameterList *parameters = shuffle_pattern_wrapper.getParameters();
                create_low_memory_parameter_controls("Shuffle patterns", parameters, C_WHITE);
            #endif

            menu->remember_opened_page();
        }
    #endif

    #ifdef ENABLE_PARAMETERS
        ParameterList* ShufflePatternWrapperManager::getParameters() {
            if(parameters!=nullptr)
                return parameters;
            
            parameters = new ParameterList();

            for (size_t i = 0 ; i < shuffle_pattern_wrapper.getCount() ; i++) {
                char label[32];
                snprintf(label, sizeof(label), "Shuffle amount %u", (unsigned)i);
                parameters->add(new LDataParameter<float>(
                    label,
                    [=] (float v) { 
                        //if (Serial) Serial.printf("Shuffle amount %i set to %f\n", i, v);
                        //ATOMIC() {
                            shuffle_pattern_wrapper[i]->set_amount(v); 
                            shuffle_pattern_wrapper[i]->update_target();
                        //}
                    },
                    [=] () -> float { return shuffle_pattern_wrapper[i]->get_amount(); },
                    -1.0f,
                    1.0f
                ));
            }

            parameter_manager->addParameters(parameters);

            return parameters;
        }
    #endif

#endif