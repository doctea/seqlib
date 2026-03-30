#ifdef ENABLE_SCREEN
    #include "outputs/output_processor.h"

    #include "mymenu.h"
    #include "menuitems_object_multitoggle.h"

    #ifdef ENABLE_SCALES
        #include "scales.h"
        #include "mymenu/menuitems_scale.h"
        #include "mymenu/menuitems_harmony.h"
    #endif

    //FLASHMEM
    void MIDIOutputProcessor::create_menu_items(bool combine_pages) {
        for (unsigned int i = 0 ; i < this->nodes->size() ; i++) {
            BaseOutput *node = this->nodes->get(i);
            node->make_menu_items(menu, i);
            #ifdef ENABLE_PARAMETERS
                node->make_parameter_menu_items(menu, i, C_WHITE, combine_pages);
            #endif
        }

        menu->add_page("Enable outputs", C_WHITE);

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

        // todo: implement a CombinePageOptions-style mask, like we did with the sequencer menu items, 
        // to allow the programmer to choose whether to combine the scale/quantise stuff with the output toggles on the same page or not.
        
        #ifdef ENABLE_SCALES
            menu->add_page("Global key and scale");

            menu->add(new LambdaScaleMenuItemBar(
                "Scale / Key", 
                [=](scale_index_t scale) -> void { get_global_scale_identity()->scale_number = scale; }, 
                [=]() -> scale_index_t { return get_global_scale_identity()->scale_number; },
                [=](int_fast8_t scale_root) -> void { get_global_scale_identity()->root_note = scale_root; },
                [=]() -> int_fast8_t { return get_global_scale_identity()->root_note; },
                false,  // don't allow global, cos this IS the setting for global!
                true,
                false                
            ));

            menu->add(new HarmonyDisplay(
                "Output", 
                get_global_scale_identity(),
                nullptr, 
                nullptr
            ));
        #endif

    }
#endif