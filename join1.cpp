#include <iostream>
#include "file_manager.h"
#include "errors.h"
#include <cstring>
#include <fstream>
#include <climits>
#include <exception>

#include <vector> // only used for debugging purpose
#include <algorithm> // only used for debugging purpose (sort)

using namespace std;

// global variables
char *input1_file_path;
char *input2_file_path;
char *output_file_path; // create output in pwd
int MINUS_ONE = -1;
int int_min = INT_MIN;

// declarations
int getLastPageNumber(FileHandler &fh, bool keep_pinned);
PageHandler getLastPageHandler(FileHandler &fh, bool keep_pinned);
PageHandler getFirstPageHandler(FileHandler &fh, bool keep_pinned);
void fillWithIntMin(PageHandler &ph, const int &integers_per_page, int &integers_written_on_output_page);
void validateAnswers(FileManager &fm);
void linearSearch(const int &integers_per_page, char *input1_data, char *input2_data, FileHandler &output_handler, 
                  PageHandler &output_page_handler, int &integers_written_on_output_page, bool &join_exists);
void updateFilePaths(int argc, char **argv);

int main(int argc, char **argv) {
    updateFilePaths(argc, argv);
    int integers_per_page = PAGE_CONTENT_SIZE / sizeof(int);
    FileManager fm;
    FileHandler output_handler = fm.CreateFile(output_file_path);
    bool join_exists = false;
    PageHandler output_page_handler;
    int integers_written_on_output_page = 0;

    try {
        FileHandler input1_handler = fm.OpenFile(input1_file_path); // input1
        FileHandler input2_handler = fm.OpenFile(input2_file_path); // input2
        PageHandler input1_page_handler = getLastPageHandler(input1_handler, /*keep pinned*/true); // pinned
        int input2_last_page_number = getLastPageNumber(input2_handler, /*keep pinned*/false);// unpinned
        int counter = 0; // even for backward traversal and odd for forward traversal
        while (true) { // traversing from back
            char *input1_data = input1_page_handler.GetData();
            PageHandler input2_page_handler;
            if (counter %2 == 0)
                input2_page_handler = getLastPageHandler(input2_handler, /*keep_pinned*/false); // unpinned
            else
                input2_page_handler = getFirstPageHandler(input2_handler, /*keep_pinned*/false); // unpinned
            
            while( true ) { // traversing from back

                char *input2_data = input2_page_handler.GetData();
                linearSearch(/*passed by value*/integers_per_page, input1_data, input2_data, output_handler, 
                            /*passed by ref*/output_page_handler, integers_written_on_output_page, join_exists);

                if (input2_page_handler.GetPageNum() == 0 && counter % 2==0) break; // finished reading input2 file
                if (input2_page_handler.GetPageNum() == input2_last_page_number && counter % 2==1) break; // finished reading input2 file
                if (counter %2 == 0)
                    input2_page_handler = input2_handler.PrevPage(input2_page_handler.GetPageNum()); // pinned
                else
                    input2_page_handler = input2_handler.NextPage(input2_page_handler.GetPageNum()); // pinned
                input2_handler.UnpinPage(input2_page_handler.GetPageNum()); // unpinned
            }

            if (input1_page_handler.GetPageNum() == 0) break;
            input1_handler.FlushPage(input1_page_handler.GetPageNum()); // will page get flushed even if pinned ??
            input1_page_handler = input1_handler.PrevPage(input1_page_handler.GetPageNum()); // bringing input1 page in buffer
            ++counter;
        }

        if (join_exists) fillWithIntMin(output_page_handler, integers_per_page, integers_written_on_output_page);
        output_handler.FlushPages(); // flush all pages since we are done
        fm.CloseFile(input1_handler);
        fm.CloseFile(input2_handler);
    } catch (const char * error) {
        cout << error << endl;
    } catch (exception &e) {
        cout << e.what() << endl;
    }
    fm.CloseFile (output_handler);

    validateAnswers(fm); // TODO: only for debugging. Remove it in final submission
    return 0;
}

// Helper functions

void updateFilePaths(int argc, char **argv) {
    /* update file paths using command line args*/
    if (argc != 4) {
        cout << "ERROR: commandline arguments expected\n";
        exit(0);
    }
    input1_file_path = argv[1];
    input2_file_path = argv[2];
    output_file_path = argv[3];
}

inline int charToInt(char *array, int offset) {
    /* returns int(array[offset]); */
    int data;
    memcpy(&data, &array[offset], sizeof(int));
    return data;
}

void linearSearch(const int &integers_per_page, char *input1_data, char *input2_data, FileHandler &output_handler,
                  PageHandler &output_page_handler, int &integers_written_on_output_page, bool &join_exists) {
    /*
        for every integer in input1 data, for every integer in input2 data, if both integers
        are same then it writes in the output page. If output page is full then it will flush
        it and create a new page. Therefore last two parameters can be updated by this function.
        This function implements line-3, 4, 5, 6 of join1 algorithm-4 as mentioned in assignment pdf.
    */
    if (input1_data == nullptr or input2_data == nullptr) {
        throw "Exception: One of the input file data is emtpy. So output file will also be emtpy";
    }
    for (int i=0; i<integers_per_page; ++i) {
        int input1 = charToInt(input1_data, i*sizeof(int));
        if (input1 == INT_MIN) break;
        for (int j=0; j<integers_per_page; ++j) {
            int input2 = charToInt(input2_data, j*sizeof(int));
            if (input2 == INT_MIN) break;
            if (input1 == input2) { // write input1 on output page
                if (join_exists == false) {
                    // creating the first page since join has some result
                    output_page_handler = output_handler.NewPage();
                }
                join_exists = true;
                if (integers_written_on_output_page >= integers_per_page) { // output page is full. Get a new page.
                    output_handler.FlushPage(output_page_handler.GetPageNum()); // write otuput page to disk and remove from buffer
                    output_page_handler = output_handler.NewPage();	// pinned and dirty
                    integers_written_on_output_page = 0; // new page is empty
                }
                char *output_data = output_page_handler.GetData();
                memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &input1, sizeof(int));
                ++integers_written_on_output_page;
            }
        }
    }
}

