#include "sequencer/Sequencer.h"
#include "sequencer/Patterns.h"

#include "debug.h"

#include "menu_messages.h"

//#include "outputs/base_output.h"

void BaseSequencer::configure_pattern_output(int index, BaseOutput *output) {
    if (index >= this->number_patterns) {
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