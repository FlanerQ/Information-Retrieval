#include "cppjieba/Jieba.hpp"
#include "libstemmer.h"
#include <nlohmann/json.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

//jieba相关文件路径
const char* const DICT_PATH = "dict/jieba.dict.utf8";
const char* const HMM_PATH = "dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "dict/user.dict.utf8";
const char* const IDF_PATH = "dict/idf.utf8";
const char* const STOP_WORD_PATH = "dict/stop_words.utf8";
//声明结巴对象
cppjieba::Jieba jieba(DICT_PATH, HMM_PATH, 
USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH);

// record the id of term
std::unordered_map<std::string, int> term_id_map;

// record words count
std::map<int, int> word_count;

// map<term,map<docid,vector<position>>> 倒排索引
std::map<std::string, std::map<int, std::vector<int>>> inverted_index;

//<term,<docID,TF-IDF>> 每个term-doc的tf-idf值
std::map<int, std::unordered_map<int, double>> docs_TF_IDF;

// 记录每个term的idf值 用于计算query的tf-idf值
std::unordered_map<std::string, double> IDF_Matrix;   // IDF Matrix

// 纯文本路径
std::string txts_path = "/home/qym/Information-Retrieval/final/txts";

// 文件属性json文件路径
std::string json_path = "/home/qym/Information-Retrieval/final/json";

// 序列化文件路径
std::string s_path = "/home/qym/Information-Retrieval/final/serialize/"
                     "main.bin";

// 声明stemmer
struct sb_stemmer* stemmer;

// doc属性结构
struct doc_metadata
{
    std::string type;   // 类型
    std::string dept;   // 发文单位
    std::string date;   // 发表日期
    std::string url;    // url
    int dayy;           // 与系统时间相比的间隔 (1 7 30 ...)

    // 用于序列化
    template<class Archive> void serialize(Archive& ar, const unsigned int version)
    {
        ar& type;
        ar& dept;
        ar& date;
        ar& dayy;
        ar& url;
    }
};

// 每个doc的属性
std::unordered_map<int, doc_metadata> docs_metadata;