void fillWithIntMin(PageHandler &ph, const int &integers_per_page, int &integers_written_on_output_page) {
    /*
     *  This function fills the remaining page with INT_MIN.
     *  Inputs:
     *      ph : page handler of a page where remaining entries have to be filled with INT_MIN
    */

    for (int j=integers_written_on_output_page; j<= integers_per_page; ++j) {	
        char *output_data = ph.GetData();
        memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &int_min, sizeof(int));
        ++integers_written_on_output_page;
    }
}
int getLastPageNumber(FileHandler &fh, bool keep_pinned=false) {
    /*
     *	When we call fh.LastPage().getPageNum(), it brings the last page
     *	into memory if not already in memory. By default it is pinned.
     *	Inputs:
     *		fh : file handler from wich to find the page number of last page
     *		keep_pinned : whether to keep the last page pinned or not
     *	Outputs: page number of the last page
     */
    PageHandler last_page_handler = fh.LastPage();
    int page_number = last_page_handler.GetPageNum();
    if (not keep_pinned)	fh.UnpinPage(page_number);
    return page_number;
}

PageHandler getLastPageHandler(FileHandler &fh, bool keep_pinned=false) {
    /*
     *	When we call fh.LastPage().getPageNum(), it brings the last page
     *	into memory if not already in memory. By default it is pinned.
     *	Inputs:
     *		fh : file handler from wich to find the page number of last page
     *		keep_pinned : whether to keep the last page pinned or not
     *	Outputs: pagehandler of the last page
     */
    PageHandler last_page_handler = fh.LastPage();
    int page_number = last_page_handler.GetPageNum();
    if (not keep_pinned)	fh.UnpinPage(page_number);
    return last_page_handler;
}

PageHandler getFirstPageHandler(FileHandler &fh, bool keep_pinned=false) {
    /*
     *	When we call fh.LastPage().getPageNum(), it brings the last page
     *	into memory if not already in memory. By default it is pinned.
     *	Inputs:
     *		fh : file handler from wich to find the page number of last page
     *		keep_pinned : whether to keep the last page pinned or not
     *	Outputs: pagehandler of the last page
     */
    PageHandler first_page_handler = fh.FirstPage();
    int page_number = first_page_handler.GetPageNum();
    if (not keep_pinned)	fh.UnpinPage(page_number);
    return first_page_handler;
}
vector<int> getAnswers(FileManager &fm, char *file_path, string title) {
    /*
     *	This function returns all integers stored in the file
     *	This function is mainly written for matching our answer with
     *	ground truth answer provided by TAs.
     */
    vector<int> answers;
    int integers_per_page = PAGE_CONTENT_SIZE / sizeof(int);
    FileHandler file_handler = fm.OpenFile(file_path);
    int last_page_num = getLastPageNumber(file_handler, /*keep pinned*/ false);
    PageHandler page_handler = file_handler.FirstPage(); // pinned

    while (true) {
        char *data = page_handler.GetData();
        for (int i=0; i<=(integers_per_page-2)*sizeof(int); i+= 2*sizeof(int)) {
            // read two integers in pair starting from location i
            int first_num, sec_num;
            memcpy(&first_num, &data[i], sizeof(int));
            memcpy(&sec_num, &data[i+sizeof(int)], sizeof(int));
            answers.push_back(first_num);
            answers.push_back(sec_num);
        }
        if (page_handler.GetPageNum() == last_page_num) break;
        file_handler.UnpinPage(page_handler.GetPageNum()); // unpinned
        page_handler = file_handler.NextPage(page_handler.GetPageNum()); // pinned
    }
    file_handler.UnpinPage(page_handler.GetPageNum());
    fm.CloseFile(file_handler);

    return answers;
}

void printVector(const vector<int> &v, const string &title) {
    /* prints vector v on STDOUt */
    cout << title << endl;
    int size = v.size();
    for (int i=0; i<size; ++i)
        cout << v[i] << ", ";
    cout << endl << endl;
}

void validateAnswers(FileManager &fm) {
    /*
        This function gets our answer and TA's answer in vector form.
        It then sorts it and sees if corresponding entries are same or not.
        Ideally after sorting, corresponding entries mush match.
    */
    char *my_output = "./output_join1";
    char *ta_output = "./TestCases/TC_join1/output_join1";
    vector<int> my_answers = getAnswers(fm, my_output, "My output");
    vector<int> ta_answers = getAnswers(fm, ta_output, "TA output");
    int size = my_answers.size();
    sort(&my_answers[0], (&my_answers[0]) + size);
    sort(&ta_answers[0], (&ta_answers[0]) + size);
    printVector(my_answers, "my answers");
    printVector(ta_answers, "ta answers");
    bool correct = true;
    for (int i=0; i<size; ++i) {
        if (my_answers[i] != ta_answers[i]) {
            cout << "ERROR: answer mismatch\n";
            cout << "your answer: " << my_answers[i] << endl;
            cout << "TA's answer: " << ta_answers[i] << endl;
            correct = false;
        }
    }
    if (correct)    cout << "Your answer is correct !!\n";
    else cout << "Your answer has some mismatches !!\n";
}









