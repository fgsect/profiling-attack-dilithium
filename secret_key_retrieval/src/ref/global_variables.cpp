#include "global_variables.h"
#include "polyvec.h"
#include "poly.h"
#include "symmetric.h"
#include <atomic>
extern const char log_output_filename[] = "dilithium_log.log";
std::atomic<int> try_number(0);
std::atomic<int> global_noise(0);
std::atomic<int> global_save_signatures(0);
std::atomic<int> global_make_predictions(0);
std::atomic<int> global_wrongly_rejected(0);
int max_difference_with_zero = 0;
#if defined(DILITHIUM_PARALLEL)
    thread_local polyveck global_current_round_w;
    thread_local polyvecl global_current_round_y;
    #define POLY_UNIFORM_GAMMA1_NBLOCKS ((POLYZ_PACKEDBYTES + STREAM256_BLOCKBYTES - 1)/STREAM256_BLOCKBYTES)
    thread_local uint8_t global_current_round_seedbuf[POLY_UNIFORM_GAMMA1_NBLOCKS*STREAM256_BLOCKBYTES];
    thread_local uint8_t global_current_round_rhoprime[64];
    thread_local uint16_t global_current_round_nonce;
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