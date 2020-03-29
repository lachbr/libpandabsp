//
// Created by Clytie on 2018/9/10.
//

#ifndef CKDTREE_UTILS_H
#define CKDTREE_UTILS_H

#include <vector>
#include <utility>


namespace utils {
    template <typename T>
    T mean(std::vector<std::vector<T> > & arr, unsigned int axis) {
        auto nrow = arr.size();
        if (nrow <= 0) {
            return T();
        }
        auto ncol = arr.front().size();
        if (ncol <= 0) {
            return T();
        }
        T sum = T();
        for (auto row = 0; row < nrow; ++row) {
            sum += arr[row][axis];
        }
        return sum / (double)nrow;
    }

    template <typename T>
    double variance(std::vector<std::vector<T> > & arr, unsigned int axis) {
        auto nrow = arr.size();
        if (nrow <= 0) {
            return T();
        }
        auto ncol = arr.front().size();
        if (ncol <= 0) {
            return T();
        }
        T average = utils::mean(arr, axis);
        double var = 0;
        for (auto row = 0; row < nrow; ++row) {
            var += arr[row][axis] * arr[row][axis];
        }
        var /= (double)nrow;
        var -= average * average;
        return var;
    }

    template <typename T>
    class mergeSort {
    public:
        explicit mergeSort() = default;
        std::vector<unsigned int> mergesort(std::vector<std::pair<T, unsigned int> > & arr) {
            mergesort(arr, 0, (unsigned int)(arr.size() - 1));
            std::vector<unsigned int>argsort;
            for (const auto & iarr : arr) {
                argsort.emplace_back(iarr.second);
            }
            return argsort;
        }
    private:
        void mergesort(std::vector<std::pair<T, unsigned int> > & arr, unsigned int left, unsigned int right) {
            if (left < right) {
                unsigned int pivot = (left + right) / 2;
                mergesort(arr, left, pivot);
                mergesort(arr, pivot + 1, right);
                std::vector<std::pair<T, unsigned int> > C(right - left + 1);
                unsigned int Actr = left, Bctr = pivot + 1, Cctr = 0;
                while (Actr <= pivot && Bctr <= right) {
                    if (arr[Actr].first < arr[Bctr].first) {
                        C[Cctr++] = arr[Actr++];
                    } else {
                        C[Cctr++] = arr[Bctr++];
                    }
                }
                for (; Actr <= pivot;) {
                    C[Cctr++] = arr[Actr++];
                }
                for (; Bctr <= right;) {
                    C[Cctr++] = arr[Bctr++];
                }
                for (unsigned int i = left; i <= right; ++i) {
                    arr[i] = C[i - left];
                }
            }
        }
    };
}
#endif //CKDTREE_UTILS_H
