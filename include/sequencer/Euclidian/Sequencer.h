#pragma once

#include "Euclidian.h"
#include "Pattern.h"

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

        // call base on_tick to trigger the appropriate callbacks for the current tick, step, beat, bar, and phrase
        BaseSequencer::on_tick(tick);

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
        // FLASHMEM
        // virtual void make_menu_items(Menu *menu) override {
        //     this->make_menu_items(menu, 0);
        // }
        //FLASHMEM
        virtual void make_menu_items(Menu *menu, int combine_pages);
        //FLASHMEM
        virtual void create_menu_euclidian_mutation(CombinePageOption combine_setting);
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
