#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

std::vector<std::vector<double>> TF_IDF_Matrix; // TF_IDF Matrix

std::vector<std::vector<double>> cosine_similarity;

std::vector<int> word_count; // record words count

std::map<std::string, std::map<int, std::vector<int>>> inverted_index;

int docs_num;
int terms_num;

void show(const std::vector<int> &l) {
    for (auto i : l)
        std::cout << i << " ";
    std::cout << std::endl;
}

std::string parse_word(std::string s) {
    std::string res;
    for (char c : s) {
        if ( isalpha(c) || isdigit(c) || c == '-') {
            res += tolower(c);
        }
    }
    return res;
}

void build_inverted_index() {
    std::fstream fs;
    fs.open("txt1/hw4_1.txt", std::ios::in);
    if (fs.is_open()) {
        std::string doc;
        std::stringstream ss;
        int docID = 1;
        int pos = 1; // record term position in a doc
        while (getline(fs, doc)) {
            ss.str(doc);
            std::string word;
            int count = 0; // record words count of docs[count]
            while (ss >> word) {
                count++;
                inverted_index[parse_word(word)][docID].emplace_back(pos);
                pos++;
            }
            word_count.emplace_back(count);
            docID++;
            pos = 1;
            ss.clear();
        }
        terms_num = inverted_index.size();
        docs_num = docID - 1;
        fs.close();
    }
    fs.open("txt1/inverted_1.txt", std::ios::out);
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

//------------------SparseMatrix---------------------

// struct to store non-zero elements in sparse matrix
struct Element {
    int row;
    int col;
    double value;
};

// Define a class for the sparse matrix
class SparseMatrix {
  private:
    int rows;
    int cols;
    std::vector<Element> elements;

  public:
    SparseMatrix() {}

    SparseMatrix(int r, int c) {
        rows = r;
        cols = c;
    }

    void setSize(int r, int c) {
        rows = r;
        cols = c;
    }

    // Add element to the matrix
    void addElement(int row, int col, double value) {
        if (value == 0)
            return;
        Element e = {row, col, value};
        elements.emplace_back(e);
    }

    // Calculate the cosine similarity between two docs
    double cosineSimilarity(int row1, int row2) {
        std::unordered_map<int, double> m1;
        std::unordered_map<int, double> m2;

        for (Element e : elements) {
            if (e.row == row1) {
                m1[e.col] = e.value;
            }
            if (e.row == row2) {
                m2[e.col] = e.value;
            }
        }

        double dotProduct = 0.0;
        double norm1 = 0.0;
        double norm2 = 0.0;

        // get the dot product and norm1
        for (auto &i : m1) {
            int col = i.first;
            double row1Value = i.second;
            if (m2.count(col)) {
                double row2Value = m2[col];
                dotProduct += row1Value * row2Value;
            }
            norm1 += row1Value * row1Value;
        }
        //get norm2
        for (auto &i : m2) {
            double row2Value = i.second;
            norm2 += row2Value * row2Value;
        }
        // get cosine similarity
        double cs = dotProduct / (sqrt(norm1) * sqrt(norm2));
        return cs;
    }
};

//--------------build-TF-IDF-&-get-cosine-similiraty-------------

SparseMatrix sm;

void build_TF_IDF_matrix() {
    TF_IDF_Matrix.resize(docs_num, std::vector<double>(terms_num, 0.0));
    int k = 0;
    for (auto &i : inverted_index) {
        // get IDF value of term
        // idf = 1 + ln(Total Number Of Documents / Number Of Documents with term in it)
        double idf = 1 + log(double(docs_num) / double(i.second.size()));
        for (auto &j : i.second) {
            // get tf & normalize
            // TF_doc_term = (frequency of terms) / (total number of terms)
            double tf =
                double(j.second.size()) / double(word_count[j.first - 1]);
            double tf_idf = tf * idf;
            sm.addElement(j.first - 1, k, tf_idf);// add to sparse matrix
            TF_IDF_Matrix[j.first - 1][k] = tf_idf; // add to the two-dimensional matrix
       }
        k++;
    }
    // print result to txt
    std::fstream fs;
    fs.open("txt1/TF_IDF_matrix_1.txt", std::ios::out);
    for (int i = 0; i < docs_num; ++i) {
        for (int j = 0; j < terms_num; ++j) {
            fs << TF_IDF_Matrix[i][j] << " ";
        }
        fs << std::endl;
    }
}

void getCosineSimilarity() {
    sm.setSize(docs_num, terms_num);
    build_TF_IDF_matrix();
    cosine_similarity.resize(docs_num, std::vector<double>(docs_num, 0.0));
    for (int i = 0; i < docs_num; i++) {
        for (int j = 0; j < docs_num; j++) {
            cosine_similarity[i][j] = sm.cosineSimilarity(i, j);
        }
    }
    // print result to txt
    std::fstream fs;
    fs.open("txt1/cosine_similarity_1.txt", std::ios::out);
    for (int i = 0; i < docs_num; ++i) {
        for (int j = 0; j < docs_num; ++j) {
            fs << std::fixed << std::setprecision(7) << cosine_similarity[i][j]
               << " ";
        }
        fs << std::endl;
    }
}

//--------------------get-Top10---------------------
bool cmp(const std::pair<double, int> &a, const std::pair<double, int> &b) {
    if (a.first == b.first) {
        return a.second < b.second;
    }
    return a.first > b.first;
}

void showTopK(int t) {
    std::vector<double> nums = cosine_similarity[t];
    std::vector<std::pair<double, int>> a;
    for (int i = 0; i < nums.size(); i++) {
        if (i != t)
            a.push_back({nums[i], i});
    }
    std::sort(a.begin(), a.end(), cmp);
    std::vector<std::pair<double, int>> res(a.begin(), a.begin() + 10);
    std::cout << "docID " << (t + 1) << std::endl;
    for (auto &i : res) {
        std::cout << "(" << i.second + 1 << " " << std::fixed
                  << std::setprecision(7) << i.first << ")"
                  << " ";
    }
    std::cout << std::endl << std::endl;
}

int main() {
    build_inverted_index();
    getCosineSimilarity();
    for (int i = 0; i < 10; i++) {
        showTopK(i);
    }
}
