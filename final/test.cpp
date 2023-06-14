#include <iostream>

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
#include<iomanip>
#include <nlohmann/json.hpp>

std::string json_path = "/home/qym/Information-Retrieval/final/parsed_lists";
std::string dir_path = "/home/qym/Information-Retrieval/final/json/";

using json = nlohmann::json;
void change_json() {
    std::cout << "Start building docs metadata" << std::endl;
    for (const auto &entry : boost::filesystem::directory_iterator(json_path)) {
        std::ofstream output_file(dir_path+entry.path().filename().string());
        if (!is_directory(entry.path())) {
            std::cout << entry.path().filename()
                      << std::endl;
            boost::filesystem::ifstream f(entry.path());
            json data = json::parse(f);
            for (auto &i : data) {
                std::string id=i["id"];
                i["url"] = "https://www1.szu.edu.cn/board/view.asp?id="+id;
            }
            output_file <<data.dump(4);
            output_file.close();
        }
    }
}

int main() {
}
