//
// Created by Clytie on 2018/9/10.
//


#include "KDTree.h"


bool KDTree::KDTreeNode::is_leaf() {
    return (left == nullptr) && (right == nullptr);
}

bool KDTree::KDTreeNode::has_two_child() {
    return (left != nullptr) && (right != nullptr);
}

bool KDTree::smaller(unsigned int row1, unsigned int row2, unsigned int axis) {
    bool smaller;
    if (Datas[row1][axis] < Datas[row2][axis]) {
        smaller = true;
    } else if (Datas[row1][axis] > Datas[row2][axis]) {
        smaller = false;
    } else {
        unsigned int next_axis = (axis == dim - 1)? 0 : (axis + 1);
        while (Datas[row1][next_axis] == Datas[row2][next_axis]) {
            next_axis = (next_axis == dim - 1)? 0 : (next_axis + 1);
        }
        smaller = Datas[row1][next_axis] < Datas[row2][next_axis];
    }
    return smaller;
}

void KDTree::build(vector<vector<double> > & datas) {
    Datas.swap(datas);
    num_samples = (unsigned int)Datas.size();
    assert(num_samples > 0 && dim > 0 && dim <= Datas.front().size());
    vector<vector<unsigned int> > args = argsort();
    queue<KDTree::KDTreeNode *> nodes;
    unsigned int pivot = num_samples / 2;
    unsigned int split_axis = 0;
    double max_var = 0;
    for (unsigned int axis = 0; axis < dim; ++axis) {
        double var = utils::variance<double>(Datas, axis);
        if (var > max_var) {
            max_var = var;
            split_axis = axis;
        }
    }
    root = new KDTreeNode(args[split_axis][pivot], split_axis, args);
    nodes.push(root);
    while (!nodes.empty()) {
        //cout << "size" << nodes.size() << endl;
        KDTree::KDTreeNode * cur = nodes.front();
        //cout << "index" << cur->index << endl;
        //cout << endl;
        nodes.pop();
        split_axis = cur->split_axis;
        auto length = (unsigned int)cur->indexs.front().size();
        //cout << length << endl;
        pivot = length / 2;
        //cout << pivot << endl;
        vector<vector<unsigned int> > left_indexs, right_indexs;
        for (unsigned int axis = 0; axis < dim; ++axis) {
            vector<unsigned int> left_index, right_index;
            if (axis != split_axis) {
                for (auto row = 0; row < length; ++row) {
                    if (cur->indexs[axis][row] == cur->index) {
                        continue;
                    }
                    /*
                    if (Datas[cur->indexs[axis][row]][split_axis] < Datas[cur->index][split_axis]) {
                        left_index.emplace_back(cur->indexs[axis][row]);
                    } else if (Datas[cur->indexs[axis][row]][split_axis] > Datas[cur->index][split_axis]) {
                        right_index.emplace_back(cur->indexs[axis][row]);
                    } else {
                        unsigned int next_axis = (split_axis == dim - 1)? 0 : (split_axis + 1);
                        while (Datas[cur->indexs[axis][row]][next_axis] == Datas[cur->index][next_axis]) {
                            next_axis = (next_axis == dim - 1)? 0 : (next_axis + 1);
                        }
                        if (Datas[cur->indexs[axis][row]][next_axis] < Datas[cur->index][next_axis]) {
                            left_index.emplace_back(cur->indexs[axis][row]);
                        } else {
                            right_index.emplace_back(cur->indexs[axis][row]);
                        }
                    }
                     */
                    if (smaller(cur->indexs[axis][row], cur->index, split_axis)) {
                        left_index.emplace_back(cur->indexs[axis][row]);
                    } else {
                        right_index.emplace_back(cur->indexs[axis][row]);
                    }
                }
            } else {
                for (auto row = 0; row < length; ++row) {
                    if (row < pivot) {
                        left_index.emplace_back(cur->indexs[axis][row]);
                    } else if (row > pivot) {
                        right_index.emplace_back(cur->indexs[axis][row]);
                    }
                }
            }
            left_indexs.emplace_back(left_index);
            right_indexs.emplace_back(right_index);
            //cout << "left" << left_index.size() << endl;
            //cout << "right" << right_index.size() << endl;
        }
        split_axis = (split_axis == dim - 1)? 0 : (split_axis + 1);
        if (left_indexs.front().size() > 1) {
            auto left_pivot = (unsigned int)left_indexs.front().size() / 2;
            cur->left = new KDTreeNode(left_indexs[split_axis][left_pivot], split_axis, left_indexs, cur);
            nodes.push(cur->left);
        } else if (left_indexs.front().size() == 1) {
            cur->left = new KDTreeNode(left_indexs[split_axis].front(), split_axis, left_indexs, cur);
        }
        if (right_indexs.front().size() > 1) {
            auto right_pivot = (unsigned int)right_indexs.front().size() / 2;
            cur->right = new KDTreeNode(right_indexs[split_axis][right_pivot], split_axis, right_indexs, cur);
            nodes.push(cur->right);
        } else if (right_indexs.front().size() == 1) {
            cur->right = new KDTreeNode(right_indexs[split_axis].front(), split_axis, left_indexs, cur);
        }
    }
}

