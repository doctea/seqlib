#pragma once

#include "sequencer/Base/Patterns.h"
#include "menuitems.h"

#include <bpm.h>
#include <clock.h>

#ifdef ENABLE_SHUFFLE
    #include "sequencer/shuffle.h"
#endif

class SingleCircleDisplay : public MenuItem {
    public:

        bool recalculate_needed = true; // indicate whether we need to recalculate the coordinates for the steps; this is set to true when we change the pattern or its settings, and we recalculate in display() if it's true, then set it back to false
        int_fast8_t last_steps = -1; // used to track whether the number of steps has changed since the last time we calculated coordinates, so we know whether we need to recalculate

        uint32_t shuffle_last_changed_at = 0;
        bool last_is_shuffled = false;
        uint8_t last_shuffle_track = 0;
        uint16_t last_shuffle_output_ppqn = 0;

        SimplePattern *target_pattern = nullptr;
        int_fast16_t coordinates_x[TIME_SIG_MAX_STEPS_PER_BAR];  // we generate these in setup_coordinates() and then reuse them in display() to save time on trig calculations
        int_fast16_t coordinates_y[TIME_SIG_MAX_STEPS_PER_BAR];  // 
        SingleCircleDisplay(const char *label, SimplePattern *target_pattern, bool show_header = false) : MenuItem(label, false, show_header) {
            this->set_target(target_pattern);
            // TODO: add a custom redraw policy so that we can redraw this only when the pattern has changed
            IF_MENU_PERF_PARTIAL_UPDATES(add_redraw_policy(REDRAW_ON_BPM_CLOCK_CHANGE | REDRAW_ON_TICK);)
        }

        void on_add() override {
            MenuItem::on_add();
            this->available_height = tft->width()/2;
            setup_coordinates();
            recalculate_needed = true;   // recalculate coordinates on first display, in case the layout has changed 
        }

        void set_target(SimplePattern *target_pattern) {
            this->target_pattern = target_pattern;
            this->recalculate_needed = true;
        }

        // TODO: something so that we can ensure the circle display only uses the actual available vertical space
        int available_height = 0, last_available_height = 0;   // used to tell the circle display how much vertical space it has to work with, so it can scale the circle size accordingly
        float circle_size = 20.0;
        void setup_coordinates() {
            const size_t divisions = STEPS_PER_BAR;
            const float degrees_per_step = 360.0 / divisions;
            #ifdef ENABLE_SHUFFLE
                const float degrees_per_tick = degrees_per_step * 4.0 / uClock.getOutputPPQN();
            #endif
            circle_size = 20.0 * this->available_height * PI / 180.0;
            int position = STEPS_PER_BAR / STEPS_PER_BEAT;
            for (unsigned int i = 0 ; i < divisions; i++) {
                float shuffled_degrees = i * degrees_per_step;
                #ifdef ENABLE_SHUFFLE
                    if (this->target_pattern != nullptr && this->target_pattern->is_shuffled()) {
                        shuffled_degrees += degrees_per_tick * this->target_pattern->get_shuffle_offset(position);
                    }
                #endif
                const float angle_radians = shuffled_degrees * PI / 180.0;
                coordinates_x[position] = (int)(circle_size * cos(angle_radians));
                coordinates_y[position] = (int)(circle_size * sin(angle_radians));
                position++;
                position = position % divisions;
            }
            recalculate_needed = false;
            this->last_available_height = this->available_height;
        }

        uint_fast16_t circle_center_y;  // calculated during first run and when time signature changes
        virtual int display(Coord pos, bool selected, bool opened) override {
            pos.y = header(label, pos, selected, opened);

            if (this->target_pattern == nullptr) {
                tft->println("No pattern selected");
                return tft->getCursorY();
            }

            //tft->printf("ticks:%4i step:%i\n", ticks, BPM_CURRENT_STEP_OF_BAR);

            static const uint_fast16_t tft_width_quartered = tft->width()/4;
            const uint_fast16_t circle_center_x = tft_width_quartered;

            #ifdef ENABLE_SHUFFLE
                const bool is_shuffled = this->target_pattern->is_shuffled();
                const uint8_t shuffle_track = this->target_pattern->get_shuffle_track();
                const uint32_t changed_at = shuffle_pattern_wrapper.get_shuffle_last_changed_at(shuffle_track);
                const uint16_t shuffle_output_ppqn = uClock.getOutputPPQN();
                if (this->last_is_shuffled != is_shuffled || this->last_shuffle_track != shuffle_track || this->shuffle_last_changed_at != changed_at || this->last_shuffle_output_ppqn != shuffle_output_ppqn) {
                    recalculate_needed = true;
                }
                this->last_is_shuffled = is_shuffled;
                this->last_shuffle_track = shuffle_track;
                this->shuffle_last_changed_at = changed_at;
                this->last_shuffle_output_ppqn = shuffle_output_ppqn;
            #endif

            if (recalculate_needed || this->last_steps != STEPS_PER_BAR || this->last_available_height != this->available_height) {
                setup_coordinates();
                this->last_steps = STEPS_PER_BAR;
            }
            circle_center_y = 6 + pos.y + circle_size;

            //tft->drawLine(0, pos.y, tft->width(), pos.y, random(0, 65535));

            tft->setCursor(pos.x, pos.y);

            // draw circle
            int_fast16_t first_x, first_y;
            int_fast16_t last_x = -1, last_y = -1;
            int_fast8_t count = 0;
            //BasePattern *pattern = target_sequencer->get_pattern(seq);
            //int16_t colour = color565(255 * seq, 255 - (255 * seq), seq) + (seq*8);
            uint_fast16_t pattern_colour = target_pattern->get_colour();
            bool is_step_on = target_pattern->query_note_on_for_step(BPM_CURRENT_STEP_OF_PHRASE);
            if (!is_step_on) pattern_colour = tft->halfbright_565(pattern_colour);

            uint16_t max_height_reached = circle_center_y + circle_size + 6;  // start with the lowest possible point on the circle, plus the radius of the circles we're about to draw for each step

            // todo: if STEPS_PER_PHRASE is a multiple of get_steps, should be able to limit number of loops we do here?
            for (int i = 0 ; i < STEPS_PER_PHRASE/*max(target_pattern->get_steps(),16*/ ; i++) {
                const int_fast16_t coord_x = circle_center_x + coordinates_x[i%STEPS_PER_BAR];
                const int_fast16_t coord_y = circle_center_y + coordinates_y[i%STEPS_PER_BAR];
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
            } else if (count==1) {
                // only one event - draw it as a filled circle so it can be seen
                tft->fillCircle(last_x, last_y, 5, pattern_colour);
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

            return max_height_reached;
        }
};
