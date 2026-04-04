#pragma once

#include "Euclidian.h"

class EuclidianPattern : public SimplePattern {
    public:

    bool locked = false;
    bool initialised = false;

    arguments_t arguments;
    arguments_t last_arguments;
    arguments_t default_arguments;
    arguments_t used_arguments;
    int maximum_steps = TIME_SIG_MAX_STEPS_PER_BAR;

    int8_t global_density_group = 0;

    EuclidianPattern(LinkedList<BaseOutput*> *available_outputs, int8_t global_density_group, int steps = MAX_STEPS, int pulses = 0, int rotation = -1, int duration = -1, int tie_on = -1) 
        : SimplePattern(available_outputs)
        {
            default_arguments.steps = steps;
            default_arguments.pulses = pulses;
            default_arguments.rotation = rotation;
            default_arguments.duration = duration;
            default_arguments.tie_on = tie_on;

            this->maximum_steps = steps > 0 ? steps : default_arguments.steps;

            this->set_global_density_group(global_density_group);
            set_arguments(&default_arguments);
            make_euclid();
        }
    virtual ~EuclidianPattern() {};

    virtual void restore_default_arguments() override {
        this->set_arguments(&this->default_arguments);
    }
    virtual void store_current_arguments_as_default() override {
        set_default_arguments(&this->arguments);
    }

    virtual void set_default_arguments(arguments_t *default_arguments_to_use) {
        memcpy(&this->default_arguments, default_arguments_to_use, sizeof(arguments_t));
        set_arguments(&this->default_arguments);
    }
    virtual void set_arguments(arguments_t *arguments_to_use) {
        memcpy(&this->arguments, arguments_to_use, sizeof(arguments_t));
        memcpy(&this->used_arguments, arguments_to_use, sizeof(arguments_t));
    }

    virtual const char *get_summary() override {
        static char summary[32];
        snprintf(summary, 32, 
            "%-2i %-2i %-2i", // [%c]",
            last_arguments.steps, last_arguments.pulses, last_arguments.rotation
            //this->query_note_on_for_step(BPM_CURRENT_STEP_OF_BAR) ? 'X' : ' '
        );
        return summary;
    }

    virtual float get_global_density() {
        return all_global_density[this->global_density_group];
    }
    virtual int8_t get_global_density_group() {
        return this->global_density_group;
    }
    virtual void set_global_density_group(int8_t channel) {
        this->global_density_group = channel % NUM_GLOBAL_DENSITY_GROUPS;
    }

    FLASHMEM
    virtual void make_euclid() {
        //if (Serial) Serial.println("make_euclid.."); Serial.flush();
        //Serial.printf("used_arguments is %p, global_density points to %p\n", &used_arguments, global_density);
        this->used_arguments.effective_euclidian_density = this->get_global_density();

        if (initialised && 0==memcmp(&this->used_arguments, &this->last_arguments, sizeof(arguments_t))) {
            if (Serial) { Serial.println("nothing changed, don't do anything"); Serial.flush(); }
            // nothing changed, dont do anything
            return;
        }

        int_fast8_t original_pulses = this->used_arguments.pulses;
        float multiplier = 1.5f*(MINIMUM_DENSITY + this->used_arguments.effective_euclidian_density);
        int_fast8_t temp_pulses = 0.5f + (((float)original_pulses) * multiplier);
        //int_fast8_t temp_pulses = original_pulses;    // for disabling density for testing purposes

        if (used_arguments.steps > maximum_steps) {
            //messages_log_add(String("arguments.steps (") + String(arguments.steps) + String(") is more than maximum steps (") + String(maximum_steps) + String(")"));
            maximum_steps = used_arguments.steps;
        }

        int bucket = 0;
        for (int_fast8_t i = 0 ; i < this->used_arguments.steps ; i++) {
            int_fast8_t rotation = this->used_arguments.rotation;
            //int new_i = ((used_arguments.steps - rotation) + i) % used_arguments.steps;
            int_fast8_t new_i = (rotation + i) % used_arguments.steps;
            bucket += temp_pulses;
            if (bucket >= this->used_arguments.steps) {
                bucket -= this->used_arguments.steps;
                this->set_event_for_tick(new_i * TICKS_PER_STEP);
            } else {
                this->unset_event_for_tick(new_i * TICKS_PER_STEP);
            }
        }

        memcpy(&this->last_arguments, &this->used_arguments, sizeof(arguments_t));
    }

