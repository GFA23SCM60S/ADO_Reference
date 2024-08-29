gWall  = -g -Wall -std=c99
ofiles = test_assign2_1.o storage_mgr.o dberror.o buffer_mgr.o buffer_mgr_stat.o
hfiles = test_assign2_1.c dberror.h storage_mgr.h test_helper.h

default: test1

test1: $(ofiles)
	gcc $(gWall) -o test1 $(ofiles) -lm

test_assign2_1.o: $(hfiles) buffer_mgr.h buffer_mgr_stat.h
	gcc $(gWall) -c test_assign2_1.c -lm

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
	gcc $(gWall) -c buffer_mgr_stat.c

buffer_mgr.o: buffer_mgr.c buffer_mgr.h dt.h storage_mgr.h
	gcc $(gWall) -c buffer_mgr.c

storage_mgr.o: storage_mgr.c storage_mgr.h 
	gcc $(gWall) -c storage_mgr.c -lm

dberror.o: dberror.c dberror.h 
	gcc $(gWall) -c dberror.c

clean: 
	$(RM) test1 *.o *~

run_test1:
	./test1