void KDTree::mergesort(vector<vector<double> > & arr,
                       vector<unsigned int> & args,
                       unsigned int axis,
                       unsigned int left,
                       unsigned int right) {
    if (left < right) {
        unsigned int pivot = (left + right) / 2;
        mergesort(arr, args, axis, left, pivot);
        mergesort(arr, args, axis, pivot + 1, right);
        vector<unsigned int> C(right - left + 1);
        unsigned int Actr = left, Bctr = pivot + 1, Cctr = 0;
        while (Actr <= pivot && Bctr <= right) {
            if (arr[args[Actr]][axis] < arr[args[Bctr]][axis]) {
                C[Cctr++] = args[Actr++];
            } else if (arr[args[Actr]][axis] > arr[args[Bctr]][axis]) {
                C[Cctr++] = args[Bctr++];
            } else {
                unsigned int next_axis = (axis == dim - 1)? 0 : (axis + 1);
                while (arr[args[Actr]][next_axis] == arr[args[Bctr]][next_axis]) {
                    next_axis = (next_axis == dim - 1)? 0 : (next_axis + 1);
                }
                if (arr[args[Actr]][next_axis] < arr[args[Bctr]][next_axis]) {
                    C[Cctr++] = args[Actr++];
                } else {
                    C[Cctr++] = args[Bctr++];
                }
            }
        }
        for (; Actr <= pivot;) {
            C[Cctr++] = args[Actr++];
        }
        for (; Bctr <= right;) {
            C[Cctr++] = args[Bctr++];
        }
        for (unsigned int i = left; i <= right; ++i) {
            args[i] = C[i - left];
        }
    }
}

vector<vector<unsigned int> > KDTree::argsort() {
    vector<vector<unsigned int> > argsorts;
    for (unsigned int i = 0; i < dim; ++i) {
        vector<unsigned int> args;
        for (unsigned int j = 0; j < num_samples; ++j) {
            args.emplace_back(j);
        }
        mergesort(Datas, args, i, 0, num_samples - 1);
        argsorts.emplace_back(args);
    }
    return argsorts;
}

unsigned int KDTree::depth(KDTree::KDTreeNode * node) {
    if (node == nullptr) {
        return 0;
    }
    unsigned int left = depth(node->left);
    unsigned int right = depth(node->right);
    return (left > right)? (left + 1) : (right + 1);
}

unsigned int KDTree::depth() {
    return depth(root);
}

void KDTree::render(KDTree::KDTreeNode * node) {
    if (node != nullptr) {
        printf("%u: ", node->index);
        for (int i = 0; i < Datas[node->index].size(); ++i) {
            if (i != Datas[node->index].size() - 1) {
                printf("%.2f,", Datas[node->index][i]);
            } else {
                printf("%.2f ", Datas[node->index][i]);
            }
        }
        printf("%u\n", node->split_axis);
        render(node->left);
        render(node->right);
    }
}

void KDTree::render() {
    render(root);
}

double KDTree::computeDistance(vector<double> & data1, vector<double> & data2) {
    assert(data1.size() == dim);
    double distance = 0;
    for (int i = 0; i < dim; ++i) {
        double diff = data1[i] - data2[i];
        distance += diff * diff;
    }
    return distance;
}

