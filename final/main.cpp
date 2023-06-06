#include "cppjieba/Jieba.hpp"
#include "libstemmer.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

const char *const DICT_PATH = "dict/jieba.dict.utf8";
const char *const HMM_PATH = "dict/hmm_model.utf8";
const char *const USER_DICT_PATH = "dict/user.dict.utf8";
const char *const IDF_PATH = "dict/idf.utf8";
const char *const STOP_WORD_PATH = "dict/stop_words.utf8";

cppjieba::Jieba jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH,
                      STOP_WORD_PATH);

std::vector<std::vector<double>> TF_IDF_Matrix; // TF_IDF Matrix

std::unordered_map<std::string, double> IDF_Matrix; // IDF Matrix

std::unordered_map<std::string, int> term_id_map; //record the id of term

std::unordered_map<std::string, int> doc_id_map; //record the id of doc

std::vector<std::vector<double>> cosine_similarity;

std::vector<int> word_count; // record words count

std::map<std::string, std::map<int, std::vector<int>>> inverted_index;

std::map<std::string, std::set<int>> class_map;

std::string folder_path =
    "/home/qym/Information-Retrieval/final/txts"; //要遍历的文件夹路径

int docs_num;
int terms_num;

void build_inverted_index() {
    //create stemmer
    struct sb_stemmer *stemmer = sb_stemmer_new("english", NULL);
    if (!stemmer) {
        std::cerr << "Error creating stemmer" << std::endl;
    }
    for (const auto &entry : std::filesystem::directory_iterator(folder_path)) {
        if (entry.is_directory()) {
            std::string class_name = entry.path().filename();
            std::set<int> &docs_in_class = class_map[class_name];
            for (const auto &sub_entry :
                 std::filesystem::directory_iterator(entry.path())) {
                if (!sub_entry.is_directory()) {
                    int doc_name = std::stoi(sub_entry.path().filename());
                    std::ifstream file(sub_entry.path());
                    // std::cout << "File name: " << sub_entry.path().filename() << std::endl;
                    std::string line;
                    std::string doc;
                    while (std::getline(file, line)) {
                        doc += line;
                    }
                    std::string word;
                    std::vector<cppjieba::Word> words;
                    jieba.CutForSearch(doc, words, true);
                    for (auto word : words) {
                        std::string tag =
                            jieba.LookupTag(word.word); //根据tag删去符号
                        if (tag == "x" || tag == "m")
                            continue;
                        if (tag == "eng") {
                            //stemming when term is english word
                            sb_symbol *input = (sb_symbol *)word.word.c_str();
                            const sb_symbol *stemmed = sb_stemmer_stem(
                                stemmer, input, word.word.size());
                            std::string stem_res((char *)stemmed);
                            inverted_index[stem_res][doc_name].emplace_back(
                                word.offset);
                            continue;
                        }
                        inverted_index[word.word][doc_name].emplace_back(
                            word.offset);
                    }
                    docs_in_class.insert(doc_name);
                    docs_num++;
                }
            }
        }
    }
    //delete stemmer
    sb_stemmer_delete(stemmer);

    terms_num = inverted_index.size();

    std::fstream fs;
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
    std::cout << "Inverted index built" << std::endl;
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
    //row1 & row2 = doc_name1 & doc_name2
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
        term_id_map[i.first] = k;
        IDF_Matrix[i.first] = idf;
        for (auto &j : i.second) {
            // get tf & normalize
            // TF_doc_term = (frequency of terms) / (total number of terms)
            double tf =
                double(j.second.size()) / double(word_count[j.first - 1]);
            double tf_idf = tf * idf;
            sm.addElement(j.first, k, tf_idf); // add to sparse matrix
            // TF_IDF_Matrix[j.first - 1][k] =
            //     tf_idf; // add to the two-dimensional matrix
        }
        k++;
    }
    // print result to txt
    // std::fstream fs;
    // fs.open("txt1/TF_IDF_matrix_1.txt", std::ios::out);
    // for (int i = 0; i < docs_num; ++i) {
    //     for (int j = 0; j < terms_num; ++j) {
    //         fs << TF_IDF_Matrix[i][j] << " ";
    //     }
    //     fs << std::endl;
    // }
    std::cout << "TF-IDF matrix built" << std::endl;
}

void getCosineSimilarity() {
    sm.setSize(docs_num, terms_num);
    build_TF_IDF_matrix();
    cosine_similarity.resize(docs_num, std::vector<double>(docs_num, 0.0));
    // for (int i = 0; i < docs_num; i++) {
    //     for (int j = 0; j < docs_num; j++) {
    //         cosine_similarity[i][j] = sm.cosineSimilarity(i, j);
    //     }
    // }
    // print result to txt
    // std::fstream fs;
    // fs.open("txt1/cosine_similarity_1.txt", std::ios::out);
    // for (int i = 0; i < docs_num; ++i) {
    //     for (int j = 0; j < docs_num; ++j) {
    //         fs << std::fixed << std::setprecision(2) << cosine_similarity[i][j]
    //            << " ";
    //     }
    //     fs << std::endl;
    // }
    std::cout << "cosine similarity built" << std::endl;
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
    std::cout << "doc_name " << (t + 1) << std::endl;
    for (auto &i : res) {
        std::cout << "(" << i.second + 1 << " " << std::fixed
                  << std::setprecision(2) << i.first << ")"
                  << " ";
    }
    std::cout << std::endl << std::endl;
}


int main() { build_inverted_index(); }
