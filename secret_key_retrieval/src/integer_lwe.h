#ifndef INTEGER_LWE_H
    #define INTEGER_LWE_H 

    #include <NTL/ZZ_p.h>
    #include <NTL/LLL.h>
    #include <NTL/mat_ZZ_p.h>
    #include <vector>
    #include <time.h>
    #include "ref/polyvec.h"
    namespace IntegerLWE {
        struct SolveReturnStruct {
            std::vector<long> solution;
            std::vector<size_t> idx_sorted_by_uncertainty; 
            std::vector<double> solution_not_rounded;
        };
        SolveReturnStruct solve_armadillo(NTL::Mat<NTL::ZZ_p> NtlA, NTL::Vec<NTL::ZZ_p> Ntlb,  polyvecl *actual_s1 = NULL);
        SolveReturnStruct solve(NTL::Mat<NTL::ZZ_p> NtlA, NTL::Vec<NTL::ZZ_p> Ntlb,  polyvecl *actual_s1 = NULL);
        SolveReturnStruct solve_with_weights(NTL::Mat<NTL::ZZ_p> NtlA, NTL::Vec<NTL::ZZ_p> Ntlb, std::vector<double> equation_weight, polyvecl *actual_s1);
    }
#endif