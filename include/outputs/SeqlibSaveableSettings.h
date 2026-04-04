// SaveableParameter implementations for handling special things like Pattern outputs which aren't just simple variables

#pragma once

#include <Arduino.h>

#include "outputs/base_outputs.h"
#include "outputs/output.h"
//#include "sequencer/Base/Patterns.h"

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

