/**
 * @file tester.cpp
 */

#include <memory>
#include <vector>
#include <utility>
#include <map>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <omp.h>

#include "tokenizers/parse_tree.h"
#include "classify/knn.h"
#include "io/textfile.h"
#include "io/compressed_file_reader.h"
#include "io/compressed_file_writer.h"
#include "stemmers/porter2_stemmer.h"
#include "index/lexicon.h"
#include "index/inverted_index.h"
#include "tokenizers/ngram_tokenizer.h"
#include "util/invertible_map.h"
#include "test/unit_test.h"

using std::vector;
using std::make_pair;
using std::pair;
using std::multimap;
using std::unordered_map;
using std::ofstream;
using std::cerr;
using std::cout;
using std::endl;

unordered_map<char, size_t> getFreqs(string filename)
{
    unordered_map<char, size_t> freqs;
    TextFile textfile(filename);

    char* start = textfile.opentext();
    unsigned int length = textfile.get_size();
    unsigned int index = 0;

    while(index < length)
    {
        ++freqs[start[index]];
        if(++index % 1000 == 0)
            cerr << "  " << (double) index / length * 100 << "%    \r";
    }
    cerr << "  100.00%  " << endl;

    textfile.closetext();
    return freqs;
}

void compress(string filename, string outfilename, const InvertibleMap<char, unsigned int> & mapping)
{
    TextFile textfile(filename);
    CompressedFileWriter writer(outfilename);

    char* start = textfile.opentext();
    unsigned int length = textfile.get_size();
    unsigned int index = 0;

    while(index < length)
    {
        unsigned int toWrite = mapping.getValueByKey(start[index]);
        writer.write(toWrite);
        if(++index % 1000 == 0)
            cerr << "  " << (double) index / length * 100 << "%    \r";
    }
    cerr << "  100.00%  " << endl;

    textfile.closetext();
}

void decompress(string infilename, string outfilename, const InvertibleMap<char, unsigned int> & mapping)
{
    CompressedFileReader reader(infilename);
    ofstream writer(outfilename);
    unsigned int val;
    while(reader.hasNext())
    {
        val = reader.next();
        writer << mapping.getKeyByValue(val);
    }
    writer.close();
}

InvertibleMap<char, unsigned int> getMapping(const unordered_map<char, size_t> & freqs)
{
    multimap<size_t, char> sorted;
    for(auto & it: freqs)
        sorted.insert(make_pair(it.second, it.first));

    InvertibleMap<char, unsigned int> mapping;
    unsigned int value = 1;
    for(auto it = sorted.rbegin(); it != sorted.rend(); ++it)
    {
        mapping.insert(it->second, value);
        ++value;
    }

    return mapping;
}

void testCompression(string filename)
{
    cerr << "Getting frequencies..." << endl;
    double start = omp_get_wtime();
    unordered_map<char, size_t> freqs = getFreqs(string(filename));
    cerr << "  found " << freqs.size() << " unique characters" << endl;
    cerr << "  " << omp_get_wtime() - start << " seconds elapsed" << endl;

    InvertibleMap<char, unsigned int> mapping = getMapping(freqs);

    cerr << "Compressing..." << endl;
    start = omp_get_wtime();
    compress(string(filename), "compressed.txt", mapping);
    cerr << "  " << omp_get_wtime() - start << " seconds elapsed" << endl;

    cerr << "Decompressing..." << endl;
    start = omp_get_wtime();
    decompress("compressed.txt", "uncompressed.txt", mapping);
    cerr << "  " << omp_get_wtime() - start << " seconds elapsed" << endl;
}

vector<Document> getDocs(const string & filename, const string & prefix)
{
    vector<Document> docs;
    Parser parser(filename, "\n");
    while(parser.hasNext())
    {
        string file = parser.next();
        docs.push_back(Document(prefix + file));
    }
    return docs;
}

void testIndexCreation()
{
    string prefix = "/home/sean/projects/senior-thesis-data/20newsgroups/";
    //string prefix = "/home/sean/projects/senior-thesis-data/6reviewers/";
    //string prefix = "/home/sean/projects/senior-thesis-data/10authors/";
    //string prefix = "/home/sean/projects/senior-thesis-data/simple/";
    string lexicon = "lexiconFile";
    string postings = "postingsFile";
    vector<Document> trainDocs = getDocs(prefix + "train.txt", prefix);
    std::shared_ptr<Tokenizer> tokenizer(new NgramTokenizer(1, NgramTokenizer::Word));
    std::unique_ptr<Index> index(new InvertedIndex(lexicon, postings, tokenizer));
    index->indexDocs(trainDocs, 1);
}

void testIndex()
{
    string prefix = "/home/sean/projects/senior-thesis-data/20newsgroups/";
    //string prefix = "/home/sean/projects/senior-thesis-data/6reviewers/";
    //string prefix = "/home/sean/projects/senior-thesis-data/10authors/";
    //string prefix = "/home/sean/projects/senior-thesis-data/simple/";
    string lexicon = "lexiconFile";
    string postings = "postingsFile";
    vector<Document> testDocs = getDocs(prefix + "test.txt", prefix);
    std::shared_ptr<Tokenizer> const tokenizer(new NgramTokenizer(1, NgramTokenizer::Word));
    std::shared_ptr<Index> index(new InvertedIndex(lexicon, postings, tokenizer));

    size_t numQueries = 1;
    size_t numCorrect = 0;
    for(auto & query: testDocs)
    {
        string result = KNN::classify(query, index, 1);
        if(result == query.getCategory())
        {
            ++numCorrect;
            cout << " -> " << Common::makeGreen("OK");
        }
        else
            cout << " -> " << Common::makeRed("incorrect");
        cout << " (" << result << ")" << endl << "  -> " << ((double) numCorrect / numQueries * 100)
             << "% accuracy, " << numQueries << "/" << testDocs.size() << " processed " << endl;
        ++numQueries;
    }
}

void testParseTrees()
{
    vector<ParseTree> trees = ParseTree::getTrees("input.tree");
    ofstream out("output.tree");
    for(auto & tree: trees)
        out << tree.getString() << endl;
    out.close();
}

void thing()
{
    ASSERT(1 == 1);
    ASSERT(2 == 3);
    PASS;
}

void thing1()
{
    int* p = NULL;
    *p = 1;
    PASS;
}

void thing2()
{
    while(1);
    PASS;
}

void thing3()
{
    PASS;
}

void testTest()
{
    cout << "Running tests..." << endl;
    UnitTests::runTest("MyFirstTest", thing);
    UnitTests::runTest("MySecondTest", thing1);
    UnitTests::runTest("InfiniteLoopTest", thing2, 3);
    UnitTests::runTest("LastTest", thing3);
}

int main(int argc, char* argv[])
{
    //testCompression(string(argv[1]));
    //testIndexCreation();
    //testIndex();
    //testParseTrees();
    testTest();

    return 0;
}