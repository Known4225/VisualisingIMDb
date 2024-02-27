/* select OS */
#define OS_WINDOWS
// #define OS_LINUX

/* macros */
#define _FILE_OFFSET_BITS 64 // so ftello can handle files above 2GB

#include "include/ribbon.h"
#include "include/popup.h"
#ifdef OS_WINDOWS
#include "include/win32Tools.h"
#endif
#ifdef OS_LINUX
#include "include/zenityFileDialog.h"
#endif
#include <time.h>

GLFWwindow* window; // global window

/* class types */
enum GraphType {
    GRAPH_NORMAL,
    GRAPH_NODE
};

/* scale settings */
enum Scale {
    SCALE_LINEAR = 0,
    SCALE_LOG2 = 1,
    SCALE_LOGE = 2,
    SCALE_LOG10 = 3,
    SCALE_LOG = 2 // generic log, base e
};

// graph "class"
typedef struct {
    /* identifier */
    char type;
    /* titles */
    char *title;
    char *axisXLabel;
    char *axisYLabel;
    /* define area on screen */
    double minX;
    double minY;
    double maxX;
    double maxY;

    /* graph */
    char scaleType; // Scale enum type
    double scale; // scale
} graph_t;

// node graph "class"
typedef struct {
    /* identifier */
    char type;
    /* define area on screen */
    double minX;
    double minY;
    double maxX;
    double maxY;
    /* graph */
    list_t *nodes; // spec: ID, name, list of connections (ID)
    int highlight;
    /* mouse */
    double screenX;
    double screenY;
    double globalsize;
    double focalX;
    double focalY;
    int hover;
} graph_node_t;

// shared state
typedef struct {
    /*
    Actor-actor data visualisation:

    We need a custom dataset with
    name, connections, titles, date born, date died, recognisability, primarygenre, primarygenre(number)
    
    
    
    
    
    */
    list_t *fileList; // list of filenames
    double colours[42]; // theme colours
    list_t *graphs; // support for multiple free-form graphs
    list_t *columns;
    int loadingSegments; // number of segments to display on import file loading bar
} visual;

void init(visual *selfp) {
    visual self = *selfp;
    self.fileList = list_init();
    self.columns = list_init();
    self.graphs = list_init();
    /* graph layout:
    1 large graph
    */
    /* create graph */
    graph_node_t *graph = malloc(sizeof(graph_node_t));
    graph -> type = GRAPH_NODE;
    // graph -> title = "Title"; // string literals are globa
    // graph -> axisXLabel = "Name";
    // graph -> axisYLabel = "Box Office Revenue";
    graph -> minX = -320.0;
    graph -> minY = -180.0;
    graph -> maxX = 320.0;
    graph -> maxY = 180.0;
    graph -> nodes = list_init();
    // graph -> scaleType = SCALE_LINEAR;
    // graph -> scale = 100.0;
    list_append(self.graphs, (unitype) (void *) graph, 'p');




    double coloursPrep[42] = {
        30, 30, 30, // background colour
        70, 70, 70, // axis colour
        120, 120, 120, // text colour
        255, 0, 0, // bar colours
        0, 0, 0, // free colours
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0,
        0, 0, 0 
    };
    memcpy(self.colours, coloursPrep, 42 * sizeof(double));
    self.loadingSegments = 20; // 20 loading segments
    *selfp = self;
}

