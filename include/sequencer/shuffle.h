#pragma once

#ifdef ENABLE_SHUFFLE
    #include "uClock.h"

    #ifndef NUMBER_SHUFFLE_PATTERNS
        #define NUMBER_SHUFFLE_PATTERNS 1
    #endif
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
                uClock.setShuffle(active, this->track_number);
            }
            bool is_active() {
                return uClock.isShuffled(this->track_number);
            }
    
            void set_steps(int8_t *steps, int8_t size) {
                this->size = size;
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
                if (this->amount < 0.0f || this->amount > 0.0f)
                    this->set_active(true);
                else
                    this->set_active(false);
            }
            float get_amount() {
                return amount;
            }
    
            int8_t last_sent_size = -1;
            void update_target(bool force = false) {
                if (force || last_sent_size != size) {
                    uClock.setShuffleSize(this->size, this->track_number);
                    this->last_sent_size = size;
                }
                uClock.setShuffle(this->amount < 0.01f || this->amount > 0.01f, this->track_number);
                for (int i = 0 ; i < size ; i++) {
                    int t = (float)step[i] * this->amount;
                    if (force || t!=last_sent_step[i]) {
                        //if (Serial) Serial.printf("setTrackShuffleData(%i, %i, %i)\n", track_number, i, t);
                        uClock.setShuffleData(i, t, this->track_number);
                        last_sent_step[i] = t;
                    }                
                }
            }
    };
    
    class ShufflePatternWrapperManager {
        public:
            typedef ShufflePatternWrapper* ShufflePatternWrapperPtr;
            ShufflePatternWrapperPtr *shuffle_patterns = nullptr;

            size_t number_shuffle_wrappers = 0;
            size_t getCount() {
                return number_shuffle_wrappers;
            }

            ShufflePatternWrapperManager(int number_shuffle_wrappers) {
                this->number_shuffle_wrappers = number_shuffle_wrappers;

                this->shuffle_patterns = new ShufflePatternWrapperPtr[number_shuffle_wrappers];
                for (int i = 0 ; i < number_shuffle_wrappers ; i++) {
                    shuffle_patterns[i] = new ShufflePatternWrapper(i);
                }
            }

            ~ShufflePatternWrapperManager() {
                for (int i = 0 ; i < number_shuffle_wrappers ; i++) {
                    delete shuffle_patterns[i];
                }
            }

            ShufflePatternWrapper* operator[](size_t index) {
                if (index < 0 || index >= number_shuffle_wrappers) {
                    return nullptr;
                }
                return shuffle_patterns[index];
            }
    };
    extern ShufflePatternWrapperManager shuffle_pattern_wrapper;
    
#endif