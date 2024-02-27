all:
	gcc VisualisingIMDb.c -L./Windows -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -o VisualisingIMDb.exe
trunc:
	gcc TruncateFile.c -L./Windows -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -o TruncateFile.exe
trunclin:
	gcc TruncateFile.c -L./Linux -lglfw3 -ldl -lm -lX11 -lglad -lGL -lGLU -lpthread -g -Wall -o TruncateFile.o
data:
	gcc CreateDataset.c -L./Windows -lglfw3 -lopengl32 -lgdi32 -lglad -lole32 -luuid -Wall -o CreateDataset.exe
datalin:
	gcc CreateDataset.c -L./Linux -lglfw3 -ldl -lm -lX11 -lglad -lGL -lGLU -lpthread -O3 -Wall -o CreateDataset.o
val:
	gcc logicgatesLinux.c -L./Linux -lglfw3 -ldl -lm -lX11 -lglad -lGL -lGLU -lpthread -g -o logicgatesLinux.o
clean:
	rm logicgatesLinux.o