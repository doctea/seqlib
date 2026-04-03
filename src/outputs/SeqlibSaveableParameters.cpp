#include "outputs/SeqlibSaveableParameters.h"
#include "sequencer/Base/Patterns.h"

String PatternOutputSaveableParameter::get_line() {
    if (this->target!=nullptr) {
        return String(this->label) + String('=') + this->target->get_output_label();
    } else {
        return String("; ") + String(this->label) + warning_label;
    }
}
bool PatternOutputSaveableParameter::parse_key_value(String key, String value) {
    if (this->target==nullptr) {
        Serial.printf("WARNING!!! PatternOutputSaveableParameter::parse_key_value called, but target to set output on is null! (key=%s, value=%s)\n", key.c_str(), value.c_str());
        return false;
    }
    if (key.equals(this->label)) {
        //this->set((DataType)0,value);
        this->target->set_output_by_name(value.c_str());
        return true;
    }
    return false;
}
