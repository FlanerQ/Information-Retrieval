#include "cppjieba/Jieba.hpp"
#include "libstemmer.h"
#include <algorithm>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/unordered_set.hpp>
#include <boost/serialization/vector.hpp>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

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

std::map<std::string, std::unordered_set<int>> class_map;

std::string folder_path =
    "/home/qym/Information-Retrieval/final/txts_1"; //要遍历的文件夹路径

std::string s_path =
    "/home/qym/Information-Retrieval/final/serialize/main.bin_1";//序列化文件路径

struct sb_stemmer *stemmer;

std::string parse_word(std::string word, std::string tag, sb_stemmer *stemmer) {
    if (tag == "eng" ) {
        //stemming when term is english word
        sb_symbol *input = (sb_symbol *)word.c_str();
        const sb_symbol *stemmed = sb_stemmer_stem(stemmer, input, word.size());
        std::string stem_res((char *)stemmed);
        //tolower
        std::transform(stem_res.begin(), stem_res.end(), stem_res.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        // std::cout<<stem_res<<std::endl;
        return std::move(stem_res);
    }
    return word;
}

void build_inverted_index() {
    for (const auto &entry : std::filesystem::directory_iterator(folder_path)) {
        if (entry.is_directory()) {
            std::string class_name = entry.path().filename();
            std::unordered_set<int> &docs_in_class = class_map[class_name];
            std::cout << "Start class " << class_name << std::endl;
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
                        if (tag == "x" || tag == "m" ||tag =="eng" && word.word.find('.') != std::string::npos)
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
}

//--------------build-TF-IDF-&-get-cosine-similiraty-------------

std::map<int, std::unordered_map<int, double>> docs_TF_IDF;

void build_TF_IDF_matrix() {
    int k = 0;
    int docs_num = docs_id.size();
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

//--------------------get-Top10---------------------
bool cmp(const std::pair<double, int> &a, const std::pair<double, int> &b) {
    if (a.first == b.first) {
        return a.second > b.second;
    }
    return a.first > b.first;
}

void showTopK(std::vector<std::pair<double, int>> &a) {
    std::sort(a.begin(), a.end(), cmp);
    if (a.size() == 0 || std::isnan(a.begin()->first)) {
        std::cout << "No matching result" << std::endl;
        return;
    }
    int k_size = (a.size() >= 10) ? 10 : a.size();
    std::vector<std::pair<double, int>> res(a.begin(), a.begin() + k_size);
    for (auto &i : res) {
        std::cout << "docID: " << i.second << " Score: " << std::fixed
                  << std::setprecision(2) << i.first << std::endl;
    }
}

void query(std::string que, std::vector<std::pair<double, int>> &res_cosine) {
    std::map<std::string, int> query_term_count;
    std::unordered_map<int, double> query_TF_IDF;
    std::set<int> query_term_contained_doc;
    std::vector<cppjieba::Word> words;
    jieba.CutForSearch(que, words);
    for (auto &word : words) {
        std::string tag = jieba.LookupTag(word.word); //根据tag删去符号
        if (tag == "x" || tag == "m")
            continue;
        std::string res = parse_word(word.word, tag, stemmer);
        if (query_term_count.count(res) == 0)
            query_term_count[res] = 1;
        else
            query_term_count[res]++;
        if (inverted_index.count(res) == 0)
            continue;
        for (auto &i : inverted_index[res]) {
            query_term_contained_doc.insert(i.first);
        }
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
    for (auto &i : query_term_contained_doc) {
        res_cosine.push_back(
            {getCosineSimilarity(query_TF_IDF, docs_TF_IDF[i]), i});
    }
    showTopK(res_cosine);
}

void wait_query() {
    std::string que;
    std::cout << std::endl << "Please enter query: " << std::endl;
    while (getline(std::cin, que)) {
        std::cout << "Start query: " << que << std::endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        // query(que);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        std::cout << "Query Time taken: " << duration.count() / 1000.0 << "s"
                  << std::endl
                  << std::endl;
        std::cout << "Please enter query: " << std::endl;
    }
}

void Serialization() {
    // 将myMap序列化到磁盘文件"map.bin"中
    std::cout << "Start serializing" << std::endl;
    std::ofstream ofs(s_path);
    boost::archive::text_oarchive oa(ofs);
    oa << inverted_index << class_map << IDF_Matrix << term_id_map
       << docs_TF_IDF;
}

void Deserialization() {
    std::cout << "Start deserializing" << std::endl;
    std::ifstream ifs(s_path);
    boost::archive::text_iarchive ia(ifs);
    ia >> inverted_index >> class_map >> IDF_Matrix >> term_id_map >>
        docs_TF_IDF;
}

template <typename TimeT = std::chrono::milliseconds> struct measure {
    template <typename F, typename... Args> static typename TimeT::rep execution(F &&func,  Args &&... args) {
        auto start = std::chrono::high_resolution_clock::now();
        std::forward<decltype(func)>(func)(std::forward<decltype(args)>(args)...);
        auto duration = std::chrono::duration_cast<TimeT>(
            std::chrono::high_resolution_clock::now() - start);
        return duration.count();
    }
};

//-------------------websocket-------------------
typedef websocketpp::server<websocketpp::config::asio> server;
server ws_server;

void exit_handler() { ws_server.stop(); }

void func(std::string a) { std::cout << "func: " << a << std::endl; }

void on_message(server *s, websocketpp::connection_hdl hdl,
                server::message_ptr msg) {
    std::string que = msg->get_payload();

    std::cout << "Start query: " << que << std::endl;
    std::vector<std::pair<double, int>> res;
    auto timeTaken = measure<>::execution(query,que,res);

    std::stringstream resp;
    int k = 0;
    for (const auto &i : res) {
        if (k++ == 10)
            break;
        std::string title;
        for (auto &c : class_map) {
            if (c.second.find(i.second) != c.second.end()) {
                std::string file_path = folder_path + "/" + c.first + "/" +
                                        std::to_string(i.second);
                std::cout << file_path << std::endl;
                std::ifstream file(file_path);
                getline(file, title);
                file.close();
                break;
            }
        }
        // if (i == *(std::prev(res.end())))
        //     resp << "{\"docid\":" << i.second << ",\"title\":" << title
        //          << ",\"similarity\":" << i.first << "}";
        // else
        resp << "{\"docid\":" << i.second << ",\"title\":\"" << title
             << "\",\"similarity\":" << i.first << "},";
    }
    resp << "{\"time_taken\":" << timeTaken << "}";
    std::cout << "Query Time taken: " << timeTaken / 1000.0 << "s"
              << std::endl
              << std::endl;

    // 发送响应消息
    std::string payload = "[" + resp.str() + "]";
    s->send(hdl, payload, msg->get_opcode());
}

void start_server() {
    ws_server.init_asio();
    ws_server.set_message_handler(std::bind(
        &on_message, &ws_server, std::placeholders::_1, std::placeholders::_2));
    ws_server.listen(8080);
    ws_server.start_accept();
    std::cout << "Server start" << std::endl;

    ws_server.run();
}

int main() {
    //creat stemmer
    stemmer = sb_stemmer_new("english", NULL);

    //build_inverted_index
    auto timeTaken = measure<>::execution(build_inverted_index);
    std::cout << "Inverted index built. Time taken: " << timeTaken / 1000.0
              << std::fixed << std::setprecision(2) << "s" << std::endl;

    //build_TF_IDF_matrix
    timeTaken = measure<>::execution(build_TF_IDF_matrix);
    std::cout << "TF-IDF matrix built. Time taken: " << timeTaken / 1000.0
              << std::fixed << std::setprecision(2) << "s" << std::endl;

    // //Serialization
    // timeTaken = measure<>::execution(Serialization);
    // std::cout << "Data Serialized. Time taken: " << timeTaken / 1000.0
    //           << std::fixed << std::setprecision(2) << "s" << std::endl;

    // //Deserialization
    // auto timeTaken = measure<>::execution(Deserialization);
    // std::cout << "Data Deserialized. Time taken: " << timeTaken / 1000.0
    //           << std::fixed << std::setprecision(2) << "s" << std::endl;

    // start_server();

    wait_query();
}
