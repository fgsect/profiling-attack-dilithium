#ifndef GLOBAL_VARIABLES_H
#include "polyvec.h"
#include "poly.h"
#include <atomic>
#define GLOBAL_VARIABLES_H
extern const char log_output_filename[];
extern std::atomic<int> try_number;
extern std::atomic<int> global_noise;
extern std::atomic<int> global_save_signatures;
extern std::atomic<int> global_make_predictions;
extern std::atomic<int> global_wrongly_rejected;
 
extern int max_difference_with_zero;
    #if defined(DILITHIUM_PARALLEL)
        extern thread_local polyveck global_current_round_w;
        extern thread_local polyvecl global_current_round_y;
        extern thread_local uint8_t global_current_round_seedbuf[];
        extern thread_local uint8_t global_current_round_rhoprime[];
        extern thread_local uint16_t global_current_round_nonce;
    #else
        polyveck global_current_round_w;
        polyvecl global_current_round_y;
        uint8_t buf[];
    #endif
    #if defined(DILITHIUM_PARALLEL)
        extern thread_local polyvecl global_current_round_cs;
        extern thread_local polyvecl global_current_round_s;
        extern thread_local poly global_current_round_c;
        extern thread_local polyveck global_as;
        extern thread_local polyveck global_current_round_cs2;
    #else
        polyvecl global_current_round_cs;
        polyvecl global_current_round_s;
        poly global_current_round_c;
        polyveck global_as;
        polyveck global_current_round_cs2;
    #endif
#endif