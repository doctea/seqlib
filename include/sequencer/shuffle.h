#pragma once

#ifdef ENABLE_SHUFFLE
    #include "uClock.h"
    #define NUMBER_SHUFFLE_PATTERNS 16

    class ShufflePatternWrapper {
        public:
            int8_t track_number = 0;
            int8_t step[MAX_SHUFFLE_TEMPLATE_SIZE] = {0};
            int8_t size = 16; //MAX_SHUFFLE_TEMPLATE_SIZE;
            float amount = 1.0f;
    
            int8_t last_sent_step[MAX_SHUFFLE_TEMPLATE_SIZE] = {0};
    
            ShufflePatternWrapper(int8_t track_number) {
                this->track_number = track_number;
                for (int i = 0 ; i < MAX_SHUFFLE_TEMPLATE_SIZE ; i++) {
                    set_step(i, 0);
                }
                update_target(true);
            }
    
            void set_active(bool active) {
                uClock.setTrackShuffle(this->track_number, active);
            }
            bool is_active() {
                return uClock.isTrackShuffled(this->track_number);
            }
    
            void set_steps(int8_t *steps, int8_t size) {
                for (int i = 0 ; i < size ; i++) {
                    set_step(i, steps[i]);
                }
                update_target();
            }
            void set_step(int8_t step_number, int8_t value) {
                if (step_number >= 0 && step_number < MAX_SHUFFLE_TEMPLATE_SIZE) {
                    step[step_number] = value;
                }
            }
            void set_step_and_update(int8_t step_number, int8_t value) {
                set_step(step_number, value);
                update_target();
            }
    
            int8_t get_step(int8_t step_number) {
                if (step_number >= 0 && step_number < MAX_SHUFFLE_TEMPLATE_SIZE) {
                    return step[step_number];
                }
                return 0;
            }
            void set_track_number(int8_t track_number) {
                this->track_number = track_number;
            }
    
            void set_amount(float amount) {
                this->amount = amount;
                if (this->amount > 0.0f)
                    this->set_active(true);
                else
                    this->set_active(false);
            }
            float get_amount() {
                return amount;
            }
    
            void update_target(bool force = false) {
                uClock.setTrackShuffleSize(this->track_number, this->size);
                uClock.setTrackShuffle(this->track_number, this->amount > 0.0f);
                for (int i = 0 ; i < size ; i++) {
                    int t = (float)step[i] * this->amount;
                    if (force || t!=last_sent_step[i]) {
                        uClock.setTrackShuffleData(this->track_number, i, t);
                        last_sent_step[i] = t;
                    }                
                }
            }
    };
    
    extern ShufflePatternWrapper *shuffle_pattern_wrapper[NUMBER_SHUFFLE_PATTERNS];
    
#endif