#pragma once

#include "search_server.h"
#include "document.h"
#include <cassert>


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint);


template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
                     const std::string& func, unsigned line, const std::string& hint) {
    using namespace std;                     
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}


//для макросов ASSERT и ASSERT_HINT
void AssertImpl(const bool& expr, const std::string& text, const std::string& file, unsigned line, const std::string& func, const std::string& hint);




//макросы ASSERT и ASSERT_HINT
#define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __LINE__, __FUNCTION__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __LINE__, __FUNCTION__, (hint))

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

// -------- Начало модульных тестов поисковой системы ----------

void TestExcludeStopWordsFromAddedDocumentContent();

void TestAddDocument();

void TestStopWords();

void TestMinusWords();

void TestMatchedDocuments();

void TestSortFindedDocumentsByRelevance();

void TestFindByPredicate();

void TestFindByStatus();

void TestComputeAverageRating();

void TestComputeRelevance();

template <typename T>
void RunTestImpl(T function, std::string name);

template <typename T>
void RunTestImpl(T function, std::string name) {
    using namespace std;
    function();
    cerr << name << " OK"s << endl;
}

#define RUN_TEST(function)  RunTestImpl((function), #function)

void TestSearchServer();

// --------- Окончание модульных тестов поисковой системы -----------

