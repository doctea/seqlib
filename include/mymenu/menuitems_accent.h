#ifdef ENABLE_ACCENTS

// menuitems_accent.h
// UI page for controlling the global StepAccentSource.
//
// Add to any Menu with:
//   menu->add(new GlobalAccentStepPresetControl());
//
// Expects global_accent_source to be a StepAccentSource* at runtime.
// (It casts safely; if the source is null or a different type the page
//  shows "--" and ignores edits.)

#pragma once

#if defined(ENABLE_SCREEN) && defined(ENABLE_SCREEN)

#include "submenuitem_bar.h"
#include "menuitems_lambda.h"
#include "accent/IAccentSource.h"
#include "accent/StepAccentSource.h"

#define STEP_EDITOR_GRAPH_HEIGHT 50


// ---------------------------------------------------------------------------
// GlobalAccentStepEditorControl
//
// Draws a mini bar-chart of all step accent levels from the global
// StepAccentSource.  Navigate left/right to select a step; press select to
// enter edit mode, then use encoder up/down (increase/decrease) to adjust the
// level in 1/8 increments.  Press select again (or back) to leave edit mode.
// ---------------------------------------------------------------------------
class GlobalAccentStepEditorControl : public MenuItem {
    int  selected_step = 0;
    bool editing       = false;

    static constexpr float STEP_SIZE    = 0.125f;   // 8 levels between 0 and 1
    static constexpr float ACCENT_MIN   =  0.0f;
    static constexpr float ACCENT_MAX   =  1.0f;

    static StepAccentSource* source() {
        return global_accent_source ? global_accent_source->as_step_source() : nullptr;
    }

public:
    GlobalAccentStepEditorControl(const char* label = "Step Accents")
        : MenuItem(label) {}

    // -----------------------------------------------------------------------
    // Display — bar-chart rows
    // -----------------------------------------------------------------------
    virtual int renderValue(bool selected, bool opened, uint16_t /*max_character_width*/) override {
        // Must set text size explicitly — caller (MenuItem::display) sets size 0.
        tft->setTextSize(1);

        Coord pos(tft->getCursorX(), tft->getCursorY());
        const int initial_x = pos.x;
        const int initial_y = pos.y;

        StepAccentSource* src = source();

        if (!src) {
            colours(false);
            tft->println("(no StepAccentSource)");
            return tft->getCursorY();
        }

        const uint16_t num_steps = src->get_num_steps();
        // Match LoopMarkerPanel sizing: fixed bar width based on STEPS_PER_PHRASE
        // so bars align with the beat/bar tick marks above.
        const int     avail_w   = tft->width();
        const float   bar_w     = max(1, avail_w / (float)STEPS_PER_PHRASE);
        const int     bar_max_h = STEP_EDITOR_GRAPH_HEIGHT;   // fixed pixel height for bars, independent of text size
        const bool    show_labels = (bar_w >= tft->characterWidth());

        // Single clear of the entire bar area — much faster than erasing each bar individually.
        tft->fillRect(0, initial_y, avail_w, bar_max_h, BLACK);

        // Border indicates focus state:
        //   selected (hovering, not yet opened) → YELLOW outline
        //   opened (navigating/editing steps)   → dim outline only; cursor colour distinguishes
        if (selected && !opened)
            tft->drawRect(0, initial_y, avail_w, bar_max_h, YELLOW);

        for (int i = 0; i < (int)num_steps; i++) {
            const float level = src->get_accent_raw(i);
            const int   bar_h = (int)(level * bar_max_h);
            const int   x     = (int)((float)i * bar_w);
            const int   y_top = initial_y + bar_max_h - bar_h;

            const bool is_selected = (i == selected_step);

            uint16_t fill_colour;
            if (is_selected && editing)
                fill_colour = GREEN;                            // actively editing this step
            else if (is_selected && opened)
                fill_colour = ORANGE;                          // cursor on step, navigating
            else if (is_selected)
                fill_colour = YELLOW;                          // hovering over editor, not yet opened
            else if (level >= 1.0f)
                fill_colour = tft->halfbright_565(C_WHITE);
            else if (level <= 0.0f)
                fill_colour = GREY;
            else
                fill_colour = tft->halfbright_565(C_WHITE);

            if (bar_h > 0)
                tft->fillRect(x, y_top, (int)bar_w - 1, bar_h, fill_colour);
            else
                tft->drawRect(x, initial_y + bar_max_h - 1, (int)bar_w - 1, 1, GREY);

            // digit label — only when bars are wide enough to fit a character
            if (show_labels) {
                this->colours(is_selected && (selected || opened), fill_colour, this->default_bg);
                tft->setCursor(x, initial_y + bar_max_h + 1);
                tft->printf("%1X", (int)roundf(level / STEP_SIZE));
            }
        }

        const int label_row_h = show_labels ? tft->getRowHeight() : 0;
        tft->setCursor(0, initial_y + bar_max_h + label_row_h);
        //tft->println();

        return tft->getCursorY();
    }

