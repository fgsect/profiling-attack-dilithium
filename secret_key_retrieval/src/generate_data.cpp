#include "data_generator.h"
#include "ref/params.h"
int main(int argc, char *argv[]) {
    std::string out_path = argv[1]; 
    DataGenerator::DataGeneratorClass dataGeneratorObj = DataGenerator::DataGeneratorClass();
    dataGeneratorObj.generate_data(out_path, false);
   
}