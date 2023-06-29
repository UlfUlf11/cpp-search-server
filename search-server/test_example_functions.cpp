#include "test_example_functions.h"



using namespace std;



ostream& operator<< (ostream& out, const DocumentStatus& t)

{

    switch (t)
    {

    case DocumentStatus::ACTUAL:
    {

        string s = "ACTUAL"s;

        out << s;

        return out;

        break;

    }

    case DocumentStatus::IRRELEVANT:
    {

        string s = "IRRELEVANT"s;

        out << s;

        return out;

        break;

    }

    case DocumentStatus::BANNED:
    {

        string s = "BANNED"s;

        out << s;

        return out;

        break;

    }



    case DocumentStatus::REMOVED:
    {

        string s = "REMOVED"s;

        out << s;

        return out;

        break;

    }

    }

    return out;

}



//��� �������� ASSERT � ASSERT_HINT

void AssertImpl(const bool& expr, const std::string& text, const std::string& file, unsigned line, const std::string& func, const std::string& hint)

{

    using namespace std;

    if (!expr)

    {

        cerr << boolalpha;

        cerr << file << "("s << line << "): "s << func << ": "s;

        cerr << "ASSERT("s << text << ") failed."s;

        if (!hint.empty())

        {

            cout << " Hint: "s << hint;

        }



        cerr << endl;

        abort();

    }



}



// ���� ���������, ��� ��������� ������� ��������� ����-����� ��� ���������� ����������

void TestExcludeStopWordsFromAddedDocumentContent()
{

    using namespace std;

    const int doc_id = 42;

    const string content = "cat in the city"s;

    const vector<int> ratings = { 1, 2, 3 };

    {

        SearchServer server;

        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto found_docs = server.FindTopDocuments("in"s);

        ASSERT_EQUAL(found_docs.size(), 1);

        const Document& doc0 = found_docs[0];

        ASSERT_EQUAL(doc0.id, doc_id);

    }



    {

        SearchServer server("in the"s);

        //server.SetStopWords("in the"s);

        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),

            "Stop words must be excluded from documents"s);

    }

}



//���������� ����������. ����������� �������� ������ ���������� �� ���������� �������,

//������� �������� ����� �� ���������.

void TestAddDocument()

{

    const int doc_id = 42;

    const string content = "cat in the city"s;

    const vector<int> ratings = { 1, 2, 3 };



    // ��������� �������� � ��������� ��� ��� ����� ����� �� ������ ����� �� ���������

    {

        SearchServer server;

        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto found_docs = server.FindTopDocuments("cat in the city"s);

        ASSERT_EQUAL(found_docs.size(), 1);

        const Document& doc0 = found_docs[0];

        ASSERT_EQUAL(doc0.id, doc_id);

    }

}



//��������� ����-����. ����-����� ����������� �� ������ ����������.

void TestStopWords()

{

    const int doc_id = 42;

    const string content = "cat in the city"s;

    const vector<int> ratings = { 1, 2, 3 };



    {

        SearchServer server("in the"s);

        //server.SetStopWords("in the"s);

        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        ASSERT(server.FindTopDocuments("in"s).empty());

    }



}



//��������� �����-����.

//���������, ���������� �����-����� ���������� �������, �� ������ ���������� � ���������� ������.

void TestMinusWords()

{

    const int doc_id = 42;

    const string content = "cat in the city"s;

    const vector<int> ratings = { 1, 2, 3 };



    {

        SearchServer server;

        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        const auto found_docs = server.FindTopDocuments("cat in the city"s);

        ASSERT_EQUAL(found_docs.size(), 1);

        const Document& doc0 = found_docs[0];

        ASSERT_EQUAL(doc0.id, doc_id);

    }



    //��������� ����� �����

    {

        SearchServer server;

        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

        ASSERT(server.FindTopDocuments("-in"s).empty());

    }



}



//������� ����������. ��� �������� ��������� �� ���������� ������� ������ ���� ���������� ��� ����� �� ���������� �������,

//�������������� � ���������. ���� ���� ������������ ���� �� �� ������ �����-�����, ������ ������������ ������ ������ ����.

void TestMatchedDocuments()

{

    {

        SearchServer server;

        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, -3 });

        const int document_count = server.GetDocumentCount();

        for (int document_id = 0; document_id < document_count; ++document_id)

        {

            //������� ������ � ����������

            const auto& [words, status] = server.MatchDocument("fluffy cat"s, document_id);

            ASSERT_EQUAL(words.size(), 1);

            ASSERT_EQUAL(document_id, 0);

            ASSERT_EQUAL(status, DocumentStatus::ACTUAL);

        }

    }



    {

        SearchServer server("cat"s);

        //server.SetStopWords("cat"s);

        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, -3 });

        const int document_count = server.GetDocumentCount();

        for (int document_id = 0; document_id < document_count; ++document_id)

        {

            //������� ������ � ����������, �� ����������� ��� ���� �����

            const auto& [words, status] = server.MatchDocument("fluffy cat"s, document_id);

            ASSERT_EQUAL(words.size(), 0);

            ASSERT_EQUAL(document_id, 0);

            ASSERT_EQUAL(status, DocumentStatus::ACTUAL);

        }

    }



    {

        SearchServer server;

        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, -3 });

        const int document_count = server.GetDocumentCount();

        for (int document_id = 0; document_id < document_count; ++document_id)

        {

            //������� ������ � ����������, �� ��������� ����� ����� � ������

            const auto& [words, status] = server.MatchDocument("fluffy -cat"s, document_id);

            ASSERT_EQUAL(words.size(), 0);

            ASSERT_EQUAL(document_id, 0);

            ASSERT_EQUAL(status, DocumentStatus::ACTUAL);

        }

    }

}