unordered_map<unsigned int, double> KDTree::query(vector<double> & data, unsigned int k) {
    //assert(k <= num_samples && k >= 1);
    vector<pair<double, unsigned int> > distances;
    make_heap(distances.begin(), distances.end());
    stack<KDTree::KDTreeNode *> nodes;
    KDTree::KDTreeNode * cur = root;
    nodes.push(cur);
    bool need_add = true;
    bool reverse = false;
    while (!nodes.empty()) {
        cur = nodes.top();
        if (need_add) {
            while (!cur->is_leaf()) {
                if (reverse) {
                    nodes.pop();
                    if (data[cur->split_axis] < Datas[cur->index][cur->split_axis]) {
                        nodes.push(cur->right);
                    } else {
                        nodes.push(cur->left);
                    }
                    reverse = false;
                } else {
                    if (cur->has_two_child()) {
                        if (data[cur->split_axis] < Datas[cur->index][cur->split_axis]) {
                            nodes.push(cur->left);
                        } else {
                            nodes.push(cur->right);
                        }
                    } else {
                        if (cur->left != nullptr) {
                            nodes.push(cur->left);
                        } else if (cur->right != nullptr) {
                            nodes.push(cur->right);
                        }
                    }
                }
                cur = nodes.top();
            }
            need_add = false;
        } else {
            double distance = computeDistance(data, Datas[cur->index]);

            if (distances.size() < k) {
                distances.emplace_back(make_pair(distance, cur->index));
                push_heap(distances.begin(), distances.end());
            } else {
                if (distance < distances.front().first) {
                    distances.emplace_back(make_pair(distance, cur->index));
                    push_heap(distances.begin(), distances.end());
                    pop_heap(distances.begin(), distances.end());
                    distances.pop_back();
                }
            }
            double max_distance = distances.front().first;
            /*
            for (const auto & dis : distances) {
                cout << "distances: " << dis.second << " " << dis.first << endl;
            }
            cout << max_distance << " " << distance << " " << cur->index << " " << distances.size() << endl;
             */
            double diff = data[cur->split_axis] - Datas[cur->index][cur->split_axis];
            //cout << "diff " << diff * diff << endl;
            //cout << endl;
            if (cur->has_two_child() && (max_distance >= diff * diff || distances.size() < k)) {
                need_add = true;
                reverse = true;
                continue;
            }
            nodes.pop();
        }
    }
    unordered_map<unsigned int, double> query_result;
    for (const auto & distance : distances) {
        query_result.insert(make_pair(distance.second, sqrt(distance.first)));
        //cout << distance.second << " " << sqrt(distance.first) << endl;
    }
    return query_result;
}

pair<unsigned int, double> KDTree::query(vector<double> & data) {
    unordered_map<unsigned int, double> query_result = query(data, 1);
    return *query_result.begin();
}

unordered_map<unsigned int, double> KDTree::query_distance(vector<double> & data, double dist) {
    assert(dist >= 0);
    double distance_threshold = dist * dist;
    unordered_map<unsigned int, double> distances;
    stack<KDTree::KDTreeNode *> nodes;
    KDTree::KDTreeNode * cur = root;
    nodes.push(cur);
    bool need_add = true;
    bool reverse = false;
    while (!nodes.empty()) {
        cur = nodes.top();
        if (need_add) {
            while (!cur->is_leaf()) {
                if (reverse) {
                    nodes.pop();
                    if (data[cur->split_axis] < Datas[cur->index][cur->split_axis]) {
                        nodes.push(cur->right);
                    } else {
                        nodes.push(cur->left);
                    }
                    reverse = false;
                } else {
                    if (cur->has_two_child()) {
                        if (data[cur->split_axis] < Datas[cur->index][cur->split_axis]) {
                            nodes.push(cur->left);
                        } else {
                            nodes.push(cur->right);
                        }
                    } else {
                        if (cur->left != nullptr) {
                            nodes.push(cur->left);
                        } else if (cur->right != nullptr) {
                            nodes.push(cur->right);
                        }
                    }
                }
                cur = nodes.top();
            }
            need_add = false;
        } else {
            double distance = computeDistance(data, Datas[cur->index]);
            if (distance <= distance_threshold) {
                distances.insert(make_pair(cur->index, distance));
            }
            /*
            for (const auto & dis : distances) {
                cout << "distances: " << dis.second << " " << dis.first << endl;
            }
            cout << max_distance << " " << dist << " " << cur->index << " " << distances.size() << endl;
             */
            double diff = data[cur->split_axis] - Datas[cur->index][cur->split_axis];
            //cout << "diff " << diff * diff << endl;
            //cout << endl;
            if (cur->has_two_child() && distance_threshold >= diff * diff) {
                need_add = true;
                reverse = true;
                continue;
            }
            nodes.pop();
        }
    }
    unordered_map<unsigned int, double> query_result;
    for (const auto & distance : distances) {
        query_result.insert(make_pair(distance.first, sqrt(distance.second)));
        //cout << dist.second << " " << sqrt(dist.first) << endl;
    }
    return query_result;
}