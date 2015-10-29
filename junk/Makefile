C=clang
CPP=clang++
NAME1=run-shell
CPPFLAGS=$(shell PKG_CONFIG_PATH='/usr/local/lib/pkgconfig/:/usr/local/lib64/pkgconfig/' pkg-config --cflags liblxcpp libLogger) -g -std=c++11
LDFLAGS=$(shell PKG_CONFIG_PATH='/usr/local/lib/pkgconfig/:/usr/local/lib64/pkgconfig/' pkg-config --libs liblxcpp libLogger) -g

OBJECTS1=$(NAME1).o
HEADERS=

all: $(NAME1)

%.o: %.cpp $(HEADERS)
	$(CPP) -c -o $@ $< $(CPPFLAGS)

$(NAME1): $(OBJECTS1)
	$(CPP) $(OBJECTS1) $(LDFLAGS) -o $(NAME1)

clean:
	rm -f $(OBJECTS1) $(NAME1)
