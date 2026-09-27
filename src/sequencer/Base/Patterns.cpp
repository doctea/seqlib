//#include "Config.h"

#include "sequencer/Base/Patterns.h"
#include "outputs/base_outputs.h"

void BasePattern::set_output_by_name(const char *output_name) {
    if (this->available_outputs!=nullptr) {
        for (auto* o : *this->available_outputs) {
            if (o->matches_label(output_name)) {
                this->set_output(o);
                return;
            }
        }
    }
}

const char *BasePattern::get_output_label() {        
    if (this->output!=nullptr)
        return this->output->label;
    return "None";
}

void SimplePattern::trigger_on_for_step(int step) {
    this->triggered_on_step = step;
    this->triggered_on_tick = ticks;
    if (this->debug)
        Serial.printf("SimplePattern::trigger_on_for_step: output=%s, step=%i, ticks=%6u, current_duration=(%i -> ", this->get_output_label(), step, ticks, this->current_duration);
    this->current_duration = this->get_tick_duration();
    #ifdef ENABLE_SHUFFLE
        if (this->is_shuffled() && this->query_note_on_for_step(step + 1)) {
            const int16_t output_ppqn = (int16_t)uClock.getOutputPPQN();
            const int16_t output_ticks_per_step = output_ppqn / STEPS_PER_BEAT;
            const int16_t next_onset_ticks = output_ticks_per_step + this->get_shuffle_length();
            const int16_t ticks_before_next_onset = (next_onset_ticks * PPQN) / output_ppqn;
            const int16_t maximum_duration = max((int16_t)1, ticks_before_next_onset - 1);
            this->current_duration = min(this->current_duration, maximum_duration);
        }
    #endif
    if (this->debug)
        Serial.printf("%i)\n", this->current_duration);

    if (this->output!=nullptr) {
        this->output->receive_event(1,0,-1,this->get_velocity());
        this->output->process();
        note_held = true;
    }
}
void SimplePattern::trigger_off_for_step(int step) {
    this->triggered_on_step = -1;
    this->triggered_on_tick = -1;
    if (this->output!=nullptr) {
        this->output->receive_event(0,1,-1,this->get_velocity());
        this->output->process();
        note_held = false;
    }
};

#ifdef ENABLE_PARAMETERS
    ParameterList *BasePattern::getParameters(unsigned int i) {
        if (this->parameters==nullptr)
            this->parameters = new ParameterList();
        return this->parameters;
    }
#endif

#ifdef ENABLE_SCREEN
    void BasePattern::create_menu_items(Menu *menu, int pattern_index, BaseSequencer *, int combine_patterns, const char *group_name) {
        // nothing to be done for base pattern case
        //pattern_index += 1;
    }
#endif

/*void SimplePattern::create_menu_items(Menu *menu, int pattern_index) {
    // nothing to be done for simple pattern case
}
*/
