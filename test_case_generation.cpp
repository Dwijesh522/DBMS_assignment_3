//Sample file for students to get their code running

#include <iostream>
#include "file_manager.h"
#include "errors.h"
#include <cstring>
#include <fstream>
#include <climits>
#include <assert.h>

using namespace std;

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
            cout << first_num <<" " << sec_num << " ";
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

void updateFile1(FileHandler &test_file1) {
    /* This funtion adds following numbers in test_file1
        1 2 3 4 4 5 6 6 - - - -
        !!! Assumption: 6 integers per page !!!
    */
    int arr[12] = {1, 2, 3, 4, 4, 5, 6, 6, INT_MIN, INT_MIN, INT_MIN, INT_MIN};
    PageHandler first_page = test_file1.NewPage(); // dirty, pinned
    char *data = first_page.GetData();
    for (int i=0; i<6; ++i) {
        memcpy(&data[i*sizeof(int)], &arr[i], sizeof(int));
    }
    test_file1.FlushPage(first_page.GetPageNum());
    PageHandler second_page = test_file1.NewPage(); // dirty, pinned
    data = second_page.GetData();
    for (int i=6; i<12; ++i) {
        memcpy(&data[(i-6)*sizeof(int)], &arr[i], sizeof(int));
    }
    test_file1.FlushPage(second_page.GetPageNum());
}

void writeIntoPage(FileHandler &fh, int *arr, int size) {
    /* Create new page, write content, flush page*/
    PageHandler new_page = fh.NewPage(); // dirty, pinned
    char *data = new_page.GetData();
    for (int i=0; i<size; ++i) {
        memcpy(&data[i*sizeof(int)], &arr[i], sizeof(int));
    }
    fh.FlushPage(new_page.GetPageNum());
}

void updateFile(FileHandler &fh, int *arr, int size) {
    /* write elements of arr into fh */
    assert(size%6 == 0);
    int rounds = size/6;
    for (int r=0; r<rounds; ++r) {
        writeIntoPage(fh, &arr[r*6], 6);
    }
}

void writeIntoTextFile(string filename, string prefix, int arr[], int size) {
    /* create/open file, write "prefix arr[i]" in consecutive lines*/
    ofstream myfile;
    myfile.open(filename);
    for (int i=0; i<size; ++i) {
        myfile << prefix << " " << arr[i] << endl;
    }
    myfile.close();
}

void printTextFile(string filename, string title) {
    /* write file content on stdout*/
    cout << endl << title << endl;
    ifstream myfile (filename);
    string line;
    while (getline(myfile, line)) {
        cout << line << endl;
    }
    myfile.close();
}

int main(int argc, char **argv) {
    int integers_per_page = PAGE_CONTENT_SIZE / sizeof(int);
    FileManager fm;
    FileHandler test_file1 = fm.CreateFile("test_file1");
    FileHandler test_file2 = fm.CreateFile("test_file2");
    FileHandler test_file3 = fm.CreateFile("test_file3");
    FileHandler test_file4 = fm.CreateFile("test_file4");
    FileHandler test_file5 = fm.CreateFile("test_file5");

    updateFile1(test_file1);

    int arr2[18] = {1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, INT_MIN, INT_MIN};
    int arr3[12] = {1, 2, 3, 4, 5, 6, 7, 8, INT_MIN, INT_MIN, INT_MIN, INT_MIN};
    int arr4[12] = {9, 10, 11, 12, 13, 14, 15, INT_MIN, INT_MIN, INT_MIN, INT_MIN, INT_MIN};
    updateFile(test_file2, arr2, 18);
    updateFile(test_file3, arr3, 12);
    updateFile(test_file4, arr4, 12);

    fm.CloseFile(test_file4);
    fm.CloseFile(test_file3);
    fm.CloseFile(test_file2);
    fm.CloseFile(test_file1);
    fm.CloseFile(test_file5);

    char *file_path1 = "test_file1";
    char *file_path2 = "test_file2";
    char *file_path3 = "test_file3";
    char *file_path4 = "test_file4";
    printAnswers(fm, file_path1, "test_file1");
    printAnswers(fm, file_path2, "test_file2");
    printAnswers(fm, file_path3, "test_file3");
    printAnswers(fm, file_path4, "test_file4");
    cout << "\ntest_file5\n" << "Empty file with no page\n";

    int search_arr[4] = {1, 5, 6, 9};
    writeIntoTextFile("testcase_search.txt", "SEARCH", search_arr, 4);
    writeIntoTextFile("testcase_delete.txt", "DELETE", search_arr, 4);
    printTextFile("testcase_search.txt", "testcase_search.txt");
    printTextFile("testcase_delete.txt", "testcase_delete.txt");
    return 0;
}















