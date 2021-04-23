//Sample file for students to get their code running
//Lat page initialized to -2
#include <iostream>
#include "file_manager.h"
#include "errors.h"
#include <cstring>
#include <fstream>
#include <climits>
#include <vector>

using namespace std;

int integers_per_page = PAGE_CONTENT_SIZE / sizeof(int);
int last_index = PAGE_CONTENT_SIZE - sizeof(int);
char *input_file_path;
char *query_file_path;
int MINUS_ONE = -1;
int int_min = INT_MIN;
int last_page = -1;


void updateFilePaths(int argc, char **argv) {
    /* This file updates file paths using command line arguments */
    if (argc != 3) {
        cout << "ERROR: command line arguments expected\n";
        exit(0);
    }
    input_file_path = argv[1];
    query_file_path = argv[2];
}

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
	
	 // *	When we call fh.LastPage().getPageNum(), it brings the last page
	 // *	into memory if not already in memory. By default it is pinned.
	 // *	Inputs:
	 // *		fh : file handler from wich to find the page number of last page
	 // *		keep_pinned : whether to keep the last page pinned or not
	 // *	Outputs: pagehandler of the last page
	 
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
	if (last_page == -1){
		int last_pg_num = getLastPageNumber(fh,false);
		last_page = last_pg_num;
	}
	int mid_num = last_page/2;
	PageHandler mid_page_handler = getPageHandler(fh, mid_num, true);
	return mid_page_handler;
}

