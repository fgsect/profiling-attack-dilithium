#include "data_generator.h"
#include "side_channel_attack.h"
#include "ref/params.h"
#include "ref/global_variables.h"
#include "ref/randombytes.h"
#include "ref/sign.h"
#include "ref/rounding.h"
#define MLEN 59
#define PREDICTED_COEFF 0

std::mutex mtx;



namespace DataGenerator
{
    void read_to_byte_array(const char *filepath, uint8_t *out){
        std::ifstream input_stream( filepath, std::ios::binary );
        std::vector<char> buffer(std::istreambuf_iterator<char>(input_stream), {});
        copy(buffer.begin(), buffer.end(), out);
    }
    void DataGeneratorClass::sign_random_messages(int thread_number)
    {
        size_t mlen, smlen;
        uint8_t sm[MLEN + CRYPTO_BYTES];
        uint8_t m[MLEN + CRYPTO_BYTES];
        while (true)
        {
            if (thread_number == 1)
            {
                std::cout << "\rAt try " << try_number << ", " << this->tries_with_zeros << "/" << try_number << " tries with 0 coefficient";
                std::cout << std::flush;
            }
            randombytes(m, MLEN);
            crypto_sign(sm, &smlen, m, MLEN, this->sk_bytes);
            /*if (thread_number == 1)
            {
                for (int j = 0; j < 4; ++j)
                {
                    for (int h = 0; h < 4; ++h)
                    {
                        std::cout << "gamma1 " << GAMMA1 <<  " y_coeff " << global_current_round_y.vec[j].coeffs[h] << std::endl;
                        std::cout << std::flush;
                    }
                }
            }*/
            ++try_number;
            if (!SideChannelAttack::SideChannelAttackClass::CouldBeZero(sm, PREDICTED_COEFF))
            {
                continue;
            }
            SideChannelAttack::Signature sigObj(sm);
            int save_round_to_file = 0;
            sigObj.w = global_current_round_w;
            sigObj.y = global_current_round_y;
            memcpy(sigObj.rhoprime, global_current_round_rhoprime, 64);
            sigObj.nonce = global_current_round_nonce;

            //memcpy(sigObj.seedbuf, global_current_round_seedbuf, 680);
            //memcpy(sigObj.rhoprime, global_current_round_rhoprime, 64);
            for (int j = 0; j < L; ++j)
            {
                for (int h = 0; h < N; ++h)
                {
                    if (global_current_round_y.vec[j].coeffs[h] == PREDICTED_COEFF)
                    {
                        {
                            std::lock_guard<std::mutex> lk(mtx);
                            tries_with_zeros++;
                        }
                    }
                }
            }
            /*for (int j = 0; j < L; ++j)
            {
                for (int h = 0; h < N; ++h)
                {
                    if (global_current_round_y.vec[0].coeffs[0] == 0)
                    {
                        {
                            std::lock_guard<std::mutex> lk(mtx);
                            this->tries_with_zeros++;
                        }
                    }
                }
            }*/
            {
                std::lock_guard<std::mutex> lk(this->signatures_mtx);
                this->collectedSignatures.push_back(sigObj);
            }

            if (this->tries_with_zeros >= 2700)
            {
                break;
            }
        }
    }

