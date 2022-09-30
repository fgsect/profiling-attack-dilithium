#include "ilp_solver.h"
namespace ILPSolver {
    std::vector<long> solve_ilp(NTL::Mat<NTL::ZZ> NtlA, NTL::Vec<NTL::ZZ> Ntlb, IntegerLWE::SolveReturnStruct integer_lwe_ret_struct,double distance_threshold, bool use_hint){
        // Create the mip solver with the SCIP backend.
        int m = NtlA.NumRows();
        int n = NtlA.NumCols();
        std::unique_ptr<operations_research::MPSolver> solver(operations_research::MPSolver::CreateSolver("SCIP"));
        if (!solver) {
            LOG(WARNING) << "SCIP solver unavailable.";
            std::vector<long> s;
            return s;
        }

        const double infinity = solver->infinity();
        std::vector<const operations_research::MPVariable*> x(m);
        std::vector<operations_research::MPVariable*> s(n);
        for (int i = 0; i < m; ++i) {
            x[i] = solver->MakeBoolVar("");
        }
        for (int j = 0; j < n; ++j){
            s[j] = solver->MakeIntVar(-1*ETA,ETA,"");
        }
        //Add the big constraints
        //b - c_i s \leq M*(1-x) <> b - c_i s \leq M-MX
        //b - c_i s \geq -M*(1-x) <> b - c_i \ geq -M+MX
        int M = 200;
        for(int i=0;i < m;++i){
            operations_research::LinearExpr lin_exp_rhs = M*(1-operations_research::LinearExpr(x[i])); //-Mx
            operations_research::LinearExpr lin_exp_lhs;
            std::vector<int64_t> c_coeffs;
            for(int j=0;j<n;++j){
                lin_exp_lhs += reduce32(NTL::conv<int32_t>(NtlA[i][j]))*operations_research::LinearExpr(s[j]);
                //c_coeffs.push_back((-1)*reduce32(NTL::conv<int32_t>(NtlA[i][j])));
            }
            lin_exp_lhs = reduce32(NTL::conv<int32_t>(Ntlb[i]))-lin_exp_lhs;
            operations_research::MPConstraint* constraint = solver->MakeRowConstraint(lin_exp_lhs <= lin_exp_rhs);
            lin_exp_rhs = (-1)*M*(1-operations_research::LinearExpr(x[i]));
            constraint = solver->MakeRowConstraint(lin_exp_lhs >= lin_exp_rhs);
            //std::cout << "Constraint " << constraint << std::endl;
            //MPConstraint* constraint = solver->MakeRowConstraint(LinearExpr::Mul, data.bounds[i], "");
        }
        if (use_hint){
            std::vector<std::pair<const operations_research::MPVariable*, double>> start_hint;
            for(int j=0;j<n;++j){
                double distance_to_zero_dot_five = std::abs((0.5-std::abs((integer_lwe_ret_struct.solution_not_rounded[j]-floor(integer_lwe_ret_struct.solution_not_rounded[j])))));
                if (distance_to_zero_dot_five > distance_threshold){
                    s[j]->SetLB(integer_lwe_ret_struct.solution[j]);
                    s[j]->SetUB(integer_lwe_ret_struct.solution[j]);
                }
                start_hint.push_back(std::make_pair(s[j], int(integer_lwe_ret_struct.solution[j])));
                if (integer_lwe_ret_struct.solution_not_rounded[j]>0){
                    s[j]->SetLB(int(integer_lwe_ret_struct.solution_not_rounded[j]));
                    s[j]->SetUB(int(integer_lwe_ret_struct.solution_not_rounded[j])+1);
                } else {
                    s[j]->SetLB(int(integer_lwe_ret_struct.solution_not_rounded[j])-1);
                    s[j]->SetUB(int(integer_lwe_ret_struct.solution_not_rounded[j]));
                }
            }
        }
        operations_research::MPObjective* const objective = solver->MutableObjective();
        for(int i=0;i<m;++i){
            objective->SetCoefficient(x[i], 1);
        }
        objective->SetMaximization();
        operations_research::LinearExpr sum_constraint_lhs;
        for(int i=0;i<m;++i){
            sum_constraint_lhs += x[i];
        }
        //operations_research::MPConstraint* constraint = solver->MakeRowConstraint(sum_constraint_lhs >= int(0.75*m));
        solver->SetTimeLimit(absl::Minutes(30));
        //solver->SetHint(start_hint);
        solver->EnableOutput();
        operations_research::MPSolverParameters *RG = new operations_research::MPSolverParameters();
        RG->SetDoubleParam(operations_research::MPSolverParameters::DoubleParam::RELATIVE_MIP_GAP, 0.58);
        const operations_research::MPSolver::ResultStatus result_status = solver->Solve(*RG);
        // Check that the problem has an optimal solution.
        if (result_status != operations_research::MPSolver::OPTIMAL) {
            LOG(INFO) << "The problem does not have an optimal solution!";
        }

        LOG(INFO) << "Solution:";
        //LOG(INFO) << "Objective value = " << objective->Value();
        std::vector<long> ret_value;
        for(int j=0;j<n;++j){
            ret_value.push_back(long(s[j]->solution_value()));
        }
        return ret_value;


    }
}
