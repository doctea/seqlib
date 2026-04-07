#ifdef ENABLE_ACCENTS

// menuitems_accent.h
// UI page for controlling the global StepAccentSource.
//
// Add to any Menu with:
//   menu->add(new GlobalAccentPage());
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
        const int     avail_w   = tft->width() - initial_x;
        const int     bar_w     = max(1, avail_w / (int)num_steps);
        const int     bar_max_h = STEP_EDITOR_GRAPH_HEIGHT;   // fixed pixel height for bars, independent of text size
        const bool    show_labels = (bar_w >= tft->characterWidth());

        // Single clear of the entire bar area — much faster than erasing each bar individually.
        tft->fillRect(initial_x, initial_y, avail_w, bar_max_h, BLACK);

        for (int i = 0; i < (int)num_steps; i++) {
            const float level = src->get_accent_raw(i);
            const int   bar_h = (int)(level * bar_max_h);
            const int   x     = initial_x + i * bar_w;
            const int   y_top = initial_y + bar_max_h - bar_h;

            const bool is_selected = (i == selected_step);

            uint16_t fill_colour;
            if (is_selected && editing)
                fill_colour = GREEN;
            else if (is_selected)
                fill_colour = C_WHITE;
            else if (level >= 1.0f)
                fill_colour = tft->halfbright_565(C_WHITE);
            else if (level <= 0.0f)
                fill_colour = GREY;
            else
                fill_colour = tft->halfbright_565(C_WHITE);

            if (bar_h > 0)
                tft->fillRect(x, y_top, bar_w - 1, bar_h, fill_colour);
            else
                tft->drawRect(x, initial_y + bar_max_h - 1, bar_w - 1, 1, GREY);

            // digit label — only when bars are wide enough to fit a character
            if (show_labels) {
                this->colours(is_selected && (selected || opened), fill_colour, this->default_bg);
                tft->setCursor(x, initial_y + bar_max_h + 1);
                tft->printf("%1X", (int)roundf(level / STEP_SIZE));
            }
        }

        const int label_row_h = show_labels ? tft->getRowHeight() : 0;
        tft->setCursor(initial_x, initial_y + bar_max_h + label_row_h);
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
            if (selected_step == 0)
                selected_step = src->get_num_steps() - 1;
            else
                selected_step--;
            return true;
        } else {
            float v = src->get_accent_raw(selected_step);
            v += STEP_SIZE;
            if (v > ACCENT_MAX) v = ACCENT_MAX;
            src->set_accent(selected_step, v);
            return true;
        }
    }

    // Select — toggle edit mode
    virtual bool button_select() override {
        editing = !editing;
        return true;
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
// GlobalAccentPage
//
// A SubMenuItemBar page with the step editor always drawn full-width at the
// top, and utility controls (All-same knob, presets) laid out in a row below.
//
// The step editor is held as a direct member (not in items) so SubMenuItemBar's
// auto-layout does not shrink it.  Utility items live in items[] and are sized
// equally across the remaining space.
//
// Usage:
//   menu->add(new GlobalAccentPage());
// ---------------------------------------------------------------------------
class GlobalAccentPage : public SubMenuItemBar {
    GlobalAccentStepEditorControl* step_editor = nullptr;

public:
    GlobalAccentPage(const char* label = "Accent") : SubMenuItemBar(label) {
        step_editor = new GlobalAccentStepEditorControl("Step Accents");

        // Utility controls laid out horizontally below the step editor.
        // "All" quick-set: sets all steps to the same level (0-8 → 0.0-1.0)
        this->add(new LambdaNumberControl<int8_t>(
            "All",
            [](int8_t v) {
                StepAccentSource* src = global_accent_source ? global_accent_source->as_step_source() : nullptr;
                if (src) src->set_all((float)v / 8.0f);
            },
            []() -> int8_t {
                StepAccentSource* src = global_accent_source ? global_accent_source->as_step_source() : nullptr;
                if (!src) return 8;
                return (int8_t)roundf(src->get_accent_raw(0) * 8.0f);
            },
            nullptr,
            (int8_t)0, (int8_t)8,
            /*go_back_on_select=*/false,
            /*direct=*/true
        ));

        SubMenuItemColumns* presets = new SubMenuItemColumns("Presets", 4, true, true);

        // Preset: 110% on first beat of phrase, 95% on first beats of bars, 
        // 85% on other strong beats, 62.5% on everything else
        // go up to 64 steps to cover a full 4-bar phrase at 16th-note resolution
        presets->add(new LambdaActionItem("4/4 Var.", []() {
            StepAccentSource* src = global_accent_source ? global_accent_source->as_step_source() : nullptr;
            if (!src) return;
            const uint8_t strong_110[] = {0};   // +10% on first beat of phrase
            const uint8_t strong_95[]  = {16, 32, 48};   // +5% on first beat of bars
            const uint8_t strong_85[]  = {4, 12, 20, 28, 36, 44, 52, 60};   // +5% on other strong beats
            src->set_all(0.7f);   // 70% on everything else
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

    virtual void on_add() override {
        SubMenuItemBar::on_add();
        if (step_editor) {
            step_editor->set_tft(this->tft);
            step_editor->on_add();
        }
    }

    // Draw: step editor full-width at top; utility items in an equal-width row below.
    virtual int display(Coord pos, bool selected, bool opened) override {
        pos.y = header(this->label, pos, selected, opened);
        tft->setCursor(pos.x, pos.y);
        colours(selected || opened, opened ? GREEN : this->default_fg, this->default_bg);

        // Step editor gets the full width
        if (step_editor)
            pos.y = step_editor->display(pos, selected, opened);

        // Utility controls laid out side-by-side below
        if (this->items && this->items->size() > 0) {
            const int n          = (int)this->items->size();
            const int item_w     = tft->width() / n;
            int finish_y         = pos.y;

            for (int i = 0; i < n; i++) {
                MenuItem* item = this->items->get(i);
                if (!item) continue;
                int x = pos.x + i * item_w;
                tft->setCursor(x, pos.y);
                int end_y = item->display(Coord(x, pos.y), selected, opened);
                if (end_y > finish_y) finish_y = end_y;
            }
            pos.y = finish_y;
        }

        return pos.y;
    }
};

#endif // ENABLE_SCREEN

#endif