    // rotate the pattern around specifed number of steps -- 
    // this is done during make_euclid now, but perhaps this function will come in useful elsewhere
    /*void rotate_pattern(int rotate) {
        //unsigned long rotate_time = millis();
        bool temp_pattern[steps];
        int offset = steps - rotate;
        for (int i = 0 ; i < steps ; i++) {
            //stored[i] = p->stored[abs( (i + offset) % p->steps )];
            temp_pattern[i] = this->query_note_on_for_tick(
                (abs( (i + offset) % steps) * ticks_per_step)
            );
        }
        for (int i = 0 ; i < steps ; i++) {
            if (temp_pattern[i])
                this->set_event_for_tick(i * ticks_per_step);
            else 
                this->unset_event_for_tick(i * ticks_per_step);
        }
    }*/

    void mutate() {
        //if (Serial) Serial.println("mutate the pattern!");
        int r = random(0, 100);
        if (r > 50) {
            if (r > 75) {
                this->set_pulses(this->get_pulses() + 1);
            } else {
                this->set_pulses(this->get_pulses() - 1);
            }
        } else if (r < 25) {
            this->set_rotation(this->get_rotation() + 1);
        } else if (r > 25) {
            this->set_pulses(this->get_pulses() * 2);
        } else {
            this->set_pulses(this->get_pulses() / 2);
        }
        if (this->get_pulses() >= this->get_steps() || this->get_pulses() <= 0) {
            this->set_pulses(1);
        }
        //r = random(this->steps/2, this->maximum_steps);
        //this->arguments.steps = r;
        this->make_euclid();
    }

    virtual int8_t get_effective_steps() override {
        return this->used_arguments.steps;
    }
    virtual int8_t get_steps() override {
        return this->arguments.steps;
    }
    virtual void set_steps(int8_t steps) override {
        this->arguments.steps = steps;
        this->used_arguments.steps = steps;
    }
    virtual int8_t get_pulses() {
        return this->arguments.pulses;
    }
    virtual void set_pulses(int8_t pulses) {
        this->arguments.pulses = pulses;
        this->used_arguments.pulses = pulses;
    }
    virtual int8_t get_rotation() {
        return this->arguments.rotation;
    }
    virtual void set_rotation(int8_t rotation) {
        this->arguments.rotation = rotation;
        this->used_arguments.rotation = rotation;
    }
    virtual int8_t get_duration() {
        return this->arguments.duration;
    }
    virtual void set_duration(int8_t duration) {
        this->arguments.duration = duration;
        used_arguments.duration = duration;
    }

    virtual int8_t get_tick_duration() override {
        return this->used_arguments.duration 
        #ifdef ENABLE_SHUFFLE 
            + this->get_shuffle_length()
        #endif
        ;
    }

    #ifdef ENABLE_SCREEN
        //FLASHMEM
        virtual void create_menu_items(Menu *menu, int index, BaseSequencer *target_sequencer, int combine_settings = (Euclidian::CombinePageOption)Euclidian::COMBINE_NONE) override;
    #endif
    
    #if defined(ENABLE_PARAMETERS)
        virtual LinkedList<FloatParameter*> *getParameters(unsigned int i) override;
    #endif

    virtual void add_saveable_settings(int pattern_index) override {
        SimplePattern::add_saveable_settings(pattern_index);

        register_setting(new LSaveableSetting<int_fast8_t>("steps", "EuclidianPattern", &this->arguments.steps), true);
        register_setting(new LSaveableSetting<int8_t>("global_density_group", "EuclidianPattern", &this->global_density_group));
        register_setting(new LSaveableSetting<int_fast8_t>("pulses", "EuclidianPattern", &this->arguments.pulses));
        register_setting(new LSaveableSetting<int_fast8_t>("rotation", "EuclidianPattern", &this->arguments.rotation));
        register_setting(new LSaveableSetting<int_fast8_t>("duration", "EuclidianPattern", &this->arguments.duration));
        //register_setting(new LSaveableSetting<float>(String(prefix) + String("effective_euclidian_density"), "EuclidianPattern", &this->arguments.effective_euclidian_density));
        register_setting(new LSaveableSetting<int_fast8_t>("tie_on", "EuclidianPattern", &this->arguments.tie_on));
    }

};
