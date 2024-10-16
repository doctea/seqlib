#include <Arduino.h>
//#include "Config.h"

#include <LinkedList.h>
#include "sequencer/Euclidian.h"

#include "outputs/base_outputs.h"

//#ifdef ENABLE_EUCLIDIAN
    arguments_t initial_arguments[] = {
        { LEN,    4, 1,   DEFAULT_DURATION }, //, TRIGGER_KICK },// get_trigger_for_pitch(GM_NOTE_ELECTRIC_BASS_DRUM) },    // kick
        { LEN,    5, 1,   DEFAULT_DURATION }, //, TRIGGER_SIDESTICK }, //get_trigger_for_pitch(GM_NOTE_SIDE_STICK) },    // stick
        { LEN,    2, 5,   DEFAULT_DURATION }, //, TRIGGER_CLAP }, //get_trigger_for_pitch(GM_NOTE_HAND_CLAP) },    // clap
        { LEN,    3, 5,   DEFAULT_DURATION }, //, TRIGGER_SNARE }, //get_trigger_for_pitch(GM_NOTE_ELECTRIC_SNARE) },   // snare
        { LEN,    3, 3,   DEFAULT_DURATION }, //, TRIGGER_CRASH_1 }, //get_trigger_for_pitch(GM_NOTE_CRASH_CYMBAL_1) },    // crash 1
        { LEN,    7, 1,   DEFAULT_DURATION }, //, TRIGGER_TAMB }, //get_trigger_for_pitch(GM_NOTE_TAMBOURINE) },    // tamb
        { LEN,    9, 1,   DEFAULT_DURATION }, //, TRIGGER_HITOM }, //get_trigger_for_pitch(GM_NOTE_HIGH_TOM) },    // hi tom!
        { LEN/4,  2, 3,   DEFAULT_DURATION }, //, TRIGGER_LOTOM }, //get_trigger_for_pitch(GM_NOTE_LOW_TOM) },    // low tom
        { LEN/2,  2, 3,   DEFAULT_DURATION }, //, TRIGGER_PEDALHAT }, //get_trigger_for_pitch(GM_NOTE_PEDAL_HI_HAT) },    // pedal hat
        { LEN,    4, 3,   DEFAULT_DURATION }, //, TRIGGER_OPENHAT }, //get_trigger_for_pitch(GM_NOTE_OPEN_HI_HAT) },    // open hat
        { LEN,    16, 0,  DEFAULT_DURATION }, //, TRIGGER_CLOSEDHAT }, //get_trigger_for_pitch(GM_NOTE_CLOSED_HI_HAT) }, //DEFAULT_DURATION },   // closed hat
        { LEN*2,  1, 1,   DEFAULT_DURATION_ENVELOPES }, //, TRIGGER_CRASH_2 }, //get_trigger_for_pitch(GM_NOTE_CRASH_CYMBAL_2) },   // crash 2
        { LEN*2,  1, 5,   DEFAULT_DURATION_ENVELOPES }, //, TRIGGER_SPLASH }, //get_trigger_for_pitch(GM_NOTE_SPLASH_CYMBAL) },   // splash
        { LEN*2,  1, 9,   DEFAULT_DURATION_ENVELOPES }, //, TRIGGER_VIBRA }, //get_trigger_for_pitch(GM_NOTE_VIBRA_SLAP) },    // vibra
        { LEN*2,  1, 13,  DEFAULT_DURATION_ENVELOPES }, //, TRIGGER_RIDE_BELL }, //get_trigger_for_pitch(GM_NOTE_RIDE_BELL) },   // bell
        { LEN*2,  5, 13,  DEFAULT_DURATION_ENVELOPES }, //, TRIGGER_RIDE_CYM }, //get_trigger_for_pitch(GM_NOTE_RIDE_CYMBAL_1) },   // cymbal
        { LEN,    4, 3,   2 }, //, PATTERN_BASS, 6 },  // bass (neutron) offbeat with 6ie of 6
        { LEN,    4, 3,   1 }, //, PATTERN_MELODY }, //NUM_TRIGGERS+NUM_ENVELOPES },  // melody as above
        { LEN,    4, 1,   4 }, //, PATTERN_PAD_ROOT }, // root pad
        { LEN,    4, 5,   4 } //,   PATTERN_PAD_PITCH); // root pad
    };
//#endif


//#if defined(ENABLE_CV_INPUT)
    #include "parameters/Parameter.h"
    #include "parameters/ProxyParameter.h"
    #include "ParameterManager.h"

    LinkedList<FloatParameter*> *EuclidianSequencer::getParameters() {
        static LinkedList<FloatParameter*> *parameters = nullptr;
        
        if (parameters!=nullptr)
            return parameters;
            
        parameters = new LinkedList<FloatParameter*>();

        // todo: store these in the object, create a page for the local-only ones

        parameters->add(new DataParameter<EuclidianSequencer,float> (
            "Density",
            this,
            &EuclidianSequencer::set_density,
            &EuclidianSequencer::get_density,
            MINIMUM_DENSITY,
            MAXIMUM_DENSITY
        ));

        parameters->add(new ProxyParameter<int_fast8_t>(
            "Mutation amount", 
            &this->mutation_count,
            &this->effective_mutation_count,
            1,
            8
        ));

        for (int i = 0 ; i < this->number_patterns ; i++) {
            EuclidianPattern *pattern = (EuclidianPattern *)this->get_pattern(i);
            //LinkedList<FloatParameter*> *pattern_parameters = 
            pattern->getParameters(i);
            //parameter_manager->addParameters(parameters);
            /*for (int i = 0 ; i < pattern_parameters->size() ; i++) {
                parameters->add(pattern_parameters->get(i));
            }*/
        }

        parameter_manager->addParameters(parameters);

        return parameters;
    }