// 对英文token 词干提取 转小写
std::string parse_word(std::string word, std::string tag, sb_stemmer* stemmer)
{
    if (tag == "eng") {
        // stemming when term is english word
        sb_symbol* input = (sb_symbol*)word.c_str();
        const sb_symbol* stemmed = sb_stemmer_stem(stemmer, input, word.size());
        std::string stem_res((char*)stemmed);
        // tolower
        std::transform(stem_res.begin(), stem_res.end(), stem_res.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        // std::cout<<stem_res<<std::endl;
        return std::move(stem_res);
    }
    return word;
}

// split函数
std::vector<std::string> split(const std::string& s, const std::string& delimiters = " ")
{
    std::vector<std::string> tokens;
    std::string ::size_type lastPos = s.find_first_not_of(delimiters, 0);
    std::string ::size_type pos = s.find_first_of(delimiters, lastPos);
    while (std::string ::npos != pos || std::string ::npos != lastPos) {
        tokens.push_back(s.substr(lastPos, pos - lastPos));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
    return std::move(tokens);
}

// 获取每个doc的的dayy属性
int get_dayy(std::string date)
{
    std::tm tm = {};
    std::istringstream ss(date);
    ss >> std::get_time(&tm, "%Y-%m-%d");
    std::time_t t = std::mktime(&tm);
    auto timestamp_input = std::chrono::system_clock::from_time_t(t);
    auto now = std::chrono::system_clock::now();
    auto timestamp_now = std::chrono::system_clock::to_time_t(now);
    auto interval =
        std::chrono::duration_cast<std::chrono::hours>(now - timestamp_input).count() / 24;
    if (interval <= 1)
        return 1;
    else if (interval > 1 && interval <= 7)
        return 7;
    else if (interval > 7 && interval <= 30)
        return 30;
    return 2023;
}

//load json & get metadata of docs
using json = nlohmann::json;
void build_docs_metadata()
{
    std::cout << "Start building docs metadata" << std::endl;
    for (const auto& entry : boost::filesystem::directory_iterator(json_path)) {
        if (!is_directory(entry.path())) {
            std::cout << split(entry.path().filename().string(), ".")[0] << std::endl;
            boost::filesystem::ifstream f(entry.path());
            json data = json::parse(f);
            for (auto& i : data) {
                doc_metadata data;
                data.type = i.at("type");
                data.dept = i.at("dept");
                data.url = i.at("url");
                std::string date = i.at("date");
                data.date = date;
                int year = std::stoi(split(date, "-")[0]);
                if (year != 2023)
                    data.dayy = year;
                else
                    data.dayy = get_dayy(date);
                std::string sid = i.at("id");
                docs_metadata[std::stoi(sid)] = data;
            }
        }
    }
}

// print to inverted.txt
void print_inverted_index()
{
    std::fstream fs;
    fs.open("inverted.txt", std::ios::out);
    for (auto& i : inverted_index) {
        fs << i.first << ": ";
        for (auto& j : i.second) {
            fs << j.first << "<" << j.second.front();
            for (int k = 1; k < j.second.size(); k++) {
                fs << "," << j.second[k];
            }
            fs << "> ";
        }
        fs << std::endl;
    }
}

void build_inverted_index()
{
    std::cout << "Start building inverted index" << std::endl;
    for (const auto& entry : boost::filesystem::directory_iterator(txts_path)) {
        if (is_directory(entry.path())) {
            std::string class_name = entry.path().filename().string();
            std::cout << class_name << std::endl;
            for (const auto& sub_entry : boost::filesystem::directory_iterator(entry.path())) {
                if (!is_directory(sub_entry.path())) {
                    int doc_name = std::stoi(sub_entry.path().filename().string());
                    boost::filesystem::ifstream file(sub_entry.path());
                    std::string line;
                    std::string doc;
                    while (std::getline(file, line)) {
                        doc += line;
                    }
                    std::vector<cppjieba::Word> words;
                    jieba.CutForSearch(doc, words, true);
                    int count = 0;
                    for (auto word : words) {
                        std::string tag = jieba.LookupTag(word.word);   // 根据tag删去符号
                        if (tag == "x" || tag == "m" ||
                            tag == "eng" && word.word.find('.') != std::string::npos)
                            continue;
                        std::string res = parse_word(word.word, tag, stemmer);
                        inverted_index[res][doc_name].emplace_back(word.offset);
                        count++;
                    }
                    word_count[doc_name] = count;
                }
            }
        }
    }
    // print to inverted.txt
    //  print_inverted_index();
}

//--------------build-TF-IDF-&-get-cosine-similiraty-------------

void build_TF_IDF_matrix()
{
    int k = 0;
    int docs_num = docs_metadata.size();
    for (auto& i : inverted_index) {
        // get IDF value of term
        // idf = ln(Total Number Of Documents / Number Of Documents with term in it + 1)
        double idf = log(double(docs_num) / double(i.second.size()+1));
        term_id_map[i.first] = k;
        IDF_Matrix[i.first] = idf;
        for (auto& j : i.second) {
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
double getCosineSimilarity(std::unordered_map<int, double> m1, std::unordered_map<int, double> m2)
{
    double dotProduct = 0.0;
    double norm1 = 0.0;
    double norm2 = 0.0;

    // get the dot product and norm1
    for (auto& i : m1) {
        int col = i.first;
        double row1Value = i.second;
        if (m2.count(col)) {
            double row2Value = m2[col];
            dotProduct += row1Value * row2Value;
        }
        norm1 += row1Value * row1Value;
    }
    // get norm2
    for (auto& i : m2) {
        double row2Value = i.second;
        norm2 += row2Value * row2Value;
    }
    // get cosine similarity
    double cs = dotProduct / (sqrt(norm1) * sqrt(norm2));
    return cs;
}

//--------------------get-Top10---------------------
bool cmp(const std::pair<double, int>& a, const std::pair<double, int>& b)
{
    if (a.first == b.first) {
        return a.second > b.second;
    }
    return a.first > b.first;
}

void getTopK(std::vector<std::pair<double, int>>& a)
{
    std::sort(a.begin(), a.end(), cmp);
    if (a.size() == 0 || std::isnan(a.begin()->first)) {
        std::cout << "No matching result" << std::endl;
        return;
    }
    int k_size = (a.size() >= 10) ? 10 : a.size();
    std::vector<std::pair<double, int>> res(a.begin(), a.begin() + k_size);
    for (auto& i : res) {
        std::cout << "docID: " << i.second << " Score: " << std::fixed << std::setprecision(2)
                  << i.first << std::endl;
    }
}

//query & get Top 10
void query(std::string que, std::vector<std::pair<double, int>>& res_cosine)
{
    std::map<std::string, int> query_term_count;
    std::unordered_map<int, double> query_TF_IDF;
    std::set<int> query_term_contained_doc;
    std::vector<cppjieba::Word> words;
    jieba.CutForSearch(que, words);
    for (auto& word : words) {
        std::string tag = jieba.LookupTag(word.word);   // 根据tag删去符号
        if (tag == "x" || tag == "m") continue;
        // record count of terms for cal query's tf-idf
        std::string res = parse_word(word.word, tag, stemmer);
        if (query_term_count.count(res) == 0)
            query_term_count[res] = 1;
        else
            query_term_count[res]++;
        if (inverted_index.count(res) == 0) continue;
        // record docs which contain terms of query
        for (auto& i : inverted_index[res]) {
            query_term_contained_doc.insert(i.first);
        }
    }
    // get tf-idf for query
    for (auto& i : query_term_count) {
        if (term_id_map.count(i.first) == 0) continue;
        double tf = (double)i.second / (query_term_count.size());
        double idf = IDF_Matrix[i.first];
        query_TF_IDF[term_id_map[i.first]] = tf * idf;
    }
    // Cosine Similarity for query & docs
    for (auto& i : query_term_contained_doc) {
        res_cosine.push_back({getCosineSimilarity(query_TF_IDF, docs_TF_IDF[i]), i});
    }
    getTopK(res_cosine);
}

void wait_query()
{
    std::string que;
    std::cout << std::endl << "Please enter query: " << std::endl;
    while (getline(std::cin, que)) {
        std::cout << "Start query: " << que << std::endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        // query(que);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << "Query Time taken: " << duration.count() / 1000.0 << "s" << std::endl
                  << std::endl;
        std::cout << "Please enter query: " << std::endl;
    }
}

void Serialization()
{
    std::cout << "Start serializing" << std::endl;
    std::ofstream ofs(s_path);
    boost::archive::text_oarchive oa(ofs);
    oa << inverted_index << docs_metadata << IDF_Matrix << term_id_map << docs_TF_IDF;
}

void Deserialization()
{
    std::cout << "Start deserializing" << std::endl;
    std::ifstream ifs(s_path);
    boost::archive::text_iarchive ia(ifs);
    ia >> inverted_index >> docs_metadata >> IDF_Matrix >> term_id_map >> docs_TF_IDF;
}

//函数计时
template<typename TimeT = std::chrono::milliseconds> struct measure
{
    template<typename F, typename... Args>
    static typename TimeT::rep execution(F&& func, Args&&... args)
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::forward<decltype(func)>(func)(std::forward<decltype(args)>(args)...);
        auto duration =
            std::chrono::duration_cast<TimeT>(std::chrono::high_resolution_clock::now() - start);
        return duration.count();
    }
};

//-------------------websocket-------------------
typedef websocketpp::server<websocketpp::config::asio> server;
server ws_server;

void on_message(server* s, websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    std::vector<std::string> tokens = split(msg->get_payload(), "/");
    std::string que = tokens[0];
    std::string type = tokens[1];
    int dayy = std::stoi(tokens[2]);
    std::string dept = tokens[3];
    std::cout << "Start query: " << que << " Type: " << type << " Dayy: " << dayy
              << " dept: " << dept << std::endl;
    std::vector<std::pair<double, int>> res;
    auto timeTaken = measure<>::execution(query, que, res);
    auto start = std::chrono::high_resolution_clock::now();

    int k = 10;
    //根据类别筛选结果
    if (que != "none_query") {
        res.erase(
            std::remove_if(res.begin(),
                           res.end(),
                           [&type, &dayy, &dept](const auto& i) {
                               return type != "none_type" && docs_metadata[i.second].type != type ||
                                      dayy != -1 && docs_metadata[i.second].dayy != dayy ||
                                      dept != "none_dept" && docs_metadata[i.second].dept != dept;
                           }),
            res.end());
    }
    timeTaken += std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::high_resolution_clock::now() - start)
                     .count();
    //将query结果转为json类型 并回复给html
    std::stringstream resp;
    for (const auto& i : res) {
        if (k-- == 0) break;
        std::string title;
        std::string date = docs_metadata[i.second].date;
        std::string dept = docs_metadata[i.second].dept;
        std::string url = docs_metadata[i.second].url;
        std::string file_path =
            txts_path + "/" + docs_metadata[i.second].type + "/" + std::to_string(i.second);
        std::ifstream file(file_path);
        getline(file, title);
        file.close();
        std::string parse_title;
        parse_title.reserve(title.size());
        //转移引号
        std::transform(
            title.begin(), title.end(), std::back_inserter(parse_title), [](char c) -> char {
                if (c == '\"') {
                    return '\\' + c;
                }
                else {
                    return c;
                }
            });
        resp << "{\"url\":\"" << url << "\",\"date\":\"" << date << "\",\"title\":\"" << parse_title
             << "\",\"dept\":\"" << dept << "\",\"similarity\":" << i.first << "},";
    }
    //发送查询用时
    resp << "{\"time_taken\":" << timeTaken << "}";
    std::cout << "Query Time taken: " << timeTaken / 1000.0 << "s" << std::endl << std::endl;
    //回复给html
    std::string payload = "[" + resp.str() + "]";
    s->send(hdl, payload, msg->get_opcode());
}

void start_server()
{
    ws_server.init_asio();
    ws_server.set_message_handler(
        std::bind(&on_message, &ws_server, std::placeholders::_1, std::placeholders::_2));
    ws_server.listen(8080);
    ws_server.start_accept();
    std::cout << "Server start" << std::endl;
    ws_server.run();
}

int main()
{
    // create stemmer
    stemmer = sb_stemmer_new("english", NULL);

    // //buiild_docs_metadata
    // auto timeTaken = measure<>::execution(build_docs_metadata);
    // std::cout << "Docs metadata built. Time taken: " << timeTaken / 1000.0
    //           << std::fixed << std::setprecision(2) << "s" << std::endl;

    // //build_inverted_index
    // timeTaken = measure<>::execution(build_inverted_index);
    // std::cout << "Inverted index built. Time taken: " << timeTaken / 1000.0
    //           << std::fixed << std::setprecision(2) << "s" << std::endl;

    // //build_TF_IDF_matrix
    // timeTaken = measure<>::execution(build_TF_IDF_matrix);
    // std::cout << "TF-IDF matrix built. Time taken: " << timeTaken / 1000.0
    //           << std::fixed << std::setprecision(2) << "s" << std::endl;

    // //Serialization
    // timeTaken = measure<>::execution(Serialization);
    // std::cout << "Data Serialized. Time taken: " << timeTaken / 1000.0
    //           << std::fixed << std::setprecision(2) << "s" << std::endl;

    // Deserialization
    auto timeTaken = measure<>::execution(Deserialization);
    std::cout << "Data Deserialized. Time taken: " << timeTaken / 1000.0 << std::fixed
              << std::setprecision(2) << "s" << std::endl;
    std::cout<<"Terms count: " << inverted_index.size()<<std::endl;

    start_server();

    // wait_query();
}
