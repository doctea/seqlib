#pragma once

#include "sequencer/Base/Patterns.h"
#include "menuitems.h"

#include <bpm.h>
#include <clock.h>

class SingleCircleDisplay : public MenuItem {
    public:

        bool recalculate_needed = true; // indicate whether we need to recalculate the coordinates for the steps; this is set to true when we change the pattern or its settings, and we recalculate in display() if it's true, then set it back to false
        int_fast8_t last_steps = -1; // used to track whether the number of steps has changed since the last time we calculated coordinates, so we know whether we need to recalculate

        BasePattern *target_pattern = nullptr;
        int_fast16_t coordinates_x[TIME_SIG_MAX_STEPS_PER_BAR];  // we generate these in setup_coordinates() and then reuse them in display() to save time on trig calculations
        int_fast16_t coordinates_y[TIME_SIG_MAX_STEPS_PER_BAR];  // 
        SingleCircleDisplay(const char *label, BasePattern *target_pattern, bool show_header = false) : MenuItem(label, false, show_header) {
            this->set_target(target_pattern);
        }

        void on_add() override {
            MenuItem::on_add();
            setup_coordinates();
        }

        void set_target(BasePattern *target_pattern) {
            this->target_pattern = target_pattern;
            this->recalculate_needed = true;
        }

        float circle_size = 20.0;
        void setup_coordinates() {
            Debug_printf("SingleCircleDisplay() setup_coordinates, tft width is %i\n", tft->width()); Serial.flush();
            //this->set_pattern(target_pattern);
            const size_t divisions = STEPS_PER_BAR;
            const float degrees_per_iter = 360.0 / divisions;
            circle_size = 20.0*(tft->width()/2);
            int position = STEPS_PER_BAR / STEPS_PER_BEAT;
            for (unsigned int i = 0 ; i < divisions; i++) {
                Debug_printf("generating coordinate for position %i:\trad(cos()) is %f\n", i, radians(cos(i*degrees_per_iter*PI/180)));
                Debug_printf("generating coordinate for position %i:\trad(sin()) is %f\n", i, radians(sin(i*degrees_per_iter*PI/180)));
                coordinates_x[position] = (int)((float)circle_size * radians(cos(((float)i)*degrees_per_iter*PI/180.0)));
                coordinates_y[position] = (int)((float)circle_size * radians(sin(((float)i)*degrees_per_iter*PI/180.0)));
                Debug_printf("generating coordinate for position %i:\t[%i,%i]\n---\n", i, coordinates_x[i], coordinates_y[i]); Serial.flush();
                position++;
                position = position % divisions;
            }
            recalculate_needed = false;
            //this->set_sequencer(sequencer);
            Debug_println("SingleCircleDisplay() finished setup_coordinates"); Serial.flush();
        }

        uint_fast16_t circle_center_y;  // calculated during first run and when time signature changes
        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);

            //tft->printf("ticks:%4i step:%i\n", ticks, BPM_CURRENT_STEP_OF_BAR);

            static const uint_fast16_t tft_width_quartered = tft->width()/4;
            const uint_fast16_t circle_center_x = tft_width_quartered;

            if (recalculate_needed || this->last_steps != STEPS_PER_BAR) {
                Debug_printf("SingleCircleDisplay() detected change in steps from %i to %i, recalculating coordinates\n", this->last_steps, STEPS_PER_BAR); Serial.flush();
                setup_coordinates();
                this->last_steps = STEPS_PER_BAR;
                circle_center_y = 6 + pos.y + coordinates_y[STEPS_PER_BAR/2];
            }

            if (this->target_pattern==nullptr) {
                tft->println("No pattern selected"); Serial.flush();
                return tft->getCursorY();
            }

            //tft->drawLine(0, pos.y, tft->width(), pos.y, random(0, 65535));

            tft->setCursor(pos.x, pos.y);

            // draw circle
            int_fast8_t first_x, first_y;
            int_fast8_t last_x, last_y;
            int_fast8_t count = 0;
            //BasePattern *pattern = target_sequencer->get_pattern(seq);
            //int16_t colour = color565(255 * seq, 255 - (255 * seq), seq) + (seq*8);
            uint_fast16_t pattern_colour = target_pattern->get_colour();
            bool is_step_on = target_pattern->query_note_on_for_step(BPM_CURRENT_STEP_OF_PHRASE);
            if (!is_step_on) pattern_colour = tft->halfbright_565(pattern_colour);
            // todo: if STEPS_PER_PHRASE is a multiple of get_steps, should be able to limit number of loops we do here?
            for (int i = 0 ; i < STEPS_PER_PHRASE/*max(target_pattern->get_steps(),16*/ ; i++) {
                const uint_fast16_t coord_x = circle_center_x + coordinates_x[i%STEPS_PER_BAR];
                const uint_fast16_t coord_y = circle_center_y + coordinates_y[i%STEPS_PER_BAR];
                if (target_pattern->query_note_on_for_step(i)) {
                    if (count>0) {
                        tft->drawLine(
                            last_x, last_y, coord_x, coord_y,
                            pattern_colour
                        );
                    } else {
                        first_x = coord_x;
                        first_y = coord_y;
                    }
                    last_x = circle_center_x + coordinates_x[i%STEPS_PER_BAR];
                    last_y = circle_center_y + coordinates_y[i%STEPS_PER_BAR];

                    tft->fillCircle(last_x, last_y, 6, (is_step_on ? target_pattern->get_colour() : pattern_colour));  // draw a slightly larger colour circle
                    count++;
                }
            }
            if (count>1) {
                tft->drawLine(last_x, last_y, first_x, first_y, pattern_colour);
            }

            // draw step markers around circle
            const int_fast8_t radius = 2;
            for (uint_fast8_t i = 0 ; i < STEPS_PER_BAR ; i++) {
                const uint_fast16_t colour = BPM_CURRENT_STEP_OF_BAR == i ? RED : (is_step_on ? target_pattern->get_colour() : pattern_colour);
                tft->fillCircle(
                    circle_center_x + coordinates_x[i], 
                    circle_center_y + coordinates_y[i], 
                    radius, 
                    colour
                );
            }

            return tft->height();
        }
};
