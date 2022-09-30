#include <algorithm>
#include <thread>
#include <time.h>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "side_channel_attack.h"
namespace DataGenerator {
    class DataGeneratorClass {
        public:
            std::mutex signatures_mtx;
            std::string this_out_path;
            uint8_t pk_bytes[CRYPTO_PUBLICKEYBYTES];
            uint8_t sk_bytes[CRYPTO_SECRETKEYBYTES];
            uint32_t tries_with_zeros = 0;
            std::vector<SideChannelAttack::Signature> collectedSignatures;
            void generate_data(std::string &out_path, bool reuse_keys=true);
            void dump_data(std::string &out_path);
            void dump_keys(std::string &out_path);
            void sign_random_messages(int thread_number);
            void dump_signature(SideChannelAttack::Signature &sigObj, std::string &out_path);
            void dump_y_data(std::string &out_path);
    };
}