#include "sequencer/Base/Sequencer.h"
#include "sequencer/Base/Patterns.h"
#include "debug.h"
#include "menu_messages.h"

#ifdef ENABLE_SHUFFLE
    #include "sequencer/shuffle.h"
    ShufflePatternWrapperManager shuffle_pattern_wrapper(NUMBER_SHUFFLE_PATTERNS);
#endif

void SimpleSequencer::on_tick(int tick) {
    BaseSequencer::on_tick(tick);

    for (unsigned int i = 0 ; i < this->get_number_patterns() ; i++) {
        this->get_pattern(i)->process_tick(tick);
    }
};

void BaseSequencer::configure_pattern_output(int index, BaseOutput *output) {
    if (index >= (int)this->get_number_patterns()) {
        String message = String("Attempted to configure pattern with invalid index ") + String(index);
        #ifdef ENABLE_SCREEN
            messages_log_add(message);
        #endif
        if (Serial) {
            Serial.printf("BaseSequencer::configure_pattern_output(%i, %p) gives %s\n", index, output, message.c_str()); 
            Serial.flush();
        }
        return;
    }
    SimplePattern *p = this->get_pattern(index);
    if (p!=nullptr)
        p->set_output(output);
}

#ifdef ENABLE_SCREEN
    #include "mymenu/menuitems_sequencer.h"
    void BaseSequencer::make_menu_items(Menu *menu, int combine_pages) {
        menu->add_page("BaseSequencer");
        for (unsigned int i = 0 ; i < this->get_number_patterns() ; i++) {
            char label[MENU_C_MAX];
            snprintf(label, MENU_C_MAX, "Pattern %i", i);
            menu->add(new PatternDisplay(label, this->get_pattern(i)));
            this->get_pattern(i)->set_colour(menu->get_next_colour());
        }

        for (unsigned int i = 0 ; i < this->get_number_patterns() ; i++) {
            //Serial.printf("adding controls for pattern %i..\n", i);
            BasePattern *p = (BasePattern *)this->get_pattern(i);

            p->create_menu_items(menu, i, this, combine_pages);
        }
    }
#endif

#ifdef ENABLE_PARAMETERS
    LinkedList<FloatParameter*>* BaseSequencer::getParameters() {
        return nullptr;
    }
    // todo: is this necessary?
    FloatParameter* BaseSequencer::getParameterByName(const char *name) {
        LinkedList<FloatParameter*> *params = this->getParameters();
        if (params==nullptr)
            return nullptr;
        for (unsigned int i = 0 ; i < params->size() ; i++) {
            FloatParameter *p = params->get(i);
            if (p!=nullptr && strcmp(p->label, name)==0)
                return p;
        }
        return nullptr;
    }
#endif

void BaseSequencer::setup_saveable_settings() {
    ISaveableSettingHost::setup_saveable_settings();

    if (Serial) Serial.printf("\n=== BaseSequencer::setup_saveable_settings() for sequencer %p with %i patterns...\n", this, this->get_number_patterns()); Serial.flush();
    size_t pattern_count = this->get_number_patterns();
    for (uint_fast8_t i = 0 ; i < pattern_count ; i++) {
        //if (Serial) Serial.printf("BaseSequencer::setup_saveable_settings() for pattern [%i/%i]...\n", i+1, pattern_count); Serial.flush();
        BasePattern *p = this->get_pattern(i);
        if (p==nullptr) {
            if (Serial) Serial.printf("\tWARN: pattern %i is nullptr!\n", i); Serial.flush();
            continue;
        }
        register_child(p);
        p->add_saveable_settings(i);   
    }
    
    if (Serial) Serial.printf("... BaseSequencer::setup_saveable_settings() after doing settings, free ram is %u\n\n", rp2040.getFreeHeap()); Serial.flush();

    // register parameters for this output
    LinkedList<FloatParameter*> *parameters = this->getParameters();
    if (parameters!=nullptr) {
        for (int i = 0 ; i < parameters->size() ; i++) {
            FloatParameter *param = parameters->get(i);
            register_child(param);
        }
    }
    
    if (Serial) Serial.printf("=== BaseSequencer::setup_saveable_settings() done for sequencer  %p, free ram is %u\n\n", this, rp2040.getFreeHeap()); Serial.flush();
    if (Serial) Serial.flush();
}

void SimpleSequencer::on_step(int step) {
    //Serial.printf("EuclidianSequencer::on_step(%i), is_shuffle_enabled()=%i\n", step, is_shuffle_enabled());
    for (uint_fast8_t i = 0 ; i < get_number_patterns() ; i++) {
        #ifdef ENABLE_SHUFFLE
            if (!is_shuffle_enabled() || (is_shuffle_enabled() && !this->patterns->get(i)->is_shuffled())) {
                //if (Serial) Serial.printf("at tick %i, received on_step(%i, %i) callback for non-shuffled pattern\n", ticks, step, i);
                this->patterns->get(i)->process_step(step);
            }
        #else
            SimplePattern *p = this->get_pattern(i);
            if (p!=nullptr)
                p->process_step(step);
        #endif
    }
};

void SimpleSequencer::on_step_end(int step) {
    //Serial.printf("at tick %i, on_step_end(%i)\n", ticks, step);
    for (uint_fast8_t i = 0 ; i < get_number_patterns() ; i++) {
        #ifdef ENABLE_SHUFFLE
            if (!is_shuffle_enabled() || (is_shuffle_enabled() && !this->patterns->get(i)->is_shuffled())) {
                SimplePattern *p = this->get_pattern(i);
                if (p!=nullptr)
                    p->process_step_end(step);
            }
        #else
            SimplePattern *p = this->get_pattern(i);
            if (p!=nullptr)
                p->process_step_end(step);
        #endif
    }
}

#ifdef ENABLE_SHUFFLE
    void SimpleSequencer::on_step_shuffled(int8_t track, int step) {
        if (!is_shuffle_enabled()) return;

        for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
            if (this->get_pattern(i)->is_shuffled() && this->get_pattern(i)->get_shuffle_track()==track) {
                //if (Serial) Serial.printf("at tick %i, received on_step_shuffled(%i, %i) callback for shuffled track %i\n", ticks, track, step, track);
                this->get_pattern(i)->process_step(step);
            }
        }
    };

    void SimpleSequencer::on_step_end_shuffled(int8_t track, int step) {
        if (!is_shuffle_enabled()) return;

        for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
            if (this->get_pattern(i)->is_shuffled() && this->get_pattern(i)->get_shuffle_track()==track) {
                this->get_pattern(i)->process_step_end(step);
            }
        }
    }
#endif