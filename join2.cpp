#include <iostream>
#include "file_manager.h"
#include "errors.h"
#include <cstring>
#include <fstream>
#include <climits>

#include <vector> // only used for debugging purpose
#include <algorithm> // only used for debugging purpose (sort)

using namespace std;

// global variables
const char *input1_file_path = "./TestCases/TC_join2/input1_join2";
const char *input2_file_path = "./TestCases/TC_join2/input2_join2_updated";
const char *output_file_path = "./output_join2"; // create output in pwd
int MINUS_ONE = -1;
int int_min = INT_MIN;
int integers_per_page = PAGE_CONTENT_SIZE / sizeof(int);
int last_index = sizeof(int)*(integers_per_page-1);

// declarations
int getLastPageNumber(FileHandler &fh, bool keep_pinned);
PageHandler getLastPageHandler(FileHandler &fh, bool keep_pinned);
PageHandler getFirstPageHandler(FileHandler &fh, bool keep_pinned);
void fillWithIntMin(PageHandler &ph, const int &integers_per_page, int &integers_written_on_output_page);
void validateAnswers(FileManager &fm);

inline int charToInt(char *array, int offset) {
    /* returns int(array[offset]); */
    int data;
    memcpy(&data, &array[offset], sizeof(int));
    return data;
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


vector<int> binary_search_start(int target_number, FileHandler &input_handler){

    vector<int> answer(2);
    answer[0] = INT_MAX, answer[1] = INT_MAX;

    // since the input file is sorted, once processed all target values,
    // we need not process the remaining values
    bool go_bwd = false;
    bool bwd_search_done = false;
    int total_pages = getLastPageNumber(input_handler, /*keep pinned*/ false);
    int top_pg = 0;
    int bottom_pg = total_pages;
    int curr_page_number;
    // Page found by binary search
    int index_page_number=-1;
    PageHandler curr_page_handler;
    int search_count = 0;
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

        // searched back and forth from index page. Exit.
        if(bwd_search_done)
            break;

        curr_page_handler = input_handler.PageAt(curr_page_number);
        input_handler.UnpinPage(curr_page_number); //unpinned
        char *data = curr_page_handler.GetData();

        // first entry on curr page
        int curr_number_top = charToInt(data, 0);
        // last entry on curr page
        int curr_number_bottom = charToInt(data, last_index);

        if(curr_number_top < target_number){
            // not found yet
            if (curr_number_bottom < target_number && curr_number_bottom>INT_MIN){
                // if already go_bwd then stop bwd search
                if (go_bwd){ bwd_search_done = true;}
                // update top_pg for binary search
                else {top_pg = curr_page_number+1;}
            }
            // Found the start
            else if(curr_number_bottom >= target_number || curr_number_bottom==INT_MIN){
                answer[0] = curr_page_number;
                // Found the index page - now just go up and down in directory from this page
                if(index_page_number<0)
                    index_page_number = curr_page_number;
                go_bwd = true, bwd_search_done = true; /* no need to check pages before as curr_number_top < target_number */
                bool found_it = false;
                for (int i=0; i<=last_index; i+= sizeof(int)) {
                    // traversing from the last entry of the page to first entry
                    int curr_number;
                    memcpy(&curr_number, &data[i], sizeof(int));
                    if (curr_number == target_number) {
                        found_it = true;
                        // store (page num, offset) into the output file page
                        int offset = i/sizeof(int);
                        //update start point
                        answer[1] = offset;
                        break;
                    }
                    /*else if (curr_number != target_number && found_it) {
                        bwd_search_done = true;
                        break;
                    }*/
                }
            }
        }

        if(curr_number_top == target_number){
            if(index_page_number<0)
                index_page_number = curr_page_number;
            // now search before and after this page
            go_bwd = true;
            if(answer[0]>curr_page_number){
                answer[0] = curr_page_number;
                answer[1] = 0;
            }
        }

        if(curr_number_top > target_number){
            bottom_pg = curr_page_number -1;
        }
        // since we are done using the current input page, we can unpin it
        input_handler.UnpinPage(curr_page_handler.GetPageNum());
        // Done all search up and down
        if (bwd_search_done) break;
    }
    if (answer[0]==INT_MAX || answer[1]==INT_MAX) {answer[0] = -1;}
//    cout<<"Page, off = "<<answer[0]<<", "<<answer[1]<<endl;
	return answer;
}

