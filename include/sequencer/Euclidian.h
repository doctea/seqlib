#pragma once

#ifndef FLASHMEM
    #define FLASHMEM
#endif

//#include "Config.h"

// todo: move this to build flags or something...
#define SEQLIB_MUTATE_EVERY_TICK

#include "debug.h"
#include <LinkedList.h>
#include "Patterns.h"
#include "Sequencer.h"
#include <bpm.h>

extern const int num_initial_arguments;

class FloatParameter;
class Menu;

#define MINIMUM_DENSITY 0.0f  // 0.10f
#define MAXIMUM_DENSITY 1.5f
#define DEFAULT_DENSITY 0.6666f
#ifndef NUM_GLOBAL_DENSITY_CHANNELS
    #define NUM_GLOBAL_DENSITY_CHANNELS 4
#endif

#define MINIMUM_MUTATION_COUNT 0
#define MAXIMUM_MUTATION_COUNT 8
#define DEFAULT_MUTATION_COUNT 3

#define DEFAULT_DURATION 2      // minimum duration needs to be >=2 ticks -- if any lower then can end up with on+off happening within the same tick and usb_teensy_clocker gets notes stuck!
#define MINIMUM_DURATION 2
#define DEFAULT_DURATION_ENVELOPES 8

#define BARS_PER_PHRASE 4

//float effective_euclidian_density = 0.75f;

#define SEQUENCE_LENGTH_STEPS 16
const int LEN = SEQUENCE_LENGTH_STEPS;
    
struct arguments_t {
    const char *associated_label = nullptr;
    int_fast8_t steps = SEQUENCE_LENGTH_STEPS;
    int_fast8_t pulses = steps/2;
    int_fast8_t rotation = 1;
    int_fast8_t duration = 1;
    float effective_euclidian_density = 0.6666;
    int_fast8_t tie_on = -1;
};

extern arguments_t initial_arguments[];
extern float all_global_density[];

class EuclidianPattern : public SimplePattern {
    public:

    bool locked = false;
    bool initialised = false;

    //bool active_status = true;
    //int pulses, rotation, duration;
    arguments_t arguments;
    arguments_t last_arguments;
    arguments_t default_arguments;
    arguments_t used_arguments;
    int maximum_steps = steps;
    //int tie_on = 0;

    //float *global_density = nullptr;
    int8_t global_density_channel = 0;

    //EuclidianPattern() : SimplePattern() {}

    EuclidianPattern(LinkedList<BaseOutput*> *available_outputs, int8_t global_density_channel, int steps = MAX_STEPS, int pulses = 0, int rotation = -1, int duration = -1, int tie_on = -1) 
        //: arguments.pulses(pulses), arguments.rotation(arguments.rotation), arguments.duration(arguments.duration), tie_on(tie_on)
        : SimplePattern(available_outputs)
          //default_arguments { .steps = steps, .pulses = pulses, .rotation = rotation, .duration = duration, .tie_on = tie_on }
        {
            default_arguments.steps = steps;
            default_arguments.pulses = pulses;
            default_arguments.rotation = rotation;
            default_arguments.duration = duration;
            default_arguments.tie_on = tie_on;

            this->maximum_steps = steps > 0 ? steps : default_arguments.steps;
            //if (steps>0)
                //make_euclid(default_arguments.steps, default_arguments.pulses, default_arguments.rotation, default_arguments.duration, tie_on);
            //make_euclid();
            // todo: this cause usb_teensy_clocker to crash on initialisation?  because of uninitialised global_density!!

            this->set_global_density_channel(global_density_channel);
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

    /*void make_euclid(arguments_t arguments) {
        this->make_euclid(
            arguments.steps,
            arguments.pulses,
            arguments.rotation,
            arguments.duration,
            arguments.tie_on
        );
    }*/

    virtual float get_global_density() {
        return all_global_density[this->global_density_channel];
    }
    virtual int8_t get_global_density_channel() {
        return this->global_density_channel;
    }
    virtual void set_global_density_channel(int8_t channel) {
        this->global_density_channel = channel % NUM_GLOBAL_DENSITY_CHANNELS;
    }

    //void make_euclid(int steps = 0, int pulses = 0, int rotation = -1, int duration = -1, /*, int trigger = -1,*/ int tie_on = -1) { //}, float effective_euclidian_density = 0.75f) {
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

        /*if (!initialised) {
            this->set_default_arguments(&this->used_arguments);
            initialised = true;
        */

        int_fast8_t original_pulses = this->used_arguments.pulses;
        float multiplier = 1.5f*(MINIMUM_DENSITY + this->used_arguments.effective_euclidian_density);
        int_fast8_t temp_pulses = 0.5f + (((float)original_pulses) * multiplier);
        //int_fast8_t temp_pulses = original_pulses;    // for disabling density for testing purposes

        if (used_arguments.steps > maximum_steps) {
            //messages_log_add(String("arguments.steps (") + String(arguments.steps) + String(") is more than maximum steps (") + String(maximum_steps) + String(")"));
            maximum_steps = used_arguments.steps;
        }

        int bucket = 0;
        //if (this->used_arguments.rotation!=0)
        for (int_fast8_t i = 0 ; i < this->used_arguments.steps ; i++) {
            int_fast8_t rotation = this->used_arguments.rotation;
            //int new_i = ((used_arguments.steps - rotation) + i) % used_arguments.steps;
            int_fast8_t new_i = (rotation + i) % used_arguments.steps;
            bucket += temp_pulses;
            if (bucket >= this->used_arguments.steps) {
                bucket -= this->used_arguments.steps;
                this->set_event_for_tick(new_i * ticks_per_step);
            } else {
                this->unset_event_for_tick(new_i * ticks_per_step);
            }
        }
        //this->maximum_steps = this->arguments.steps;
        
        /*if (this->used_arguments.rotation > 0) {
            this->rotate_pattern(this->used_arguments.rotation);
        }*/

        memcpy(&this->last_arguments, &this->used_arguments, sizeof(arguments_t));
    }

