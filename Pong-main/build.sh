g++ -O0 -g main.cpp -o pong.exe $(pkg-config --cflags --libs raylib) -lopengl32 -lgdi32 -lwinmm -mwindows