//���������� ��������� ���������� �� �������������.

//������������ ��� ������ ���������� ���������� ������ ���� ������������� � ������� �������� �������������.

void TestSortFindedDocumentsByRelevance()

{

    {

        SearchServer server;



        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });

        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });

        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });



        const auto& documents = server.FindTopDocuments("fluffy soigne cat"s);



        int doc_size = static_cast<int>(documents.size());

        for (int i = 0; i < doc_size; ++i)

        {

            ASSERT(documents[i].relevance > documents[i + 1].relevance);

        }

    }



}



//���� ���������, ��� ������� ���������� ��������� � �������������� ���������, ���������� �������������

void TestFindByPredicate()

{



    {

        SearchServer server;



        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });

        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });

        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });





        const auto& found_docs = server.FindTopDocuments("fluffy soigne cat"s, [](int document_id, DocumentStatus status, int rating)
            {
                return status == DocumentStatus::BANNED;
            });



        {

            //������� ������ � ����������

            ASSERT_EQUAL(found_docs.size(), 1);

            const Document& doc0 = found_docs[0];

            ASSERT_EQUAL(doc0.id, 3);

        }

    }



}



void TestFindByStatus()

{

    {

        SearchServer server;



        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });

        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });

        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });



        const auto& found_docs = server.FindTopDocuments("fluffy soigne cat"s, DocumentStatus::BANNED);



        {

            //������� ������ � ����������

            ASSERT_EQUAL(found_docs.size(), 1);

            const Document& doc0 = found_docs[0];

            ASSERT_EQUAL(doc0.id, 3);

        }

    }



    {

        SearchServer server;



        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });

        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });

        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });



        const auto& found_docs = server.FindTopDocuments("fluffy soigne cat"s, DocumentStatus::ACTUAL);



        {

            //������� ������ � �����������

            ASSERT_EQUAL(found_docs.size(), 3);

        }

    }



    {

        SearchServer server;



        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });

        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });

        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });







        {

            ASSERT(server.FindTopDocuments("fluffy soigne cat"s, DocumentStatus::IRRELEVANT).empty());

        }

    }



    {

        SearchServer server;



        server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 8, -3 });

        server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

        server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, -12, 2 });

        server.AddDocument(3, "soigne starling eugeny"s, DocumentStatus::BANNED, { 9 });







        {

            ASSERT(server.FindTopDocuments("fluffy soigne cat"s, DocumentStatus::REMOVED).empty());

        }

    }



}



void TestComputeAverageRating()
{

    SearchServer server;



    server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });

    server.AddDocument(1, "fluffy cat fluffy tail"s, DocumentStatus::ACTUAL, { -7, -2, -7 });

    server.AddDocument(2, "soigne dog expressive eyes"s, DocumentStatus::ACTUAL, { 5, 12, -2 });



    {

        const auto found_docs = server.FindTopDocuments("white"s);

        ASSERT_EQUAL(found_docs[0].rating, (1 + 2 + 3) / 3);

    }



    {

        const auto found_docs = server.FindTopDocuments("tail"s);

        ASSERT_EQUAL(found_docs[0].rating, (-7 - 2 - 7) / 3);

    }



    {

        const auto found_docs = server.FindTopDocuments("dog"s);

        ASSERT_EQUAL(found_docs[0].rating, (5 + 12 - 2) / 3);

    }

}



void TestComputeRelevance()
{

    const auto EPSILON = 1e-6;

    SearchServer server;



    server.AddDocument(0, "white cat and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, -3 });



    const auto& documents = server.FindTopDocuments("fluffy soigne cat"s);



    double relevance = 0.2 * log(1);



    {

        ASSERT(abs(documents[0].relevance - relevance) < EPSILON);

    }

}



// ������� TestSearchServer �������� ������ ����� ��� ������� ������

void TestSearchServer()
{

    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);

    // �� �������� �������� ��������� ����� �����

    RUN_TEST(TestAddDocument);

    RUN_TEST(TestStopWords);

    RUN_TEST(TestMinusWords);

    RUN_TEST(TestMatchedDocuments);

    RUN_TEST(TestSortFindedDocumentsByRelevance);





    RUN_TEST(TestFindByPredicate);

    RUN_TEST(TestFindByStatus);

    RUN_TEST(TestComputeAverageRating);

    RUN_TEST(TestComputeRelevance);

}
