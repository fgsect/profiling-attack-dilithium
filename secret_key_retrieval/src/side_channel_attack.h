#ifndef SIDE_CHANNEL_ATTACK_H
#define SIDE_CHANNEL_ATTACK_H
#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <list>
#include <random>
#include <vector>
#include <NTL/ZZ_p.h>
#include <NTL/LLL.h>
#include <NTL/mat_ZZ_p.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <map>
#include "integer_lwe.h"
// extern "C" {
#include "ref/params.h"
#include "ref/polyvec.h"
#include "ref/packing.h"
#include "ref/reduce.h"
#include "ref/poly.h"
//}

namespace SideChannelAttack
{
    class PublicKey
    {
    public:
        polyveck t1;
        polyveck t1_ntt;
        polyveck t1_2d;
        uint8_t rho[SEEDBYTES];
        polyvecl A[K];
        polyvecl A_ntt[K];
        uint8_t pk_bytes[CRYPTO_PUBLICKEYBYTES];
        PublicKey(const uint8_t pk[CRYPTO_PUBLICKEYBYTES]);
        int crypto_sign_signature_s1(uint8_t *sig,
                                     size_t *siglen,
                                     const uint8_t *m,
                                     size_t mlen, polyvecl s1);
    };

    class Signature
    {
    public:
        poly c_poly;
        polyvecl z;
        polyveck h;
        polyveck w;
        polyvecl y;
        std::tuple<int, int> zero_coefficient;
        long az_minus_c2dt1;
        polyveck Az;
        polyvecl Ac[K];
        polyveck c2dt1;
        uint8_t sig_bytes[CRYPTO_BYTES];
        uint8_t seedbuf[680];
        uint8_t rhoprime[64];
        uint16_t nonce;
        Signature(const uint8_t sig[CRYPTO_BYTES]);
        NTL::Vec<NTL::ZZ_p> A_row; // The row that comes from this Signature
        NTL::ZZ_p b;
        NTL::ZZ_p z_minus_y;
        NTL::ZZ_p z_minus_y_vec[L];
        NTL::Vec<NTL::ZZ_p> cs_row;
        NTL::Vec<NTL::ZZ_p> cs_row_for_poly;
        int pos_i;
        int pos_j;
        int actual_coeff;
    };

    class SideChannelAttackClass
    {
    public:
        std::unique_ptr<PublicKey> pkObj;
        std::vector<std::shared_ptr<Signature>> collectedSignatures;

        std::vector<long> solve_for_secret_key_y(polyvecl actual_s1);

        std::condition_variable cv;
        std::mutex cv_m;
        std::mutex result_mtx;
        std::vector<std::vector<long>> result_vector;
        std::map<int, std::unordered_map<int, int>> idx_stored;
        polyveck t0;
        polyveck s2;
        bool HasEnoughSignatures();
        void AddSignature(const uint8_t sigBytes[CRYPTO_BYTES], int i, int j, int predicted_coeff, int actual_coeff);
        static bool CouldBeZero(const uint8_t sigBytes[CRYPTO_BYTES], int predicted_coeff = 0);
        SideChannelAttackClass(const uint8_t pk[CRYPTO_PUBLICKEYBYTES]);
        int num_incorrect_predictions;
        int num_falsely_rejected;
        bool Oracle(polyvecl guessed_s1, polyveck t0, polyveck s2);

    private:
        std::mutex mtx;
    };
}
#endif