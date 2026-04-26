#define ENABLE_OUTPUT_SELECTOR      // euclidian output selector control that is causing us so many crashy problems
#define ENABLE_STEP_DISPLAYS
#define ENABLE_OTHER_CONTROLS

#if defined(ENABLE_SCREEN) && defined(ENABLE_EUCLIDIAN)

#include "submenuitem_bar.h"

#include "mymenu/menuitems_sequencer.h"
#include "mymenu/menuitems_sequencer_circle_single.h"

#include "mymenu/menuitems_outputselectorcontrol.h"
#include "menuitems_lambda.h"

// compound control for Euclidian Patterns; shows step sequence view, animated circle sequence view, and controls 
class EuclidianPatternControl : public SubMenuItemBar {
    BaseSequencer *target_sequencer = nullptr;

    public:

    #ifdef ENABLE_STEP_DISPLAYS
        SingleCircleDisplay *circle_display = nullptr;
        PatternDisplay *step_display = nullptr;
    #endif
    EuclidianPattern *pattern = nullptr;

    EuclidianPatternControl(const char *label, EuclidianPattern *pattern, BaseSequencer *target_sequencer) : SubMenuItemBar(label), pattern(pattern) {
        #ifdef ENABLE_STEP_DISPLAYS
            this->circle_display = new SingleCircleDisplay(label, pattern);     // circle display first - don't add this as a submenu item, because it isn't selectable
            this->step_display = new PatternDisplay(label, pattern, false, false);    // step sequence view next - not selectable, don't draw header
            this->target_sequencer = target_sequencer;
        #endif

        #ifdef ENABLE_OUTPUT_SELECTOR
            OutputSelectorControl<EuclidianPattern> *selector = new OutputSelectorControl<EuclidianPattern>(
                "Output",
                pattern,
                &EuclidianPattern::set_output,
                &EuclidianPattern::get_output,
                pattern->available_outputs,
                pattern->output
            );
            selector->go_back_on_select = true;
            this->add(selector);
        #endif

        #ifdef ENABLE_OTHER_CONTROLS
            //SubMenuItemBar *bar = new SubMenuItemBar("Arguments");
            //Menu *bar = menu;
            // Controls read/write default_arguments so that edits are immediately 'stored'.
            // arguments is kept in sync so that the ProxyParameter modulation system has the correct base.
            // The post-modulation effective values (last_arguments) are displayed in the circle center.
            this->add(new LambdaNumberControl<int8_t> ("Steps",
                [=](int8_t v) -> void { pattern->default_arguments.steps = v; pattern->arguments.steps = v; pattern->mark_dirty(); },
                [=]() -> int8_t { return pattern->default_arguments.steps; },
                nullptr, 1, TIME_SIG_MAX_STEPS_PER_BAR, true, true));
            this->add(new LambdaNumberControl<int8_t> ("Pulses",
                [=](int8_t v) -> void { pattern->default_arguments.pulses = v; pattern->arguments.pulses = v; pattern->mark_dirty(); },
                [=]() -> int8_t { return pattern->default_arguments.pulses; },
                nullptr, 1, TIME_SIG_MAX_STEPS_PER_BAR, true, true));
            this->add(new LambdaNumberControl<int8_t> ("Rotation",
                [=](int8_t v) -> void { pattern->default_arguments.rotation = v; pattern->arguments.rotation = v; pattern->mark_dirty(); },
                [=]() -> int8_t { return pattern->default_arguments.rotation; },
                nullptr, 1, TIME_SIG_MAX_STEPS_PER_BAR, true, true));
            this->add(new LambdaNumberControl<int8_t> ("Duration",  // minimum duration needs to be 2, otherwise can end up with note ons getting missed by usb_teensy_clocker!
                [=](int8_t v) -> void { pattern->default_arguments.duration = v; pattern->arguments.duration = v; pattern->mark_dirty(); },
                [=]() -> int8_t { return pattern->default_arguments.duration; },
                nullptr, MINIMUM_DURATION, TICKS_PER_BAR, true, true));

            // choose global density channel to use
            this->add(new LambdaNumberControl<int8_t> ("Density group", [=](int8_t v) -> int8_t { pattern->set_global_density_group(v); return pattern->get_global_density_group(); }, [=]() -> int8_t { return pattern->get_global_density_group(); }, nullptr, 0, NUM_GLOBAL_DENSITY_GROUPS-1, true, true));

            //menu->debug = true;
            this->add(new ObjectToggleControl<EuclidianPattern> ("Locked", pattern, &EuclidianPattern::set_locked, &EuclidianPattern::is_locked));

            // Shuffle # is last so it overflows to the under-circle (-1 column) position
            #ifdef ENABLE_SHUFFLE
                this->add(new LambdaNumberControl<int8_t> (
                    "Shuffle #", 
                    [=](int8_t track) -> void { pattern->set_shuffle_track(track); }, 
                    [=]() -> int8_t { return pattern->get_shuffle_track(); },
                    nullptr, 0, shuffle_pattern_wrapper.getCount()-1, true, true
                ));
            #endif
        #endif
    }

    virtual void on_add() override {
        SubMenuItemBar::on_add();
        #ifdef ENABLE_STEP_DISPLAYS
            if (this->circle_display!=nullptr) {
                this->circle_display->set_tft(this->tft);
                this->circle_display->on_add();
            }
            if (this->step_display!=nullptr) {
                this->step_display->set_tft(this->tft);
                this->step_display->on_add();
            }
        #endif
    }