vector<vector<int> > binary_search(int target_number, FileHandler &input_handler, PageHandler &start_page_handler){

    vector<vector<int> > answer(2, vector<int>(2,INT_MIN));
    answer[0][0] = INT_MAX, answer[0][1] = INT_MAX;

//    cout<<"Target : "<<target_number<<endl;
    // since the input file is sorted, once processed all target values,
    // we need not process the remaining values
    bool go_fwd = false, go_bwd = false, query_processed = false;
    bool bwd_search_done = false, fwd_search_done = false;
    int total_pages = last_page;//getLastPageNumber(input_handler, /*keep pinned*/ false);
//    cout<<total_pages<<endl;
    int top_pg = 0;
    int bottom_pg = last_page;
    int curr_page_number;
    // Page found by binary search
    int index_page_number=-1;
    PageHandler curr_page_handler;
    int search_count = 0;
    int count_fwd = 0;
    int count_bwd = 0;
    bool start = false;
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

        // cout<<"Page "<<curr_page_number<<endl;
        if (!start){
            curr_page_handler = start_page_handler;
            start = true;
        }
        else
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
            if (curr_number_bottom>INT_MIN && curr_number_bottom < target_number){
                // if already go_bwd then stop bwd search
                if (go_bwd){ bwd_search_done = true;}
                // update top_pg for binary search
                else {top_pg = curr_page_number+1;}
            }

            // Found the start
            else if(curr_number_bottom >= target_number || curr_number_bottom==INT_MIN){
                start_page_handler = curr_page_handler;  //setting page handler to start occurrence
                // Found the index page - now just go up and down in directory from this page
                if(index_page_number<0)
                    index_page_number = curr_page_number;
                go_bwd = true, bwd_search_done = true; /* no need to check pages before as curr_number_top < target_number */
                go_fwd = true;

                bool found_it = false;
                for (int i=0; i<= sizeof(int)*(integers_per_page-1); i+= sizeof(int)) {
                    // traversing from the last entry of the page to first entry
                    int curr_number;
                    memcpy(&curr_number, &data[i], sizeof(int));
//                    cout<<"Page numb:"<<curr_page_number<<" val "<<curr_number<<endl;
                    if (curr_number == target_number) {
                        found_it = true;
                        // store (page num, offset) into the output file page
                        int offset = i/sizeof(int);
                        // cout<<"Page numb:"<<curr_page_number<<" at "<<offset<< " val = "<< curr_number<<endl;
                        //update start point
                        if (answer[0][0] > curr_page_number){
                            answer[0][0] = curr_page_number;
                            answer[0][1] = offset;
                        }
                        else if(answer[0][0] == curr_page_number && answer[0][1]>offset){
                            answer[0][1] = offset;
                        }
                        //update end point
                        if (answer[1][0] < curr_page_number){
                            answer[1][0] = curr_page_number;
                            answer[1][1] = offset;
                        }
                        else if(answer[1][0] == curr_page_number && answer[1][1]<offset){
                            answer[1][1] = offset;
                        }

                    }
                    else if (curr_number != target_number && found_it) {
                        fwd_search_done = true;  /* no need to search prev pages */
                        break;
                    }
                }
            }
        }

        if(curr_number_top == target_number){
            if(index_page_number<0){
                index_page_number = curr_page_number;
                go_bwd = true;
                go_fwd = true;
            }

            if(go_bwd && !bwd_search_done)
                start_page_handler = curr_page_handler;


            /* search within the page */
            bool found_it = false;
            for (int i=0; i<= sizeof(int)*(integers_per_page-1); i+= sizeof(int)) {
                // traversing from the last entry of the page to first entry
                int curr_number;
                memcpy(&curr_number, &data[i], sizeof(int));
//                cout<<"Page numb:"<<curr_page_number<<" val "<<curr_number<<endl;
                if (curr_number == target_number) {
                    found_it = true;
                    // store (page num, offset) into the output file page
                    int offset = i/sizeof(int);
                    // cout<<"Page numb:"<<curr_page_number<<" at "<<offset<< " val = "<< curr_number<<endl;

                    //update start point
                    if (answer[0][0] > curr_page_number){
                        answer[0][0] = curr_page_number;
                        answer[0][1] = offset;
                    }
                    else if(answer[0][0] == curr_page_number && answer[0][1]>offset){
                        answer[0][1] = offset;
                    }
                    //update end point
                    if (answer[1][0] < curr_page_number){
                        answer[1][0] = curr_page_number;
                        answer[1][1] = offset;
                    }
                    else if(answer[1][0] == curr_page_number && answer[1][1]<offset){
                        answer[1][1] = offset;
                    }
                }
                else if (curr_number != target_number && found_it) {
                    // we are done with bwd search since looped backward
                    fwd_search_done = true;  /* no need to search next pages */
                    break;
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

    // cout<<"Page num: "<<start_page_handler.GetPageNum()<<endl;
	return answer;
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
	FileManager fm;
	FileHandler input_handler = fm.OpenFile(input_file_path);
	PageHandler bin_start_handler;

	// reading query files
	ifstream query_file (query_file_path);
	if (query_file.is_open()) {
		string line;
		while (getline(query_file, line)) {
			int target_number = getDeleteValue(line); // value to search for
			
			try{
				// since the input file is sorted, once processed all target values,
				// we need not process the remaining values
				bin_start_handler = getMidPageHandler(input_handler);	
				vector<vector<int> > loc;
				loc = binary_search(target_number, input_handler, bin_start_handler);
				// Binary search procedure updates the bin_start_handler to point to the page of first occurence of the integer to be deleted.
				// cout << loc[0][0] << " " << loc[0][1] << " " << loc[1][0] << " " << loc[1][1] << endl;
				int write_page_num = loc[0][0];
				int write_offset = loc[0][1] * sizeof(int);
				int rd_pg_num = loc[1][0];
				int rd_offset = (loc[1][1] * sizeof(int)) + sizeof(int);

				if (write_page_num == INT_MAX){
					// cout << "Number not found!" << endl;
					continue;
				}

				bin_start_handler = input_handler.PageAt(loc[0][0]);
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
						last_page--;
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
					input_handler.MarkDirty(write_page_num);
					input_handler.UnpinPage(write_page_num);
					input_handler.FlushPage(write_page_num);
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
						// cout << "Start Val is " << val << endl;
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
						if (write_offset == 0){
							while(rd_pg_num >= write_page_num){
								input_handler.DisposePage(rd_pg_num);
								input_handler.FlushPage(rd_pg_num);
								rd_pg_num--;
								last_page--;
							}
							continue;
						}
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
							last_page--;
							rd_pg_num--;
							if (rd_pg_num != write_page_num)
								last_occur_pg_handler = input_handler.PageAt(rd_pg_num);
						}
						input_handler.FlushPages(); // Sanity step: Flush all pages in buffer once my query is done! Can be removed
						continue;
					}
					else{
						while(val != int_min && (rd_pg_num != last_page || rd_offset != last_index + sizeof(int))) { // Main data copying loop
							// cout << "First write of " << val << " at offset " << write_offset << " on page number " << write_page_num << endl;
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
								last_page--;
							}
						}
						else{ //Remaining spots left are full pages and we can delete them directly.
							while(rd_pg_num >= write_page_num){
								input_handler.DisposePage(rd_pg_num);
								input_handler.FlushPage(rd_pg_num);
								rd_pg_num--;
								last_page--;
							}
						}
					}
				}
			}
			catch(...) {
				cout << "Oops! Something went wrong. Exception Encountered" << endl;
				continue;
			}
			// char *my_output = "./TestCases/TC_delete/sorted_input";
			// char *ta_output = "./TestCases/TC_delete/output_delete";
			// printAnswers(fm, my_output, "My output");
			// printAnswers(fm, ta_output, "TA output");
		}
	}
	else cout << "Unable to open query file\n";
	// cout<<"Exited"<<endl;
	// char *my_output = "./TestCases/TC_delete/sorted_input";
	// char *ta_output = "./TestCases/TC_delete/output_delete";
	cout << last_page << endl;
	printAnswers(fm, input_file_path, "My output");
	fm.CloseFile(input_handler);
	// printAnswers(fm, ta_output, "TA output");
	return 0;
}
