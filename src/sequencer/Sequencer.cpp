#include "sequencer/Sequencer.h"
#include "sequencer/Patterns.h"

#include "debug.h"

#include "menu_messages.h"

//#include "outputs/base_output.h"

void BaseSequencer::configure_pattern_output(int index, BaseOutput *output) {
    if (index >= (int)this->number_patterns) {
        String message = String("Attempted to configure pattern with invalid index ") + String(index);
        messages_log_add(message);
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

//#ifdef ENABLE_CV_INPUT
    LinkedList<FloatParameter*> *BaseSequencer::getParameters() {
        return nullptr;
    }
//#endif

void BaseSequencer::setup_saveable_parameters() {
    if (this->saveable_parameters==nullptr) {
        ISaveableParameterHost::setup_saveable_parameters();

        // todo: setup the saveable parameters for all the Patterns owned by this *Sequencer...
        for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
            Serial.printf("BaseSequencer::setup_saveable_parameters() for pattern [%i/%i]...\n", i+1, number_patterns); Serial.flush();
            BasePattern *p = this->get_pattern(i);
            if (p==nullptr) {
                Serial.printf("\tWARN: pattern %i is nullptr!\n", i); Serial.flush();
                continue;
            }
            p->add_saveable_parameters(i, this->saveable_parameters);
        }
        Serial.printf("Finished BaseSequencer::setup_saveable_parameters.");
    }
}