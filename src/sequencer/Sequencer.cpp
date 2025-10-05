#include "sequencer/Sequencer.h"
#include "sequencer/Patterns.h"
#include "debug.h"
#include "menu_messages.h"

#ifdef ENABLE_SHUFFLE
    #include "sequencer/shuffle.h"
    ShufflePatternWrapperManager shuffle_pattern_wrapper(NUMBER_SHUFFLE_PATTERNS);
#endif

void BaseSequencer::configure_pattern_output(int index, BaseOutput *output) {
    if (index >= (int)this->number_patterns) {
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
    void BaseSequencer::make_menu_items(Menu *menu) {

    }
#endif

#ifdef ENABLE_PARAMETERS
    LinkedList<FloatParameter*>* BaseSequencer::getParameters() {
        return nullptr;
    }
    FloatParameter* BaseSequencer::getParameterByName(const char *name) {
        LinkedList<FloatParameter*> *params = this->getParameters();
        if (params==nullptr)
            return nullptr;
        for (int i = 0 ; i < params->size() ; i++) {
            FloatParameter *p = params->get(i);
            if (p!=nullptr && strcmp(p->label, name)==0)
                return p;
        }
        return nullptr;
    }
#endif

void BaseSequencer::setup_saveable_parameters() {
    if (this->saveable_parameters==nullptr) {
        ISaveableParameterHost::setup_saveable_parameters();

        // todo: setup saveable parameters for the shuffle patterns

        for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
            if (Serial) Serial.printf("BaseSequencer::setup_saveable_parameters() for pattern [%i/%i]...\n", i+1, number_patterns); Serial.flush();
            BasePattern *p = this->get_pattern(i);
            if (p==nullptr) {
                if (Serial) Serial.printf("\tWARN: pattern %i is nullptr!\n", i); Serial.flush();
                continue;
            }
            p->add_saveable_parameters(i, this->saveable_parameters);
        }
        if (Serial) Serial.printf("Finished BaseSequencer::setup_saveable_parameters.");
    }
}