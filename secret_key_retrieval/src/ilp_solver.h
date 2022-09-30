#ifndef ILP_SOLVER_H
    #define ILP_SOLVER_H 
    #include "ortools/linear_solver/linear_solver.h"
    #include <NTL/ZZ_p.h>
    #include <NTL/LLL.h>
    #include <NTL/mat_ZZ_p.h>
    #include <vector>
    #include <time.h>
    #include "ref/polyvec.h"
    #include "integer_lwe.h"
    #include "ref/reduce.h"


    namespace ILPSolver {
        std::vector<long> solve_ilp(NTL::Mat<NTL::ZZ> NtlA, NTL::Vec<NTL::ZZ> Ntlb, IntegerLWE::SolveReturnStruct integer_lwe_ret_struct, double distance_threshold=0.50, bool use_hint=false);
    }
#endif