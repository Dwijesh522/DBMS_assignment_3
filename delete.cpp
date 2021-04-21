//Sample file for students to get their code running

#include <iostream>
#include "file_manager.h"
#include "errors.h"
#include <cstring>
#include <fstream>
#include <climits>
#include "binary_search.cpp"

using namespace std;

int getDeleteValue(const string &line) {
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

const char *input_file_path = "./TestCases/TC_delete/sorted_input";
const char *query_file_path = "./TestCases/TC_search/query_delete.txt";
const char *output_file_path = "./output_delete"; // create output in pwd
int MINUS_ONE = -1;
int int_min = INT_MIN;
int last_page = -1;

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

PageHandler getMidPageHandler(FileHandler &fh){
	int last_pg_num = getLastPageNumber(fh,false);
	last_page = last_pg_num;
	int mid_num = last_pg_num/2;
	PageHandler mid_page_handler = getPageHandler(fh, mid_num, true);
	return mid_page_handler;
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
	FileHandler input_handler = fm.OpenFile(input_file_path);

	PageHandler bin_start_handler = getMidPageHandler(input_handler);

	// reading query files
	ifstream query_file (query_file_path);
	if (query_file.is_open()) {
		string line;
		while (getline(query_file, line)) {
			int target_number = getDeleteValue(line); // value to search for
            cout<<"Target : "<<target_number<<endl;
			
			// since the input file is sorted, once processed all target values,
			// we need not process the remaining values
			loc = binary_search(target_number, input_handler, bin_start_handler);
			// Binary search procedure updates the bin_start_handler to point to the page of first occurence of the integer to be deleted.
			int write_page_num = loc[0][0];
			int write_offset = loc[0][1];
			int rd_pg_num = loc[1][0];
			int rd_offset = loc[1][1] + sizeof(int);

			if (write_page_num == INT_MAX){
				cout << "Number not found!" << endl;
				continue;
			}

			char *data = bin_start_handler.GetData();
			// Page where number first occurred is in memory already
			if (write_page_num == last_page){
				int val;
				if (rd_offset < last_index + sizeof(int))
					memcpy(&val, &data[rd_offset], sizeof(int));
				else{
					//If last occurrence is at last offset of the last page. No INT_MIN after that.
					if (rd_offset == last_index + sizeof(int))
						val = int_min;
				}
				if (write_offset == 0 && val == int_min){ // Whole page is going to be INT_MIN only!
					input_handler.DisposePage(write_page_num);
					input_handler.FlushPage(write_page_num);
					continue;
				}
				while (val != int_min){ //Shifting the data
					memcpy(&data[write_offset], &val, sizeof(int));
					write_offset += sizeof(int);
					rd_offset += sizeof(int);
					if (rd_offset == last_index + sizeof(int)) // If the page is fully filled with distinct data, need to place INT_MIN after page finishes
						val = int_min;
					else
						memcpy(&val, &data[rd_offset], sizeof(int));
				}
				while(write_offset != rd_offset){ // Fill the remaining spots with int_min.
					memcpy(&data[write_offset], &int_min, sizeof(int));
					write_offset += sizeof(int);
				}
				continue;
			}
			else{
				PageHandler last_occur_pg_handler;
				char *last_data;
				int val;
				if (rd_offset < last_index + sizeof(int)){ // If in middle of some page, then read the data simply!
					last_occur_pg_handler = input_handler.PageAt(rd_pg_num);
					last_data = last_occur_pg_handler.GetData();
					memcpy(&val, &last_data[rd_offset], sizeof(int));
				}
				else{
					//If last occurrence is at last offset of the last page. No INT_MIN after that.
					if (rd_offset == last_index + sizeof(int)){
						if (rd_pg_num == last_page)
							val = int_min;
						else{ // You need to start shifting the data which is at the beginning of the next page
							rd_pg_num++;
							last_occur_pg_handler = input_handler.PageAt(rd_pg_num);
							last_data = last_occur_pg_handler.GetData();
							rd_offset = 0;
							memcpy(&val, &last_data[rd_offset], sizeof(int));
						}
					}
				}
				if (rd_pg_num == last_page && val == int_min){
					while(write_offset != (last_index + sizeof(int))) { // Fill the remaining spots on the current write page with int_min.
						memcpy(&data[write_offset], &int_min, sizeof(int));
						write_offset += sizeof(int);
					}
					// Write the changes made to the current page to the disk
					input_handler.MarkDirty(write_page_num);
					input_handler.UnpinPage(write_page_num);
					input_handler.FlushPage(write_page_num);
					// Dispose all the remaining pages from write page number to read_page_number
					while (rd_pg_num != write_page_num){
						input_handler.DisposePage(rd_pg_num);
						input_handler.FlushPage(rd_pg_num);
						rd_pg_num--;
						if (rd_pg_num != write_page_num)
							last_occur_pg_handler = input_handler.PageAt(rd_pg_num);
					}
					input_handler.FlushPages(); // Sanity step: Flush all pages in buffer once my query is done! Can be removed
					continue;
				}
				else{
					while(val != int_min && (rd_pg_num != last_page || rd_offset != last_index + sizeof(int))) { // Main data copying loop
						memcpy(&data[write_offset], &val, sizeof(int));
						if (write_offset == last_index){ //If writing to a page complete, proceed to the next page
							input_handler.MarkDirty(write_page_num);
							input_handler.UnpinPage(write_page_num);
							input_handler.FlushPage(write_page_num);
							write_page_num++;
							bin_start_handler = input_handler.PageAt(write_page_num);
							data = bin_start_handler.GetData();
							write_offset = 0;
						}
						else{
							write_offset += sizeof(int); // Continue writing on same page
						}
						if (rd_offset == last_index){
							if (write_page_num != rd_pg_num){
								input_handler.UnpinPage(rd_pg_num); //May have to flush page! Review once..
							}
							if (rd_pg_num != last_page){ // Advance read head to the next page if one exists.
								rd_pg_num++;
								last_occur_pg_handler = input_handler.PageAt(rd_pg_num);
								last_data = last_occur_pg_handler.GetData();
								rd_offset = 0;
							}
							else{ // If read head already at end of last page, then just don't move forward. return INT_MIN value.
								val = int_min;
								continue;
							}
						}
						else{
							rd_offset += sizeof(int); //Continue reading on the same page
						}
						memcpy(&val, &last_data[rd_offset], sizeof(int)); // Read the data into the variable
					}
					if (write_offset != 0){ // Write head has space left to write on the current page
						while(write_offset != last_index + sizeof(int)){ //Fill the remaining space with INT_MIN
							memcpy(&data[write_offset], &int_min, sizeof(int));
							write_offset += sizeof(int);
						}
						// Write the changes made to current page in disk
						input_handler.MarkDirty(write_page_num);
						input_handler.UnpinPage(write_page_num);
						input_handler.FlushPage(write_page_num);

						// Dispose any pages left
						while(rd_pg_num != write_page_num){
							input_handler.DisposePage(rd_pg_num);
							input_handler.FlushPage(rd_pg_num);
							rd_pg_num--;
						}
					}
					else{ //Remaining spots left are full pages and we can delete them directly.
						while(rd_pg_num >= write_page_num){
							input_handler.DisposePage(rd_pg_num);
							input_handler.FlushPage(rd_pg_num);
							rd_pg_num--;
						}
					}
				}
			}
		}
	}
	else cout << "Unable to open query file\n";

	cout<<"Exited"<<endl;
	
	// #TODO: following lines are only for debugging. Remove it in final submission
	// char *my_output = "./output_search";
	char *ta_output = "./TestCases/TC_delete/output_delete";
	printAnswers(fm, my_output, "My output");
	printAnswers(fm, ta_output, "TA output");
	return 0;
}