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

const char *input_file_path = "./TestCases/TC_search/sorted_input";
const char *query_file_path = "./TestCases/TC_search/query_search.txt";
const char *output_file_path = "./output_search"; // create output in pwd
int MINUS_ONE = -1;
int int_min = INT_MIN;

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

PageHandler getPageHandler(FileHandler &fh, int page_number, bool keep_pinned=false) {
	/*
	 *	When we call fh.FirstPage().getPageNum(), it brings the last page
	 *	into memory if not already in memory. By default it is pinned.
	 *	Inputs:
	 *		fh : file handler from wich to find the page number of last page
	 *		keep_pinned : whether to keep the last page pinned or not
	 *	Outputs: pagehandler of the last page
	 */
	PageHandler page_handler = fh.PageAt(page_number);
	if (not keep_pinned)	fh.UnpinPage(page_number);
	return page_handler;
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

int main() {

	int integers_per_page = PAGE_CONTENT_SIZE / sizeof(int);
	int last_index = PAGE_CONTENT_SIZE - sizeof(int);
	FileManager fm;
	FileHandler output_handler = fm.CreateFile(output_file_path);
	// by default following page is pinned and marked dirty
	PageHandler output_page_handler = output_handler.NewPage();
	int integers_written_on_output_page = 0;
	// reading query files
	ifstream query_file (query_file_path);
	if (query_file.is_open()) {
		string line;
		while (getline(query_file, line)) {
			int target_number = getSearchValue(line); // value to search for
            cout<<"Target : "<<target_number<<endl;
			// since the input file is sorted, once processed all target values,
			// we need not process the remaining values
			bool go_fwd = false, go_bwd = false, query_processed = false;
			bool bwd_search_done = false, fwd_search_done = false;
			FileHandler input_handler = fm.OpenFile(input_file_path);
            int total_pages = getLastPageNumber(input_handler, /*keep pinned*/ false);
			int top_pg = 0;
			int bottom_pg = total_pages;
			int curr_page_number;
			// Page found by binary search
			int index_page_number=-1;
			PageHandler curr_page_handler;
            int search_count = 0;
            int count_fwd = 0;
            int count_bwd = 0;
			// Binary Search on pages
			while (top_pg <= bottom_pg && search_count<=total_pages) {
			    search_count++;
			    // Update to next page
			     if(index_page_number<0){
                    curr_page_number = (top_pg+bottom_pg)/2;
                }
                // index_page found, go backwards
                if (go_bwd && !bwd_search_done){
                    count_bwd++;
                    if(index_page_number - count_bwd >=0)
                        curr_page_number = index_page_number - count_bwd;
                    else{
                        bwd_search_done = true;
                    }

                }
                // index page found and backward search done, now do forward search
                if(go_fwd && !fwd_search_done && bwd_search_done){
                    count_fwd++;
                    if(index_page_number + count_fwd < total_pages + 1)
                        curr_page_number = index_page_number+count_fwd;
                    else{
                        fwd_search_done = true;
                    }
                }

                // searched back and forth from index page. Exit.
                if(fwd_search_done && bwd_search_done)
                    break;

			    curr_page_handler = getPageHandler(input_handler, curr_page_number, /*keep_pinned*/true);
                char *data = curr_page_handler.GetData();

                // first entry on curr page
                int curr_number_top;
				memcpy(&curr_number_top, &data[0], sizeof(int));

				// last entry on curr page
                int curr_number_bottom;
				memcpy(&curr_number_bottom, &data[last_index], sizeof(int));

				if(curr_number_top < target_number){
                    // not found yet
				    if (curr_number_bottom < target_number){
				        // if already go_bwd then stop bwd search
				        if (go_bwd){ bwd_search_done = true;}
				        // update top_pg for binary search
				        else {top_pg = curr_page_number+1;}
				    }
                    // Found the start
				    else if(curr_number_bottom >= target_number){

				        // Found the index page - now just go up and down in directory from this page
				        if(index_page_number<0)
				            index_page_number = curr_page_number;
				        go_bwd = true, bwd_search_done = true; /* no need to check pages before as curr_number_top < target_number */
				        go_fwd = true;

                        for (int i=sizeof(int)*(integers_per_page-1); i>= 0; i-= sizeof(int)) {
                            // traversing from the last entry of the page to first entry
                            int curr_number;
                            memcpy(&curr_number, &data[i], sizeof(int));
                            bool found_it = false;
                            if (curr_number == target_number) {
                                found_it = true;
                                // store (page num, offset) into the output file page
                                int offset = i/sizeof(int);
                                cout<<"Page numb:"<<curr_page_number<<" at "<<offset<< "val = "<< curr_number<<endl;

                                if (integers_written_on_output_page >= integers_per_page) {
                                    output_handler.UnpinPage(output_page_handler.GetPageNum());//so that it can be flushed
                                    output_handler.FlushPages(); // write otuput page to disk and remove from buffer
                                    output_page_handler = output_handler.NewPage();	// create new output page in buffer
                                    integers_written_on_output_page = 0; // new page is empty
                                }
                                char *output_data = output_page_handler.GetData();
                                memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &curr_page_number, sizeof(int));
                                ++integers_written_on_output_page;
                                memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &offset, sizeof(int));
                                ++integers_written_on_output_page;
                            }
                            else if (curr_number != target_number && found_it) {
                                fwd_search_done = true;  /* no need to search next pages */
                            }
                        }
				    }
				}

				if(curr_number_top == target_number){
				    if(index_page_number<0)
				        index_page_number = curr_page_number;
                    // now search before and after this page
                    go_bwd = true;
                    go_fwd = true;

                    /* search within the page */
                    for (int i=sizeof(int)*(integers_per_page-1); i>= 0; i-= sizeof(int)) {
                        // traversing from the last entry of the page to first entry
                        int curr_number;
                        memcpy(&curr_number, &data[i], sizeof(int));
                        bool found_it = false;
                        if (curr_number == target_number) {
                            found_it = true;
                            // store (page num, offset) into the output file page
                            int offset = i/sizeof(int);
                            cout<<"Page numb:"<<curr_page_number<<" at "<<offset<< "val = "<< curr_number<<endl;

                            if (integers_written_on_output_page >= integers_per_page) {
                                output_handler.UnpinPage(output_page_handler.GetPageNum());//so that it can be flushed
                                output_handler.FlushPages(); // write otuput page to disk and remove from buffer
                                output_page_handler = output_handler.NewPage();	// create new output page in buffer
                                integers_written_on_output_page = 0; // new page is empty
                            }
                            char *output_data = output_page_handler.GetData();
                            memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &curr_page_number, sizeof(int));
                            ++integers_written_on_output_page;
                            memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &offset, sizeof(int));
                            ++integers_written_on_output_page;
                        }
                        else if (curr_number != target_number && found_it) {
                            // we are done with fwd search.
                            fwd_search_done = true;
                        }
                    }
				}

				if(curr_number_top > target_number){
				    // if already go_fwd then stop fwd search here
                    if(go_fwd){fwd_search_done = true;}
                    // missed : go back in binary search
                    else {bottom_pg = curr_page_number -1;}
				}

				// since we are done using the current input page, we can unpin it
				input_handler.UnpinPage(curr_page_handler.GetPageNum());
				// Done all search up and down
				if (fwd_search_done && bwd_search_done) break;
			}

			fm.CloseFile(input_handler);
			// Since we are done with one query, writing (-1, -1) pair in output page
			if (integers_written_on_output_page >= integers_per_page) {
				output_handler.UnpinPage(output_page_handler.GetPageNum());//so that it can be flushed
				output_handler.FlushPages(); // write otuput page to disk and remove from buffer
				output_page_handler = output_handler.NewPage();	// create new output page in buffer
				integers_written_on_output_page = 0; // new page is empty
			}
			char *output_data = output_page_handler.GetData();
			memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &MINUS_ONE, sizeof(int));
			++integers_written_on_output_page;
			memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &MINUS_ONE, sizeof(int));
			++integers_written_on_output_page;

			// break; // #TODO: remove this line for processing all queries
		}
		// fill the empty space with int_min
		for (int j=integers_written_on_output_page; j<= integers_per_page; ++j) {
			char *output_data = output_page_handler.GetData();
			memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &int_min, sizeof(int));
			++integers_written_on_output_page;
		}
	}
	else cout << "Unable to open query file\n";

	cout<<"Exited"<<endl;
	output_handler.UnpinPage(output_page_handler.GetPageNum());//so that it can be flushed
	output_handler.FlushPages(); // flush output pages
	fm.CloseFile (output_handler);

	// #TODO: following lines are only for debugging. Remove it in final submission
	char *my_output = "./output_search";
	char *ta_output = "./TestCases/TC_search/output_search";
	printAnswers(fm, my_output, "My output");
	printAnswers(fm, ta_output, "TA output");
	return 0;
}
