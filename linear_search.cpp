//Sample file for students to get their code running

#include <iostream>
#include "file_manager.h"
#include "errors.h"
#include <cstring>
#include <fstream>

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
	if (not keep_pinned)	fh.UnpinPage(page_number);
	return last_page_handler;
}

int main() {

	int integers_per_page = PAGE_CONTENT_SIZE / sizeof(int);
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
			int target_value = getSearchValue(line); // value to search for

			// since the input file is sorted, once processed all target values,
			// we need not process the remaining values
			bool found_it = false, query_processed = false;
			FileHandler input_handler = fm.OpenFile(input_file_path);
			PageHandler curr_page_handler = getLastPageHandler(input_handler, /*keep_pinned*/true);
			
			// Looping from last page to first page
			do {
				char *data = curr_page_handler.GetData();
				for (int i=sizeof(int)*(integers_per_page-1); i>= 0; i-= 4) {
					// traversing from the last entry of the page to first entry
					int curr_number;
					memcpy(&curr_number, &data[i], sizeof(int));
					if (curr_number == target_number) {
						found_it = true;
						// store (page num, offset) into the output file page
						int curr_page_number = curr_curr_page_handler.GetPageNum();
						int offset = i;
						if (integers_written_on_output_page >= integers_per_page) {
							// flushPages will unpin it and write it back to odisk
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
					else if (curr_number != target_number and found_it == true) {
						// we are done with search. Terminate it.
						query_processed = true;
						break;
					}
				}
				// since we are done using the current input page, we can unpin it
				input_handler.UnpinPage(curr_page_handler.GetPageNum());
				if (query_processed) break;
			} while (curr_page_handler.GetPageNum() != 0/*page number of the first page*/);
			
			fm.CloseFile(input_handler);
			// Since we are done with one query, writing (-1, -1) pair in output page
			if (integers_written_on_output_page >= integers_per_page) {
				// flushPages will unpin it and write it back to disk
				output_handler.FlushPages(); // write otuput page to disk and remove from buffer
				output_page_handler = output_handler.NewPage();	// create new output page in buffer
				integers_written_on_output_page = 0; // new page is empty
			}
			char *output_data = output_page_handler.GetData();
			memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &MINUS_ONE, sizeof(int));
			++integers_written_on_output_page;
			memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &MINUS_ONE, sizeof(int));
			++integers_written_on_output_page;

			break; // #TODO: remove this line for processing all queries
		}
		// fill the empty space with int_min
		for (int j=integers_written_on_output_page; j<= integers_per_page; ++j) {	
			char *output_data = output_page_handler.GetData();
			memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &int_min, sizeof(int));
			++integers_written_on_output_page;
		}
	}
	else cout << "Unable to open query file\n";
	// flush output pages
	output_handler.FlushPages();
	fm.CloseFile (output_handler);
	return 0;
}
