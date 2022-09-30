
#include <Eigen/Dense>
#include <armadillo>
#include "integer_lwe.h"
#include <iostream>
#include "ref/reduce.h"
#include "ref/polyvec.h"
#include <cmath>
#include <numeric>


namespace IntegerLWE{


    template <typename T>
    std::vector<size_t> sort_indexes(const std::vector<T> &v) {

        // initialize original index locations
        std::vector<size_t> idx(v.size());
        iota(idx.begin(), idx.end(), 0);

        // sort indexes based on comparing values in v
        // using std::stable_sort instead of std::sort
        // to avoid unnecessary index re-orderings
        // when v contains elements of equal values 
        stable_sort(idx.begin(), idx.end(),
            [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});
        /*for(int i=0;i<1024;++i){
            std::cout << " idx " << idx[i] << " vector " << v[idx[i]] << std::endl;
        }*/
        return idx;
    }

    template<typename T>
    std::vector<long> scale(T solutions_vector, polyvecl *actual_s1){
        //https://stats.stackexchange.com/questions/281162/scale-a-number-between-a-range
	std::cout << "Scaling to " << -ETA << " to " << ETA << std::endl;
        double min_measurement = 100;
        double max_measurement = 0;
        double desired_min = -1.0*ETA;
        double desired_max = 1.0*ETA;
        for(int i=0; i<solutions_vector.size();++i){
            if (solutions_vector(i) < min_measurement){
                min_measurement = solutions_vector(i);
            }
            if (solutions_vector(i) > max_measurement){
                max_measurement = solutions_vector(i);
            }
        }
        std::vector<long> result_vec;
        std::vector<int> buckets(10, 0);
        for(int i=0; i<solutions_vector.size();++i){
            double m = ((solutions_vector(i)-min_measurement)/(max_measurement-min_measurement))*(desired_max-desired_min)+desired_min;
             if (solutions_vector(i) < -ETA){
                m = -1.0*ETA; 
            } else if(solutions_vector(i) > ETA){
                m = 1.0*ETA;
            } else {
                m = solutions_vector(i);
            }

            result_vec.push_back(round(m));

            
        }
        return result_vec;
    }
    SolveReturnStruct solve(NTL::Mat<NTL::ZZ_p> NtlA, NTL::Vec<NTL::ZZ_p> Ntlb, polyvecl *actual_s1){
        Eigen::MatrixXf A(NtlA.NumRows(), NtlA.NumCols());
        Eigen::VectorXf b(NtlA.NumRows());
        for(int i=0;i<NtlA.NumRows();++i){
            for(int j=0;j<NtlA.NumCols();++j){
                A(i,j) = static_cast<float>(reduce32(static_cast<int32_t>(NTL::conv<long>(NtlA[i][j]))));
            }
            b[i] = static_cast<float>(reduce32(static_cast<int32_t>(NTL::conv<long>(Ntlb[i]))));
        }
        Eigen::VectorXf solutions_vector =  ((A.transpose() * A).inverse())*A.transpose()*b;//A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
        SolveReturnStruct ret_struct;
        ret_struct.solution = scale(solutions_vector, actual_s1);
        std::vector<double> distance_to_zero_dot_five;
        for(int i=0;i<solutions_vector.size();++i){
            distance_to_zero_dot_five.push_back(abs(0.5-abs((solutions_vector(i)-(int)solutions_vector(i)))));
            ret_struct.solution_not_rounded.push_back(solutions_vector(i));
        }
        ret_struct.idx_sorted_by_uncertainty = sort_indexes(distance_to_zero_dot_five);
        return ret_struct;
    }
}
