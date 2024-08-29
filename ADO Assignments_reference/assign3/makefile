CC = gcc
CFLAGS  = -g -Wall -std=c99

.PHONY: all
all: test_assign3_1 test_expr

test_assign3_1: test_assign3_1.c storage_mgr.c buffer_mgr.c buffer_mgr_stat.c dberror.c expr.c rm_serializer.c record_mgr.c 
	$(CC) $(CFLAGS) -o test_assign3_1 test_assign3_1.c storage_mgr.c buffer_mgr.c buffer_mgr_stat.c dberror.c expr.c rm_serializer.c record_mgr.c 
test_expr: test_expr.c storage_mgr.c buffer_mgr.c buffer_mgr_stat.c dberror.c expr.c rm_serializer.c record_mgr.c 
	$(CC) $(CFLAGS) -o test_expr test_expr.c storage_mgr.c buffer_mgr.c buffer_mgr_stat.c dberror.c expr.c rm_serializer.c record_mgr.c 

clean: 
	$(RM) recordmgr test_expr *.o *~
