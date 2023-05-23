#include "cppjieba/Jieba.hpp"
#include <algorithm>
#include <climits>
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

std::map<std::string, std::map<int, std::vector<int>>> inverted_index;
std::map<std::string, int> term_count;
std::map<std::string, std::set<int>> class_map;

// map<class, vector<pair<term, utility>>
using ClassFeatureUtilityMap =
    std::map<std::string, std::vector<std::pair<std::string, double>>>;

ClassFeatureUtilityMap class_feature_mi; //每个类的特征-mi
ClassFeatureUtilityMap class_feature_x2; //每个类的特征-x2

std::string folder_path =
    "/home/qym/Information-Retrieval/hw5/txts"; // 替换为您要遍历的文件夹路径

int docs_num = 0;
int terms_num = 0;

double pri_prob = 0.2;
std::map<std::string, std::map<std::string, double>> Tct; // map<c, map<t, cnt>>

using PtcMap =
    std::map<std::string,
             std::map<std::string, double>>; // map<t, map<c, log(Ptc)>>

//选择的特征个数
#define VOCAB_FEATURE_LIMIT 10

void show(const std::vector<int> &l) {
    for (auto i : l)
        std::cout << i << " ";
    std::cout << std::endl;
}

void build_inverted_index() {
    for (const auto &entry : std::filesystem::directory_iterator(folder_path)) {
        if (entry.is_directory()) {
            std::string class_name = entry.path().filename();
            std::set<int> &docs_in_class = class_map[class_name];
            int num = 0;
            for (const auto &sub_entry :
                 std::filesystem::directory_iterator(entry.path())) {
                if (num == 30)
                    break;
                if (!sub_entry.is_directory()) {
                    int docID = std::stoi(sub_entry.path().filename());
                    std::ifstream file(sub_entry.path());
                    // std::cout << "File name: " << sub_entry.path().filename() << std::endl;
                    std::string line;
                    std::string doc;
                    while (std::getline(file, line)) {
                        doc += line;
                    }
                    std::string word;
                    std::vector<cppjieba::Word> words;
                    jieba.Cut(doc, words, true);
                    for (auto word : words) {
                        std::string tag =
                            jieba.LookupTag(word.word); //根据tag删去符号
                        if (tag == "x" || tag == "m")
                            continue;
                        inverted_index[word.word][docID].emplace_back(
                            word.offset);
                        Tct[class_name]
                           [word.word]++; //计算频数,为计算后验概率做准备
                    }
                    docs_in_class.insert(docID);
                    docs_num++;
                }
                num++;
            }
        }
    }
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

std::vector<std::string>
gen_vocab_from_feature_terms(const ClassFeatureUtilityMap &cfum) {
    std::vector<std::string> vocab;
    for (const auto &i : cfum) {
        int n = 0;
        for (const auto &j : i.second) {
            vocab.push_back(j.first);
            if (++n >= VOCAB_FEATURE_LIMIT)
                break;
        }
    }
    return std::move(vocab);
}

PtcMap train_NB(int mode) {
    // feature select
    std::vector<std::string> vocab;
    if (mode == 2) {
        for (const auto &i : inverted_index) {
            vocab.push_back(i.first);
        }
    } else if (mode == 0) {
        vocab = gen_vocab_from_feature_terms(class_feature_mi);
    } else if (mode == 1) {
        vocab = gen_vocab_from_feature_terms(class_feature_x2);
    }

    // train
    PtcMap Ptc;
    for (const auto &i : class_map) {
        const std::string &class_name = i.first;
        double TctSum = 0;
        for (const std::string &term : vocab) {
            TctSum += Tct[class_name][term] + 1;
        }
        for (const std::string &term : vocab) {
            Ptc[term][class_name] = log((Tct[class_name][term] + 1) / TctSum);
        }
    }
    return std::move(Ptc);
}

std::map<std::string, double> apply_NB(const std::vector<cppjieba::Word> &words,
                                       const PtcMap &Ptc) {
    std::map<std::string, double> score;

    for (const auto &w : words) {
        if (Ptc.count(w.word) == 0) {
            continue;
        }
        for (const auto &i : class_map) {
            const std::string &class_name = i.first;
            score[class_name] += Ptc.at(w.word).at(class_name);
        }
    }
    return std::move(score);
}

void check_class(int mode, const PtcMap &Ptc) {
    std::fstream fs;
    fs.open("class.txt", std::ios::out);
    for (const auto &entry : std::filesystem::directory_iterator(folder_path)) {
        if (entry.is_directory()) {
            int num = 0;
            for (const auto &sub_entry :
                 std::filesystem::directory_iterator(entry.path())) {
                num++;
                if (num <= 30)
                    continue;
                if (!sub_entry.is_directory()) {
                    std::map<std::string, double> prob;
                    int docID = std::stoi(sub_entry.path().filename());
                    std::ifstream file(sub_entry.path());
                    // std::cout << "File name: " << sub_entry.path().filename() << std::endl;
                    std::string line;
                    std::string doc;
                    while (std::getline(file, line)) {
                        doc += line;
                    }
                    std::string word;
                    std::vector<cppjieba::Word> words;
                    jieba.Cut(doc, words, true);

                    prob = apply_NB(words, Ptc);

                    std::string max_class;
                    double max = -INFINITY;
                    for (auto &i : prob) {
                        if (i.second > max) {
                            max = i.second;
                            max_class = i.first;
                        }
                        // std::cout<<i.second<<std::endl;
                    }
                    fs << entry.path().filename().string() << "/"
                       << sub_entry.path().filename().string() << " "
                       << max_class << std::endl;
                }
            }
            fs << std::endl;
        }
    }
}

bool cmp(const std::pair<std::string, double> &a,
         const std::pair<std::string, double> &b) {
    return a.second > b.second;
}

void MI_X2(std::string c) {
    std::vector<std::pair<std::string, double>> res_mi;
    std::vector<std::pair<std::string, double>> res_x2;
    std::set<int> docs_in_class = class_map[c];
    double N = docs_num;
    for (auto &i : inverted_index) {
        double N11 = 0;
        for (auto &j : i.second) {
            if (docs_in_class.count(j.first))
                N11++;
        }
        double N10 = i.second.size() - N11;
        double N01 = docs_in_class.size() - N11;
        double N00 = N - N01 - N10 - N11;

        double mi =
            (N11 / N) * log2((N * N11 + 1) / ((N10 + N11) * (N01 + N11) + 1)) +
            (N01 / N) * log2((N * N01 + 1) / ((N00 + N01) * (N01 + N11) + 1)) +
            (N10 / N) * log2((N * N10 + 1) / ((N10 + N11) * (N00 + N10) + 1)) +
            (N00 / N) * log2((N * N00 + 1) / ((N00 + N01) * (N00 + N10) + 1));

        double x2 = (N * pow((N11 * N00 - N10 * N01), 2)) /
                    ((N11 + N01) * (N11 + N10) * (N10 + N00) * (N01 + N00));

        res_mi.push_back({i.first, mi});
        res_x2.push_back({i.first, x2});
    }
    std::sort(res_mi.begin(), res_mi.end(), cmp);
    std::sort(res_x2.begin(), res_x2.end(), cmp);
    class_feature_mi[c] = res_mi;
    class_feature_x2[c] = res_x2;

    std::fstream fs;
    fs.open("mi/" + c + "_mi.txt", std::ios::out);
    for (auto &i : res_mi) {
        fs << i.first << " " << i.second << std::endl;
    }
    fs.close();
    fs.open("x2/" + c + "_x2.txt", std::ios::out);
    for (auto &i : res_x2) {
        fs << i.first << " " << i.second << std::endl;
    }
    std::cout << "Class " << c << " completed" << std::endl;
}

int main() {
    build_inverted_index();

    int check_mode = 1;
    if (check_mode == 2)
        std::cout << "Check class without feature" << std::endl;
    else {
        MI_X2("dzbgs");
        MI_X2("jwb");
        MI_X2("kxjsb");
        MI_X2("yjsy");
        MI_X2("zsbgs");
        if (check_mode == 0)
            std::cout << "Check class with x2" << std::endl;
        if (check_mode == 1)
            std::cout << "Check class with mi" << std::endl;
    }
    PtcMap Ptc(train_NB(check_mode));
    check_class(check_mode, Ptc);
}