//#endif

#if defined(ENABLE_SCREEN) //&& defined(ENABLE_CV_INPUT)
    #include "mymenu.h"
    //#include "mymenu/menuitems_pattern_euclidian.h"

    LinkedList<FloatParameter*> *EuclidianPattern::getParameters(int i) {
        if (parameters!=nullptr)
            return parameters;

        BasePattern::getParameters(i);

        char label[MENU_C_MAX];
        snprintf(label, MENU_C_MAX, "Pattern %i steps", i);
        parameters->add(
            new ProxyParameter<int>(
                label, 
                &this->arguments.steps,
                &this->used_arguments.steps,
                1, 
                this->maximum_steps
            ));

        snprintf(label, MENU_C_MAX, "Pattern %i pulses", i);
        parameters->add(
            new ProxyParameter<int>(
                label,
                &this->arguments.pulses,
                &this->used_arguments.pulses,
                0,
                this->maximum_steps
            )
        );

        snprintf(label, MENU_C_MAX, "Pattern %i rotation", i);
        parameters->add(
            new ProxyParameter<int>(
                label,
                &this->arguments.rotation,
                &this->used_arguments.rotation,
                1,
                this->maximum_steps
            )
        );

        snprintf(label, MENU_C_MAX, "Pattern %i duration", i);
        parameters->add(
            new ProxyParameter<int>(
                label,
                &this->arguments.duration,
                &this->used_arguments.duration,
                1,
                PPQN * 4
            )
        );

        parameter_manager->addParameters(parameters);

        return parameters;
    }

    #include "mymenu_items/ParameterMenuItems_lowmemory.h"
    #include "mymenu/menuitems_pattern_euclidian.h"

    void EuclidianPattern::create_menu_items(Menu *menu, int pattern_index) {
        char label[MENU_C_MAX];
        snprintf(label, MENU_C_MAX, "Pattern %i", pattern_index);
        menu->add_page(label, this->get_colour());

        EuclidianPatternControl *epc = new EuclidianPatternControl(label, this);
        menu->add(epc);

        snprintf(label, MENU_C_MAX, "Pattern %i mod", pattern_index);
        menu->add_page(label, this->get_colour(), false);

        //snprintf(label, MENU_C_MAX, "Pattern %i")
        LinkedList<FloatParameter*> *parameters = this->getParameters(pattern_index);
        
        /*for (int i = 0 ; i < parameters->size() ; i++) {
            menu->add(parameter_manager->makeMenuItemsForParameter(parameters->get(i)));
        }*/
        create_low_memory_parameter_controls(label, parameters, this->get_colour());

        #ifdef SIMPLE_SELECTOR
            OutputSelectorControl<EuclidianPattern> *selector = new OutputSelectorControl<EuclidianPattern>(
                "Output",
                this,
                &EuclidianPattern::set_output,
                &EuclidianPattern::get_output,
                this->available_outputs,
                this->output
            );
            selector->go_back_on_select = true;
            menu->add(selector);
        #endif
    }

    #include "mymenu.h"
    #include "submenuitem_bar.h"
    #include "mymenu/menuitems_sequencer.h"
    #include "mymenu/menuitems_sequencer_circle.h"
    #include "mymenu/menuitems_outputselectorcontrol.h"
    #include "menuitems_object_multitoggle.h"

    // todo: this should really be called create_menu_items, since it directly adds to menu
    // todo: do we really need to pass in menu here for some reason?
    void EuclidianSequencer::make_menu_items(Menu *menu, bool combine_pages) {
        // add a page for the 'boxed' sequence display of all tracks
        menu->add_page("Euclidian", TFT_CYAN);
        for (int i = 0 ; i < this->number_patterns ; i++) {
            char label[MENU_C_MAX];
            snprintf(label, MENU_C_MAX, "Pattern %i", i);
            menu->add(new PatternDisplay(label, this->get_pattern(i)));
            this->get_pattern(i)->colour = menu->get_next_colour();
        }

        // add a page for the circle display that shows all tracks simultaneously
        if (combine_pages) {
            menu->add_page("Circle & locks");
        } else {          
            menu->add_page("Circle");
        }
        menu->add(new CircleDisplay("Circle", this));

        // multitoggle to lock patterns
        if (!combine_pages)
            menu->add_page("Pattern locks", C_WHITE, false);
        ObjectMultiToggleColumnControl *toggle = new ObjectMultiToggleColumnControl("Allow changes", true);
        for (unsigned int i = 0 ; i < this->number_patterns ; i++) {
            BasePattern *p = (BasePattern *)this->get_pattern(i);

            PatternMultiToggleItem *option = new PatternMultiToggleItem(
                (new String(String("Pattern ") + String(i)))->c_str(),
                //p->get_output_label(),  // todo: make class auto-update 
                p,
                &BasePattern::set_locked,
                &BasePattern::is_locked,
                true
            );
            toggle->addItem(option);
        }
        menu->add(toggle);

        menu->add_page("Mutation");
        if (!combine_pages)
            create_menu_euclidian_mutation(2);
        else
            create_menu_euclidian_mutation(0);

        /*
        // create a dedicated page for the sequencer modulations
        menu->add_page("Sequencer mods");
        LinkedList<FloatParameter*> *parameters = getParameters();
        //parameter_manager->addParameters(parameters);
        for (int i = 0 ; i < parameters->size() ; i++) {
            menu->add(parameters->get(i)->makeControls());
        }*/

        //using option=ObjectSelectorControl<EuclidianPattern,BaseOutput*>::option;
        /*LinkedList<BaseOutput*> *nodes = new LinkedList<BaseOutput*>();
        for (int i = 0 ; i < output_processor.nodes.size() ; i++) {
            nodes->add(output_processor.nodes.get(i));
        }*/



        // ask each pattern to add their menu pages
        for (int i = 0 ; i < this->number_patterns ; i++) {
            //Serial.printf("adding controls for pattern %i..\n", i);
            BasePattern *p = (BasePattern *)this->get_pattern(i);

            p->create_menu_items(menu, i);
        }
    }


