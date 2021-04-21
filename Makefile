# by default make command will only build the first target
# $ make linear_search sample_run
# will build both targets. So call make command with specific target

sampleobjects = buffer_manager.o file_manager.o sample_run.o
linear_search_objects = buffer_manager.o file_manager.o linear_search.o
binary_search_objects = buffer_manager.o file_manager.o binary_search.o
join1_objects = buffer_manager.o file_manager.o join1.o
join2_objects = buffer_manager.o file_manager.o join2.o
delete_objects = buffer_manager.o file_manager.o binary_search_del.o delete.o

linear_search : $(linear_search_objects)
	g++ -std=c++11 -o linear_search $(linear_search_objects)

binary_search : $(binary_search_objects)
	g++ -std=c++11 -o binary_search $(binary_search_objects)

join1 : $(join1_objects)
	g++ -std=c++11 -o join1 $(join1_objects)

join2 : $(join2_objects)
	g++ -std=c++11 -o join2 $(join2_objects)

sample_run : $(sampleobjects)
	g++ -std=c++11 -o sample_run $(sampleobjects)

delete : $(delete_objects)
	g++ -std=c++11 -o delete $(delete_objects)


sample_run.o : sample_run.cpp
	g++ -std=c++11 -c sample_run.cpp

linear_search.o : linear_search.cpp
	g++ -std=c++11 -c linear_search.cpp

binary_search.o : binary_search.cpp
	g++ -std=c++11 -c binary_search.cpp

binary_search_del.o : binary_search_del.cpp
	g++ -std=c++11 -c binary_search_del.cpp

join1.o : join1.cpp
	g++ -std=c++11 -c join1.cpp

join2.o : join2.cpp
	g++ -std=c++11 -c join2.cpp

delete.o : delete.cpp
	g++ -std=c++11 -c delete.cpp

buffer_manager.o : buffer_manager.cpp
	g++ -std=c++11 -c buffer_manager.cpp

file_manager.o : file_manager.cpp
	g++ -std=c++11 -c file_manager.cpp

clean :
	rm -f *.o
	rm -f sample_run
	rm -f linear_search
	rm -f join1