int import(visual *selfp, char *filename) {
    visual self = *selfp;
    /* identify file type
    Supported types:
     - tsv

    Structure:
    */
    int filenameLen = strlen(filename);
    char delimeter = ',';
    if (filename[filenameLen - 1] == 'v' && filename[filenameLen - 2] == 's' && filename[filenameLen - 3] == 'c' && filename[filenameLen - 4] == '.') {
        delimeter = ',';
    } else if (filename[filenameLen - 1] == 'v' && filename[filenameLen - 2] == 's' && filename[filenameLen - 3] == 't' && filename[filenameLen - 4] == '.') {
        delimeter = '\t';
    }
    list_append(self.fileList, (unitype) filename, 's');
    FILE *fp = fopen(filename, "r");
    long long fileSize = 0;
    if (fp == NULL) {
        printf("could not open file %s\n", filename);
        fclose(fp);
        return -1;
    } else {
        // a little estimation
        fseek(fp, 0, SEEK_END);
        fileSize = ftello(fp);
        fseek(fp, 0, SEEK_SET);
        printf("Loading file %s\nFile size: %lld\n", filename, fileSize);
    }
    
    /* set up loading bar */
    long long nextThresh = 0;
    long long lineLength = 1000;
    long long threshLoaded = 0;
    long long iters = 0;
    turtlePenColor(0, 0, 0);
    turtlePenSize(5);
    turtleGoto(-310, 160);
    turtlePenDown();
    turtleGoto(310, 160);
    turtlePenUp();
    turtlePenColor(255, 255, 255);
    turtlePenSize(4);
    turtleGoto(-310, 160);
    turtlePenDown();
    turtleUpdate();

    /* read headers */
    list_t *stringify = list_init();
    int index = 0;
    char read = '\0';
    char *tempStr;
    while (read != '\n' && fscanf(fp, "%c", &read) != EOF) {
        if (read == delimeter) {
            list_append(self.columns, (unitype) list_init(), 'r');
            tempStr = list_to_string(stringify);
            list_append(self.columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            stringify -> length = 0; // this should work, reduces malloc and realloc calls, but it could cause a memory leak
            index++;
        } else {
            list_append(stringify, (unitype) read, 'c');
        }
    }
    list_pop(stringify); // delete newline
    list_append(self.columns, (unitype) list_init(), 'r');
    tempStr = list_to_string(stringify);
    list_append(self.columns -> data[index].r, (unitype) tempStr, 's');
    free(tempStr);
    stringify -> length = 0;
    index = 0;
    read = '\0';


    // estimate line length
    while (read != '\n' && fscanf(fp, "%c", &read) != EOF) {
        if (read == delimeter) {
            tempStr = list_to_string(stringify);
            list_append(self.columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            stringify -> length = 0;
            index++;
        } else {
            list_append(stringify, (unitype) read, 'c');
        }
    }
    list_pop(stringify); // delete newline
    tempStr = list_to_string(stringify);
    list_append(self.columns -> data[index].r, (unitype) tempStr, 's');
    free(tempStr);
    stringify -> length = 0;
    index = 0;
    lineLength = ftello(fp) / 1; /*  we use the first row to estimate the number of lines, but this is not optimal.
    It's really just dependends on how representative the first row is of the lengths of all the others */
    nextThresh = fileSize / self.loadingSegments / lineLength;
    read = '\0';
    
    // read the rest
    while (fscanf(fp, "%c", &read) != EOF) {
        if (read == '\n') {
            // end of line
            char *tempStr = list_to_string(stringify);
            list_append(self.columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            stringify -> length = 0;
            index = 0;
            iters++;
        } else if (read == delimeter) {
            // end of column
            char *tempStr = list_to_string(stringify);
            list_append(self.columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            list_clear(stringify);
            index++;
        } else {
            list_append(stringify, (unitype) read, 'c');
        }
        if (iters > nextThresh) {
            printf("read %lld/%lld lines\n", iters, fileSize / lineLength); // this is just an estimation, it is not 100% accurate
            if (threshLoaded <= self.loadingSegments) {
                turtleGoto(620 / self.loadingSegments * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += fileSize / self.loadingSegments / lineLength;
            threshLoaded++;
            turtleUpdate();
        }
    }
    list_print(self.columns -> data[1].r);
    fclose(fp);
    *selfp = self;
    return 0;
}

void export(visual *selfp, char *filename) {

}

void drawGraph(visual *selfp) {
    visual self = *selfp;
    for (int i = 0; i < self.graphs -> length; i++) {
        graph_t *graphptr = (graph_t *) self.graphs -> data[i].p; // um so it's like polymorphism
        if (graphptr -> type == GRAPH_NORMAL) {
            graph_t graph = *((graph_t *) (self.graphs -> data[i].p)); // cursed
            turtlePenColor(self.colours[3], self.colours[4], self.colours[5]);
            turtlePenSize(3);
            turtleGoto(graph.minX, graph.maxY);
            turtlePenDown();
            turtleGoto(graph.minX, graph.minY);
            turtleGoto(graph.maxX, graph.minY);
            turtlePenUp();
        } else if (graphptr -> type == GRAPH_NODE) {
            graph_node_t graph = *((graph_node_t *) (self.graphs -> data[i].p)); // cursed
            turtlePenColor(self.colours[3], self.colours[4], self.colours[5]);
            turtlePenSize(2);
            turtleGoto(graph.minX, graph.minY);
            turtlePenDown();
            turtleGoto(graph.minX, graph.maxY);
            turtleGoto(graph.maxX, graph.maxY);
            turtleGoto(graph.maxX, graph.minY);
            turtleGoto(graph.minX, graph.minY);
            turtlePenUp();
        }
    }
    *selfp = self;
}

void mouseTick(visual *selfp) {

}

void hotkeyTick(visual *selfp) {

}

void scrollTick(visual *selfp) {

}

void parseRibbonOutput(visual *selfp) {
    visual self = *selfp;
    if (ribbonRender.output[0] == 1) {
        ribbonRender.output[0] = 0; // untoggle
        if (ribbonRender.output[1] == 0) { // file
            if (ribbonRender.output[2] == 1) { // new
                printf("New file created\n");
                // clearAll(&self);
                // strcpy(win32FileDialog.selectedFilename, "null");
            }
            if (ribbonRender.output[2] == 2) { // save
                if (strcmp(win32FileDialog.selectedFilename, "null") == 0) {
                    if (win32FileDialogPrompt(1, "") != -1) {
                        // printf("Saved to: %s\n", win32FileDialog.selectedFilename);
                        // export(&self, win32FileDialog.selectedFilename);
                    }
                } else {
                    // printf("Saved to: %s\n", win32FileDialog.selectedFilename);
                    // export(&self, win32FileDialog.selectedFilename);
                }
            }
            if (ribbonRender.output[2] == 3) { // save as
                if (win32FileDialogPrompt(1, "") != -1) {
                    // printf("Saved to: %s\n", win32FileDialog.selectedFilename);
                    // export(&self, win32FileDialog.selectedFilename);
                }
            }
            if (ribbonRender.output[2] == 4) { // load
                if (win32FileDialogPrompt(0, "") != -1) {
                    // printf("Loaded data from: %s\n", win32FileDialog.selectedFilename);
                    // clearAll(&self);
                    import(&self, win32FileDialog.selectedFilename);
                }
            }
        }
    }
    *selfp = self;
}

int main(int argc, char *argv[]) {
    // GLFWwindow* window; // made into global at the top of file
    /* Initialize glfw */
    if (!glfwInit()) {
        return -1;
    }
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA (Anti-Aliasing) with 4 samples (must be done before window is created (?))

    /* Create a windowed mode window and its OpenGL context */
    const GLFWvidmode *monitorSize = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int windowHeight = monitorSize -> height * 0.85;
    window = glfwCreateWindow(windowHeight * 16 / 9, windowHeight, "Logic Gates", NULL, NULL);
    if (!window) {
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    glfwSetWindowSizeLimits(window, GLFW_DONT_CARE, GLFW_DONT_CARE, windowHeight * 16 / 9, windowHeight);
    gladLoadGL();

    /* initialise FileDialog */
    #ifdef OS_WINDOWS
    win32ToolsInit();
    win32FileDialogAddExtension("txt"); // add txt to extension restrictions
    win32FileDialogAddExtension("csv"); // add csv to extension restrictions
    win32FileDialogAddExtension("tsv"); // add tsv to extension restrictions
    char constructedPath[MAX_PATH + 1];
    #endif
    #ifdef OS_LINUX
    zenityFileDialogInit(argv[0]); // must include argv[0] to get executableFilepath
    zenityFileDialogAddExtension("txt"); // add txt to extension restrictions
    zenityFileDialogAddExtension("lg"); // add lg to extension restrictions
    char constructedPath[4097];
    #endif

    #ifdef OS_WINDOWS
    strcpy(constructedPath, win32FileDialog.executableFilepath);
    #endif
    #ifdef OS_LINUX
    strcpy(constructedPath, zenityFileDialog.executableFilepath);
    #endif

    /* initialize turtle */
    turtleInit(window, -320, -180, 320, 180);
    /* initialise textGL */
    #ifdef OS_WINDOWS
    strcpy(constructedPath, win32FileDialog.executableFilepath);
    #endif
    #ifdef OS_LINUX
    strcpy(constructedPath, zenityFileDialog.executableFilepath);
    #endif
    strcat(constructedPath, "include/fontBez.tgl");
    textGLInit(window, constructedPath);
    /* initialise ribbon */
    #ifdef OS_WINDOWS
    strcpy(constructedPath, win32FileDialog.executableFilepath);
    #endif
    #ifdef OS_LINUX
    strcpy(constructedPath, zenityFileDialog.executableFilepath);
    #endif
    strcat(constructedPath, "include/ribbonConfig.txt");
    ribbonInit(window, constructedPath);

    int tps = 60; // ticks per second (locked to fps in this case)
    clock_t start, end;
    visual self;
    init(&self); // initialise the logicgates

    turtleBgColor(self.colours[0], self.colours[1], self.colours[2]);
    ribbonDarkTheme();

    if (argc > 1) {
        import(&self, argv[1]);
        #ifdef OS_WINDOWS
        strcpy(win32FileDialog.selectedFilename, argv[1]);
        #endif
        #ifdef OS_LINUX
        strcpy(zenityFileDialog.selectedFilename, argv[1]);
        #endif
    }
    int frame = 0;
    while (turtle.close == 0) {
        start = clock(); // for frame syncing
        turtleClear(); // clear the screen
        turtleGetMouseCoords(); // get the mouse coordinates (turtle.mouseX, turtle.mouseY)
        drawGraph(&self);
        ribbonUpdate(); // do ribbon before mouseTick
        parseRibbonOutput(&self);
        mouseTick(&self);
        hotkeyTick(&self);
        scrollTick(&self);
        turtleUpdate(); // update the screen
        end = clock();
        if (frame % 60 == 0) {
            frame = 0;
        }
        frame += 1;
        while ((double) (end - start) / CLOCKS_PER_SEC < (1 / (double) tps)) {
            end = clock();
        }
    }
}