    // -----------------------------------------------------------------------
    // Navigation
    // -----------------------------------------------------------------------
    virtual bool knob_left() override {
        StepAccentSource* src = source();
        if (!src) return false;

        if (!editing) {    
            if (selected_step == 0)
                selected_step = src->get_num_steps() - 1;
            else
                selected_step--;
            return true;
        } else {
            float v = src->get_accent_raw(selected_step);
            v -= STEP_SIZE;
            if (v < ACCENT_MIN) v = ACCENT_MIN;
            src->set_accent(selected_step, v);
            return true;
        }
    }

    virtual bool knob_right() override {
        StepAccentSource* src = source();
        if (!src) return false;

        if (!editing) {
            selected_step++;
            if (selected_step >= (int)src->get_num_steps())
                selected_step = 0;
            return true;
        } else {
            float v = src->get_accent_raw(selected_step);
            v += STEP_SIZE;
            if (v > ACCENT_MAX) v = ACCENT_MAX;
            src->set_accent(selected_step, v);
            return true;
        }
    }

    // Select — toggle edit mode.
    // Return false so SubMenuItem keeps this item "opened" and continues
    // routing knob events here (instead of closing it on first press).
    virtual bool button_select() override {
        editing = !editing;
        return false;
    }

    virtual bool button_back() override {
        if (editing) {
            editing = false;
            return true;    // consume — don't propagate
        }
        return MenuItem::button_back();
    }
};


