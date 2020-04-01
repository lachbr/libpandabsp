//
// Created by Clytie on 2018/9/10.
//

#ifndef CKDTREE_KDTREE_H
#define CKDTREE_KDTREE_H


#include <cmath>
#include <stack>
#include <queue>
#include <cfloat>
#include <vector>
#include <utility>
#include <cassert>
#include <iostream>
#include <unordered_map>
#include "utils.h"
#include "config_bsp.h"

#include <referenceCount.h>

using namespace std;

class EXPCL_PANDABSP KDTree : public ReferenceCount {
public:
    explicit KDTree(unsigned int dim)
            : dim(dim), num_samples(0), Datas(), root(nullptr) {}
    ~KDTree() = default;

    void build(vector<vector<double> > & datas);
    unsigned int depth();
    void render();
    unordered_map<unsigned int, double> query(vector<double> & data, unsigned int k);
    unordered_map<unsigned int, double> query_distance(vector<double> & data, double dist);
    pair<unsigned int, double> query(vector<double> & data);

    unsigned int dim; // number of features
    unsigned int num_samples; // number of samples
    vector<vector<double> > Datas;

private:
    class KDTreeNode {
    public:
        explicit KDTreeNode(unsigned int index,
                           unsigned int split_axis,
                           vector<vector<unsigned int> > & indexs,
                           KDTreeNode * parent=nullptr,
                           KDTreeNode * left=nullptr,
                           KDTreeNode * right=nullptr)
                : index(index),
                  split_axis(split_axis),
                  indexs(indexs),
                  parent(parent),
                  left(left),
                  right(right) {}

        unsigned int index;
        unsigned int split_axis;
        vector<vector<unsigned int> > indexs;
        KDTreeNode * parent;
        KDTreeNode * left;
        KDTreeNode * right;

        bool is_leaf();
        bool has_two_child();
    };

    KDTreeNode * root;
    unsigned int depth(KDTreeNode * node);
    void render(KDTreeNode * node);
    void mergesort(vector<vector<double> > & arr, vector<unsigned int> & args, unsigned int axis, unsigned int left, unsigned int right);
    vector<vector<unsigned int> > argsort();
    bool smaller(unsigned int row1, unsigned int row2, unsigned int axis);
    double computeDistance(vector<double> & data1, vector<double> & data2);
};


#endif //CKDTREE_KDTREE_H
