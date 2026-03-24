// wrapper class to allow multiple Sequencers to be used simultaneously

#pragma once

#include "../Base/Sequencer.h"

class MultiSequencer : public BaseSequencer {
    public:
    LinkedList<BaseSequencer*> *sequencers = nullptr;

    MultiSequencer() : BaseSequencer() {
        this->sequencers = new LinkedList<BaseSequencer*>();
    }
    virtual ~MultiSequencer() {};

    virtual void addSequencer(BaseSequencer *sequencer) {
        this->sequencers->add(sequencer);
    }

    virtual SimplePattern *get_pattern(unsigned int pattern) override {
        int offset = 0;

        for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
            BaseSequencer *s = this->sequencers->get(i);
            if (s!=nullptr) {
                if (pattern < offset + s->get_number_patterns()) {
                    return s->get_pattern(pattern - offset);
                }
                offset += s->get_number_patterns();
            }
        }

        return nullptr;
    }

    virtual void on_tick(int tick) override {
        for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
            this->sequencers->get(i)->on_tick(tick);
        }
    }

    virtual void on_loop(int tick) override {
        for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
            this->sequencers->get(i)->on_loop(tick);
        }
    }
    virtual void on_beat(int beat) override {
        for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
            this->sequencers->get(i)->on_beat(beat);
        }
    };
    virtual void on_bar(int bar) override {
        for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
            this->sequencers->get(i)->on_bar(bar);
        }
    };
    virtual void on_phrase(int phrase) override {
        for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
            this->sequencers->get(i)->on_phrase(phrase);
        }
    }

    virtual void on_step(int step) override {
        for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
            this->sequencers->get(i)->on_step(step);
        }
    };
    virtual void on_step_end(int step) {
        for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
            this->sequencers->get(i)->on_step_end(step);
        }
    }

    #ifdef ENABLE_SHUFFLE
        virtual void on_step_shuffled(int8_t track, int step) override {
            for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
                this->sequencers->get(i)->on_step_shuffled(track, step);
            }
        };
        virtual void on_step_end_shuffled(int8_t track, int step) override {
            for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
                this->sequencers->get(i)->on_step_end_shuffled(track, step);
            }
        };
        
        virtual bool is_shuffle_enabled() override {
            return this->shuffle_enabled;
        }
        virtual void set_shuffle_enabled(bool state = true) override {
            this->shuffle_enabled = state;
            for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
                this->sequencers->get(i)->set_shuffle_enabled(state);
            }
        }
    #endif

    //virtual void configure_pattern_output(int index, BaseOutput *output);
    
    #if defined(ENABLE_PARAMETERS)
        virtual LinkedList<FloatParameter*> *getParameters() {
            LinkedList<FloatParameter*> *params = new LinkedList<FloatParameter*>();
            for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
                BaseSequencer *s = this->sequencers->get(i);
                LinkedList<FloatParameter*> *sub_params = s->getParameters();
                if (sub_params!=nullptr) {
                    for (unsigned int j = 0 ; j < sub_params->size() ; j++) {
                        FloatParameter *p = sub_params->get(j);
                        if (p!=nullptr)
                            params->add(p);
                    }
                }
            }
            return params;
        }
        //virtual FloatParameter* getParameterByName(const char *name) {}
    #endif

    #if defined(ENABLE_SCREEN)
        virtual void make_menu_items(Menu *menu, int combine_pages) override {
            for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
                BaseSequencer *s = this->sequencers->get(i);
                if (s!=nullptr)
                    s->make_menu_items(menu, combine_pages);
            }
        }
    #endif

    // save/load stuff
    // todo: this is pretty much untested, and might not work as intended -- need to check that the keys are unique across the sequencers, otherwise we might end up with some weird overwriting behaviour
    virtual LinkedList<String> *save_pattern_add_lines(LinkedList<String> *lines) {
        for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
            BaseSequencer *s = this->sequencers->get(i);
            if (s!=nullptr)
                s->add_save_lines_saveable_parameters(lines);
        }
        return lines;
    }
    virtual bool load_parse_key_value(String key, String value) {
        for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
            BaseSequencer *s = this->sequencers->get(i);
            if (s!=nullptr)
                s->load_parse_key_value_saveable_parameters(key, value);
        }
        return false;
    }

    virtual void setup_saveable_parameters() override {
        if (this->saveable_parameters==nullptr) {
            ISaveableParameterHost::setup_saveable_parameters();

            for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
                BaseSequencer *s = this->sequencers->get(i);
                if (s!=nullptr)
                    s->setup_saveable_parameters();
            }
        }
    }
};