#include "outputs/SeqlibSaveableSettings.h"
#include "sequencer/Base/Patterns.h"

const char* PatternOutputSaveableSetting::get_line() {
    static char buf[128];
    if (this->target != nullptr) {
        snprintf(buf, sizeof(buf), "%s=%s", this->label, this->target->get_output_label());
    } else {
        snprintf(buf, sizeof(buf), "; %s%s", this->label, warning_label);
    }
    return buf;
}
bool PatternOutputSaveableSetting::parse_key_value(const char* key, const char* value) {
    if (this->target==nullptr) {
        Serial.printf("WARNING!!! PatternOutputSaveableSetting::parse_key_value called, but target to set output on is null! (key=%s, value=%s)\n", key, value);
        return false;
    }
    if (strcmp(key, this->label) == 0) {
        //this->set((DataType)0,value);
        this->target->set_output_by_name(value);
        return true;
    }
    return false;
}
