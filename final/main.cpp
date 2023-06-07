#include "cppjieba/Jieba.hpp"
#include "libstemmer.h"
#include <algorithm>
#include <chrono>
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

std::unordered_map<std::string, double> IDF_Matrix; // IDF Matrix

std::unordered_map<std::string, int> term_id_map; //record the id of term

std::vector<int> docs_id; //record the id of doc

std::map<int, int> word_count; // record words count

std::map<std::string, std::map<int, std::vector<int>>> inverted_index;

std::map<std::string, std::set<int>> class_map;

std::string folder_path =
    "/home/qym/Information-Retrieval/final/txts"; //要遍历的文件夹路径

struct sb_stemmer *stemmer;

std::string parse_word(std::string word, std::string tag, sb_stemmer *stemmer) {
    if (tag == "eng") {
        //stemming when term is english word
        sb_symbol *input = (sb_symbol *)word.c_str();
        const sb_symbol *stemmed = sb_stemmer_stem(stemmer, input, word.size());
        std::string stem_res((char *)stemmed);
        // std::cout<<stem_res<<std::endl;
        return std::move(stem_res);
    }
    return word;
}

void build_inverted_index() {
    for (const auto &entry : std::filesystem::directory_iterator(folder_path)) {
        if (entry.is_directory()) {
            std::string class_name = entry.path().filename();
            std::set<int> &docs_in_class = class_map[class_name];
            for (const auto &sub_entry :
                 std::filesystem::directory_iterator(entry.path())) {
                if (!sub_entry.is_directory()) {
                    int doc_name = std::stoi(sub_entry.path().filename());
                    docs_id.push_back(doc_name);
                    std::ifstream file(sub_entry.path());
                    // std::cout << "File name: " << sub_entry.path().filename() << std::endl;
                    std::string line;
                    std::string doc;
                    while (std::getline(file, line)) {
                        doc += line;
                    }
                    std::vector<cppjieba::Word> words;
                    jieba.CutForSearch(doc, words, true);
                    int count = 0;
                    for (auto word : words) {
                        std::string tag =
                            jieba.LookupTag(word.word); //根据tag删去符号
                        if (tag == "x" || tag == "m")
                            continue;
                        std::string res = parse_word(word.word, tag, stemmer);
                        inverted_index[res][doc_name].emplace_back(word.offset);
                        count++;
                    }
                    word_count[doc_name] = count;
                    docs_in_class.insert(doc_name);
                }
            }
        }
    }
}

//--------------build-TF-IDF-&-get-cosine-similiraty-------------

std::map<int, std::unordered_map<int, double>> docs_TF_IDF;

void build_TF_IDF_matrix() {
    int k = 0;
    int docs_num=docs_id.size();
    for (auto &i : inverted_index) {
        // get IDF value of term
        // idf = 1 + ln(Total Number Of Documents / Number Of Documents with term in it)
        double idf = 1 + log(double(docs_num) / double(i.second.size()));
        term_id_map[i.first] = k;
        IDF_Matrix[i.first] = idf;
        for (auto &j : i.second) {
            // get tf & normalize
            // TF_doc_term = (frequency of terms) / (total number of terms)
            double tf = double(j.second.size()) / double(word_count[j.first]);
            double tf_idf = tf * idf;
            docs_TF_IDF[j.first][k] = tf_idf;
        }
        k++;
    }
}

//--------------------get-Top10---------------------
bool cmp(const std::pair<double, int> &a, const std::pair<double, int> &b) {
    if (a.first == b.first) {
        return a.second < b.second;
    }
    return a.first > b.first;
}

void showTopK(std::vector<std::pair<double, int>> &a) {
    std::sort(a.begin(), a.end(), cmp);
    std::vector<std::pair<double, int>> res(a.begin(), a.begin() + 10);
    for (auto &i : res) {
        std::cout << "docID: " << i.second << " Score: " << std::fixed
                  << std::setprecision(2) << i.first << std::endl;
    }
}

// Calculate the cosine similarity between two docs
// row1 & row2 = doc_name1 & doc_name2
double getCosineSimilarity(std::unordered_map<int, double> m1,
                           std::unordered_map<int, double> m2) {
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

void query(std::string que) {
    std::map<std::string, int> query_term_count;
    std::unordered_map<int, double> query_TF_IDF;
    std::vector<cppjieba::Word> words;
    jieba.CutForSearch(que, words, true);
    for (auto word : words) {
        std::string tag = jieba.LookupTag(word.word); //根据tag删去符号
        if (tag == "x" || tag == "m")
            continue;
        std::string res = parse_word(word.word, tag, stemmer);
        if (query_term_count.count(res) == 0)
            query_term_count[res] = 1;
        else
            query_term_count[res]++;
    }
    //get tf-idf for query
    for (auto &i : query_term_count) {
        if (term_id_map.count(i.first) == 0)
            continue;
        double tf = (double)i.second / (query_term_count.size());
        double idf = IDF_Matrix[i.first];
        query_TF_IDF[term_id_map[i.first]] = tf * idf;
    }
    // Cosine Similarity for query & docs
    std::vector<std::pair<double, int>> res_cosine;
    for (auto &i : docs_id) {
        res_cosine.push_back(
            {getCosineSimilarity(query_TF_IDF, docs_TF_IDF[i]), i});
    }
    showTopK(res_cosine);
}

void wait_query() {
    std::string que = "教务部 MOOC";
    // while(std::cin>>que){
    std::cout << "Start query: " << que << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    query(que);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Query Time taken: " << duration.count() / 1000.0 << "s"
              << std::endl;
    // }
}

int main() {
    stemmer = sb_stemmer_new("english", NULL);
    auto start_time = std::chrono::high_resolution_clock::now();
    build_inverted_index();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "Inverted index built. Time taken: "
              << duration.count() / 1000.0 << std::fixed << std::setprecision(2)
              << "s" << std::endl;

    start_time = std::chrono::high_resolution_clock::now();
    build_TF_IDF_matrix();
    end_time = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    std::cout << "TF-IDF matrix built. Time taken: "
              << duration.count() / 1000.0 << std::fixed << std::setprecision(2)
              << "s" << std::endl;
    std::cout << std::endl;
    wait_query();
}