    void DataGeneratorClass::generate_data(std::string &out_path, bool reuse_keys)
    {
        this->this_out_path = out_path;
        uint8_t pk[CRYPTO_PUBLICKEYBYTES];
        uint8_t sk[CRYPTO_SECRETKEYBYTES];
        srand(time(NULL));
        if (reuse_keys == false){

            crypto_sign_keypair(pk, sk);
            memcpy(this->pk_bytes, pk, CRYPTO_PUBLICKEYBYTES);
            memcpy(this->sk_bytes, sk, CRYPTO_SECRETKEYBYTES);
            this->dump_keys(out_path);
        } else {
            std::string pkPath = out_path + "/pk.bin";
            std::string skPath = out_path + "/sk.bin";
            read_to_byte_array(pkPath.c_str(), pk);
            read_to_byte_array(skPath.c_str(), sk);
        }
        const auto processor_count = std::thread::hardware_concurrency() - 3;
        std::vector<std::thread> v;
        for (int i = 0; i < processor_count; ++i)
        {
            //printf("creating new thread %d\n", i);
            std::thread t([this, i]
                          { this->sign_random_messages(i); });
            v.push_back(std::move(t));
        }
        for (std::thread &th : v)
        {
            if (th.joinable())
            {
                th.join();
            }
        }
        std::string y_out_path = out_path + "/y_polys.bin";
        std::cout << "y_out_path " << y_out_path << std::endl;
        this->dump_y_data(y_out_path);
        std::string z_out_path = out_path + "/signatures.bin";
        std::ofstream file(z_out_path, std::ios::binary | std::ios::app);
        for (std::size_t idx = 0; idx < this->collectedSignatures.size(); ++idx)
        {
            file.write((char *)(&this->collectedSignatures[idx].sig_bytes), CRYPTO_BYTES);
            /*for (int i = 0; i < L; ++i)
            {
                for (int h = 0; h < N; ++h)
                {
                    file.write((char *)(&this->collectedSignatures[idx].z.vec[i].coeffs[h]), 4);
                }
            }*/
        }
    }
    void DataGeneratorClass::dump_signature(SideChannelAttack::Signature &sigObj, std::string &out_path)
    {
        //For each signature, store:
        //z, c, w, w_decompose
        nlohmann::json output_json = nlohmann::json::array();
        //std::cout << "\r Serializing " << idx << "/" << this->collectedSignatures.size() << " signature";
        //std::vector<std::vector<int32_t>> z_coefficients(L);
        std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> w_coefficients(K);
        //std::vector<int32_t> c_coefficients;
        /*for (int i = 0; i < L; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                z_coefficients[i].push_back(freeze(sigObj.z.vec[i].coeffs[j]));
            }
        }
        for (int i = 0; i < N; ++i)
        {
            c_coefficients.push_back(freeze(sigObj.c_poly.coeffs[i]));
        }*/
        for (int i = 0; i < K; ++i)
        {
            for (int j = 0; j < N; ++j)
            {
                int32_t w1;
                int32_t w0;
                w1 = decompose(&w0, sigObj.w.vec[i].coeffs[j]);
                w_coefficients[i].push_back(std::make_tuple(sigObj.w.vec[i].coeffs[j], w1, w0));
            }
        }
        nlohmann::json signature_map = nlohmann::json();
        //signature_map["z_coefficients"] = z_coefficients;
        signature_map["w_coefficients"] = w_coefficients;
        //signature_map["c_coefficients"] = c_coefficients;
        signature_map["signature_bytes"] = sigObj.sig_bytes;
        {
            std::lock_guard<std::mutex> lk(mtx);
            std::ofstream outfile;
            //std::string signatures_out_path = out_path + "/signatures.json";
            outfile.open(out_path, std::ios_base::app); // append instead of overwrite
            outfile << signature_map.dump() << std::endl;
        }
    }

    void DataGeneratorClass::dump_keys(std::string &out_path)
    {
        std::string pkPath = out_path + "/pk.bin";
        std::string skPath = out_path + "/sk.bin";
        {
            std::cout << "Writing to " << pkPath << std::endl;
            std::ofstream file(pkPath, std::ios::binary);
            file.write((char *)this->pk_bytes, CRYPTO_PUBLICKEYBYTES);
        }
        {
            std::ofstream file(skPath, std::ios::binary);
            file.write((char *)this->sk_bytes, CRYPTO_SECRETKEYBYTES);
        }
    }

