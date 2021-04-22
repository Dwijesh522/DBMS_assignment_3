//Sample file for students to get their code running

#include <iostream>
#include "file_manager.h"
#include "errors.h"
#include <cstring>
#include <fstream>
#include <climits>

using namespace std;

int getSearchValue(const string &line) {
    /*
    *	Given a line read from the search query file of the form:
    *	SEARCH <VALUE>
    *	this funtion returns integer(<VALUE>)
    */
    int length = line.length();
    string number = "";
    for (int i=7; i<length; ++i)
        number += line[i];
    return stoi(number);
}

char *input_file_path;
char *query_file_path;
char *output_file_path; // create output in pwd
int MINUS_ONE = -1;
int int_min = INT_MIN;

void updateFilePaths(int argc, char **argv) {
    /* This file updates file paths using command line arguments */
    if (argc != 4) {
        cout << "ERROR: command line arguments expected\n";
        exit(0);
    }
    input_file_path = argv[1];
    query_file_path = argv[2];
    output_file_path = argv[3];
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

void printAnswers(FileManager &fm, char *file_path, string title) {
    /*
     *	This function prints all integers stored in the file in pairs
     *	This function is mainly written for matching our answer with
     *	ground truth answer provided by TAs. Call this funtion on both
     *	files one after another and check correctness manually.
     */
    cout << endl << title << endl;
    int integers_per_page = PAGE_CONTENT_SIZE / sizeof(int);
    FileHandler file_handler = fm.OpenFile(file_path);
    int last_page_num = getLastPageNumber(file_handler, /*keep pinned*/ false);
    PageHandler page_handler = file_handler.FirstPage(); // pinned

    while (true) {
        char *data = page_handler.GetData();
        cout << "page number: " << page_handler.GetPageNum() << ": ";
        for (int i=0; i<=(integers_per_page-2)*sizeof(int); i+= 2*sizeof(int)) {
            // read two integers in pair starting from location i
            int first_num, sec_num;
            memcpy(&first_num, &data[i], sizeof(int));
            memcpy(&sec_num, &data[i+sizeof(int)], sizeof(int));
            cout << "(" << first_num <<", " << sec_num << ") ";
        }
        if (page_handler.GetPageNum() == last_page_num) break;
        file_handler.UnpinPage(page_handler.GetPageNum()); // unpinned
        page_handler = file_handler.NextPage(page_handler.GetPageNum()); // pinned
        cout << endl;
    }
    file_handler.UnpinPage(page_handler.GetPageNum());
    fm.CloseFile(file_handler);
    cout << endl;
}

int main(int argc, char **argv) {
    updateFilePaths(argc, argv);
    int integers_per_page = PAGE_CONTENT_SIZE / sizeof(int);
    FileManager fm;
    FileHandler output_handler = fm.CreateFile(output_file_path);
    FileHandler input_handler = fm.OpenFile(input_file_path);
    // by default following page is pinned and marked dirty
    PageHandler output_page_handler = output_handler.NewPage();
    int integers_written_on_output_page = 0;
    // reading query files
    ifstream query_file (query_file_path);
    if (query_file.is_open()) {
        string line;
        while (getline(query_file, line)) {
            int target_number = getSearchValue(line); // value to search for

            // since the input file is sorted, once processed all target values,
            // we need not process the remaining values
            bool found_it = false, query_processed = false;
            PageHandler curr_page_handler = getLastPageHandler(input_handler, /*keep_pinned*/true);

            // Looping from last page to first page
            while (true) {
                char *data = curr_page_handler.GetData();
                for (int i=sizeof(int)*(integers_per_page-1); i>= 0; i-= sizeof(int)) {
                    // traversing from the last entry of the page to first entry
                    int curr_number;
                    memcpy(&curr_number, &data[i], sizeof(int));
                    if (curr_number == target_number) {
                        found_it = true;
                        // store (page num, offset) into the output file page
                        int curr_page_number = curr_page_handler.GetPageNum();
                        int offset = i/sizeof(int);
                        if (integers_written_on_output_page >= integers_per_page) {
                            output_handler.UnpinPage(output_page_handler.GetPageNum());//so that it can be flushed
                            output_handler.FlushPage(output_page_handler.GetPageNum()); // write otuput page to disk and remove from buffer
                            output_page_handler = output_handler.NewPage();	// create new output page in buffer
                            integers_written_on_output_page = 0; // new page is empty
                        }
                        char *output_data = output_page_handler.GetData();
                        memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &curr_page_number, sizeof(int));
                        ++integers_written_on_output_page;
                        memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &offset, sizeof(int));
                        ++integers_written_on_output_page;
                    }
                    // TODO: remove this else block: input is not sorted
                    //else if (curr_number != target_number and found_it == true) {
                        // we are done with search. Terminate it.
                    //    query_processed = true;
                    //    break;
                    //}
                }
                // since we are done using the current input page, we can unpin it
                input_handler.UnpinPage(curr_page_handler.GetPageNum());
                if (query_processed) break;
                if (curr_page_handler.GetPageNum() == 0) break; // we are done processing all input pages
                curr_page_handler = input_handler.PrevPage(curr_page_handler.GetPageNum()); // by default pinned
            }

            input_handler.UnpinPage(curr_page_handler.GetPageNum());// so that it can be replaced
            // Since we are done with one query, writing (-1, -1) pair in output page
            if (integers_written_on_output_page >= integers_per_page) {
                output_handler.UnpinPage(output_page_handler.GetPageNum());//so that it can be flushed
                output_handler.FlushPage(output_page_handler.GetPageNum()); // write otuput page to disk and remove from buffer
                output_page_handler = output_handler.NewPage();	// create new output page in buffer
                integers_written_on_output_page = 0; // new page is empty
            }
            char *output_data = output_page_handler.GetData();
            memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &MINUS_ONE, sizeof(int));
            ++integers_written_on_output_page;
            memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &MINUS_ONE, sizeof(int));
            ++integers_written_on_output_page;

            //break; // #TODO: remove this line for processing all queries
        }
        // fill the empty space with int_min
        for (int j=integers_written_on_output_page; j<= integers_per_page; ++j) {	
            char *output_data = output_page_handler.GetData();
            memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &int_min, sizeof(int));
            ++integers_written_on_output_page;
        }
    }
    else cout << "Unable to open query file\n";
    output_handler.UnpinPage(output_page_handler.GetPageNum());//so that it can be flushed
    output_handler.FlushPages(); // flush output pages
    fm.CloseFile (output_handler);
    fm.CloseFile(input_handler);

    // #TODO: following lines are only for debugging. Remove it in final submission
    char *my_output = "./output_search";
    char *ta_output = "./TestCases/TC_search/output_search";
    printAnswers(fm, my_output, "My output");
    printAnswers(fm, ta_output, "TA output");
    return 0;
}
