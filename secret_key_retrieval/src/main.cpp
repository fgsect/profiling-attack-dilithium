// clang++ -g -std=c++11 -pthread main.cpp -o main -lntl -lgmp -lm
#include <NTL/ZZ_p.h>
#include <NTL/LLL.h>
#include <NTL/mat_ZZ_p.h>
#include "side_channel_attack.h"
#include <thread> // std::thread
#include <mutex>  // std::mutex
#include <ctime>
#include <vector>
#include <time.h>
#include <fstream>
#include <sstream>
#include <set>
#include <string>
// extern "C" {
#include "ref/params.h"
#include "ref/sign.h"
#include "ref/global_variables.h"
#include "ref/randombytes.h"
//}

#define MLEN 59
#define NTESTS 2147483647
// std::mutex mtx;
int global_collect_signatures = 3000;
double global_false_positive = 0.9;
double global_true_positive = 0.9;
int global_false_positive_threshold = 0;
int global_true_positive_threshold = 0;
int MAX_RANGE_UNIFORM_DISTRIB = 100000;
int global_threshold = 5;
int global_equations = 0;

int sign_random_messages(uint8_t *pk, uint8_t *sk, SideChannelAttack::SideChannelAttackClass *side_channel_attack, int thread_number)
{
    std::set<int> predicted_coefficients = {0}; //{0,5904383,5904384,8380416};
    size_t mlen, smlen;
    uint8_t sm[MLEN + CRYPTO_BYTES];
    uint8_t m[MLEN + CRYPTO_BYTES];
    std::random_device rd;  // Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(1, MAX_RANGE_UNIFORM_DISTRIB);
    // int noise = 0;
    while (true)
    {
        // printf("\rAt try %d, %d/%d tries with 0 coefficient", try_number, side_channel_attack->collectedSignatures.size(), try_number);
        // fflush(stdout);
        if (thread_number == 1)
        {
            std::cout << "\rAt try " << try_number << ", " << side_channel_attack->collectedSignatures.size() << "/" << try_number << " tries with 0 coefficient, need to save " << global_save_signatures << " tries";
            std::cout << std::flush;
        }
        randombytes(m, MLEN);
        crypto_sign(sm, &smlen, m, MLEN, sk);
        SideChannelAttack::Signature sig(sm);
        if (side_channel_attack->CouldBeZero(sm, 0))
        {
            ++global_save_signatures;
        }
        else
        {

            ++try_number;
            continue;
        }
        int save_round_to_file = 0;
        for (int j = 0; j < L; ++j)
        {
            for (int h = 0; h < N; h += 1)
            {
                if (abs(reduce32(sig.z.vec[j].coeffs[h])) > global_threshold)
                {
                    if (global_current_round_y.vec[j].coeffs[h] == 0)
                    {
                        ++global_wrongly_rejected;
                    }
                    continue;
                }
                else
                {
                    ++global_make_predictions;
                }
                int random_number = distrib(gen);
                const bool is_in = predicted_coefficients.find(global_current_round_y.vec[j].coeffs[h]) != predicted_coefficients.end();
                if (is_in && random_number <= global_true_positive_threshold)
                {
                    side_channel_attack->AddSignature(sm, j, h, global_current_round_y.vec[j].coeffs[h], global_current_round_y.vec[j].coeffs[h]);
                }
                else
                {
                    if (random_number <= global_false_positive_threshold) 
                    {
                        side_channel_attack->AddSignature(sm, j, h, 0, global_current_round_y.vec[j].coeffs[h]);
                    }
                }
            }
        }

        //if (side_channel_attack->collectedSignatures.size() >= global_collect_signatures)
        if (try_number >= global_collect_signatures)
        {
            //std::cout << "Added " << noise << " wrong equations" << std::endl;
            break;
        }
        /*if (side_channel_attack->HasEnoughSignatures()){
            break;
        }*/
        // mtx.lock();
        ++try_number;
        // mtx.unlock();
    }
    if (thread_number == 1)
    {
        global_noise = side_channel_attack->num_incorrect_predictions;
        global_equations = side_channel_attack->collectedSignatures.size();
        std::cout << std::endl;
        std::cout << "Have " << side_channel_attack->num_incorrect_predictions << " wrong equations"
                  << "out of " << side_channel_attack->collectedSignatures.size() << " and " << try_number << " tries" << std::endl;
        std::cout << "Need to save " << global_save_signatures << " and wrongly rejected" << global_wrongly_rejected << " coefficients" << std::endl;
        std::cout << "Need to make " << global_make_predictions << " predictions" << std::endl;
    }
    return 0;
}

