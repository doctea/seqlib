#pragma once

#define MAX_LABEL 40

#ifdef ENABLE_SCREEN
    class Menu;
    #include "menu.h"
#endif

#include "saveload_settings.h"

class ISequencerEventReceiver {
    public:
    virtual void receive_event(int_fast8_t event_value_1, int_fast8_t event_value_2, int_fast8_t event_value_3) = 0;
};

#include "parameters/Parameter.h"

// class to receive triggers from a sequencer and return values to the owner Processor
class BaseOutput : public ISequencerEventReceiver, public ISaveableSettingHost {
    public:
    bool enabled = true;
    bool has_gone_on = false;

    char label[MAX_LABEL];
    BaseOutput (const char *label) {
        strncpy(this->label, label, MAX_LABEL);
    }
    
    // event_value_1 = send a note on
    // event_value_2 = send a note off
    // event_value_3 = ??
    virtual void receive_event(int_fast8_t event_value_1, int_fast8_t event_value_2, int_fast8_t event_value_3) = 0;
    virtual void reset() = 0;
    virtual bool matches_label(const char *compare) {
        return strcmp(compare, this->label)==0;
    }

    virtual bool has_gone_on_this_time() {
        return this->has_gone_on;
    }
    virtual bool should_go_on() = 0;
    virtual bool should_go_off() = 0;

    virtual void stop() {};
    virtual void process() {
        this->has_gone_on = false;
    };

    virtual void loop() {};

    void set_enabled(bool state) {
        this->enabled = state;
    }
    bool is_enabled() {
        return this->enabled;
    }

    #ifdef ENABLE_SCREEN
        //FLASHMEM
        virtual void make_menu_items(Menu *menu, int index) {}
        //FLASHMEM
        #ifdef ENABLE_PARAMETERS
            //FLASHMEM
            virtual void make_parameter_menu_items(Menu *menu, int index, uint16_t colour = C_WHITE, bool combine_pages = false);
        #endif
    #endif

    #ifdef ENABLE_PARAMETERS
        virtual LinkedList<FloatParameter*> *get_parameters() {
            return nullptr;
        }
    #endif

    virtual void setup_saveable_settings() override {
        // inherit parent's settings
        ISaveableSettingHost::setup_saveable_settings();

        register_setting(
            new LSaveableSetting<bool>(
                "Enabled",
                "BaseOutput",
                &this->enabled,
                [=](bool value) -> void {
                    this->set_enabled(value);
                },
                [=](void) -> bool {
                    return this->is_enabled();
                }
            )
        );

        // save parameters for this output
        LinkedList<FloatParameter*> *parameters = this->get_parameters();
        if (parameters!=nullptr) {
            for (int i = 0 ; i < parameters->size() ; i++) {
                FloatParameter *param = parameters->get(i);
                register_child(param);
            }
        }
    }

};

// dummy output for using as a placeholder for 'None' or 'no output' when selecting outputs in the menu
class NullOutput : public BaseOutput {
    public:
    NullOutput(const char *label) : BaseOutput(label) {}

    virtual void receive_event(int_fast8_t event_value_1, int_fast8_t event_value_2, int_fast8_t event_value_3) override {
        // do nothing
    }
    virtual void reset() override {
        // do nothing
    }
    virtual bool should_go_on() override {
        return false;
    }
    virtual bool should_go_off() override {
        return false;
    }
};