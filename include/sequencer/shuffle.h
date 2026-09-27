#pragma once

#ifdef ENABLE_SHUFFLE
    #include "uClock.h"

    #include "functional-vlpp.h"
    #include "GenericList.h"
    
    #ifdef ENABLE_PARAMETERS
        #include "parameter_list.h"
    #endif

    #ifndef NUMBER_SHUFFLE_PATTERNS
        #define NUMBER_SHUFFLE_PATTERNS 1
    #endif

    void shuffled_callback(uint32_t step, uint8_t track_number);

    typedef vl::Func<void(uint32_t, uint8_t)> shuffle_callback_def_t;

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
    
            void set_steps(const int8_t *steps, int8_t size) {
                if (steps == nullptr)
                    return;
                this->size = size < 1 ? 1 :
                    (size > MAX_SHUFFLE_TEMPLATE_SIZE ? MAX_SHUFFLE_TEMPLATE_SIZE : size);
                for (int i = 0 ; i < this->size ; i++) {
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
                    return step[step_number % size];
                }
                return 0;
            }
            void set_track_number(int8_t track_number) {
                this->track_number = track_number;
            }
    
            void set_amount(float amount) {
                this->amount = amount;
                if (this->amount < -0.01f || this->amount > 0.01f)
                    this->set_active(true);
                else
                    this->set_active(false);
            }
            float get_amount() {
                return amount;
            }
    
            int8_t last_sent_size = -1;
            void update_target(bool force = false) {
                size = size < 1 ? 1 :
                    (size > MAX_SHUFFLE_TEMPLATE_SIZE ? MAX_SHUFFLE_TEMPLATE_SIZE : size);
                bool changed = force || last_sent_size != size;
                int8_t scaled_steps[MAX_SHUFFLE_TEMPLATE_SIZE] = {0};
                int i_amount = (int)(this->amount * 1000.0f);
                for (int i = 0 ; i < size ; i++) {
                    int t = (int)(step[i] * i_amount) / 1000;
                    scaled_steps[i] = t;
                    if (t != last_sent_step[i])
                        changed = true;
                }
                if (changed) {
                    uClock.setShuffleTemplate(scaled_steps, this->size, this->track_number);
                    for (int i = 0; i < size; i++)
                        last_sent_step[i] = scaled_steps[i];
                    this->last_sent_size = size;
                }
                this->set_active(this->amount < -0.01f || this->amount > 0.01f);
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

            ShufflePatternWrapperManager(size_t number_shuffle_wrappers) {
                this->number_shuffle_wrappers = number_shuffle_wrappers;

                this->shuffle_patterns = new ShufflePatternWrapperPtr[number_shuffle_wrappers];
                for (size_t i = 0 ; i < number_shuffle_wrappers ; i++) {
                    shuffle_patterns[i] = new ShufflePatternWrapper(i);
                }

                // register our global shuffle_callback with uClock so that we get told about shuffle events
                // we then dispatch these events to the registered shuffle callbacks
                uClock.setOnStep(::shuffled_callback, this->number_shuffle_wrappers);
            }

            ~ShufflePatternWrapperManager() {
                for (size_t i = 0 ; i < number_shuffle_wrappers ; i++) {
                    delete shuffle_patterns[i];
                }
                delete[] shuffle_patterns;
                for (size_t i = 0; i < shuffle_callbacks.size(); i++) {
                    delete shuffle_callbacks.get(i);
                }
                #ifdef ENABLE_PARAMETERS
                    delete parameters;
                #endif
            }

            ShufflePatternWrapper* operator[](size_t index) {
                if (index >= number_shuffle_wrappers) {
                    return nullptr;
                }
                return shuffle_patterns[index];
            }

            // list of callbacks to notify when a shuffle callback occurs
            GenericList<shuffle_callback_def_t*> shuffle_callbacks;

            void register_shuffle_callback(shuffle_callback_def_t cb) {
                shuffle_callbacks.add(new shuffle_callback_def_t(cb));
            }

            void shuffled_callback(uint32_t step, uint8_t track_number) {
                for (size_t i = 0; i < shuffle_callbacks.size(); i++) {
                    (*shuffle_callbacks.get(i))(step, track_number);
                }
            }

            #if defined(ENABLE_PARAMETERS)
                ParameterList *parameters = nullptr;
                ParameterList* getParameters();
            #endif
    };
    extern ShufflePatternWrapperManager shuffle_pattern_wrapper;

    #ifdef ENABLE_SCREEN
        void setup_menu_shuffle();
    #endif

#endif