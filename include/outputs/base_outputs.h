#pragma once

#define MAX_LABEL 40

#ifdef ENABLE_SCREEN
    class Menu;
    #include "menu.h"
#endif

class ISequencerEventReceiver {
    public:
    virtual void receive_event(int_fast8_t event_value_1, int_fast8_t event_value_2, int_fast8_t event_value_3) = 0;
};

class FloatParameter;

// class to receive triggers from a sequencer and return values to the owner Processor
class BaseOutput : public ISequencerEventReceiver {
    public:
    bool enabled = true;

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

    virtual bool should_go_on() = 0;
    virtual bool should_go_off() = 0;

    virtual void stop() {};
    virtual void process() {};

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
};

