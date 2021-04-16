# by default make command will only build the first target
# $ make linear_search sample_run
# will build both targets. So call make command with specific target

sampleobjects = buffer_manager.o file_manager.o sample_run.o
linear_search_objects = buffer_manager.o file_manager.o linear_search.o

linear_search : $(linear_search_objects)
	g++ -std=c++11 -o linear_search $(linear_search_objects)

sample_run : $(sampleobjects)
	g++ -std=c++11 -o sample_run $(sampleobjects)

sample_run.o : sample_run.cpp
	g++ -std=c++11 -c sample_run.cpp

linear_search.o : linear_search.cpp
	g++ -std=c++11 -c linear_search.cpp

buffer_manager.o : buffer_manager.cpp
	g++ -std=c++11 -c buffer_manager.cpp

file_manager.o : file_manager.cpp
	g++ -std=c++11 -c file_manager.cpp

clean :
	rm -f *.o
	rm -f sample_run
	rm -f linear_search