#include "global_variables.h"
#include "polyvec.h"
#include "poly.h"
extern const char log_output_filename[] = "dilithium_log.log";
int try_number = 0;
int max_difference_with_zero = 0;
#if defined(DILITHIUM_PARALLEL)
    thread_local polyveck global_current_round_w;
#else
    polyveck global_current_round_w;
#endif
#if defined(DILITHIUM_PARALLEL)
    thread_local polyvecl global_current_round_cs;
    thread_local polyvecl global_current_round_s;
    thread_local poly global_current_round_c;
    thread_local polyveck global_as;
    thread_local polyveck global_current_round_cs2;
#else
    polyvecl global_current_round_cs;
    polyvecl global_current_round_s;
    poly global_current_round_c;
    polyveck global_as;
    polyveck global_current_round_cs2;
#endif