    virtual int display(Coord pos, bool selected, bool opened) override {
        //pos.y = header(label, pos, selected, opened);
        tft->setCursor(pos.x, pos.y);
        colours(selected || opened, opened ? GREEN : this->default_fg, this->default_bg);

        #ifdef ENABLE_STEP_DISPLAYS
            if (this->step_display!=nullptr)
                pos.y = this->step_display->display(pos, selected, opened); // display the step sequencer across the top
        #endif

        uint_fast16_t start_y = pos.y;        // y to start drawing at (just under header)
        uint_fast16_t finish_y = pos.y;       // highest y that we finished drawing at

        // draw all the sub-widgets
        //int width_per_item = this->tft->width() / (this->items->size() /*+1*/);
        int start_x = tft->width()/2;
        Debug_printf(F("display in SubMenuItemBar got width_per_item=%i\tfrom tftwidth\t%i / itemsize\t%i\n"), width_per_item, this->tft->width(), this->items->size());

        int widget_height = 0;

        const uint_fast16_t items_size = this->items->size();
        for (uint_fast16_t item_index = 0 ; item_index < items_size ; item_index++) {

            int_fast16_t column = (item_index-1)%2==1;   // first menu item ('output' should span both columns, s
            if (item_index==0   // first item forced to first column
                //|| item_index==items_size-1    // last item forced to first column?
            ) column = 0;

            // When shuffle is enabled, the last item (Shuffle #) overflows under the circle
            #ifdef ENABLE_SHUFFLE
                if (item_index >= items_size-1) {
                    column = -1;
                }
            #endif

            if (column==0)
                start_x = tft->width()/2;
            else if (column==-1) {
                // put last item under the circle display
                if (item_index < items_size-1) {
                    start_x = 0;
                    start_y = start_y - widget_height;
                } else {
                    start_x = tft->width()/4;
                }
            } else
                start_x = (tft->width()/2)  + (tft->width()/4);

            bool wrap = item_index==0   // first item moves cursor to next row
                        || column==1    // second column moves cursor to next row
                        //|| item_index==items_size-2
                    ;    // last item moves cursor to next row

            //int width = this->get_max_pixel_width(item_index);
            uint_fast16_t width = ((wrap && column==0) ? this->tft->width()/2 : this->tft->width()/4); ///this->tft->characterWidth();
            //if (item_index==items_size-1) width = width/2;

            //if (item_index==0) width -= tft->characterWidth();  // adjust by 1 character if necessary
            const uint_fast16_t temp_y = this->small_display(
                item_index, 
                start_x, 
                start_y, 
                width, //width_per_item, 
                this->currently_selected==(int)item_index, 
                this->currently_opened==(int)item_index,
                false
            );

            widget_height = temp_y - start_y;

            //start_x += width;
            //Serial.printf("for item %i '%s':\tgot column=\t%i, wrap=\t%s => start_x=%i\t, start_y=%i\t((item_index-1)%%2=%i\n", item_index, this->items->get(item_index)->label, column, wrap?"Y":"N", start_x, start_y, (item_index-1)%2);
            //if (item_index==0 || (item_index-1)%2==0 || item_index==this->items->size()-2)
            if (wrap)
                start_y = temp_y;
            if (temp_y>finish_y)
                finish_y = temp_y;
        }

        // Match SubMenuItemBar behavior: defer opened overlay draw until end of frame.
        if (opened && this->currently_opened>=0 && this->currently_opened < (int)this->items->size()) {
            MenuItem *opened_item = this->items->get(this->currently_opened);
            if (opened_item!=nullptr && opened_item->wants_fullscreen_overlay_when_opened_in_bar() && menu!=nullptr) {
                menu->pending_overlay_item = opened_item;
                menu->pending_overlay_y = pos.y;
            }
        }

        #ifdef ENABLE_STEP_DISPLAYS
            if (this->circle_display!=nullptr) {
                this->circle_display->display(pos, selected, opened);
                // Overlay the current effective (post-modulation) values in the circle center
                if (this->pattern != nullptr) {
                    const uint_fast16_t cx = tft->width()/4;
                    const uint_fast16_t cy = this->circle_display->circle_center_y;
                    const int char_w = tft->currentCharacterWidth();
                    const int char_h = tft->getSingleRowHeight();
                    // format: "S:XX P:XX R:XX" showing effective steps/pulses/rotation from last make_euclid()
                    char eff_buf[16];
                    snprintf(eff_buf, sizeof(eff_buf), "%d/%d+%d",
                        (int)this->pattern->last_post_arguments.steps,
                        (int)this->pattern->last_post_arguments.pulses,
                        (int)this->pattern->last_post_arguments.rotation);
                    const int text_w = strlen(eff_buf) * char_w;
                    const int tx = cx - text_w / 2;
                    const int ty = cy - char_h / 2;
                    tft->fillRect(tx - 1, ty - 1, text_w + 2, char_h + 2, BLACK);
                    tft->setTextColor(this->pattern->last_post_arguments.steps != this->pattern->default_arguments.steps
                        || this->pattern->last_post_arguments.pulses != this->pattern->default_arguments.pulses
                        || this->pattern->last_post_arguments.rotation != this->pattern->default_arguments.rotation
                        ? YELLOW : C_WHITE, BLACK);
                    tft->setCursor(tx, ty);
                    tft->printf("%s", eff_buf);
                }
            }
        #endif

        return finish_y;
    }

    virtual inline int get_max_pixel_width(uint_fast16_t item_number) {
        if (item_number==0 || item_number==this->items->size()-1)
            return this->tft->width() / 2;
        else
            return this->tft->width() / 4;
    }
};

#endif