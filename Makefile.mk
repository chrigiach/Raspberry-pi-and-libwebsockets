all: myclient

myclient_task_two: myclient.c
	gcc myclient.c -I /usr/local/include -l websockets -L /user/local/lib -lpthread -o myclient

clean:
	rm -f myclient
	rm -rf myclient.dSYM
