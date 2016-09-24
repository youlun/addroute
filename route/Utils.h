#pragma once
#include "stdafx.h"

inline void SplitString(std::vector<std::string> *tokens, const std::string *text, const char *sep)
{
    using std::string;
    string::size_type start = 0, end = 0;
    while ((end = text->find(sep, start)) != string::npos) {
        tokens->push_back(text->substr(start, end - start));
        start = end + 1;
    }
    tokens->push_back(text->substr(start));
}