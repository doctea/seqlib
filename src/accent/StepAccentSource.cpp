#ifdef ENABLE_ACCENTS

#ifdef ENABLE_SCREEN
    #include "mymenu/menuitems_accent.h"

    void StepAccentSource::make_menu_items() {
        // subclasses can add their own menu items if they want

        menu->add_page("Accents");
        menu->add(new GlobalAccentPage());

    }

#endif

#endif