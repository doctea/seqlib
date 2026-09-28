#pragma once

#ifdef ENABLE_SHUFFLE
    #include "uClock.h"

    #include "functional-vlpp.h"
    #include "GenericList.h"
    
    #ifdef ENABLE_PARAMETERS
        #include "parameter_list.h"
        #ifdef ENABLE_STORAGE
            #include "parameters/Parameter.h"
        #endif
    #endif

    #ifdef ENABLE_STORAGE
        #include "saveload_settings.h"
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

    #ifdef ENABLE_STORAGE
        // Stores the size byte followed by signed step bytes, encoded as hex.
        class ShuffleTemplateSetting : public SaveableSettingBase {
            ShufflePatternWrapper *pattern;
            uint8_t count;

            public:
                ShuffleTemplateSetting(const char *label, const char *category, ShufflePatternWrapper *pattern, uint8_t count)
                    : pattern(pattern), count(count) {
                    set_label(label);
                    set_category(category);
                }

                const char *get_line() override {
                    int position = snprintf(linebuf, SL_MAX_LINE, "%s=%02x", label, (unsigned)(uint8_t)pattern->size);
                    for (uint8_t step_index = 0; step_index < count && position < SL_MAX_LINE - 2; step_index++) {
                        uint8_t encoded_step = (uint8_t)pattern->step[step_index];
                        position += snprintf(linebuf + position, SL_MAX_LINE - position, "%02x", (unsigned)encoded_step);
                    }
                    linebuf[position] = '\0';
                    return linebuf;
                }

                bool parse_key_value(const char *key, const char *value) override {
                    if (strcmp(key, label) != 0 || value == nullptr || strlen(value) != ((size_t)count + 1) * 2) {
                        return false;
                    }

                    char size_text[3] = { value[0], value[1], '\0' };
                    char *size_end = nullptr;
                    long parsed_size = strtol(size_text, &size_end, 16);
                    if (size_end != size_text + 2 || parsed_size < 1 || parsed_size > count) {
                        return false;
                    }

                    int8_t parsed_steps[MAX_SHUFFLE_TEMPLATE_SIZE];
                    for (uint8_t step_index = 0; step_index < count; step_index++) {
                        size_t offset = ((size_t)step_index + 1) * 2;
                        char byte_text[3] = { value[offset], value[offset + 1], '\0' };
                        char *end = nullptr;
                        long encoded_step = strtol(byte_text, &end, 16);
                        if (end != byte_text + 2 || encoded_step < 0 || encoded_step > 255) {
                            return false;
                        }
                        parsed_steps[step_index] = (int8_t)(encoded_step < 128 ? encoded_step : encoded_step - 256);
                    }

                    pattern->size = (int8_t)parsed_size;
                    memcpy(pattern->step, parsed_steps, count * sizeof(int8_t));
                    return true;
                }

                size_t heap_size() const override { return sizeof(ShuffleTemplateSetting); }
        };
    #endif
    
    class ShufflePatternWrapperManager
    #ifdef ENABLE_STORAGE
        : public SHDynamic<0, NUMBER_SHUFFLE_PATTERNS>
    #endif  
    {
        public:

        typedef ShufflePatternWrapper* ShufflePatternWrapperPtr;
        ShufflePatternWrapperPtr *shuffle_patterns = nullptr;

        size_t number_shuffle_wrappers = 0;
        size_t getCount() {
            return number_shuffle_wrappers;
        }

        ShufflePatternWrapperManager(size_t number_shuffle_wrappers) {
            this->number_shuffle_wrappers = number_shuffle_wrappers;

            #ifdef ENABLE_STORAGE
                this->set_path_segment("shuffle_patterns");
            #endif

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

        #ifdef ENABLE_STORAGE
            virtual void setup_saveable_settings() override {
                const sl_scope_t save_scope = SL_SCOPE_SCENE | SL_SCOPE_PROJECT;
                char setting_label[SL_MAX_LABEL];
                for (size_t pattern_index = 0; pattern_index < number_shuffle_wrappers; pattern_index++) {
                    ShufflePatternWrapper *pattern = shuffle_patterns[pattern_index];

                    snprintf(setting_label, sizeof(setting_label), "pattern_%u_template", (unsigned)pattern_index);
                    register_setting(
                        new ShuffleTemplateSetting(setting_label, "Shuffle", pattern, MAX_SHUFFLE_TEMPLATE_SIZE), 
                        save_scope
                    );
                }

                #ifdef ENABLE_PARAMETERS
                    ParameterList *params = this->getParameters();
                    if (params != nullptr) {
                        for (auto* p : *params) {
                            register_child(p);
                        }
                    }
                #endif
            }

            virtual void on_after_load() override {
                for (size_t pattern_index = 0; pattern_index < number_shuffle_wrappers; pattern_index++) {
                    shuffle_patterns[pattern_index]->update_target(true);
                }
            }
        #endif
    };
    extern ShufflePatternWrapperManager shuffle_pattern_wrapper;

    #ifdef ENABLE_SCREEN
        void setup_menu_shuffle();
    #endif

#endif