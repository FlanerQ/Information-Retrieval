#include <string>
#include <vector>

#include "cppjieba/Jieba.hpp"

const char* const DICT_PATH = "dict/jieba.dict.utf8";
const char* const HMM_PATH = "dict/hmm_model.utf8";
const char* const USER_DICT_PATH = "dict/user.dict.utf8";
const char* const IDF_PATH = "dict/idf.utf8";
const char* const STOP_WORD_PATH = "dict/stop_words.utf8";

cppjieba::Jieba jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH,STOP_WORD_PATH);

std::map<std::string, std::vector<int>> inverted_index;
int docs_num;
int tokens = 0, terms;

// build inverted_index
void build_inverted_index() {
  std::fstream fs;
  fs.open("hw3.txt", std::ios::in);
  // build inverted_index
  if (fs.is_open()) {
    std::string doc;
    int line = 1;                    // record the count of docs
    while (std::getline(fs, doc)) {  // read text by line
      std::vector<std::string> words;
      jieba.CutForSearch(doc, words);
      tokens += words.size();
      for (auto word : words) {
        inverted_index[word].push_back(line);
      }
      line++;
    }
    terms = inverted_index.size();
    docs_num = line - 1;
    fs.close();
  }
  // print to txt
  fs.open("inverted.txt", std::ios::out);
  for (auto& i : inverted_index) {
    fs << i.first << ": ";
    for (auto& j : i.second) fs << j << " ";
    fs << std::endl;
  }
}

void show(const std::vector<int>& l){
    for(auto i:l) std::cout<<i<<" ";
    std::cout<<std::endl;
}

void print_query(std::string s){
	std::vector<int> q=inverted_index[s];
	std::cout<<s<<":";
	show(q);
	std::cout<<std::endl;
}

int main(int argc, char** argv) {
  build_inverted_index();
  std::cout <<"tokens : "<< tokens <<std::endl;
  std::cout <<"terms : "<< terms <<std::endl;
  std::cout<<std::endl;

  print_query("迁移");
  print_query("迁移学习");
  print_query("推荐");
  print_query("深度学习");
  print_query("隐私");
  print_query("跨领域");
  print_query("跨域");
}