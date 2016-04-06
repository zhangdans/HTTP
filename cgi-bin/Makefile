ROOT_PATH=$(shell pwd)/..
INCLUDE=-I$(ROOT_PATH)/mysql_connect -I$(ROOT_PATH)/mysql_lib/include
LIB=-L$(ROOT_PATH)/mysql_lib/lib
LDFLAGS=-lmysqlclient
REGISTER_SRC=cgi_register.cpp $(ROOT_PATH)/mysql_connect/mysql_connect.cpp

.PHONY:all
all:cgi_math cgi_register

cgi_math:cgi_math.cpp 
	g++ -o $@ $^
cgi_register:$(REGISTER_SRC)
	g++ -o $@ $^ $(INCLUDE) $(LIB) $(LDFLAGS) 

.PHONY:clean
clean:
	rm -rf cgi_math  cgi_register

.PHONY:output
output:
	cp cgi_math ../output/htdocs/cgi-bin
	cp cgi_register ../output/htdocs/cgi-bin
