// SaveableParameter implementations for handling special things like Pattern outputs which aren't just simple variables

#pragma once

#include <Arduino.h>

#include "outputs/base_outputs.h"
#include "outputs/output.h"
//#include "SaveableParameters.h"
//#include "sequencer/Base/Patterns.h"

/********* example of how to use this for a pattern output, which is a bit more complex than just saving a variable *********/
class PatternOutputSaveableParameter : public SaveableParameterBase {
    BasePattern *target = nullptr;
    public:

    // SaveableParameterBase overrides - defined in cpp due to circular dependency issues?
    virtual String get_line() override;
    virtual bool parse_key_value(String key, String value) override;

    PatternOutputSaveableParameter(const char *label, const char *category_name, BasePattern *target) 
    : SaveableParameterBase(label, category_name) {
        this->target = target;
    }
};
