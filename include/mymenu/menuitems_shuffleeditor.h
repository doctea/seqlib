#pragma once

#include "mymenu.h"

#include "sequencer/Sequencer.h"

class ShufflePatternEditorControl : public MenuItem {
    ShufflePatternWrapper *shufflewrapper = nullptr;

    int selected_step = 0;
    bool editing = false;

    const int SHUFFLE_MINIMUM = -3;
    const int SHUFFLE_MAXIMUM =  3;

    public:
        ShufflePatternEditorControl(const char *label, ShufflePatternWrapper *shuffwrapper) : MenuItem(label) {
            this->shufflewrapper = shuffwrapper;
        }

        virtual int renderValue(bool selected, bool opened, uint16_t max_character_width) override {            //return pos.y;
            Coord pos(tft->getCursorX(), tft->getCursorY());
            int initial_x = pos.x;
            int initial_y = pos.y;
            //pos.y = header(label, pos, selected, opened);
            //tft->println("?");

            this->colours(selected || opened, opened ? GREEN : this->default_fg, this->default_bg);

            tft->setTextSize(2);

            this->colours(false);

            for (int i = 0 ; i < shufflewrapper->size ; i++) {
                int row = i+1 >= (shufflewrapper->size / 2) ? 1 : 0;
                int column = i % (shufflewrapper->size / 2);

                if (column == 0 && row == 1) {
                    tft->setCursor(initial_x, initial_y + tft->getRowHeight() - 3);
                } 

                this->colours(false);

                if (column == STEPS_PER_BEAT) {
                    tft->print(" ");
                } 
               
                if (i==selected_step) 
                    this->colours(selected || opened, editing ? GREEN : this->default_fg, this->default_bg);
                else
                    this->colours(false);

                tft->printf("%-2i", shufflewrapper->get_step(i));
                //if (i+1 == shufflewrapper->size / 2)
                //    tft->println();
            }

            tft->println();

            return tft->getCursorY();
        }

        virtual bool button_right() override {
            //tft->printf("button_select on ShufflePatternEditorControl\n");
            //tft->println();
            //shuffwrapper->set_active(!shuffwrapper->is_active());
            if (editing) {
                shufflewrapper->set_step(selected_step, 0);
                selected_step++;
                if (selected_step >= shufflewrapper->size) {
                    selected_step = 0;
                }
            } else {
                for (int i = 0 ; i < shufflewrapper->size ; i++) {
                    shufflewrapper->set_step(i, random(SHUFFLE_MINIMUM, SHUFFLE_MAXIMUM));
                }
            }
            shufflewrapper->update_target();

            return true;
        }

        virtual bool button_select() override {
            //tft->printf("button_select on ShufflePatternEditorControl\n");
            //tft->println();
            if (editing) {
                editing = false;
                shufflewrapper->update_target();
            } else {
                editing = true;
            }
            return false;
        }

        virtual bool button_back() override {
            //tft->printf("button_back on ShufflePatternEditorControl\n");
            //tft->println();
            if (editing) {
                editing = false;
                shufflewrapper->update_target();
            } else {
                return false;
            }
            return true;
        }

        virtual bool knob_left() override {
            if (!editing) {
                selected_step = (selected_step - 1) % shufflewrapper->size;
                if (selected_step < 0) {
                    selected_step = shufflewrapper->size - 1;
                }
            } else {
                int v = shufflewrapper->get_step(selected_step) - 1;
                if (v < SHUFFLE_MINIMUM) v = SHUFFLE_MINIMUM;
                shufflewrapper->set_step(selected_step, v);

                shufflewrapper->update_target();
            }
            return true;
        }

        virtual bool knob_right() override {
            if (!editing) {
                selected_step = (selected_step + 1) % shufflewrapper->size;
            } else {
                int v = shufflewrapper->get_step(selected_step) + 1;
                if (v > SHUFFLE_MAXIMUM) v = SHUFFLE_MAXIMUM;
                shufflewrapper->set_step(selected_step, v);

                shufflewrapper->update_target();
            }
            return true;
        }

};