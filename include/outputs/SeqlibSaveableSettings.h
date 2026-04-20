#ifdef ENABLE_STORAGE
// SaveableParameter implementations for handling special things like Pattern outputs which aren't just simple variables

#pragma once

#include <Arduino.h>

#include "outputs/base_outputs.h"
#include "outputs/output.h"

#include "saveloadlib.h"

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
        if (!sl_callable_valid(getter_func)) {
            snprintf(buf, sizeof(buf), "; %s%s", this->label, warning_label);
            return buf;
        }

        const char *current_output = getter_func();
        if (current_output != nullptr) {
            snprintf(buf, sizeof(buf), "%s=%s", this->label, current_output);
        } else {
            snprintf(buf, sizeof(buf), "; %s%s", this->label, warning_label);
        }
        return buf;
    }
    virtual bool parse_key_value(const char* key, const char* value) override {
        if (strcmp(key, this->label) != 0) {
            return false;
        }
        if (!sl_callable_valid(setter_func)) {
            // Serial.printf("WARNING!!! LOutputSaveableSetting::parse_key_value called, but setter_func is not valid! (key=%s, value=%s)\n", key, value);
            return false;
        }
        setter_func(value);
        return true;
    }

};


#include "sequencer/Base/Patterns.h"

// Saves/loads a BasePattern's midi_note_event_t array.
// Each event is encoded as 6 hex chars: NNVVCC (note, velocity, channel),
// where each byte is stored as its uint8_t bit-pattern so that NOTE_OFF (-1)
// round-trips correctly as 0xff.
// Line format: key=NNVVCCNNVVCC...  (one 6-char group per event)
class SaveableMIDINoteArraySetting : public SaveableSettingBase {
    public:
    //BasePattern *pattern;
    midi_note_event_t *events;
    uint16_t array_size;

    SaveableMIDINoteArraySetting(const char *label, const char *category, midi_note_event_t *pattern, uint16_t array_size)
        : SaveableSettingBase() {
        this->set_label(label);
        this->set_category(category ? category : "");
        //this->pattern = nullptr;
        this->events = pattern;
        this->array_size = array_size;
    }

    const char *get_line() override {
        int pos = snprintf(linebuf, SL_MAX_LINE, "%s=", label);
        for (uint16_t i = 0; i < array_size && pos < SL_MAX_LINE - 6; i++) {
            uint8_t n = (uint8_t)events[i].note;
            uint8_t v = (uint8_t)events[i].velocity;
            uint8_t c = (uint8_t)events[i].channel;
            pos += snprintf(linebuf + pos, SL_MAX_LINE - pos, "%02x%02x%02x", n, v, c);
        }
        linebuf[pos] = '\0';
        return linebuf;
    }

    bool parse_key_value(const char *key, const char *value) override {
        if (strcmp(key, label) != 0) return false;
        if (events == nullptr || value == nullptr) return false;

        size_t value_len = strlen(value);
        // re-read live pointer in case pattern allocated events after construction
        //events = pattern->events;
        for (uint16_t i = 0; i < array_size; i++) {
            size_t base = (size_t)i * 6u;
            if (base + 5u >= value_len) break;
            char byte_str[3];
            byte_str[2] = '\0';

            byte_str[0] = value[base];     byte_str[1] = value[base + 1u];
            events[i].note     = (int8_t)(uint8_t)strtol(byte_str, nullptr, 16);

            byte_str[0] = value[base + 2u];   byte_str[1] = value[base + 3u];
            events[i].velocity = (int8_t)(uint8_t)strtol(byte_str, nullptr, 16);

            byte_str[0] = value[base + 4u];   byte_str[1] = value[base + 5u];
            events[i].channel  = (int8_t)(uint8_t)strtol(byte_str, nullptr, 16);
        }
        return true;
    }

    virtual size_t heap_size() const override { return sizeof(SaveableMIDINoteArraySetting); }
};

#endif