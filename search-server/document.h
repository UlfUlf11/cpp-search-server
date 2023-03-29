#pragma once

#include <iostream>


enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};


struct Document {
    
    Document();
 
    Document(int id, double relevance, int rating);

    
    int id;
    double relevance;
    int rating;
};
 



