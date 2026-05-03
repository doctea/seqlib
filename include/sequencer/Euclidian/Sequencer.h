#pragma once

#include "Euclidian.h"
#include "Pattern.h"

class EuclidianSequencer : public BaseSequencer
    #ifdef ENABLE_STORAGE
        , virtual public SHDynamic<25, 12>
    #endif
    {
    // todo: list of EuclidianPatterns...? althoguh array is probably fine
    EuclidianPattern **patterns = nullptr;
    int_fast8_t number_patterns = 0;
    int_fast8_t number_added_patterns = 0;  // just so we can count how many patterns have been added, so we know where to add the next one

    int seed = 0;
    uint_fast8_t mutate_minimum_pattern = 0, mutate_maximum_pattern = 0;
    uint_fast8_t mutation_count = DEFAULT_MUTATION_COUNT, effective_mutation_count = DEFAULT_MUTATION_COUNT;
    bool    reset_before_mutate = true, 
            mutate_enabled = true, 
            fills_enabled = true, 
            add_phrase_to_seed = true;
    //float global_density = 0.6666f;

    bool seed_locked = false;
    uint32_t last_locked_seed = 0;



    public:
    
    // need to pass desired number_patterns so that we can pre-allocate the patterns array... default to 20
    EuclidianSequencer(LinkedList<BaseOutput*> *available_outputs, int8_t number_patterns = 20) : BaseSequencer() {

        #ifdef ENABLE_STORAGE
             this->set_path_segment("EuclidianSequencer");
        #endif

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
            EuclidianPattern *p = new EuclidianPattern(available_outputs, i / (number_patterns / NUM_GLOBAL_DENSITY_GROUPS));
            #ifdef ENABLE_STORAGE
                 p->set_path_segment_fmt("pattern_%i", i);
            #endif
            this->add_pattern(p);

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

    // pattern management
    virtual SimplePattern *get_pattern(unsigned int pattern) override {
        if (pattern < 0 || pattern >= get_number_patterns())
            return nullptr;
        return this->patterns[pattern];
    }
    virtual uint16_t get_number_patterns() override {
        return this->number_patterns;
    }
    virtual void add_pattern(BasePattern *pattern) override {
        if (number_added_patterns >= number_patterns) {
            Serial.printf("Error: cannot add more than %i patterns to EuclidianSequencer\n", number_patterns);
            return;
        }

        this->patterns[number_added_patterns] = (EuclidianPattern*)pattern;
        number_added_patterns++;

        mutate_maximum_pattern = number_added_patterns;
    }

    // parameters and settings
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
    void set_mutate_enabled(bool v = true) {
        this->mutate_enabled = v;
    }
    bool toggle_mutate_enabled() {
        set_mutate_enabled(!is_mutate_enabled());
        return is_mutate_enabled();
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
    
    // tell all patterns what their default arguments are
    void initialise_patterns();

    // reset all patterns to their default parameters, with optional force param to ignore locked status
    void reset_patterns(bool force = false) {
        //if (Serial) Serial.println("reset_patterns!");
        for (uint_fast8_t i = 0 ; i < this->get_number_patterns() ; i++) {
            reset_pattern(i, false);
        }
    }
    // reset pattern X to its default parameter 
    void reset_pattern(uint_fast8_t i, bool force = false) {
        EuclidianPattern *p = (EuclidianPattern*)this->get_pattern(i);
        if (force || !p->is_locked()) {
            p->restore_default_arguments();
            // Defer the recompute; pattern data will be updated from loop().
            p->mark_dirty();
        }
    }

    virtual void on_tick(int tick) override {
        #ifdef SEQLIB_MUTATE_EVERY_TICK
            if (is_mutate_enabled()) {
                uint_fast8_t tick_of_step = tick % TICKS_PER_STEP;
                if (tick_of_step==(uint_fast8_t)(TICKS_PER_STEP-1)) {
                    for (uint_fast8_t i = 0 ; i < this->get_number_patterns() ; i++) {
                        if (!this->get_pattern(i)->is_locked()) {
                            //if (Serial) Serial.println("mutate every tick!");
                            // Mark dirty so make_euclid() runs in loop() rather than the ISR.
                            ((EuclidianPattern*)this->get_pattern(i))->mark_dirty();
                        }
                    }
                }
            }
        #endif

        // call base on_tick to trigger the appropriate callbacks for the current tick, step, beat, bar, and phrase
        BaseSequencer::on_tick(tick);

        for (uint_fast8_t i = 0 ; i < this->get_number_patterns() ; i++) {
            this->get_pattern(i)->process_tick(tick);
        }
    };

    virtual void on_step(int step) override {
        //Serial.printf("EuclidianSequencer::do_step(%i)\n", step);

        // At bar/phrase boundaries, on_bar() and on_phrase() fire before on_step() within the same
        // ISR call (see BaseSequencer::on_tick).  Those callbacks call mutate()/do_bar_fill() which
        // only mark_dirty() rather than calling make_euclid() immediately (the deferral-to-loop()
        // optimisation).  Without this flush the first step of the new bar fires events against the
        // stale pattern, producing incorrect notes/kicks.
        //
        // We therefore flush any pending recomputes here at bar boundaries so patterns are up-to-date
        // before process_step() consults them.  The cost is identical to what was accepted before the
        // deferral was introduced: one make_euclid() per dirty pattern, once per bar.
        //
        // To revert to fully-deferred behaviour (never in ISR): remove this block and accept that
        // bar-boundary mutations will take effect one loop() iteration late.
        if (step % STEPS_PER_BAR == 0) {
            do_deferred_recomputes();
        }

        for (uint_fast8_t i = 0 ; i < this->get_number_patterns() ; i++) {
            #ifdef ENABLE_SHUFFLE
                if (!is_shuffle_enabled() || (is_shuffle_enabled() && !this->get_pattern(i)->is_shuffled())) {
                    this->get_pattern(i)->process_step(step);
                }
            #else
                this->get_pattern(i)->process_step(step);
            #endif
        }
    };

    virtual void on_step_end(int step) override {
        //Serial.printf("EuclidianSequencer::do_step_end_dispatch(%i)\n", step);
        for (uint_fast8_t i = 0 ; i < this->get_number_patterns() ; i++) {
            #ifdef ENABLE_SHUFFLE
                if (!is_shuffle_enabled() || (is_shuffle_enabled() && !this->get_pattern(i)->is_shuffled())) {
                    this->get_pattern(i)->process_step_end(step);
                }
            #else
                this->get_pattern(i)->process_step_end(step);
            #endif
        }
    }

    #ifdef ENABLE_SHUFFLE
        virtual void on_step_shuffled(int8_t track, int step) override {
            if (!is_shuffle_enabled()) return;

            for (uint_fast8_t i = 0 ; i < this->get_number_patterns() ; i++) {
                if (this->get_pattern(i)->is_shuffled() && this->get_pattern(i)->get_shuffle_track()==track) {
                    //if (Serial) Serial.printf("at tick %i, received on_step_shuffled(%i, %i) callback for shuffled track %i\n", ticks, track, step, track);
                    this->get_pattern(i)->process_step(step);
                }
            }
        };

        virtual void on_step_end_shuffled(int8_t track, int step) override {
            if (!is_shuffle_enabled()) return;

            for (uint_fast8_t i = 0 ; i < this->get_number_patterns() ; i++) {
                if (this->get_pattern(i)->is_shuffled() && this->get_pattern(i)->get_shuffle_track()==track) {
                    this->get_pattern(i)->process_step_end(step);
                }
            }
        }
    #endif
    virtual void on_beat(int beat) override {

    };
    virtual void on_bar(int bar) override {
        if (fills_enabled && bar == BARS_PER_PHRASE - 1) {
            do_bar_fill(bar);
        }
    };

    virtual void on_phrase(int phrase) override {
        do_phrase_mutations(phrase);
    };

    void do_phrase_mutations(int phrase) {
        if (!is_mutate_enabled()) return;
        if (reset_before_mutate)
            reset_patterns();
        unsigned long seed = get_euclidian_seed();
        randomSeed(seed);
        uint_fast8_t effective_mutation_count = this->get_effective_mutation_count();
        for (uint_fast8_t i = 0 ; i < effective_mutation_count ; i++) {
            uint_fast8_t ran = random(mutate_minimum_pattern % this->get_number_patterns(), constrain(1 + mutate_maximum_pattern, (unsigned int)0, this->get_number_patterns()));
            randomSeed(seed + ran);
            if (!((EuclidianPattern*)this->get_pattern(ran))->is_locked()) {
                ((EuclidianPattern*)this->get_pattern(ran))->mutate();
            }
        }
    }

    void do_bar_fill(int bar) {
        uint_fast8_t effective_mutation_count = this->get_effective_mutation_count();
        for (uint_fast8_t i = 0 ; i < effective_mutation_count ; i++) {
            uint_fast8_t ran = random(
                mutate_minimum_pattern % (uint8_t)number_patterns,
                constrain(1 + mutate_maximum_pattern, (uint8_t)0, (uint8_t)number_patterns)
            );
            if (!this->get_pattern(ran)->is_locked()) {
                ((EuclidianPattern*)this->get_pattern(ran))->set_rotation(((EuclidianPattern*)this->get_pattern(ran))->get_rotation() + 2);
                ((EuclidianPattern*)this->get_pattern(ran))->mark_dirty();
            }
        }
        for (uint_fast8_t i = 0 ; i < effective_mutation_count ; i++) {
            uint_fast8_t ran = random(
                mutate_minimum_pattern % (uint8_t)number_patterns,
                constrain(1 + mutate_maximum_pattern, (unsigned int)0, (uint8_t)number_patterns)
            );
            if (!this->get_pattern(ran)->is_locked()) {
                ((EuclidianPattern*)this->get_pattern(ran))->set_rotation(((EuclidianPattern*)this->get_pattern(ran))->get_rotation() * 2);
                if (((EuclidianPattern*)this->get_pattern(ran))->get_pulses() > ((EuclidianPattern*)this->get_pattern(ran))->get_steps())
                    ((EuclidianPattern*)this->get_pattern(ran))->set_pulses(((EuclidianPattern*)this->get_pattern(ran))->get_pulses() / 8);
                ((EuclidianPattern*)this->get_pattern(ran))->mark_dirty();
            }
        }
    }


    
    #if defined(ENABLE_PARAMETERS)
        virtual LinkedList<FloatParameter*> *getParameters() override;
    #endif

    // Call from loop() (never from ISR) to perform any pending make_euclid() recomputes.
    virtual void do_deferred_recomputes() override {
        for (uint_fast8_t i = 0; i < this->get_number_patterns(); i++) {
            ((EuclidianPattern*)this->get_pattern(i))->do_deferred_recompute();
        }
    }

    #if defined(ENABLE_SCREEN)
        // FLASHMEM
        // virtual void make_menu_items(Menu *menu) override {
        //     this->make_menu_items(menu, 0);
        // }
        //FLASHMEM
        virtual void make_menu_items(Menu *menu, int combine_pages);
        //FLASHMEM
        virtual void create_menu_euclidian_mutation(Euclidian::CombinePageOption combine_setting);
    #endif

    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override {
            // inherit parent's settings
            BaseSequencer::setup_saveable_settings();

            register_setting(
                new VarSetting<float>("Global Density 0", "Euclidian", &all_global_density[0]), 
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow global density to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
            register_setting(
                new VarSetting<float>("Global Density 1", "Euclidian", &all_global_density[1]), 
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow global density to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
            register_setting(
                new VarSetting<float>("Global Density 2", "Euclidian", &all_global_density[2]), 
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow global density to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
            register_setting(
                new VarSetting<float>("Global Density 3", "Euclidian", &all_global_density[3]), 
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow global density to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
            register_setting(
                new VarSetting<bool>("Mutate Enabled", "Euclidian", &this->mutate_enabled),
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow mutate enabled state to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
            register_setting(
                new VarSetting<bool>("Reset Before Mutate", "Euclidian", &this->reset_before_mutate),
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow reset before mutate state to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
            register_setting(
                new VarSetting<bool>("Add Phrase To Seed", "Euclidian", &this->add_phrase_to_seed),
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow add phrase to seed state to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
            register_setting(
                new VarSetting<bool>("Fills Enabled", "Euclidian", &this->fills_enabled),
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow fills enabled state to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
            register_setting(
                new VarSetting<int>("Seed", "Euclidian", &this->seed),
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow seed state to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
            register_setting(
                new VarSetting<uint_fast8_t>("Mutation Count", "Euclidian", &this->mutation_count),
                SL_SCOPE_SCENE | SL_SCOPE_PROJECT  // allow mutation count state to be saved at scene or project level, since it's more of a preference setting than a performance setting
            );
        }
    #endif
};
