#pragma once

#include "menuitems.h"
#include "menuitems_popout.h"
#include "outputs/base_outputs.h"
#include "debug.h"

#include <LinkedList.h>

static constexpr int OUTPUT_SELECTOR_MAX_OPTIONS = 64;
static constexpr int OUTPUT_SELECTOR_MAX_SNAPSHOTS = 8;

struct OutputSelectorSnapshotEntry {
    LinkedList<BaseOutput*> *source = nullptr;
    int count = 0;
    BaseOutput *outputs[OUTPUT_SELECTOR_MAX_OPTIONS] = {nullptr};
};

class OutputSelectorSnapshotCache {
    public:
        static OutputSelectorSnapshotEntry *rebuild(LinkedList<BaseOutput*> *source) {
            if (source==nullptr)
                return nullptr;

            OutputSelectorSnapshotEntry *slot = find_slot(source);
            if (slot==nullptr)
                slot = find_empty_slot();
            if (slot==nullptr)
                slot = &entries()[0];

            slot->source = source;
            slot->count = 0;

            const int list_size = (int)source->size();
            const int copy_count = list_size < OUTPUT_SELECTOR_MAX_OPTIONS ? list_size : OUTPUT_SELECTOR_MAX_OPTIONS;
            for (int i = 0; i < copy_count; i++) {
                slot->outputs[i] = source->get(i);
            }
            for (int i = copy_count; i < OUTPUT_SELECTOR_MAX_OPTIONS; i++) {
                slot->outputs[i] = nullptr;
            }
            slot->count = copy_count;

            return slot;
        }

    private:
        static OutputSelectorSnapshotEntry *entries() {
            static OutputSelectorSnapshotEntry storage[OUTPUT_SELECTOR_MAX_SNAPSHOTS];
            return storage;
        }

        static OutputSelectorSnapshotEntry *find_slot(LinkedList<BaseOutput*> *source) {
            OutputSelectorSnapshotEntry *storage = entries();
            for (int i = 0; i < OUTPUT_SELECTOR_MAX_SNAPSHOTS; i++) {
                if (storage[i].source == source)
                    return &storage[i];
            }
            return nullptr;
        }

        static OutputSelectorSnapshotEntry *find_empty_slot() {
            OutputSelectorSnapshotEntry *storage = entries();
            for (int i = 0; i < OUTPUT_SELECTOR_MAX_SNAPSHOTS; i++) {
                if (storage[i].source == nullptr)
                    return &storage[i];
            }
            return nullptr;
        }
};

// Selector to choose an OutputNode from the available list to use as target 
template<class TargetClass>
class OutputSelectorControl : public SelectorControl<int_least16_t> {
    BaseOutput *initial_selected_object = nullptr;
    LinkedList<BaseOutput*> *available_objects = nullptr;
    OutputSelectorSnapshotEntry *snapshot = nullptr;
    int snapshot_count = 0;

    TargetClass *target_object = nullptr;
    void(TargetClass::*setter_func)(BaseOutput*) = nullptr;
    BaseOutput*(TargetClass::*getter_func)() = nullptr;

    bool show_values = false;   // whether to display the incoming values or not

    void rebuild_snapshot() {
        this->snapshot = OutputSelectorSnapshotCache::rebuild(this->available_objects);
        this->snapshot_count = this->snapshot!=nullptr ? this->snapshot->count : 0;
    }

    BaseOutput *get_snapshot_output(int index) const {
        if (this->snapshot==nullptr)
            return nullptr;
        if (index<0 || index>=this->snapshot_count)
            return nullptr;
        return this->snapshot->outputs[index];
    }

    int wrap_index(int index) {
        const int count = this->get_num_values();
        if (count<=0)
            return -1;
        while (index<0)
            index += count;
        while (index>=count)
            index -= count;
        return index;
    }

    public:

    OutputSelectorControl(
        const char *label, 
        TargetClass *target_object, 
        void(TargetClass::*setter_func)(BaseOutput*), 
        BaseOutput*(TargetClass::*getter_func)(), 
        LinkedList<BaseOutput*> *available_objects,
        BaseOutput *initial_selected_object = nullptr,
        bool show_values = false
    ) : SelectorControl(label, -1) {
        this->show_values = show_values;
        this->initial_selected_object = initial_selected_object;
        this->target_object = target_object;
        this->setter_func = setter_func;
        this->getter_func = getter_func;
        this->set_available_values(available_objects);
    };

    virtual void configure (LinkedList<BaseOutput*> *available_objects) {
        this->set_available_values(available_objects);
        const char *initial_name = (char*)"None";
        if (this->initial_selected_object!=nullptr)
            initial_name = this->initial_selected_object->label;
        actual_value_index = this->find_index_for_label(initial_name);
    }

    virtual int find_index_for_label(const char *name)  {
        if (strcmp(name, "None")==0)
            return -1;
        for (int i = 0 ; i < this->snapshot_count ; i++) {
            BaseOutput *out = this->get_snapshot_output(i);
            if (out!=nullptr && strcmp(out->label, name)==0)
                return i;
        }
        return -1;
    }

    virtual int find_index_for_object(BaseOutput *input) {
        if (input==nullptr)
            return -1;
        for (int i = 0 ; i < this->snapshot_count ; i++) {
            if (this->get_snapshot_output(i) == input)
                return i;
        }
        return this->find_index_for_label(input->label);
    }

