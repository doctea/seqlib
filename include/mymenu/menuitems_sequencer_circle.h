#pragma once

#include "sequencer/Sequencer.h"
#include "sequencer/Patterns.h"

#include "menuitems.h"

//#include "submenuitem_bar.h"
//#include "menuitems_object_selector.h"
//#define NUM_STEPS 16

#include <bpm.h>
#include <clock.h>

class CircleDisplay : public MenuItem {
    public:
        //DeviceBehaviour_Beatstep *behaviour_beatstep = nullptr;
        BaseSequencer *target_sequencer = nullptr;
        int coordinates_x[STEPS_PER_BAR];
        int coordinates_y[STEPS_PER_BAR];

        float dia = 20.0;

        CircleDisplay(const char *label, BaseSequencer *sequencer, bool show_header = false) : MenuItem(label, false, show_header) {
            this->set_sequencer(sequencer);
        }

        void on_add() override {
            MenuItem::on_add();
            setup_coordinates();
        }

        void set_sequencer(BaseSequencer *sequencer) {
            this->target_sequencer = sequencer;
        }

        void setup_coordinates() {
            Debug_printf("CircleDisplay() setup_coordinates, tft width is %i\n", tft->width()); Serial.flush();
            //this->set_pattern(target_pattern);
            const size_t divisions = STEPS_PER_BAR;
            const float degrees_per_iter = 360.0 / divisions;
            dia = 20.0*(tft->width()/2.0);
            unsigned int position = STEPS_PER_BAR / STEPS_PER_BEAT;
            for (unsigned int i = 0 ; i < divisions; i++) {
                Debug_printf("generating coordinate for position %i:\trad(cos()) is %f\n", i, radians(cos(i*degrees_per_iter*PI/180)));
                Debug_printf("generating coordinate for position %i:\trad(sin()) is %f\n", i, radians(sin(i*degrees_per_iter*PI/180)));
                coordinates_x[position] = (int)((float)dia * radians(cos(((float)i)*degrees_per_iter*PI/180.0)));
                coordinates_y[position] = (int)((float)dia * radians(sin(((float)i)*degrees_per_iter*PI/180.0)));
                Debug_printf("generating coordinate for position %i:\t[%i,%i]\n---\n", i, coordinates_x[i], coordinates_y[i]); Serial.flush();
                position++;
                position = position % divisions;
            }
            //this->set_sequencer(sequencer);
            Debug_println("CircleDisplay() finished setup_coordinates"); Serial.flush();
        }

        virtual int display(Coord pos, bool selected, bool opened) override {
            //return pos.y;
            int initial_y = pos.y;
            pos.y = header(label, pos, selected, opened);

            //tft->printf("ticks:%4i step:%2i\n", ticks, BPM_CURRENT_STEP_OF_BAR);
            
            /*static int last_rendered_step = -1;
            if (BPM_CURRENT_STEP_OF_BAR==last_rendered_step)
                return tft->height();
            last_rendered_step = BPM_CURRENT_STEP_OF_BAR;*/

            if (this->target_sequencer==nullptr) {
                tft->println("No sequencer selected"); Serial.flush();
                return tft->getCursorY();
            }

            tft->setCursor(pos.x, pos.y);

            /*static const uint_fast8_t circle_center_x = tft->width()/4;
            static const uint_fast8_t circle_center_offset = tft->width()/4;
            const uint_fast8_t circle_center_y = pos.y + circle_center_offset;*/
            static const uint_fast16_t tft_width_quartered = tft->width()/4;
            static const uint_fast16_t tft_height_quartered = tft->height()/4;
            const uint_fast16_t circle_center_x = tft_width_quartered;
            const uint_fast16_t circle_center_y = 6 + pos.y + coordinates_y[STEPS_PER_BAR/2];

            // draw circle
            for (uint_fast8_t seq = 0 ; seq < target_sequencer->number_patterns ; seq++) {
                uint_fast8_t first_x, first_y;
                uint_fast8_t last_x, last_y;
                uint_fast8_t count = 0;
                BasePattern *pattern = target_sequencer->get_pattern(seq);
                //int16_t colour = color565(255 * seq, 255 - (255 * seq), seq) + (seq*8);
                uint16_t colour = pattern->get_colour();
                if (!pattern->query_note_on_for_step(BPM_CURRENT_STEP_OF_BAR))
                    colour = tft->halfbright_565(colour);
                for (int i = 0 ; i < STEPS_PER_BAR ; i++) {
                    uint_fast8_t coord_x = circle_center_x + coordinates_x[i];
                    uint_fast8_t coord_y = circle_center_y + coordinates_y[i];
                    if (pattern->query_note_on_for_step(i)) {
                        if (count>0) {
                            tft->drawLine(
                                last_x, last_y, coord_x, coord_y,
                                colour
                            );
                        } else {
                            first_x = coord_x;
                            first_y = coord_y;
                        }
                        last_x = circle_center_x + coordinates_x[i];
                        last_y = circle_center_y + coordinates_y[i];
                        count++;
                    }
                }
                if (count>1) {
                    tft->drawLine(last_x, last_y, first_x, first_y, colour);
                }
            }

            // draw step markers around circle
            const uint_fast8_t radius = 2;
            for (uint_fast8_t i = 0 ; i < STEPS_PER_BAR ; i++) {
                uint16_t colour = BPM_CURRENT_STEP_OF_BAR == i ? RED : BLUE;
                tft->fillCircle(
                    circle_center_x + coordinates_x[i], 
                    circle_center_y + coordinates_y[i], 
                    radius, 
                    colour
                );
            }

            // draw flashy blocks for every patterns
            tft->setCursor((tft->width()/2)-8, ++initial_y);
            colours(false, C_WHITE);
            //tft->println(" St Pu Ro   St Pu Ro");
            tft->setCursor((tft->width()/2)-8, pos.y);
            static const uint_fast16_t middle_x = ((tft->width()/2)-8);
            static const uint_fast8_t column_width = (8*8)+1;
            for (uint_fast8_t seq = 0 ; seq < target_sequencer->number_patterns ; seq++) {
                uint_fast8_t column = seq / 10;
                uint_fast8_t row = 1+(seq % 10);
                tft->setCursor(middle_x + (column*column_width), initial_y + (row*8)); //tft->getCursorY());
                //tft->setCursor((tft->width()/2) + (seq/10), tft->getCursorY());
                BasePattern *pattern = target_sequencer->get_pattern(seq);
                colours(
                    pattern->query_note_on_for_step(BPM_CURRENT_STEP_OF_BAR), 
                    pattern->get_colour(),
                    //tft->dim_565(pattern->colour, min(0,(PPQN/(1+BPM_CURRENT_TICK_OF_BEAT))-1)),
                    //tft->dim_565(pattern->colour, max(0,2-(BPM_CURRENT_TICK_OF_BEAT/8))),
                    BLACK
                );
                tft->print(" ");
                //colours(false, C_WHITE);
                //tft->printf("%i %s\n", seq, pattern->get_summary());
                
                //tft->print(pattern->get_summary());
                char label[10];
                snprintf(label, 10, "%9s", (char*)pattern->get_output_label());
                tft->print(label);
                //tft->printf("%8s", (char*)pattern->get_output_label());
            }

            //tft->printf("dia = %3.3f, cursorY=%i\n\n", dia, tft->getCursorY());
            tft->setCursor(0, tft->getCursorY() + 16);

            return tft->getCursorY(); //initial_y + (size/2.0); //40; //tft->height();
        }
};