int try_with_n_signatures(int num_sigs)
{
    global_save_signatures = 0;
    global_make_predictions = 0;
    global_wrongly_rejected = 0;
    global_equations = 0;
    global_collect_signatures = num_sigs;
    try_number = 0;
    srand(time(NULL));
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    crypto_sign_keypair(pk, sk);
    uint8_t rho[SEEDBYTES];
    uint8_t tr[SEEDBYTES];
    uint8_t key[SEEDBYTES];
    polyveck s2, t0;
    polyvecl s1;
    unpack_sk(rho, tr, key, &t0, &s1, &s2, sk);
    SideChannelAttack::SideChannelAttackClass side_channel_attack(pk);
    size_t i;
    size_t smlen;
    uint8_t sm[MLEN + CRYPTO_BYTES];
    uint8_t m[MLEN + CRYPTO_BYTES];
    randombytes(m, MLEN);
    for (i = 0; i < MLEN; ++i)
    {
        sm[CRYPTO_BYTES + MLEN - 1 - i] = m[MLEN - 1 - i];
    }
    printf("Passing pointer to array %p\n", sm);
    side_channel_attack.pkObj->crypto_sign_signature_s1(sm, &smlen, sm + CRYPTO_BYTES, MLEN, s1);
    printf("Oracle returns for real s1 %d\n", side_channel_attack.Oracle(s1, t0, s2));
    s1.vec[0].coeffs[0] -= 1;
    printf("Oracle returns for false s1 %d\n", side_channel_attack.Oracle(s1, t0, s2));
    s1.vec[0].coeffs[0] += 1;
    // return 0;
    smlen += MLEN;
    const auto processor_count = std::thread::hardware_concurrency() - 3;
    std::vector<std::thread> v;
    for (int i = 0; i < processor_count; ++i)
    {
        printf("creating new thread %d\n", i);
        std::thread t(sign_random_messages, pk, sk, &side_channel_attack, i);
        v.push_back(std::move(t));
    }
    for (std::thread &th : v)
    {
        if (th.joinable())
        {
            th.join();
        }
    }

    std::vector<long> guessed_s;
    bool done = false;
    while (!done)
    {
        std::cout << std::endl
                  << "Invoking solve for secret key " << std::endl;
        guessed_s = side_channel_attack.solve_for_secret_key_y(s1);
        done = true;
    }
    polyvecl guessed_s1_polyvec;
    for (int i = 0; i < L; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            guessed_s1_polyvec.vec[i].coeffs[j] = guessed_s[(i * 256) + j];
        }
    }
    printf("Oracle for secret key: %d\n", side_channel_attack.Oracle(guessed_s1_polyvec, t0, s2));
    int num_incorrect = 0;
    for (int i = 0; i < L; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            if (guessed_s[(i * 256) + j] != s1.vec[i].coeffs[j])
            {
                printf("%d,%d, guessed: %d, real: %d\n", i, j, guessed_s[(i * 256) + j], s1.vec[i].coeffs[j]);
                num_incorrect++;
            }
        }
    }
    printf("Number incorrect coefficients: %d\n", num_incorrect);
    if (num_incorrect == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int main(int argc, char *argv[])
{
    int num_signatures = 0;
    if (argc == 5)
    {
        global_false_positive = atof(argv[1]); //
        global_true_positive = atof(argv[2]);
        global_threshold = atoi(argv[3]);
        num_signatures = atoi(argv[4]);
    }
    else {
        std::cout << "Usage: ./main <false-positive-rate> <true-positive-rate> <filter-threshold>" << std::endl;
        return 0;
    }
    global_true_positive_threshold = int(MAX_RANGE_UNIFORM_DISTRIB * global_true_positive);
    global_false_positive_threshold = int(MAX_RANGE_UNIFORM_DISTRIB * global_false_positive);
    std::cout << "global true positive threshold " << global_true_positive_threshold << std::endl;
    std::cout << "global false positive threshold " << global_false_positive_threshold << std::endl;
    std::string out_path = "ilp_evaluation.csv";
    std::ofstream outfile(out_path);
    outfile << "SecurityLevel;NumEquations;Recall;Precision;Treshold;NumSignatures;NumClassifications;WrongEquations;Success";
    int max_tries = 30;
    for(int i=0;i<max_tries;++i){

	    int success = try_with_n_signatures(num_signatures);
	    outfile << std::endl;
	    outfile << "3;" << global_equations << ";" << global_true_positive << ";" << global_false_positive << ";" << global_threshold << ";";
	    outfile << try_number << ";" << global_make_predictions << ";" << global_noise << ";" << success;
    }
    /*for (int num_equations = 1024; num_equations < 10000; num_equations += 500)
    {
        // for(int num_equations=900000;num_equations>0;num_equations-=100000){
        int num_success = 0;
        bool success = false;
        for (int i = 0; i < 30; ++i)
        {
            if (try_with_n_equations(num_equations) == 1)
            {
                ++num_success;
                success = true;
            }
            outfile << std::endl;
            outfile << "2;" << num_equations << ";" << global_true_positive << ";" << global_false_positive << ";" << global_threshold << ";";
            // outfile << "2;" << global_equations << ";" << global_recall << ";" << global_precision << ";" << global_threshold << ";";
            // outfile << num_equations << ";" << global_make_predictions << ";" <<  global_noise << ";" << success;
            outfile << try_number << ";" << global_make_predictions << ";" << global_noise << ";" << success;
        }
    }*/

    // std::cout << "Number successes " << num_success << std::endl;
    return 0;
}
