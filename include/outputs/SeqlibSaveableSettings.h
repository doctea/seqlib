// SaveableParameter implementations for handling special things like Pattern outputs which aren't just simple variables

#pragma once

#include <Arduino.h>

#include "outputs/base_outputs.h"
#include "outputs/output.h"

#include "saveloadlib.h"

/********* example of how to use this for a pattern output, which is a bit more complex than just saving a variable *********/
class PatternOutputSaveableSetting : public SaveableSettingBase {
    BasePattern *target = nullptr;
    public:

    // SaveableSettingBase overrides - defined in cpp due to circular dependency issues
    virtual const char* get_line() override;
    virtual bool parse_key_value(const char* key, const char* value) override;

    PatternOutputSaveableSetting(const char *label, const char *category_name, BasePattern *target)
        : target(target)
    {
        set_label(label);
        set_category(category_name);
    }
};

#include "functional-vlpp.h"
class LOutputSaveableSetting : public SaveableSettingBase {
    public:

    using setter_func_def = vl::Func<void(const char*)>;
    using getter_func_def = vl::Func<const char*()>;

    setter_func_def setter_func;
    getter_func_def getter_func;

    LOutputSaveableSetting(const char *label, const char *category_name, setter_func_def setter_func, getter_func_def getter_func)
        : setter_func(setter_func), getter_func(getter_func)
    {
        set_label(label);
        set_category(category_name);
    }

    virtual const char* get_line() override {
        static char buf[128];
        const char *current_output = getter_func();
        if (current_output != nullptr) {
            snprintf(buf, sizeof(buf), "%s=%s", this->label, current_output);
        } else {
            snprintf(buf, sizeof(buf), "; %s%s", this->label, warning_label);
        }
        return buf;
    }
    virtual bool parse_key_value(const char* key, const char* value) override {
        if (!sl_callable_valid(setter_func)) {
            // Serial.printf("WARNING!!! LOutputSaveableSetting::parse_key_value called, but setter_func is not valid! (key=%s, value=%s)\n", key, value);
            return false;
        }
        if (strcmp(key, this->label) == 0) {
            setter_func(value);
            return true;
        }
        return false;
    }

};