#
# Aleksander Yacovenco
#
# Makefile padrão para as aulas de Computação Gráfica
#

f ?= # exemplo: f=arquivo.c ou f=arquivo.cpp
d ?= # exemplo: d=ext.h\ lib.h\ lib02.h\ *.c
a ?= # exemplo: a=arg1\ arg2
h =-lGL -lGLU -lglut -lglui -lm # bibliotecas do OpenGL

all:
	@# se a extensão for .cpp
 	ifeq ($(findstring .cpp, $f), .cpp)
		@# $(f:.cpp=) retorna a string sem a extensão .cpp
		@# o @ antes do comando é para executar sem "eco"
	@g++ $f $d -o $(f:.cpp=) $h
	@./$(f:.cpp=) $a
	@# exceção caso d=*.cpp
 	else ifeq ($d,*.cpp)
		@g++ $d -o $f $h
		@./$f
	@# se a extensão for .c
 	else ifeq ($(findstring .c, $f), .c)
		@gcc $f $d -o $(f:.c=) $h
		@./$(f:.c=) $a
        endif
