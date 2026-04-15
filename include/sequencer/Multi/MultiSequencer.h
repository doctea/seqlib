// wrapper class to allow multiple Sequencers to be used simultaneously

#pragma once

#include "../Base/Sequencer.h"

class MultiSequencer : public SimpleSequencer {
    public:
    LinkedList<BaseSequencer*> *sequencers = nullptr;
    int number_patterns = 0;

    MultiSequencer() : SimpleSequencer(nullptr) {
        this->sequencers = new LinkedList<BaseSequencer*>();
        #ifdef ENABLE_STORAGE
             this->set_path_segment("MultiSequencer");
        #endif
    }
    virtual ~MultiSequencer() {};

    virtual void addSequencer(BaseSequencer *sequencer) {
        this->sequencers->add(sequencer);
        this->number_patterns += sequencer->get_number_patterns();
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
    virtual uint16_t get_number_patterns() override {
        // this is set whenever we add a sequencer or pattern
        //Serial.printf("MultiSequencer::get_number_patterns() returns %i\n", this->number_patterns);
        return this->number_patterns;
    }
    virtual void add_pattern(BasePattern *pattern) override {
        // add to the last sequencer by default, but this is a bit hacky and we should probably specify which sequencer to add to
        if (this->sequencers->size() > 0) {
            this->sequencers->get(this->sequencers->size() - 1)->add_pattern(pattern);
            this->number_patterns += 1;
        }
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

    virtual void do_deferred_recomputes() override {
        for (unsigned int i = 0; i < this->sequencers->size(); i++) {
            this->sequencers->get(i)->do_deferred_recomputes();
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
        LinkedList<FloatParameter*> *parameters = nullptr;
        virtual LinkedList<FloatParameter*> *getParameters() {
            if (this->parameters!=nullptr) 
                return this->parameters;

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
    #ifdef ENABLE_STORAGE
        virtual void setup_saveable_settings() override {
            // inherit parent's settings
            // don't do that here cos we want to expose each of the child sequencers as 'sub-devices' in the 
            // save/load system, so we call register_child for each of them instead 
            //BaseSequencer::setup_saveable_settings();   

            // todo: remove rp2040-specify code

            Serial.printf("\n=== MultiSequencer::setup_saveable_settings() for sequencer %p with %i child sequencers...\n", this, this->sequencers->size()); Serial.flush();
            for (unsigned int i = 0 ; i < this->sequencers->size() ; i++) {
                BaseSequencer *s = this->sequencers->get(i);
                if (s==nullptr) continue;
                register_child(s);
            }
            if (Serial) Serial.printf("=== MultiSequencer::setup_saveable_settings() done for sequencer %p, free ram is %u\n\n", this, rp2040.getFreeHeap()); Serial.flush();
            if (Serial) Serial.flush();
        }
    #endif
};