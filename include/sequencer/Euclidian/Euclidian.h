#pragma once

#ifndef FLASHMEM
    #define FLASHMEM
#endif

//#include "Config.h"

// todo: move this to build flags or something...
#define SEQLIB_MUTATE_EVERY_TICK

#include "debug.h"
#include <LinkedList.h>
#include "../Base/Patterns.h"
#include "../Base/Sequencer.h"

extern const int num_initial_arguments;

class FloatParameter;
class Menu;

#define MINIMUM_DENSITY 0.0f  // 0.10f
#define MAXIMUM_DENSITY 1.5f
#define DEFAULT_DENSITY 0.6666f
#ifndef NUM_GLOBAL_DENSITY_GROUPS
    #define NUM_GLOBAL_DENSITY_GROUPS 4
#endif

#define MINIMUM_MUTATION_COUNT 0
#define MAXIMUM_MUTATION_COUNT 8
#define DEFAULT_MUTATION_COUNT 3

#define DEFAULT_DURATION 2      // minimum duration needs to be >=2 ticks -- if any lower then can end up with on+off happening within the same tick and usb_teensy_clocker gets notes stuck!
#define MINIMUM_DURATION 2
#define DEFAULT_DURATION_ENVELOPES 8

#define SEQUENCE_LENGTH_STEPS 16
const int LEN = SEQUENCE_LENGTH_STEPS;
    
struct arguments_t {
    const char *associated_label = nullptr;
    int_fast8_t steps = SEQUENCE_LENGTH_STEPS;
    int_fast8_t pulses = steps/2;
    int_fast8_t rotation = 1;
    int_fast8_t duration = 1;
    float effective_euclidian_density = 0.6666;
    int_fast8_t tie_on = -1;
};

extern arguments_t initial_arguments[];
extern float all_global_density[];
extern float all_effective_global_density[];


#ifdef ENABLE_SCREEN
    namespace Euclidian {
        enum CombinePageOption {
            COMBINE_NONE = 0,
            COMBINE_LOCKS_WITH_CIRCLE = 1,
            COMBINE_MUTATION_WITH_LOCKS = 2,
            COMBINE_MODULATION_WITH_MUTATION = 4,
            COMBINE_PATTERN_MODULATION_WITH_PATTERN = 8,
            COMBINE_ALL = 15
        };

        void decode_combine_page_option(CombinePageOption option);
    }
#endif

