#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <set>
#include <nlohmann/json.hpp>

#include "side_channel_attack.h"
//extern "C" {
    #include "ref/params.h"
//}
void read_to_byte_array(const char *filepath, uint8_t *out){
    std::ifstream input_stream( filepath, std::ios::binary );
    std::vector<char> buffer(std::istreambuf_iterator<char>(input_stream), {});
    copy(buffer.begin(), buffer.end(), out);
}

int main(int argc, char *argv[]){
    int noise_level = 0;
    std::string out_path; 
    std::string predicted_y_path; //out_path + "/predicted_coeffs.txt";
    if (argc >= 3){
        out_path = argv[1];
        predicted_y_path = argv[2];
    } else {
        std::cout << "Usage: ./build/execute_attack <path to poc_data> <path to predicted_zero.txt> <inject noise>";
    }
    if (argc>=4){
        noise_level = atoi(argv[3]);
    }
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(1, 100);
    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    
    std::string pkPath = out_path + "/pk.bin";
    std::string skPath = out_path + "/sk.bin";
    std::string signaturesPath = out_path + "/signatures.bin";
    std::string y_poly_path = out_path + "/y_polys.bin";

    std::cout << predicted_y_path << std::endl;
    read_to_byte_array(pkPath.c_str(), pk);
    read_to_byte_array(skPath.c_str(), sk);
    SideChannelAttack::SideChannelAttackClass sideChannelAttackObj(pk);
    uint8_t rho[SEEDBYTES];
    uint8_t tr[SEEDBYTES];
    uint8_t key[SEEDBYTES];
    polyveck s2, t0;
    polyvecl s1;
    unpack_sk(rho, tr, key, &t0, &s1, &s2, sk);
    sideChannelAttackObj.t0 = t0;
    sideChannelAttackObj.s2 = s2;
    std::ifstream signatures_ifstream(signaturesPath, std::ios::binary);
    std::ifstream actual_y_ifstream(y_poly_path, std::ios::binary);
    std::ifstream predicted_y_ifstream(predicted_y_path);
    std::string current_line;
    std::map<int, std::set<std::tuple<int,int>>> predicted_coeffs;
    while (std::getline(predicted_y_ifstream, current_line, '\n')) {
        std::cout << current_line << std::endl;
            std::istringstream line_stream(current_line);
        std::string token;
            int current_signature_count;
            int poly_idx;
            int coeff_idx;
        std::getline(line_stream, token, ';');
        std::istringstream(token) >> current_signature_count;
        std::getline(line_stream, token, ';');
        std::istringstream(token) >> poly_idx;
        std::getline(line_stream, token, ';');
        std::istringstream(token) >> coeff_idx;
        std::cout << "Adding to predicted coeffs " << current_signature_count << " " << poly_idx << " " << coeff_idx << std::endl;
            predicted_coeffs[current_signature_count].insert(std::make_tuple(poly_idx, coeff_idx));
    }
    int signature_count = 0;
    char current_signatures_bytes[CRYPTO_BYTES];

    std:: cout << "predicted coeffs size " << predicted_coeffs.size() << std::endl;
    while(signatures_ifstream.read(current_signatures_bytes, CRYPTO_BYTES)){
        if (predicted_coeffs.count(signature_count) > 0){
            for(auto elem: predicted_coeffs[signature_count]){
		std::cout << " Adding signature " << std::get<0>(elem) << " " << std::get<1>(elem) << std::endl;
                sideChannelAttackObj.AddSignature((uint8_t*)current_signatures_bytes, std::get<0>(elem), std::get<1>(elem), 0, 0);
            }
        }
        SideChannelAttack::Signature this_sig((uint8_t*)current_signatures_bytes);
        if (predicted_coeffs.size() == 0){
            actual_y_ifstream.seekg(64,std::ios_base::cur);
            actual_y_ifstream.seekg(2,std::ios_base::cur);
            for (int i = 0; i < L; ++i)
            {
                for (int h = 0; h < N; ++h)
                {
                    actual_y_ifstream.read((char *)(&this_sig.y.vec[i].coeffs[h]), 4);
                }
            }
            actual_y_ifstream.seekg(L*N, std::ios_base::cur);
            if (signature_count <= 1)
            {
                std::cout << "Signature" << signature_count << " coefficient " << this_sig.y.vec[0].coeffs[0] << std::endl;
            }
            if (predicted_coeffs.size() == 0) {
                for(int j =0;j<L;++j){
                    for(int h=0;h<N;++h){
                        if (this_sig.y.vec[j].coeffs[h] == 0) {
                        std::cout << "Correct y: Adding signature " << j << " " <<h << std::endl;
                        sideChannelAttackObj.AddSignature((uint8_t*)current_signatures_bytes, j,h, 0, 0); 
                        }
                    }
                }
            }
        }
        //std::cout << "y stream " << actual_y_ifstream.tellg() << std::endl;
        for(int j =0;j<L;++j){
            int num_correct_signatures = sideChannelAttackObj.collectedSignatures.size()-sideChannelAttackObj.num_incorrect_predictions;
            if (num_correct_signatures <= 0){
                break;
            }

            if (noise_level == 0){
                break;
            }
            if (!((100*(1.0 * sideChannelAttackObj.num_incorrect_predictions/(1.0*num_correct_signatures))) <= 1.0*noise_level)) {
                break;
            }
            for(int h=0;h<N;++h){
                num_correct_signatures = sideChannelAttackObj.collectedSignatures.size()-sideChannelAttackObj.num_incorrect_predictions;
                if (num_correct_signatures <= 0){
                    break;
                }
                int random_number = distrib(gen);
                if ((100*(1.0 * sideChannelAttackObj.num_incorrect_predictions/(1.0*num_correct_signatures))) <= 1.0*noise_level) {
                    if(random_number<=5 && (100*(1.0 * sideChannelAttackObj.num_incorrect_predictions/(1.0*num_correct_signatures))) <= 1.0*noise_level) {
                        //Add noise
                        //std::cout << " Adding noise " << std::endl;
                        //++global_noise;
                        //std::uniform_int_distribution<> distrib(1, 100);
                        //int y_coeff_actual = distrib()
                        sideChannelAttackObj.AddSignature((uint8_t*)current_signatures_bytes, j, h, 0,5);
                    }
                } else {
                    break;
                }
            }
        }
	    signature_count += 1;
        /*if (signature_count >= 3000){
            break;
        }*/
    }
    int num_correct_signatures = sideChannelAttackObj.collectedSignatures.size()-sideChannelAttackObj.num_incorrect_predictions;
    std::cout << "Executing attack with noise level " << noise_level << " " << num_correct_signatures << " corret signatures " << sideChannelAttackObj.num_incorrect_predictions << " incorrect signatures" << std::endl;
    std::vector<long> guessed_s;
    bool done = false;
    while (!done){
        std::cout << std::endl << "Invoking solve for secret key " << std::endl;
        guessed_s = sideChannelAttackObj.solve_for_secret_key_y(s1);
        done = true;
    }
    polyvecl guessed_s1_polyvec;
    for(int i=0;i<L;++i){
        for(int j=0;j<N;++j){
            guessed_s1_polyvec.vec[i].coeffs[j] = guessed_s[(i*256)+j];
        }
    }
    printf("Oracle for secret key: %d\n", sideChannelAttackObj.Oracle(guessed_s1_polyvec, t0, s2));
    int num_incorrect = 0;
    for(int i=0;i<L;++i){
        for(int j=0;j<N;++j){
            if (guessed_s[(i*256)+j] != s1.vec[i].coeffs[j]){
                printf("%d,%d, guessed: %d, real: %d\n",i,j, guessed_s[(i*256)+j], s1.vec[i].coeffs[j] );
                num_incorrect++;
            }
        }
    }
    printf("Wrong count %d\n", num_incorrect);
    return 0;
}
