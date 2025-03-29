#pragma once

#include "mymenu.h"

#include "sequencer/Sequencer.h"

class ShufflePatternEditorControl : public MenuItem {
    ShufflePatternWrapper *shuffwrapper = nullptr;

    public:
        ShufflePatternEditorControl(const char *label, ShufflePatternWrapper *shuffwrapper) : MenuItem(label) {
            this->shuffwrapper = shuffwrapper;
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            //return pos.y;
            int initial_y = pos.y;
            pos.y = header(label, pos, selected, opened);

            this->colours(selected || opened, opened ? GREEN : this->default_fg, this->default_bg);

            char v[5];
            snprintf(v, 4, "%-1.1f", shuffwrapper->get_amount());
            tft->printf(v);

            for (int i = 0 ; i < shuffwrapper->size ; i++) {
                //tft->setCursor(pos.x * i + 3, pos.y);
                tft->printf("%-2i ", shuffwrapper->get_step(i));
            }

            tft->println();

            return tft->getCursorY();
        }

        virtual bool button_right() override {
            //tft->printf("button_select on ShufflePatternEditorControl\n");
            //tft->println();
            //shuffwrapper->set_active(!shuffwrapper->get_active());
            for (int i = 0 ; i < shuffwrapper->size ; i++) {
                shuffwrapper->set_step(i, random(-5, 6));
            }
            shuffwrapper->update_target();

            return true;
        }

        virtual bool knob_left() override {
            shuffwrapper->set_amount(shuffwrapper->get_amount() - 0.1f);
            if (shuffwrapper->get_amount() < 0.0f) {
                shuffwrapper->set_amount(0.0f);
            }
            shuffwrapper->update_target();

            return true;
        }

        virtual bool knob_right() override {
            shuffwrapper->set_amount(shuffwrapper->get_amount() + 0.1f);
            if (shuffwrapper->get_amount() > 1.0f) {
                shuffwrapper->set_amount(1.0f);
            }
            shuffwrapper->update_target();

            return true;
        }

};