// ---------------------------------------------------------------------------
// GlobalAccentStepPresetControl
//
// A SubMenuItemBar page with the step editor always drawn full-width at the
// top, and utility controls (All-same knob, presets) laid out in a row below.
//
// The step editor is held as a direct member (not in items) so SubMenuItemBar's
// auto-layout does not shrink it.  Utility items live in items[] and are sized
// equally across the remaining space.
//
// Usage:
//   menu->add(new GlobalAccentStepPresetControl());
// ---------------------------------------------------------------------------
class GlobalAccentStepPresetControl : public SubMenuItemBar {
    GlobalAccentStepEditorControl* step_editor = nullptr;

public:
    GlobalAccentStepPresetControl(const char* label = "Accent") : SubMenuItemBar(label) {
        step_editor = new GlobalAccentStepEditorControl("Step Accents");

        // Add step_editor as items[0] so SubMenuItem's knob/button routing
        // naturally delivers input to it when it is selected/opened.
        this->add(step_editor);

        // Utility controls laid out horizontally below the step editor.
        // "All" scale: multiplies all steps proportionately so their peak reaches v/8.
        // Preserves the relative shape of the pattern; if all steps are zero, sets flat.
        this->add(new LambdaNumberControl<int8_t>(
            "All",
            [](int8_t v) {
                StepAccentSource* src = global_accent_source ? global_accent_source->as_step_source() : nullptr;
                if (!src) return;
                const float target = (float)v / 8.0f;
                // find current peak
                float current_max = 0.0f;
                for (uint16_t i = 0; i < src->get_num_steps(); i++)
                    current_max = max(current_max, src->get_accent_raw(i));
                if (current_max <= 0.0f) {
                    // all steps are silent — just set flat
                    src->set_all(target);
                    return;
                }
                const float scale = target / current_max;
                for (uint16_t i = 0; i < src->get_num_steps(); i++)
                    src->set_accent(i, constrain(src->get_accent_raw(i) * scale, 0.0f, 1.0f));
            },
            []() -> int8_t {
                StepAccentSource* src = global_accent_source ? global_accent_source->as_step_source() : nullptr;
                if (!src) return 8;
                float current_max = 0.0f;
                for (uint16_t i = 0; i < src->get_num_steps(); i++)
                    current_max = max(current_max, src->get_accent_raw(i));
                return (int8_t)roundf(current_max * 8.0f);
            },
            nullptr,
            (int8_t)0, (int8_t)8,
            /*go_back_on_select=*/false,
            /*direct=*/true
        ));

        SubMenuItemColumns* presets = new SubMenuItemColumns("Presets", 4, false, true);

        // Preset: 110% on first beat of phrase, 95% on first beats of bars, 
        // 85% on other strong beats, 62.5% on everything else
        // go up to 64 steps to cover a full 4-bar phrase at 16th-note resolution
        presets->add(new LambdaActionItem("Gruv", []() {
            StepAccentSource* src = global_accent_source ? global_accent_source->as_step_source() : nullptr;
            if (!src) return;
            const uint8_t strong_110[] = {0};   // +10% on first beat of phrase
            const uint8_t strong_95[]  = {16, 32, 48};   // +5% on first beat of bars
            const uint8_t strong_85[]  = {4, 12, 20, 28, 36, 44, 52, 60};   // +5% on other strong beats
            src->set_all(0.7f);   // 7l0% on everything else
            src->set_pattern(strong_110, 1, 1.10f, 0.7f);
            src->set_pattern(strong_95, 3, 0.95f, 0.7f);
            src->set_pattern(strong_85, 8, 0.85f, 0.7f);
        }));

        // Preset: strong quarter-note beats (steps 0, 4, 8, 12)
        presets->add(new LambdaActionItem("4/4", []() {
            StepAccentSource* src = global_accent_source ? global_accent_source->as_step_source() : nullptr;
            if (!src) return;
            const uint8_t strong[] = {0, 4, 8, 12};
            src->set_all(0.7f);   // 70% on weak beats
            src->set_pattern(strong, 4, 1.0f, 0.7f);
        }));

        // Preset: flat full velocity
        presets->add(new LambdaActionItem("Flat", []() {
            StepAccentSource* src = global_accent_source ? global_accent_source->as_step_source() : nullptr;
            if (!src) return;
            src->set_all(1.0f);
        }));

        // Preset: half velocity across all steps
        presets->add(new LambdaActionItem("Half", []() {
            StepAccentSource* src = global_accent_source ? global_accent_source->as_step_source() : nullptr;
            if (!src) return;
            src->set_all(0.5f);
        }));

        this->add(presets);
    }

    // on_add: SubMenuItem::on_add() handles all items[], including step_editor.
    virtual void on_add() override {
        SubMenuItemBar::on_add();
    }

    // Draw: items[0] (step editor) full-width at top; items[1..] in an equal-width
    // utility row below.  Each item's selected/opened flags come from the page's
    // own currently_selected / currently_opened tracking so only one section
    // highlights at a time.
    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y = header(this->label, pos, selected, opened);
        tft->setCursor(pos.x, pos.y);

        // Step editor is items[0] — always rendered full-width from x=0.
        if (step_editor) {
            const bool step_sel  = (this->currently_selected == 0);
            const bool step_open = (this->currently_opened   == 0);
            pos.y = step_editor->display(Coord(0, pos.y), step_sel, step_open);
        }

        // Utility controls (items[1..]) laid out side-by-side below.
        const int util_start = step_editor ? 1 : 0;
        const int n = (int)this->items->size() - util_start;
        if (n > 0) {
            const int item_w = tft->width() / n;
            int finish_y = pos.y;

            for (int i = 0; i < n; i++) {
                const int idx = util_start + i;
                MenuItem* item = this->items->get(idx);
                if (!item) continue;
                const bool item_sel  = (this->currently_selected == idx);
                const bool item_open = (this->currently_opened   == idx);
                const int x = i * item_w;
                tft->setCursor(x, pos.y);
                int end_y = item->display(Coord(x, pos.y), item_sel, item_open);
                if (end_y > finish_y) finish_y = end_y;
            }
            pos.y = finish_y;
        }

        return pos.y;
    }
};

#endif // ENABLE_SCREEN

#endif