//#if defined(ENABLE_EUCLIDIAN) // /*defined(ENABLE_CV_INPUT) &&*/ 
    #include "LinkedList.h"
    #include "parameters/Parameter.h"

    #include "mymenu_items/ParameterMenuItems_lowmemory.h"

    void EuclidianSequencer::create_menu_euclidian_mutation(int number_pages_to_create) {
        if (number_pages_to_create>0) {
            menu->add_page("Mutation");
            number_pages_to_create--;
        }
        menu->add(new SeparatorMenuItem("Euclidian Mutations"));
        SubMenuItemColumns *submenu = new SubMenuItemColumns("Euclidian Mutations", 3, true, false);
        submenu->add(new ObjectNumberControl<EuclidianSequencer,float>("Density", this, &EuclidianSequencer::set_density,        &EuclidianSequencer::get_density, nullptr, MINIMUM_DENSITY, MAXIMUM_DENSITY));
        submenu->add(new ObjectToggleControl<EuclidianSequencer>("Mutate", this, &EuclidianSequencer::set_mutated_enabled,       &EuclidianSequencer::is_mutate_enabled));
        submenu->add(new ObjectToggleControl<EuclidianSequencer>("Reset", this,  &EuclidianSequencer::set_reset_before_mutate,   &EuclidianSequencer::should_reset_before_mutate));
        submenu->add(new ObjectToggleControl<EuclidianSequencer>("Add phrase", this, &EuclidianSequencer::set_add_phrase_enabled,&EuclidianSequencer::is_add_phrase_enabled));
        submenu->add(new ObjectToggleControl<EuclidianSequencer>("Fills", this,  &EuclidianSequencer::set_fills_enabled,         &EuclidianSequencer::is_fills_enabled));
        submenu->add(new ObjectNumberControl<EuclidianSequencer,int>("Seed", this, &EuclidianSequencer::set_euclidian_seed,      &EuclidianSequencer::get_euclidian_seed, nullptr, 0, 16384, true, false));
        submenu->add(new ObjectNumberControl<EuclidianSequencer,int_fast8_t>("Mut.Amt", this, &EuclidianSequencer::set_mutation_count,       &EuclidianSequencer::get_mutation_count, nullptr, 1, 8));
        menu->add(submenu);

        //#ifdef ENABLE_CV_INPUT
            if (number_pages_to_create>0) {
                menu->add_page("Mutation modulation", C_WHITE, false);
                number_pages_to_create--;
            } else {
                menu->add(new SeparatorMenuItem("Modulation"));
            }
            //menu->add(new SeparatorMenuItem("Mappable parameters"));
            // add the sequencer modulation controls to this page
            /*for (int i = 0 ; i < sequencer_parameters->size() ; i++) {
                menu->add(sequencer_parameters->get(i)->makeControls());
            }*/
            // TODO: this crashes us in the new refactor_seqlib branch?
            //   isn't RAM, must be that sequencer isn't initialised yet
            //   TODO: i think this whole block of code (from add_page("Mutation") onwards) can be moved elsewhere anyway and run at a more appropriate time?
            //Serial.printf("about to call sequencer->getParameters(), freeRam is %i\n", freeRam()); Serial_flush();
            LinkedList<FloatParameter*> *sequencer_parameters = this->getParameters();
            //Serial.printf("about to call create_low_memory_parameter_controls() for sequencer_parameters, freeRam is %i\n", freeRam()); Serial_flush();
            create_low_memory_parameter_controls("Mutation Parameters", sequencer_parameters);
        //#endif
    }
#endif

//#endif