    // rotate the pattern around specifed number of steps -- 
    // TODO: could actually not change the pattern and just use the rotation in addition to offset in the query_patterns
    void rotate_pattern(int rotate) {
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
    }  

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

    /*virtual bool is_locked() {
        return this->locked;
    }
    virtual void set_locked(bool state) {
        this->locked = state;
    }*/

    /*void trigger_on_for_step(int step) override {
        Serial.printf("trigger_on_for_step(%i)\n", step);
    }

    void trigger_off_for_step(int step) override {
        Serial.printf("trigger_off_for_step(%i)\n", step);
    }*/

    #ifdef ENABLE_SCREEN
        //FLASHMEM
        virtual void create_menu_items(Menu *menu, int index, BaseSequencer *target_sequencer, bool combine_pages = false) override;
    #endif
    
    #if defined(ENABLE_PARAMETERS)
        virtual LinkedList<FloatParameter*> *getParameters(unsigned int i) override;
    #endif

    virtual void add_saveable_parameters(int pattern_index, LinkedList<SaveableParameterBase*> *target) override {
        SimplePattern::add_saveable_parameters(pattern_index, target);
        char prefix[40];
        snprintf(prefix, 40, "track_%i_", pattern_index);

        // need to remove the parent's 'steps' parameter and replace it with the one for this pattern 
        // which uses arguments.steps instead of BasePattern::steps
        for (unsigned int i = 0 ; i < target->size() ; i++) {
            if (strcmp(target->get(i)->label, "steps")==0) {
                delete target->get(i);
                target->remove(i);
                break;
            }
        }

        target->add(new LSaveableParameter<int8_t>((String(prefix) + String("global_density_channel")).c_str(), "EuclidianPattern", &this->global_density_channel));
        target->add(new LSaveableParameter<int_fast8_t>((String(prefix) + String("steps")).c_str(), "EuclidianPattern", &this->arguments.steps));
        target->add(new LSaveableParameter<int_fast8_t>((String(prefix) + String("pulses")).c_str(), "EuclidianPattern", &this->arguments.pulses));
        target->add(new LSaveableParameter<int_fast8_t>((String(prefix) + String("rotation")).c_str(), "EuclidianPattern", &this->arguments.rotation));
        target->add(new LSaveableParameter<int_fast8_t>((String(prefix) + String("duration")).c_str(), "EuclidianPattern", &this->arguments.duration));
        //target->add(new LSaveableParameter<float>(String(prefix) + String("effective_euclidian_density"), "EuclidianPattern", &this->arguments.effective_euclidian_density));
        target->add(new LSaveableParameter<int_fast8_t>((String(prefix) + String("tie_on")).c_str(), "EuclidianPattern", &this->arguments.tie_on));
    }

};



