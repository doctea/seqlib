//#include "Config.h"

#include "sequencer/Patterns.h"
#include "outputs/base_outputs.h"

const char *BasePattern::get_output_label() {        
    if (this->output!=nullptr)
        return this->output->label;
    return "None";
}

void SimplePattern::trigger_on_for_step(int step) {
    this->triggered_on_step = step; // % (ticks_per_step*steps);
    this->triggered_on_tick = ticks;
    this->current_duration = this->get_tick_duration();

    if (this->output!=nullptr) {
        this->output->receive_event(1,0,0);
        this->output->process();
        note_held = true;
    }
}
void SimplePattern::trigger_off_for_step(int step) {
    this->triggered_on_step = -1;
    this->triggered_on_tick = -1;
    if (this->output!=nullptr) {
        this->output->receive_event(0,1,0);
        this->output->process();
        note_held = false;
    }
};

#ifdef ENABLE_PARAMETERS
    LinkedList<FloatParameter*> *BasePattern::getParameters(unsigned int i) {
        if (this->parameters==nullptr)
            this->parameters = new LinkedList<FloatParameter*>();
        return this->parameters;
    }
#endif

#ifdef ENABLE_SCREEN
    void BasePattern::create_menu_items(Menu *menu, int pattern_index, BaseSequencer *, bool combine_patterns) {
        // nothing to be done for base pattern case
        //pattern_index += 1;
    }
#endif

/*void SimplePattern::create_menu_items(Menu *menu, int pattern_index) {
    // nothing to be done for simple pattern case
}
*/