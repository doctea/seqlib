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

    for (size_t i = 0 ; i < this->get_number_patterns() ; i++) {
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
        for (size_t i = 0 ; i < this->get_number_patterns() ; i++) {
            char label[MENU_C_MAX];
            snprintf(label, MENU_C_MAX, "Pattern %i", i);
            menu->add(new PatternDisplay(label, this->get_pattern(i)));
            this->get_pattern(i)->set_colour(menu->get_next_colour());
        }

        for (size_t i = 0 ; i < this->get_number_patterns() ; i++) {
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
        for (auto* p : *params) {
            if (p!=nullptr && strcmp(p->label, name)==0)
                return p;
        }
        return nullptr;
    }
#endif

#ifdef ENABLE_STORAGE
    void BaseSequencer::setup_saveable_settings() {
        ISaveableSettingHost::setup_saveable_settings();

        if (Serial) Serial.printf("\n=== BaseSequencer::setup_saveable_settings() for sequencer %p aka %s with %i patterns, free ram is %u ...\n", this, this->get_path_segment(), this->get_number_patterns(), freeRam()); Serial.flush();
        const size_t pattern_count = this->get_number_patterns();
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
        
        if (Serial) 
            Serial.printf("... BaseSequencer::setup_saveable_settings() after doing settings, free ram is %u\n\n", freeRam()); Serial.flush();

        // register parameters for this output
        LinkedList<FloatParameter*> *parameters = this->getParameters();
        if (parameters!=nullptr) {
            for (auto* param : *parameters) {
                register_child(param);
            }
        }
        
        if (Serial) Serial.printf("=== BaseSequencer::setup_saveable_settings() done for sequencer  %p, free ram is %u\n\n", this, freeRam()); Serial.flush();
        if (Serial) Serial.flush();
    }
#endif

void SimpleSequencer::on_step(int step) {
    for (auto* p : *this->patterns) {
        #ifdef ENABLE_SHUFFLE
            if (!is_shuffle_enabled() || !p->is_shuffled()) {
                p->process_step(step);
            }
        #else
            if (p!=nullptr)
                p->process_step(step);
        #endif
    }
};

void SimpleSequencer::on_step_end(int step) {
    for (auto* p : *this->patterns) {
        #ifdef ENABLE_SHUFFLE
            if (!is_shuffle_enabled() || !p->is_shuffled()) {
                if (p!=nullptr)
                    p->process_step_end(step);
            }
        #else
            if (p!=nullptr)
                p->process_step_end(step);
        #endif
    }
}

#ifdef ENABLE_SHUFFLE
    void SimpleSequencer::on_step_shuffled(int8_t track, int step) {
        if (!is_shuffle_enabled()) return;

        for (auto* p : *this->patterns) {
            if (p!=nullptr && p->is_shuffled() && p->get_shuffle_track()==track) {
                //if (Serial) Serial.printf("at tick %i, received on_step_shuffled(%i, %i) callback for shuffled track %i\n", ticks, track, step, track);
                p->process_step(step);
            }
        }
    };

    void SimpleSequencer::on_step_end_shuffled(int8_t track, int step) {
        if (!is_shuffle_enabled()) return;

        for (auto* p : *this->patterns) {
            if (p!=nullptr && p->is_shuffled() && p->get_shuffle_track()==track) {
                p->process_step_end(step);
            }
        }
    }
#endif