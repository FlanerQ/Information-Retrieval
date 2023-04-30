#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <string>

std::map<std::string, std::list<int>> inverted_index;

int docs_num;

void show_list(const std::list<int> &l) {
    for (int i : l)
        std::cout << i << " ";
    std::cout << std::endl;
}

std::string parse_word(std::string s) {
    std::string word;
    for (char c : s) {
        if (isalpha(c) || c == '-') {
            word += tolower(c);
        }
    }
    return word;
}

// build inverted_index
void build_inverted_index() {
    std::fstream fs;
    fs.open("docs.txt", std::ios::in);
    // build inverted_index
    if (fs.is_open()) {
        std::string doc;
        std::stringstream ss;
        int line = 1;                   // record the count of docs
        while (std::getline(fs, doc)) { // read text by line
            ss.str(doc);
            std::string word;
            while (ss >> word) { // use stringstream to split string to word
                inverted_index[parse_word(word)].push_back(
                    line); // Put this word into the inverted index list
            }
            line++;
            ss.clear();
        }
        docs_num = line - 1;
        fs.close();
    }
    // print to txt
    fs.open("inverted.txt", std::ios::out);
    for (auto &it : inverted_index) {
        fs << it.first << ": ";
        for (int i : it.second)
            fs << i << " ";
        fs << std::endl;
    }
}

// AND
std::list<int> AND(std::list<int> a, std::list<int> b) {
    std::list<int> res;
    auto ia = a.begin();
    auto ib = b.begin();
    while (ia != a.end() && ib != b.end()) {
        if (*ia == *ib) {
            res.push_back(*ia);
            ia++;
            ib++;
        } else if (*ia < *ib)
            ia++;
        else
            ib++;
    }
    return res;
}

// OR
std::list<int> OR(std::list<int> a, std::list<int> b) {
    std::list<int> res;
    auto ia = a.begin();
    auto ib = b.begin();
    while (ia != a.end() && ib != b.end()) {
        if (*ia < *ib) {
            res.push_back(*ia);
            ia++;
        } else {
            res.push_back(*ib);
            ib++;
        }
    }
    while (ia != a.end()) {
        res.push_back(*ia);
        ia++;
    }
    while (ib != b.end()) {
        res.push_back(*ib);
        ib++;
    }
    return res;
}

// NOT
std::list<int> NOT(std::list<int> a) {
    std::list<int> res;
    for (int i = 1; i <= docs_num; i++)
        res.push_back(i);
    for (auto i = res.begin(); i != res.end();) {
        if (std::find(a.begin(), a.end(), *i) != a.end())
            i = res.erase(i);
        else
            i++;
    }
    return res;
}

int main() {
    build_inverted_index();

    std::list<int> l_federated = inverted_index["federated"];
    std::list<int> l_recommendation = inverted_index["recommendation"];
    std::list<int> l_transfer = inverted_index["transfer"];
    std::list<int> l_learning = inverted_index["learning"];
    std::list<int> l_filtering = inverted_index["filtering"];
    std::list<int> l_feedback = inverted_index["feedback"];

    std::list<int> q1 = AND(l_federated, l_recommendation);
    std::list<int> q2 = AND(AND(l_transfer, l_learning), l_filtering);
    std::list<int> q3 = AND(l_recommendation, l_feedback);
    std::list<int> q4 = OR(l_recommendation, l_filtering);
    std::list<int> q5 = AND(l_transfer, NOT(q4));

    show_list(q1);
    show_list(q2);
    show_list(q3);
    show_list(q4);
    show_list(q5);
}
