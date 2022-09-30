#include "ilp_solver.h"
#include "side_channel_attack.h"
#include "ref/randombytes.h"
#include "ref/fips202.h"
#include "ref/symmetric.h"
#include "ref/sign.h"
#include "integer_lwe.h"
#include <set>
#include "assert.h"
#include <future>
#include <chrono>
#include <iterator>
#include <stdexcept>
#include <fstream>
#include <string>

#include <assert.h>

namespace SideChannelAttack
{
    int py_mod(int a, int b)
    {
        int r = a % b;
        if (r < 0)
            r += b;
        return r;
    }
    PublicKey::PublicKey(const uint8_t pk[CRYPTO_PUBLICKEYBYTES])
    {
        memcpy(this->pk_bytes, pk, CRYPTO_PUBLICKEYBYTES);
        unpack_pk(this->rho, &this->t1, pk);
        polyvec_matrix_expand(this->A_ntt, rho);
        for (int i = 0; i < K; ++i)
        {
            for (int j = 0; j < L; ++j)
            {
                for (int k = 0; k < N; ++k)
                {
                    A[i].vec[j].coeffs[k] = (int64_t)A_ntt[i].vec[j].coeffs[k] * -114592 % Q;
                }
            }
        }
        for (int i = 0; i < K; ++i)
        {
            polyvecl_invntt_tomont(&A[i]);
        }
        polyveck tmpt1 = this->t1;
        for (int i = 0; i < K; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                tmpt1.vec[i].coeffs[j] *= 8192; // 2**13
                tmpt1.vec[i].coeffs[j] = freeze(tmpt1.vec[i].coeffs[j] % Q);
            }
        }
        polyveck_reduce(&tmpt1);
        this->t1_2d = tmpt1;
        polyveck_ntt(&tmpt1);
        this->t1_ntt = tmpt1;
    }

    int PublicKey::crypto_sign_signature_s1(uint8_t *sig,
                                            size_t *siglen,
                                            const uint8_t *m,
                                            size_t mlen, polyvecl s1)
    {
        bool done = false;
        unsigned int n;
        uint8_t seedbuf[3 * SEEDBYTES + 2 * CRHBYTES];
        uint8_t *rho, *tr, *key, *mu, *rhoprime;
        randombytes(seedbuf, SEEDBYTES);
        shake256(seedbuf, 2 * SEEDBYTES + CRHBYTES, seedbuf, SEEDBYTES);
        rho = seedbuf;
        memcpy(rho, this->rho, SEEDBYTES);
        tr = rho + SEEDBYTES;
        key = tr + SEEDBYTES;
        mu = key + SEEDBYTES;
        rhoprime = mu + CRHBYTES;
        uint16_t nonce = 0;
        polyvecl y, z;
        polyveck t1;
        polyveck t0, s2, w1, w0, h, w;
        poly cp;
        keccak_state state;
        unpack_pk(rho, &t1, this->pk_bytes);
        uint8_t hash_input[CRYPTO_PUBLICKEYBYTES];
        pack_pk(hash_input, rho, &t1);
        /* Recompute tr from public information */
        shake256(tr, SEEDBYTES, hash_input, CRYPTO_PUBLICKEYBYTES);
        /*Recompute mu */
        shake256_init(&state);
        shake256_absorb(&state, tr, SEEDBYTES);
        shake256_absorb(&state, m, mlen);
        shake256_finalize(&state);
        shake256_squeeze(mu, CRHBYTES, &state);
        /* Recompute rhoprime */
        shake256(rhoprime, CRHBYTES, key, SEEDBYTES + CRHBYTES);
        polyvecl s1_ntt;
        polyveck u;
        s1_ntt = s1;
        polyvecl_ntt(&s1_ntt);
        polyvec_matrix_pointwise_montgomery(&u, this->A_ntt, &s1_ntt);
        polyveck_reduce(&u);
        polyveck_invntt_tomont(&u);
        polyveck_sub(&u, &u, &this->t1_2d);
        /* Compute CRH(tr, msg) */

        done = false;
        while (!done)
        {
            polyvecl_uniform_gamma1(&y, rhoprime, nonce++);
            polyvecl y_ntt = y;
            polyvecl_ntt(&y_ntt);
            polyvec_matrix_pointwise_montgomery(&w1, this->A_ntt, &y_ntt);
            polyveck_reduce(&w1);
            polyveck_invntt_tomont(&w1);
            w = w1;
            polyveck_caddq(&w1);
            polyveck_decompose(&w1, &w0, &w1);
            polyveck_pack_w1(sig, &w1);
            shake256_init(&state);
            shake256_absorb(&state, mu, CRHBYTES);
            shake256_absorb(&state, sig, K * POLYW1_PACKEDBYTES);
            shake256_finalize(&state);
            shake256_squeeze(sig, SEEDBYTES, &state);
            poly_challenge(&cp, sig);
            poly_ntt(&cp);
            polyvecl s1_hat = s1;
            polyvecl_ntt(&s1_hat);
            polyvecl_pointwise_poly_montgomery(&z, &cp, &s1_hat);
            polyvecl_invntt_tomont(&z);
            polyvecl_add(&z, &z, &y);
            polyvecl_reduce(&z);
            if (polyvecl_chknorm(&z, GAMMA1 - BETA))
            {
                std::cout << "Repeating, z norm too big" << std::endl;
                continue;
            }
            polyveck cu;
            polyveck_reduce(&u);
            polyveck_ntt(&u);
            polyveck_pointwise_poly_montgomery(&cu, &cp, &u);
            polyveck_invntt_tomont(&cu);
            polyveck_reduce(&cu);
            polyveck minus_cu;
            for (int i = 0; i < K; ++i)
            {
                for (int j = 0; j < 256; ++j)
                {
                    minus_cu.vec[i].coeffs[j] = reduce32((-1) * cu.vec[i].coeffs[j]);
                }
            }
            polyveck w_plus_cu;
            polyveck_add(&w_plus_cu, &w, &cu);
            unsigned int n;
            n = polyveck_make_hint(&h, &minus_cu, &w_plus_cu);
            if (n > OMEGA)
            {
                std::cout << "Too many (" << n << ") ones in hint" << std::endl;
                continue;
            }
            uint8_t sig_output[mlen + CRYPTO_BYTES];
            pack_sig(sig_output, sig, &z, &h);
            *siglen = CRYPTO_BYTES;
            if (!crypto_sign_verify(sig_output, *siglen, m, mlen, this->pk_bytes))
            {
                std::cout << "Crypto sign verify failed" << std::endl;
                std::cout << std::flush;
                continue;
            }
            else
            {
                std::cout << "Signature successfully verified" << std::endl;
                std::cout << std::flush;
                done = true;
            }
            std::cout << "done " << done << std::endl;
        }
        return 0;
    }

    Signature::Signature(const uint8_t sig[CRYPTO_BYTES])
    {
        memcpy(this->sig_bytes, sig, CRYPTO_BYTES);
        uint8_t c_seed[SEEDBYTES];
        unpack_sig(c_seed, &this->z, &this->h, sig);
        poly_challenge(&this->c_poly, c_seed);
    }

    SideChannelAttackClass::SideChannelAttackClass(const uint8_t pk[CRYPTO_PUBLICKEYBYTES])
    {
        NTL::ZZ_p::init(NTL::conv<NTL::ZZ>(Q));
        this->pkObj.reset(new PublicKey(pk));
        num_incorrect_predictions = 0;
        num_falsely_rejected = 0;
    }

    bool SideChannelAttackClass::HasEnoughSignatures()
    {
        for (int i = 0; i < K; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                if (this->idx_stored[i][j] < 3)
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool SideChannelAttackClass::CouldBeZero(const uint8_t sigBytes[CRYPTO_BYTES], int predicted_coeff)
    {
        std::shared_ptr<Signature> sigObj = std::make_shared<Signature>(sigBytes);
        for (int i = 0; i < L; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                if (abs(sigObj->z.vec[i].coeffs[j] - predicted_coeff) <= 16)
                {
                    return true;
                }
            }
        }
        return false;
    }

    void SideChannelAttackClass::AddSignature(const uint8_t sigBytes[CRYPTO_BYTES], int i, int j, int predicted_coeff, int actual_coeff)
    {
        {
            std::lock_guard<std::mutex> lk(this->mtx);
            /*if (this->idx_stored[i][j] > 20){
                //std::cout << "Already have " << i << " " << j << std::endl;
                return;
            } else {
                this->idx_stored[i][j] += 1;
            }*/
            this->idx_stored[i][j] += 1;
        }
        NTL::ZZ_p::init(NTL::conv<NTL::ZZ>(Q));
        // Calculate Az, calculate c 2^d t1, calculate Ac
        std::shared_ptr<Signature> sigObj = std::make_shared<Signature>(sigBytes);
        sigObj->pos_i = i;
        sigObj->pos_j = j;
        sigObj->actual_coeff = actual_coeff;
        polyvecl tmpz = sigObj->z;
        polyveck Az;
        polyvecl_ntt(&tmpz);
        polyvec_matrix_pointwise_montgomery(&Az, this->pkObj->A_ntt, &tmpz);
        // polyveck_reduce(&Az);
        polyveck_invntt_tomont(&Az);
        sigObj->Az = Az;
        polyveck ctimes_t1;
        poly tmpc = sigObj->c_poly;
        poly_ntt(&tmpc);
        polyveck_pointwise_poly_montgomery(&ctimes_t1, &tmpc, &(this->pkObj->t1_ntt));
        polyveck_invntt_tomont(&ctimes_t1);
        polyveck_reduce(&ctimes_t1);
        sigObj->c2dt1 = ctimes_t1;
        sigObj->az_minus_c2dt1 = std::abs(reduce32(reduce32((reduce32(Az.vec[i].coeffs[j]) - reduce32(predicted_coeff))) - reduce32(ctimes_t1.vec[i].coeffs[j])));
        sigObj->zero_coefficient = std::make_tuple(i, j);
        for (int i = 0; i < K; ++i)
        {
            polyvecl_pointwise_poly_montgomery(&(sigObj->Ac[i]), &tmpc, &this->pkObj->A_ntt[i]);
        }
        for (int i = 0; i < K; ++i)
        {
            // polyvecl_reduce(&sigObj.Ac[i]);
            polyvecl_invntt_tomont(&(sigObj->Ac[i]));
        }
        int iprime = std::get<0>(sigObj->zero_coefficient);
        int jprime = std::get<1>(sigObj->zero_coefficient);
        int sign = 1;
        sigObj->A_row.SetLength(L * N);
        sigObj->cs_row.SetLength(L * N);
        sigObj->cs_row_for_poly.SetLength(N);
        for (int l = 0; l < L; ++l)
        {
            for (int i = 0; i < N; ++i)
            {
                for (int j = 0; j < N; ++j)
                {
                    if (((i + j) % N) == jprime)
                    {
                        if ((((i + j) / N) % 2) == 0)
                        {
                            sign = 1;
                        }
                        else
                        {
                            sign = -1;
                        }
                        sigObj->A_row[(l * N) + j] += NTL::conv<NTL::ZZ_p>(freeze(sign * freeze(sigObj->Ac[iprime].vec[l].coeffs[i])));
                    }
                }
            }
        }
        for (int i = 0; i < 256; ++i)
        {
            for (int j = 0; j < 256; ++j)
            {
                if (((i + j) % 256) == jprime)
                {
                    if ((((i + j) / 256) % 2) == 0)
                    {
                        sign = 1;
                    }
                    else
                    {
                        sign = -1;
                    }
                    sigObj->cs_row[(iprime * 256) + j] += NTL::conv<NTL::ZZ_p>(freeze(sign * freeze(sigObj->c_poly.coeffs[i])));
                    sigObj->cs_row_for_poly[j] += NTL::conv<NTL::ZZ_p>(freeze(sign * freeze(sigObj->c_poly.coeffs[i])));
                }
            }
        }
        sigObj->z_minus_y = NTL::conv<NTL::ZZ_p>(freeze(freeze(sigObj->z.vec[iprime].coeffs[jprime]) - freeze(predicted_coeff)));
        // sigObj->z_minus_y_vec[iprime] = NTL::conv<NTL::ZZ_p>(freeze(freeze(sigObj->z.vec[iprime].coeffs[jprime])-freeze(predicted_coeff)));
        sigObj->b = NTL::conv<NTL::ZZ_p>(freeze(freeze(sigObj->Az.vec[iprime].coeffs[jprime]) - freeze(predicted_coeff)));
        const bool prediction_incorrect = (predicted_coeff != actual_coeff);
        if (abs(reduce32(NTL::conv<long>(sigObj->z_minus_y))) > 14)
        {
            // assert(predicted_coeff != actual_coeff);
            // std::cout << "Rejecting signature, difference is too big rejection is correct:" << correct_rejection << std::endl;
            if (actual_coeff == 0)
            {
                ++this->num_falsely_rejected;
            }
            return;
        }
        if (prediction_incorrect == true)
        {
            ++this->num_incorrect_predictions;
        }
        std::lock_guard<std::mutex> lk(this->mtx);
        this->collectedSignatures.push_back(std::move(sigObj));
    }
    std::vector<long> SideChannelAttackClass::solve_for_secret_key_y(polyvecl actual_s1)
    {

        NTL::ZZ_p::init(NTL::conv<NTL::ZZ>(Q));
        int num_collected_signatures[L] = {0};
        for (int i = 0; i < this->collectedSignatures.size(); ++i)
        {
            num_collected_signatures[this->collectedSignatures[i]->pos_i]++;
        }
        NTL::Mat<NTL::ZZ_p> A[L];
        NTL::Vec<NTL::ZZ_p> b[L];
        NTL::Vec<NTL::ZZ_p> ret_vec_zz_p;
        std::vector<long> ret_vec_long;
        int sig_counter[L] = {0};
        int wrong_counter[L] = {0};
        for (int i = 0; i < L; ++i)
        {
            std::cout << "Setting dimension " << num_collected_signatures[i] << std::endl;
            A[i].SetDims(num_collected_signatures[i], 256);
            b[i].SetLength(num_collected_signatures[i]);
        }
        for (int i = 0; i < this->collectedSignatures.size(); ++i)
        {
            int32_t iprime = this->collectedSignatures[i]->pos_i;
            int sig_index = sig_counter[iprime];
            A[iprime][sig_index] = this->collectedSignatures[i]->cs_row_for_poly;
            b[iprime][sig_index] = this->collectedSignatures[i]->z_minus_y;
            sig_counter[iprime]++;
            if (this->collectedSignatures[i]->actual_coeff != 0)
            {
                wrong_counter[iprime]++;
            }
        }
        bool failure = false;
        for (int i = 0; i < L; ++i)
        {
            std::cout << "Solving with " << wrong_counter[i] << "/" << sig_counter[i] << " (" << (1.0 * wrong_counter[i]) / sig_counter[i] << "%) wrong equations" << std::endl;
            IntegerLWE::SolveReturnStruct ret_struct = IntegerLWE::solve(A[i], b[i], &actual_s1);

            std::vector<long> ret_intermediate = ret_struct.solution;
            int wrong_count = 0;

            for (int j = 0; j < N; ++j)
            {
                ret_vec_long.push_back(ret_intermediate[j]);
                if (ret_intermediate[j] != actual_s1.vec[i].coeffs[j])
                {
                    std::cout << ret_struct.solution_not_rounded[j] << " " << actual_s1.vec[i].coeffs[j] << std::endl;
                    wrong_count += 1;
                }
                if (abs(ret_intermediate[j] - actual_s1.vec[i].coeffs[j]) > 1)
                {
                    failure = true;
                }
            }
            //if (failure)
            //{
            //    continue;
            //}
            //std::cout << "Wrong count " << i << " " << wrong_count << std::endl;
            double min_distance = 0.49;
            //std::cout << "Min distance " << min_distance << std::endl;
            std::vector<long> ret_ilp_no_hint = ILPSolver::solve_ilp(NTL::conv<NTL::Mat<NTL::ZZ>>(A[i]), NTL::conv<NTL::Vec<NTL::ZZ>>(b[i]), ret_struct, min_distance, false);
            for (int j = 0; j < N; ++j)
            {
                ret_vec_long[i * N + j] = ret_ilp_no_hint[j];
            }
        }
        return ret_vec_long;
    }
    bool SideChannelAttackClass::Oracle(polyvecl guessed_s1, polyveck t0, polyveck s2)
    {
        // printf("%d \n", guessed_s1.vec[0].coeffs[1]);
        polyvecl guessed_s1_ntt = guessed_s1;
        polyvecl_ntt(&guessed_s1_ntt);
        polyveck u;
        polyvec_matrix_pointwise_montgomery(&u, this->pkObj->A_ntt, &guessed_s1_ntt);
        polyveck_invntt_tomont(&u);
        for (int i = 0; i < K; i++)
        {
            // printf("i=%d\n", i);
            for (int j = 0; j < N; ++j)
            {
                // printf("Difference at  %d,%d,%d \n", i, j, reduce32(this->pkObj->t1_2d.vec[i].coeffs[j] - u.vec[i].coeffs[j]-s2.vec[i].coeffs[j]+t0.vec[i].coeffs[j]));
                // printf("Difference at  %d,%d,%d \n", i, j, reduce32(this->pkObj->t1_2d.vec[i].coeffs[j] - u.vec[i].coeffs[j]));
                if (abs(reduce32(this->pkObj->t1_2d.vec[i].coeffs[j] - u.vec[i].coeffs[j])) > (2 << 12 + 2))
                {
                    return false;
                }
            }
        }
        return true;
    }
}
