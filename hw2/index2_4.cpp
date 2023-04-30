#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <math.h>
#include <sstream>
#include <string>
#include <vector>

std::map<std::string, std::map<int, std::vector<int>>> inverted_index;

struct INDEX_POS {
    int docID, pos1, pos2;
};

int docs_num;

void show(const std::vector<int> &l) {
    for (auto i : l)
        std::cout << i << " ";
    std::cout << std::endl;
}

std::string parse_word(std::string s) {
    std::string res;
    for (char c : s) {
        if (isalpha(c) || c == '-') {
            res += tolower(c);
        }
    }
    return res;
}

void build_inverted_index() {
    std::fstream fs;
    fs.open("HW2.txt", std::ios::in);
    if (fs.is_open()) {
        std::string doc;
        std::stringstream ss;
        int docID = 1;
        int pos = 1;
        while (getline(fs, doc)) {
            ss.str(doc);
            std::string word;
            while (ss >> word) {
                inverted_index[parse_word(word)][docID].push_back(pos);
                pos++;
            }
            docID++;
            pos = 1;
            ss.clear();
        }
        docs_num = docID - 1;
        fs.close();
    }
    fs.open("inverted.txt", std::ios::out);
    for (auto &i : inverted_index) {
        fs << i.first << ": ";
        for (auto &j : i.second) {
            fs << j.first << "<" << j.second.front();
            for (int k = 1; k < j.second.size(); k++) {
                fs << "," << j.second[k];
            }
            fs << "> ";
        }
        fs << std::endl;
    }
}

// AND
std::vector<INDEX_POS> AND(std::map<int, std::vector<int>> &a,
                           std::map<int, std::vector<int>> &b, std::string s) {
    int interval = 0;
    int state = 0; // record location relationship
    // parse s
    for (char c : s) {
        interval *= 10;
        if (c == '-')
            state = 1;
        else if (c == '+')
            state = 2;
        else {
            interval += (c - '0');
        }
    }
    std::vector<INDEX_POS> res;
    for (auto &i : a) { // find the same key between a & b
        if (b.count(i.first)) {
            std::vector<int> p = i.second; // get the vector from map
            std::vector<int> q = b[i.first];
            for (auto &pp : p) { // Iterate over two vectors
                for (auto &qq : q) {
                    // x
                    if (state == 0 && std::abs(pp - qq) <= interval)
                        res.push_back({i.first, pp, qq});
                    // -x
                    if (state == 1 && qq - pp <= interval && qq - pp > 0)
                        res.push_back({i.first, pp, qq});
                    // +x
                    if (state == 2 && pp - qq <= interval && pp - qq > 0)
                        res.push_back({i.first, pp, qq});
                }
            }
        }
    }
    return res;
}

int main() {
    build_inverted_index();
    std::string s1, s2, s3;
    while (std::cin >> s1 >> s2 >> s3) {
        std::vector<INDEX_POS> q =
            AND(inverted_index[s1], inverted_index[s2], s3);
        if (q.size() == 0)
            std::cout << "No match" << std::endl;
        for (auto &i : q) {
            std::cout << "docID: " << i.docID << " pos1: " << i.pos1
                      << " pos2: " << i.pos2 << std::endl;
        }
        std::cout << std::endl;
    }
}