int main() {

	int integers_per_page = PAGE_CONTENT_SIZE / sizeof(int);
	FileManager fm;
	FileHandler output_handler = fm.CreateFile(output_file_path);
	PageHandler output_page_handler = output_handler.NewPage(); // pinned and dirty
	int integers_written_on_output_page = 0;

    FileHandler input1_handler = fm.OpenFile(input1_file_path); // input1
    PageHandler input1_page_handler = getFirstPageHandler(input1_handler, /*keep pinned*/true); // pinned
    int total_pages1 = getLastPageNumber(input1_handler, /*keep pinned*/ false);
    cout<<"Total1 = "<<total_pages1<<endl;
    FileHandler input2_handler = fm.OpenFile(input2_file_path); // input2
    int total_pages2 = getLastPageNumber(input2_handler, /*keep pinned*/ false);
    cout<<"Total2 = "<<total_pages2<<endl;
    while (true) { // traversing from back
        char *input1_data = input1_page_handler.GetData();
        // loop in the page from top to bottom
        for (int i=0; i<= sizeof(int)*(integers_per_page-1); i+= sizeof(int)) {
            int input1 = charToInt(input1_data, i);
            if(input1==INT_MIN) continue;
            // page_number, offset of the earliest occurrence of input1 in input2
            cout<<"Target: "<<input1<<endl;
            vector<int> start_addr = binary_search_start(input1, input2_handler);
            int page_number = start_addr[0];
            int offset = start_addr[1];
            if(page_number==-1)
                continue;
            else{
                PageHandler input2_page_handler = input2_handler.PageAt(page_number);
                char *input2_data = input2_page_handler.GetData();

                while(true){
                    int input2 = charToInt(input2_data, offset*sizeof(int));
                    if (input1 == input2) { // write input1 on output page
                        cout<<"Page numb: "<<page_number<<" at "<<offset<< " val = "<< input1<<endl;
                        if (integers_written_on_output_page >= integers_per_page) { // output page is full. Get a new page.
                            output_handler.FlushPage(output_page_handler.GetPageNum()); // write otuput page to disk and remove from buffer
                            output_page_handler = output_handler.NewPage();	// pinned and dirty
                            integers_written_on_output_page = 0; // new page is empty
                        }
                        char *output_data = output_page_handler.GetData();
                        memcpy(&output_data[integers_written_on_output_page * sizeof(int)], &input1, sizeof(int));
                        ++integers_written_on_output_page;

                        if(offset<integers_per_page-1)
                            offset++;
                        else{ //move to next page
                            offset = 0;
                            if(page_number < total_pages2){
                                input2_handler.UnpinPage(page_number); //unpin last page traversed
                                ++page_number;
                                input2_page_handler = input2_handler.PageAt(page_number);
                                input2_data = input2_page_handler.GetData();
                            }
                            else //reached end of pages in input2
                                break;
                        }
                    }
                    else //in a sorted file numbers can only exist contiguously
                        break;
                }
                input2_handler.UnpinPage(page_number); //unpin it
            }
        }
        if (input1_page_handler.GetPageNum() == total_pages1) break;
        input1_handler.FlushPage(input1_page_handler.GetPageNum()); // will page get flushed even if pinned ??
        input1_page_handler = input1_handler.NextPage(input1_page_handler.GetPageNum()); // bringing input1 page in buffer
    }
    fillWithIntMin(output_page_handler, integers_per_page, integers_written_on_output_page);
	output_handler.FlushPages(); // flush all pages since we are done
	fm.CloseFile (output_handler);

    validateAnswers(fm); // TODO: only for debugging. Remove it in final submission
    char *in1_output = "./TestCases/TC_join2/input1_join2";
    char *in2_output = "./TestCases/TC_join2/input2_join2_updated";
    char *my_output = "./output_join2";
    char *ta_output = "./TestCases/TC_join2/output_join2";
//    printAnswers(fm, in1_output, "Input1 output");
//	printAnswers(fm, in2_output, "Input2 output");
//	printAnswers(fm, my_output, "My Output");
//	printAnswers(fm, ta_output, "Ta output");
	return 0;
}

// Helper functions
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
		for (int i=0; i<=(integers_per_page-2)*sizeof(int); i+= sizeof(int)) {
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
	char *my_output = "./output_join2";
	char *ta_output = "./TestCases/TC_join2/output_join2";
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









