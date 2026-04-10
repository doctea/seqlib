#include "sequencer/TuringMachine/TuringMachinePattern.h"

#include "outputs/base_outputs.h"

void TuringMachinePattern::trigger_on_for_step(int step) {
    this->triggered_on_step = step;
    this->triggered_on_tick = ticks;
    this->current_duration = this->get_tick_duration();

    // if the note is outside our specified range, transpose it by octaves until it's within the range -- this is a bit of a hack, but it seems to produce more musical results than just silencing notes outside the range, and it's not too hard to implement
    int8_t note_to_play = this->events[step%get_effective_steps()].note;
    while (is_valid_note(note_to_play) && note_to_play < getLowestNote()) {
        //Serial.printf("TuringMachinePattern: note %i is below lowest note %i, transposing up an octave\n", note_to_play, getLowestNote());
        note_to_play += 12;
    }
    while (is_valid_note(note_to_play) && note_to_play > getHighestNote()) {
        //Serial.printf("TuringMachinePattern: note %i is above highest note %i, transposing down an octave\n", note_to_play, getHighestNote());
        note_to_play -= 12;
    }

    if (is_valid_note(note_to_play)) {
        this->current_note_number = note_to_play;

        if (this->output!=nullptr) {
            //Serial.printf("TuringMachinePattern: triggering on for step %i\twith note %i\t (%s)\n", step % get_effective_steps(), this->events[step%get_effective_steps()].note, get_note_name_c(this->events[step%get_effective_steps()].note));
            this->output->receive_event(1,0,note_to_play,this->get_velocity());
            this->output->process();
            note_held = true;

            //this->last_note_number = this->events[step%get_effective_steps()].note;
        }
    }
}
void TuringMachinePattern::trigger_off_for_step(int step) {
    this->triggered_on_step = -1;
    this->triggered_on_tick = -1;
    if (this->output!=nullptr) {
        //Serial.printf("TuringMachinePattern: triggering off for step %i\twith note %i\t (%s)\n", step % get_effective_steps(), this->current_note_number, get_note_name_c(this->current_note_number));
        this->output->receive_event(0,1,this->current_note_number,this->get_velocity());
        this->output->process();
        note_held = false;
    }
};

#if defined(ENABLE_SCREEN) 
    #include "menu.h"
    #include "mymenu_items/ParameterMenuItems_lowmemory.h"
    #include "mymenu/menuitems_sequencer.h"
    #include "mymenu/menuitems_outputselectorcontrol.h"
    // #include "mymenu/menuitems_pattern_euclidian.h"

    #ifdef ENABLE_SCALES
        #include "mymenu/menuitems_scale.h"
    #endif

    void TuringMachinePattern::create_menu_items(Menu *menu, int pattern_index, BaseSequencer *target_sequencer, int combine_setting) {
        char label[MENU_C_MAX];
        snprintf(label, MENU_C_MAX, "TM %i", pattern_index);
        menu->add_page(label, this->get_colour(), false);

        menu->add(new PatternDisplay(label, this, false, false));

        OutputSelectorControl<TuringMachinePattern> *selector = new OutputSelectorControl<TuringMachinePattern>(
            "Output",
            this,
            &TuringMachinePattern::set_output,
            &TuringMachinePattern::get_output,
            this->available_outputs,
            this->output
        );
        selector->go_back_on_select = true;
        SubMenuItemBar *output_bar = new SubMenuItemBar("Output", true, false);
        output_bar->add(selector);
        menu->add(output_bar);

        // add controls for the max/min note values for random generation
        // cribbed from Nexus6 -- we might want to make a more generic "range control" menu item at some point, since this kind of thing is likely to come up a lot in different contexts
        SubMenuItemBar *transposition_bar = new SubMenuItemBar("Transpose");
        LambdaScaleNoteMenuItem<int8_t> *lowest_note_control = new LambdaScaleNoteMenuItem<int8_t>(
            "Lowest",
            [=](int8_t v) -> void { this->setLowestNote(v); },
            [=]() -> int8_t { return this->getLowestNote(); },
            nullptr,
            (int8_t)MIDI_MIN_NOTE,
            (int8_t)MIDI_MAX_NOTE,
            true,
            true
        );
        transposition_bar->add(lowest_note_control);

        LambdaScaleNoteMenuItem<int8_t> *highest_note_control = new LambdaScaleNoteMenuItem<int8_t>(
            "Highest",
            [=](int8_t v) -> void { this->setHighestNote(v); },
            [=]() -> int8_t { return this->getHighestNote(); },
            nullptr,
            (int8_t)MIDI_MIN_NOTE,
            (int8_t)MIDI_MAX_NOTE,
            true,
            true
        );
        transposition_bar->add(highest_note_control);
        menu->add(transposition_bar);


        // add parameter modulation controls -- we can choose to combine these on the same page as the main pattern controls, or put them on a separate page with a link from the main one
        if (combine_setting & COMBINE_MODULATION_WITH_MAIN) {
            menu->add(new SeparatorMenuItem("Modulation"));
        } else {
            snprintf(label, MENU_C_MAX, "Pattern %i mod", pattern_index);
            menu->add_page(label, this->get_colour(), false);
        }
        LinkedList<FloatParameter*> *parameters = this->getParameters(pattern_index);
        create_low_memory_parameter_controls(label, parameters, this->get_colour());
    }

#endif

#if defined(ENABLE_PARAMETERS)

    #include "ParameterManager.h"
    #include "parameters/ProxyParameter.h"

    LinkedList<FloatParameter*> *TuringMachinePattern::getParameters(unsigned int i) {
        if (parameters!=nullptr)
            return parameters;

        SimplePattern::getParameters(i);

        char label[MAX_LABEL];
        snprintf(label, MAX_LABEL, "TM %i probability", i);
        parameters->add(
            new ProxyParameter<float>(
                label, 
                &this->probability,
                &this->effective_probability,
                0.0f, 
                1.0f
            ));

        snprintf(label, MAX_LABEL, "TM %i steps", i);
        parameters->add(
            new ProxyParameter<uint8_t>(
                label, 
                &this->steps,
                &this->effective_steps,
                1, 
                TIME_SIG_MAX_STEPS_PER_BAR*2
            ));

        snprintf(label, MAX_LABEL, "TM %i duration", i);
        parameters->add(
            new ProxyParameter<int>(
                label, 
                &this->default_duration,
                &this->effective_duration,
                1, 
                PPQN/4
            ));

        snprintf(label, MAX_LABEL, "TM %i lowest note", i);
        parameters->add(
            new ProxyParameter<int8_t>(
                label, 
                &this->lowest_note,
                &this->effective_lowest_note,
                MIDI_MIN_NOTE, 
                MIDI_MAX_NOTE
            ));

        snprintf(label, MAX_LABEL, "TM %i highest note", i);
        parameters->add(
            new ProxyParameter<int8_t>(
                label, 
                &this->highest_note,
                &this->effective_highest_note,
                MIDI_MIN_NOTE, 
                MIDI_MAX_NOTE
            ));

        snprintf(label, MAX_LABEL, "TM %i mutlock amt", i);
        parameters->add(
            new ProxyParameter<int>(
                label,
                &this->mutation_lock_count,
                &this->mutation_lock_count,
                1,
                8
            ));
        snprintf(label, MAX_LABEL, "TM %i mutlock on", i);
        parameters->add(
            new ProxyParameter<bool>(
                label,
                &this->mutation_lock_active,
                &this->mutation_lock_active,
                false,
                true
            ));

        parameter_manager->addParameters(parameters);

        return parameters;
    }
#endif