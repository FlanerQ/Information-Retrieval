#include "libstemmer.h"
#include <iostream>
#include <string>

int main() {
    std::string s="fucking";
    sb_symbol *input=(sb_symbol*)s.c_str();
    std::cout<<"input: "<<input<<std::endl;
    struct sb_stemmer *stemmer = sb_stemmer_new("english", NULL);
    if (!stemmer) {
        std::cerr << "Error creating stemmer" << std::endl;
        return 1;
    }
    const sb_symbol *stemmed = sb_stemmer_stem(stemmer, input, s.size());
    std::string stem_res((char*)stemmed);
    std::cout<<"res: "<<stem_res<<std::endl;

    sb_stemmer_delete(stemmer);
    return 0;
}