    void DataGeneratorClass::dump_y_data(std::string &out_path)
    {
        //Dump like so:
        //rhoprime||nonce||y_coefficents
        std::cout << "Dumping y_data " << out_path << std::endl;

        std::ofstream file(out_path, std::ios::binary | std::ios::app);
        for (std::size_t idx = 0; idx < this->collectedSignatures.size(); ++idx)
        {
            /*bool dump_y = false;
            for(int j=0;j<4;++j){

                if (this->collectedSignatures[idx].y.vec[0].coeffs[j]==0) {
                    dump_y = true;
                }
            }
            if (dump_y == false){
                continue;
            }*/
            if (idx <= 1)
            {
                std::cout << "Signature " << idx << std::endl;
                std::cout << "nonce " << this->collectedSignatures[idx].nonce << std::endl;
                std::cout << "coefficient" << this->collectedSignatures[idx].y.vec[0].coeffs[0] << std::endl;
            }
            file.write((char *)this->collectedSignatures[idx].rhoprime, 64);
            file.write((char *)(&this->collectedSignatures[idx].nonce), 2);
            for (int i = 0; i < L; ++i)
            {
                for (int h = 0; h < N; ++h)
                {
                    file.write((char *)(&this->collectedSignatures[idx].y.vec[i].coeffs[h]), 4);
                }
            }
            for (int i = 0; i < L; ++i)
            {
                for (int h = 0; h < N; ++h)
                {
                    uint8_t could_be_zero = 0;
                    if (abs(reduce32(this->collectedSignatures[idx].z.vec[i].coeffs[h])) <= 14  )
                    {
                        could_be_zero = 1;
                    }
                    file.write((char *)(&could_be_zero), 1);
                }
            }
        }

    }

    void DataGeneratorClass::dump_data(std::string &out_path)
    {
        std::string pkPath = out_path + "/pk.bin";
        std::string skPath = out_path + "/sk.bin";
        {
            std::ofstream file(pkPath, std::ios::binary);
            file.write((char *)this->pk_bytes, CRYPTO_PUBLICKEYBYTES);
        }
        {
            std::ofstream file(skPath, std::ios::binary);
            file.write((char *)this->sk_bytes, CRYPTO_SECRETKEYBYTES);
        }
        std::string signaturePath = out_path + "/signatures.json";
        //For each signature, store:
        //z, c, w, w_decompose
        nlohmann::json output_json = nlohmann::json::array();
        for (std::size_t idx = 0; idx < this->collectedSignatures.size(); ++idx)
        {
            SideChannelAttack::Signature sigObj = this->collectedSignatures[idx];
            std::cout << "\r Serializing " << idx << "/" << this->collectedSignatures.size() << " signature";
            std::vector<std::vector<int32_t>> z_coefficients(L);
            std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> w_coefficients(K);
            std::vector<int32_t> c_coefficients;
            for (int i = 0; i < L; ++i)
            {
                for (int j = 0; j < N; ++j)
                {
                    z_coefficients[i].push_back(freeze(sigObj.z.vec[i].coeffs[j]));
                }
            }
            for (int i = 0; i < N; ++i)
            {
                c_coefficients.push_back(freeze(sigObj.c_poly.coeffs[i]));
            }
            for (int i = 0; i < K; ++i)
            {
                for (int j = 0; j < N; ++j)
                {
                    int32_t w1;
                    int32_t w0;
                    w1 = decompose(&w0, sigObj.w.vec[i].coeffs[j]);
                    w_coefficients[i].push_back(std::make_tuple(sigObj.w.vec[i].coeffs[j], w1, w0));
                }
            }
            nlohmann::json signature_map = nlohmann::json();
            signature_map["z_coefficients"] = z_coefficients;
            signature_map["w_coefficients"] = w_coefficients;
            signature_map["c_coefficients"] = c_coefficients;
            {
                std::ofstream outfile;
                std::string signatures_out_path = out_path + "/signatures.json";
                outfile.open(signatures_out_path, std::ios_base::app); // append instead of overwrite
                outfile << signature_map.dump() << std::endl;
            }
        }
        std::string y_polys_path = out_path + "/y_polys.bin";
        this->dump_y_data(y_polys_path);
    }
}