    virtual void on_add() override {
        //this->debug = true;
        //Serial.printf(F("%s#on_add...\n"), this->label); Serial_flush();
        actual_value_index = -1;
        /*if (this->debug) {
            Serial.printf("on_add() in ParameterSelectorControl @%p:\n", this); Serial_flush();
            Serial.printf("\tParameterSelectorControl with initial_selected_parameter @%p...\n", initial_selected_parameter_input); Serial_flush();
            if (initial_selected_parameter_input!=nullptr) {
                Serial.printf("\tParameterSelectorControl looking for '%s' @%p...\n", initial_selected_parameter_input->label, initial_selected_parameter_input); Serial_flush();
            } else 
                Serial.println("\tno initial_selected_parameter set");
        }*/

        if (this->target_object!=nullptr && this->getter_func!=nullptr) {
            BaseOutput *current = (this->target_object->*this->getter_func)();
            this->actual_value_index = this->find_index_for_object(current);
        } else if (initial_selected_object!=nullptr) {
            //Serial.printf(F("%s#on_add: got non-null initial_selected_parameter_input\n")); Serial_flush();
            //Serial.printf(F("\tand its name is %c\n"), initial_selected_parameter_input->name); Serial_flush();
            //this->actual_value_index = parameter_manager->getInputIndexForName(initial_selected_parameter_input->name); ////this->find_parameter_input_index_for_label(initial_selected_parameter_input->name);
            this->actual_value_index = this->find_index_for_label(initial_selected_object->label);
        } else {
            this->actual_value_index = -1;
        }
        this->selected_value_index = this->actual_value_index;
        //Serial.printf(F("#on_add returning"));
    }

    virtual const char *get_label_for_index(int_least16_t index) {
        if (index<0 || index >= this->snapshot_count)
            return "None";
        BaseOutput *out = this->get_snapshot_output(index);
        if (out==nullptr || out->label==nullptr)
            return "None";
        return out->label;
    }

    // update the control to reflect changes to selection (eg, called when new value is loaded from project file)
    virtual void update_source(BaseOutput *new_source)  {
        //int index = parameter_manager->getPitchInputIndex(new_source);
        //Serial.printf("update_source got index %i\n", index);
        int index = this->find_index_for_object(new_source);
        this->update_actual_index(index);
    }

    virtual void setter(int_least16_t new_value) override {
        //if (this->debug) Serial.printf(F("ParameterSelectorControl changing from %i to %i\n"), this->actual_value_index, new_value);
        selected_value_index = actual_value_index = new_value;
        if (this->target_object==nullptr || this->setter_func==nullptr)
            return;

        if(new_value>=0 && new_value < this->snapshot_count) {
            (this->target_object->*this->setter_func)(this->get_snapshot_output(new_value));
        } else {
            (this->target_object->*this->setter_func)(nullptr);
        }
    }
    virtual int_least16_t getter() override {
        return selected_value_index;
    }

    // classic fixed display version
    virtual int display(Coord pos, bool selected, bool opened) override {
        tft->setTextSize(0);

        this->num_values = this->get_num_values();

        if (!opened) {
            colours(selected, this->default_fg, BLACK);
            tft->setTextSize(2);

            if (this->actual_value_index>=0 && this->actual_value_index < num_values) {
                tft->printf((char*)"Output: %s\n", (char*)this->get_label_for_index(this->actual_value_index));
            } else {
                tft->printf((char*)"Output: none\n");
            }
        } else {
            const int display_index = (this->selected_value_index>=0 && this->selected_value_index < num_values)
                ? this->selected_value_index
                : this->actual_value_index;

            const int left_index = this->wrap_index(display_index - 1);
            const int right_index = this->wrap_index(display_index + 1);
            char left_hint[MENU_C_MAX];
            char right_hint[MENU_C_MAX];
            snprintf(left_hint, MENU_C_MAX, "< %s", this->get_label_for_index(left_index));
            snprintf(right_hint, MENU_C_MAX, "%s >", this->get_label_for_index(right_index));

            SelectorTakeoverOverlaySpec overlay;
            overlay.title = this->label;
            overlay.value = this->get_label_for_index(display_index);
            overlay.left_hint = left_hint;
            overlay.right_hint = right_hint;
            overlay.frame_colour = selected ? GREEN : C_WHITE;
            overlay.left_hint_fg = tft->halfbright_565(C_WHITE);
            overlay.right_hint_fg = tft->halfbright_565(C_WHITE);
            overlay.box_padding = 2;
            overlay.min_box_h = 26;

            return menu_draw_selector_takeover_overlay(tft, pos, overlay);
        }
        return tft->getCursorY();
    }

    virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {
        const int index_to_display = opened ? selected_value_index : actual_value_index;
        const char *lbl = this->get_label_for_index(index_to_display);

        colours(selected, C_WHITE, BLACK);
        tft->setTextSize(tft->get_textsize_for_width(lbl, max_character_width*tft->characterWidth()));
        tft->println(lbl);

        return tft->getCursorY();
    }

    virtual bool wants_fullscreen_overlay_when_opened_in_bar() override {
        return true;
    }

    virtual bool button_select() override {
        //Serial.printf("button_select with selected_value_index %i\n", selected_value_index);
        //Serial.printf("that is available_values[%i] of %i\n", selected_value_index, available_values[selected_value_index]);
        this->setter(selected_value_index);

        char msg[MENU_MESSAGE_MAX];
        //Serial.printf("about to build msg string...\n");
        const char *name = this->get_label_for_index(selected_value_index);
        //if (selected_value_index>=0)
        snprintf(msg, MENU_MESSAGE_MAX, "Set %s to %s (%i)", label, name, selected_value_index);
        //Serial.printf("about to set_last_message!");
        //msg[20] = '\0'; // limit the string so we don't overflow set_last_message
        menu_set_last_message(msg,GREEN);

        return go_back_on_select;
    }

    virtual void set_available_values(LinkedList<BaseOutput*> *available_values) {
        this->available_objects = available_values;
        this->rebuild_snapshot();
        this->num_values = this->snapshot_count;
    }

    virtual int get_num_values() {
        return this->snapshot_count;
    }

};