class EuclidianSequencer : public BaseSequencer {
    // todo: list of EuclidianPatterns...
    EuclidianPattern **patterns = nullptr;

    int seed = 0;
    uint_fast8_t mutate_minimum_pattern = 0, mutate_maximum_pattern = number_patterns;
    uint_fast8_t mutation_count = DEFAULT_MUTATION_COUNT, effective_mutation_count = DEFAULT_MUTATION_COUNT;
    bool    reset_before_mutate = true, 
            mutate_enabled = true, 
            fills_enabled = true, 
            add_phrase_to_seed = true;
    //float global_density = 0.6666f;

    bool seed_locked = false;
    uint32_t last_locked_seed = 0;

    public:
    
    EuclidianSequencer(LinkedList<BaseOutput*> *available_outputs, int8_t number_patterns = -1) : BaseSequencer() {
        EuclidianPattern *p = nullptr;
        if (number_patterns > 0) {
            this->number_patterns = number_patterns;
        } else if (number_patterns == -1) {
            number_patterns = this->number_patterns;
        } 
        this->patterns = (EuclidianPattern**) CALLOC_FUNC(number_patterns, sizeof(p));
        for (int_fast8_t i = 0 ; i < number_patterns ; i++) {
            if (this->debug && Serial) {
                Serial.printf("EuclidianSequencer constructor creating EuclidianPattern %i; available_outputs is @%p (size %i)\n", i, available_outputs, available_outputs->size()); 
                Serial.flush();
            }
            this->patterns[i] = new EuclidianPattern(available_outputs, i / (number_patterns / NUM_GLOBAL_DENSITY_CHANNELS));

            #ifdef ENABLE_SHUFFLE
                // for testing shuffled, make every other pattern shuffled
                // todo: remove this, make configurable
                //if (i%2==0) {
                //    this->patterns[i]->set_shuffled(true);
                //}
            #endif

            //this->patterns[i]->global_density = &this->global_density;
            if (this->debug && Serial) {
                Serial.flush();
            }
        }
        if (this->debug && Serial) Serial.println("Exiting EuclidianSequencer constructor.");
    }
    virtual ~EuclidianSequencer() {};

    float get_density(int8_t channel) {
        //return this->global_density;
        return all_global_density[channel];
    }
    void set_density(int8_t channel, float v) {
        //this->global_density = v;
        all_global_density[channel] = v;
    }

    bool is_mutate_enabled() {
        return this->mutate_enabled;
    }
    void set_mutated_enabled(bool v = true) {
        this->mutate_enabled = v;
    }

    int_fast8_t get_effective_mutation_count() {
        return effective_mutation_count;
    }
    void set_mutation_count(int_fast8_t v) {
        this->mutation_count = v;
    }
    int_fast8_t get_mutation_count() {
        return this->mutation_count;
    }

    bool should_reset_before_mutate() {
        return this->reset_before_mutate;
    }
    void set_reset_before_mutate(bool v = true) {
        this->reset_before_mutate = v;
    }

    bool is_fills_enabled() {
        return this->fills_enabled;
    }
    void set_fills_enabled(bool v = true) {
        this->fills_enabled = v;
    }

    bool is_add_phrase_enabled() {
        return this->add_phrase_to_seed;
    }
    void set_add_phrase_enabled(bool v = true) {
        this->add_phrase_to_seed = v;
    }

    bool is_euclidian_seed_lock() {
        return this->seed_locked;
    }
    void set_euclidian_seed_lock(bool v = true) {
        if (!is_euclidian_seed_lock() && v)
            this->last_locked_seed = get_euclidian_seed();
        this->seed_locked = v;
    }
    
    int get_euclidian_seed() {
        if (is_euclidian_seed_lock())
            return this->last_locked_seed;
        return seed + (is_add_phrase_enabled() ? BPM_CURRENT_PHRASE : 0);
    }
    void set_euclidian_seed(int seed) {
        this->seed = is_add_phrase_enabled() ? seed - BPM_CURRENT_PHRASE : seed;
    }
    
    SimplePattern *get_pattern(unsigned int pattern) {
        if (pattern < 0 || pattern >= number_patterns)
            return nullptr;
        return this->patterns[pattern];
    }

    // tell all patterns what their default arguments are
    void initialise_patterns();

