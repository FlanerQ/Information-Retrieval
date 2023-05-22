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
std::map<std::string, std::map<int, int>> class_map;
std::map<std::string, double> class_words_count;
std::map<std::string, std::vector<std::pair<std::string, double>>>
    class_feature_mi; //每个类的特征-mi
std::map<std::string, std::vector<std::pair<std::string, double>>>
    class_feature_x2; //每个类的特征-x2

std::string folder_path =
    "/home/qym/Information-Retrieval/hw5/txts"; // 替换为您要遍历的文件夹路径

int docs_num = 0;
int terms_num = 0;

double pri_prob = 0.2;
std::map<std::string, std::map<std::string, double>> poster_prob;
std::map<std::string, std::map<std::string, double>> feature_prob;

void show(const std::vector<int> &l) {
    for (auto i : l)
        std::cout << i << " ";
    std::cout << std::endl;
}

void build_inverted_index() {
    for (const auto &entry : std::filesystem::directory_iterator(folder_path)) {
        if (entry.is_directory()) {
            std::map<int, int> cm;
            std::string class_name = entry.path().filename();
            int num=0;
            for (const auto &sub_entry :
                 std::filesystem::directory_iterator(entry.path())) {
                if(num==30) break;
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
                    jieba.CutForSearch(doc, words, true);
                    class_words_count[class_name] += words.size();
                    for (auto word : words) {
                        std::string tag =
                            jieba.LookupTag(word.word); //根据tag删去符号
                        if (tag == "x" || tag == "m")
                            continue;
                        inverted_index[word.word][docID].emplace_back(
                            word.offset);
                        poster_prob
                            [word.word]
                            [class_name]++; //计算频数,为计算后验概率做准备
                    }
                    cm[docID] = 1;
                    docs_num++;
                }
                num++;
            }
            class_map[class_name] = cm;
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

void train_NB(int mode){
    if(mode==2){
        feature_prob=poster_prob;
        return;
    }
    for (auto &i : class_map) {
        std::string class_name = i.first;
        std::vector<std::pair<std::string, double>> feature;
        class_words_count[class_name]=0;
        if (mode)
            feature = class_feature_mi[class_name];
        else
            feature = class_feature_x2[class_name];
        int num = 0;
        for (auto &term : feature) {
            if (num == 10)
                break;
            feature_prob[term.first][class_name] = poster_prob[term.first][class_name];
            class_words_count[class_name]+=feature_prob[term.first][class_name];
            num++;
        }
    }
}

std::map<std::string, double> get_NB(std::vector<cppjieba::Word> words) {
    std::map<std::string, double> res;
    for (auto &i : class_map) {
        double prob = log(pri_prob);
        std::string class_name = i.first;
        double B = feature_prob.size();
        double denominator = class_words_count[class_name] + B;
        for (auto &word : words) {
            if (!feature_prob.count(word.word)) {
                prob+=log(1/denominator);
                continue;
            }
            //term在此class中出现的频数
            double term_fre = feature_prob[word.word][class_name] + 1;
            prob += log(term_fre / denominator);
        }
        res[class_name] = prob;
    }
    return res;
}

void check_class(int mode) {
    std::fstream fs;
    fs.open("class.txt", std::ios::out);
    for (const auto &entry : std::filesystem::directory_iterator(folder_path)) {
        if (entry.is_directory()) {
            int num=0;
            for (const auto &sub_entry :
                 std::filesystem::directory_iterator(entry.path())) {
                num++;
                if(num<=30) continue;
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
                    jieba.CutForSearch(doc, words, true);

                    prob = get_NB(words);

                    std::string max_class;
                    double max = INT_MIN;
                    for (auto &i : prob) {
                        if (i.second > max) {
                            max = i.second;
                            max_class = i.first;
                        }
                        // std::cout<<i.second<<std::endl;
                    }
                    fs << entry.path().filename().string() << "/"
                       << sub_entry.path().filename().string() << " " << max_class
                       << std::endl;
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
    std::map<int, int> cm = class_map[c];
    double N = docs_num;
    for (auto &i : inverted_index) {
        double N11 = 0;
        for (auto &j : i.second) {
            if (cm.count(j.first))
                N11++;
        }
        double N10 = i.second.size() - N11;
        double N01 = cm.size() - N11;
        double N00 = N - N01 - N10 - N11;

        double mi =
            (N11 / N) *
                log2((N * N11 + 1) / ((N10 + N11 + 1) * (N01 + N11 + 1))) +
            (N01 / N) *
                log2((N * N01 + 1) / ((N00 + N01 + 1) * (N01 + N11 + 1))) +
            (N10 / N) *
                log2((N * N10 + 1) / ((N10 + N11 + 1) * (N00 + N10 + 1))) +
            (N00 / N) *
                log2((N * N00 + 1) / ((N00 + N01 + 1) * (N00 + N10 + 1)));

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

    MI_X2("dzbgs");
    MI_X2("jwb");
    MI_X2("kxjsb");
    MI_X2("yjsy");
    MI_X2("zsbgs");

    int check_mode = 2;
    train_NB(check_mode);
    if (check_mode == 0)
        std::cout << "Check class with x2" << std::endl;
    if (check_mode == 1)
        std::cout << "Check class with mi" << std::endl;
    if (check_mode == 2)
        std::cout << "Check class without feature selection" << std::endl;
    check_class(check_mode);
}
