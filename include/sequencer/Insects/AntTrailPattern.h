// Ant Trail Probability Grid (Evolving Rhythm)

#pragma once

#ifndef FLASHMEM
    #define FLASHMEM
#endif

#include "debug.h"
#include <LinkedList.h>
#include "../Base/Patterns.h"
#include "../Base/Sequencer.h"
#include <bpm.h>

#define SEQUENCE_LENGTH_STEPS 16
//const int LEN = SEQUENCE_LENGTH_STEPS;

class AntTrailPattern : public SimplePattern {
    public:

    float reinforce_factor = 0.02;
    float evaporation_factor = 0.005;
    float probability_grid[SEQUENCE_LENGTH_STEPS];

    AntTrailPattern(LinkedList<BaseOutput*> *available_outputs) : SimplePattern(available_outputs) {
        for (int i = 0 ; i < SEQUENCE_LENGTH_STEPS ; i++) {
            probability_grid[i] = 0.5f;
        }
    }

    virtual const char *get_summary() override {
        return "Ant Trail";
    }

    virtual void recalculate_pattern() {
        Serial.printf("Recalculating AntTrailPattern..\n", ticks);

        for (int i = 0 ; i < this->get_steps() ; i++) {
            float r = random(10000) / 10000.0f;
            if (r < probability_grid[i]) {
                // trigger and reinforce
                probability_grid[i] += reinforce_factor;
                this->set_event_for_tick(i * ticks_per_step, 60, 127, 0);
            } else {
                // evaporate
                probability_grid[i] -= evaporation_factor;
                this->unset_event_for_tick(i * ticks_per_step);
            }
            probability_grid[i] = constrain(probability_grid[i], evaporation_factor, 1.0f);
        }
    }

    virtual void process_step(int step) override {
        // recalculate the pattern on the first step
        if (step % get_effective_steps() == 0)
            this->recalculate_pattern();

        SimplePattern::process_step(step);
    }
};