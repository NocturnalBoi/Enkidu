all: 
	g++ -I src\include -L src\lib -o enkidu src\enkidu.c -lmingw32 -lSDL2main -lSDL2