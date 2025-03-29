#include "sequencer/Sequencer.h"
#include "sequencer/Patterns.h"

#include "debug.h"

#include "menu_messages.h"

//#include "outputs/base_output.h"

ShufflePatternWrapper *shuffle_pattern_wrapper[NUMBER_SHUFFLE_PATTERNS] = {
    new ShufflePatternWrapper(0),
    new ShufflePatternWrapper(1),
    new ShufflePatternWrapper(2),
    new ShufflePatternWrapper(3),
    new ShufflePatternWrapper(4),
    new ShufflePatternWrapper(5),
    new ShufflePatternWrapper(6),
    new ShufflePatternWrapper(7),
    new ShufflePatternWrapper(8),
    new ShufflePatternWrapper(9),
    new ShufflePatternWrapper(10),
    new ShufflePatternWrapper(11),
    new ShufflePatternWrapper(12),
    new ShufflePatternWrapper(13),
    new ShufflePatternWrapper(14),
    new ShufflePatternWrapper(15)
};

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

#ifdef ENABLE_PARAMETERS
    LinkedList<FloatParameter*> *BaseSequencer::getParameters() {
        return nullptr;
    }
#endif

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