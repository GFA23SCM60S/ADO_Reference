ofiles = test_assign1_1.o storage_mgr.o  dberror.o 
helperfiles = test_assign1_1.c dberror.h storage_mgr.h test_helper.h
 
test1: $(ofiles)
	gcc -o test1 $(ofiles) -lm

test_assign1_1.o: $(helperfiles)
	gcc -c test_assign1_1.c -lm

storage_mgr.o: storage_mgr.c storage_mgr.h 
	gcc -c storage_mgr.c -lm

dberror.o: dberror.c dberror.h 
	gcc -c dberror.c

clean: 
	$(RM) test1 *.o *~ test1 

run:
	./test1