    // reset all patterns to their default parameters, with optional force param to ignore locked status
    void reset_patterns(bool force = false) {
        //if (Serial) Serial.println("reset_patterns!");
        for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
            reset_pattern(i, false);
        }
    }
    // reset pattern X to its default parameter 
    void reset_pattern(uint_fast8_t i, bool force = false) {
        EuclidianPattern *p = (EuclidianPattern*)this->get_pattern(i);
        if (force || !p->is_locked()) {
            p->restore_default_arguments();
            p->make_euclid();
        }
    }

    virtual void on_loop(int tick) override {};
    virtual void on_tick(int tick) override {
        #ifdef SEQLIB_MUTATE_EVERY_TICK
            if (is_mutate_enabled()) {
                uint_fast8_t tick_of_step = tick % TICKS_PER_STEP;
                if (tick_of_step==TICKS_PER_STEP-1) {
                    for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
                        if (!patterns[i]->is_locked()) {
                            //if (Serial) Serial.println("mutate every tick!");
                            this->patterns[i]->make_euclid();
                        }
                    }
                }
            }
        #endif

        // todo: move this basic functionality into BaseSequencer

        //BaseSequencer::on_tick(tick);

        if (is_bpm_on_phrase(tick)) {
            this->on_phrase(BPM_CURRENT_PHRASE);
        }
        if (is_bpm_on_bar(tick)) {
            this->on_bar(BPM_CURRENT_BAR_OF_PHRASE);
        }
        if (is_bpm_on_beat(tick)) {
            this->on_beat(BPM_CURRENT_BEAT_OF_BAR);
        }
        if (is_bpm_on_sixteenth(tick)) {
            //this->on_step(this->get_step_for_tick(tick));
            this->on_step(tick / TICKS_PER_STEP);
        } else if (is_bpm_on_sixteenth(tick,1)) {
            // this re-enabled 2025-05-14 for Compulidean -- if usb_teensy_clocker/Microlidian start playing up then this might be the reason?
            this->on_step_end(tick / TICKS_PER_STEP); //(PPQN/STEPS_PER_BEAT));
        }
        for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
            this->patterns[i]->process_tick(tick);
        }

    };
    virtual void on_step(int step) override {
        //Serial.printf("EuclidianSequencer::on_step(%i), is_shuffle_enabled()=%i\n", step, is_shuffle_enabled());
        for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
            #ifdef ENABLE_SHUFFLE
                if (!is_shuffle_enabled() || (is_shuffle_enabled() && !this->patterns[i]->is_shuffled())) {
                    //if (Serial) Serial.printf("at tick %i, received on_step(%i, %i) callback for non-shuffled pattern\n", ticks, step, i);
                    this->patterns[i]->process_step(step);
                }
            #else
                this->patterns[i]->process_step(step);
            #endif
        }
    };

    virtual void on_step_end(int step) override {
        //Serial.printf("at tick %i, on_step_end(%i)\n", ticks, step);
        for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
            #ifdef ENABLE_SHUFFLE
                if (!is_shuffle_enabled() || (is_shuffle_enabled() && !this->patterns[i]->is_shuffled())) {
                    this->patterns[i]->process_step_end(step);
                }
            #else
                this->patterns[i]->process_step_end(step);
            #endif
        }
    }
    #ifdef ENABLE_SHUFFLE
        virtual void on_step_shuffled(int8_t track, int step) override {
            if (!is_shuffle_enabled()) return;

            for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
                if (this->patterns[i]->is_shuffled() && this->patterns[i]->get_shuffle_track()==track) {
                    //if (Serial) Serial.printf("at tick %i, received on_step_shuffled(%i, %i) callback for shuffled track %i\n", ticks, track, step, track);
                    this->patterns[i]->process_step(step);
                }
            }
        };

        virtual void on_step_end_shuffled(int8_t track, int step) override {
            if (!is_shuffle_enabled()) return;

            for (uint_fast8_t i = 0 ; i < number_patterns ; i++) {
                if (this->patterns[i]->is_shuffled() && this->patterns[i]->get_shuffle_track()==track) {
                    this->patterns[i]->process_step_end(step);
                }
            }
        }
    #endif
    virtual void on_beat(int beat) override {

    };
    virtual void on_bar(int bar) override {
        //if (Serial) Serial.println("on_bar!");
        if (fills_enabled && bar == BARS_PER_PHRASE - 1) {
            //if (Serial) Serial.println("on_bar doing fill!");
            // do fill
            uint_fast8_t effective_mutation_count = this->get_effective_mutation_count();
            for (uint_fast8_t i = 0 ; i < effective_mutation_count ; i++) {
                uint_fast8_t ran = random(mutate_minimum_pattern % number_patterns, constrain(1 + mutate_maximum_pattern, (unsigned int)0, number_patterns));
                if (!patterns[ran]->is_locked()) {
                    //this->patterns[ran]->arguments.rotation += 2;
                    this->patterns[ran]->set_rotation(this->patterns[ran]->get_rotation() + 2);
                    this->patterns[ran]->make_euclid();
                }
            }
            for (uint_fast8_t i = 0 ; i < effective_mutation_count ; i++) {
                uint_fast8_t ran = random(mutate_minimum_pattern % number_patterns, constrain(1 + mutate_maximum_pattern, (unsigned int)0, number_patterns));
                if (!patterns[ran]->is_locked()) {
                    this->patterns[ran]->set_rotation(this->patterns[ran]->get_rotation() * 2);
                    //this->patterns[ran]->arguments.pulses *= 2;
                    if (this->patterns[ran]->get_pulses() > this->patterns[ran]->get_steps()) 
                        this->patterns[ran]->set_pulses(this->patterns[ran]->get_pulses() / 8);
                    this->patterns[ran]->make_euclid();
                }
            }
        }
    };
    virtual void on_phrase(int phrase) override {
        if (is_mutate_enabled()) {
            if (reset_before_mutate)
                reset_patterns();
            unsigned long seed = get_euclidian_seed();
            randomSeed(seed);
            uint_fast8_t effective_mutation_count = this->get_effective_mutation_count();
            for (uint_fast8_t i = 0 ; i < effective_mutation_count ; i++) {
                // choose a pattern to mutate, out of all those for whom mutate is enabled
                uint_fast8_t ran = random(mutate_minimum_pattern % number_patterns, constrain(1 + mutate_maximum_pattern, (unsigned int)0, number_patterns));
                randomSeed(seed + ran);
                if (!patterns[ran]->is_locked()) {
                    this->patterns[ran]->mutate();
                }
            }
        }
    };
    
    #if defined(ENABLE_PARAMETERS)
        virtual LinkedList<FloatParameter*> *getParameters() override;
    #endif

    #if defined(ENABLE_SCREEN)
        FLASHMEM
        virtual void make_menu_items(Menu *menu) override {
            this->make_menu_items(menu, false);
        }
        //FLASHMEM
        virtual void make_menu_items(Menu *menu, bool combine_pages);
        //FLASHMEM
        virtual void create_menu_euclidian_mutation(int number_pages_to_create = 2);
    #endif


    // save/load stuff
    virtual LinkedList<String> *save_pattern_add_lines(LinkedList<String> *lines) override {
        lines = BaseSequencer::save_pattern_add_lines(lines);

        return lines;
    }
    virtual bool load_parse_key_value(String key, String value) override {
        return BaseSequencer::load_parse_key_value(key, value);
    }

    virtual void setup_saveable_parameters() override {
        if (this->saveable_parameters==nullptr) {
            BaseSequencer::setup_saveable_parameters();

            this->saveable_parameters->add(new LSaveableParameter<float>("global_density_0", "Euclidian", &all_global_density[0]));
            this->saveable_parameters->add(new LSaveableParameter<float>("global_density_1", "Euclidian", &all_global_density[1]));
            this->saveable_parameters->add(new LSaveableParameter<float>("global_density_2", "Euclidian", &all_global_density[2]));
            this->saveable_parameters->add(new LSaveableParameter<float>("global_density_3", "Euclidian", &all_global_density[3]));
            this->saveable_parameters->add(new LSaveableParameter<bool>("mutate_enabled", "Euclidian", &this->mutate_enabled));
            this->saveable_parameters->add(new LSaveableParameter<bool>("reset_before_mutate", "Euclidian", &this->reset_before_mutate));
            this->saveable_parameters->add(new LSaveableParameter<bool>("add_phrase", "Euclidian", &this->add_phrase_to_seed));
            this->saveable_parameters->add(new LSaveableParameter<bool>("fills_enabled", "Euclidian", &this->fills_enabled));
            this->saveable_parameters->add(new LSaveableParameter<int>("seed", "Euclidian", &this->seed));
            this->saveable_parameters->add(new LSaveableParameter<uint_fast8_t>("mutation_count", "Euclidian", &this->mutation_count));
        }
    }


};

