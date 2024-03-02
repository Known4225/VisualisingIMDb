/* select OS */
#define OS_WINDOWS
// #define OS_LINUX


/*
TODO:




*/

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

/* node type */
typedef struct {
    int ID;
    int genre;
    char *name;
    double xpos;
    double ypos;
    double size;
    double colour[3];
    list_t *connections;
} node_t;

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
    int highlight; // for selecting a single person
    int genreHighlight; // for selecting a genre
    int genreLocked; // for locking a highlight
    int bounds[4];
    int legendBounds[2]; // legend x bounds
    int leftYear;
    int rightYear;
    /* mouse */
    double screenX;
    double screenY;
    double globalsize;
    double focalX;
    double focalY;
    double focalCSX;
    double focalCSY;
    double scrollSpeed;
    int hover;
} graph_node_t;

// shared state
typedef struct {
    /*
    Actor-actor data visualisation:

    We need a custom dataset with
    name, connections, titles, date born, date died, recognisability, primarygenre, primarygenre(number)
    
    Revised:
    ncost, name, primaryGenre, date born, recognisability, connections, titles
    
    There are 181,000 actors to plot.
    
    
    
    */
    list_t *fileList; // list of filenames
    double colours[42]; // theme colours
    list_t *graphs; // support for multiple free-form graphs, actually there isn't
    list_t *columns;
    list_t *selectedIDs; // network
    list_t *genreHistogram; // genre frequency
    list_t *genreList;
    list_t *genreColour;
    int loadingSegments; // number of segments to display on import file loading bar
    char keys[12];
    double mouseX;
    double mouseY;
    double mouseW;
    char parallax;
    char renderConnections;
    char renderNamesForConnections;
} visual;

void init(visual *parentp) {
    srand(time(NULL));
    visual parent = *parentp;
    parent.parallax = 0;
    parent.renderConnections = 0;
    parent.renderNamesForConnections = 0;
    parent.fileList = list_init();
    parent.columns = list_init();
    parent.selectedIDs = list_init();
    parent.genreHistogram = list_init();
    for (int i = 0; i < 12; i++) {
        parent.keys[i] = 0;
    }
    for (int i = 0; i < 30; i++) {
        list_append(parent.genreHistogram, (unitype) 0, 'i');
    }
    parent.graphs = list_init();
    /* graph layout:
    1 large graph
    */
    /* create graph */
    graph_node_t *graph = malloc(sizeof(graph_node_t));
    graph -> type = GRAPH_NODE;
    graph -> minX = -320.0;
    graph -> minY = -180.0;
    graph -> maxX = 320.0;
    graph -> maxY = 180.0;
    graph -> nodes = list_init();

    /* spacial parameters */
    graph -> globalsize = 5;
    graph -> screenX = -100;
    graph -> screenY = 0;
    graph -> scrollSpeed = 1.15;
    graph -> hover = -1;

    /* graph */
    graph -> highlight = -1;
    graph -> genreHighlight = -1;
    graph -> genreLocked = 0;
    graph -> bounds[0] = -310;
    graph -> bounds[1] = -100;
    graph -> bounds[2] = 310;
    graph -> bounds[3] = 100;
    graph -> legendBounds[0] = 220;
    graph -> legendBounds[1] = 320;
    graph ->leftYear = 1870;
    graph -> rightYear = 2005;

    list_append(parent.graphs, (unitype) (void *) graph, 'p');




    double coloursPrep[42] = {
        30, 30, 30, // background colour
        70, 70, 70, // axis colour
        250, 250, 250, // text colour
        10, 10, 10, // legend bar colour
        10, 10, 10, // timeline base
        230, 230, 230, // timeline ticks/text
        230, 230, 230, // switch on colour
        150, 150, 150, // switch off colour
        14, 94, 148, // switch on dot colour
        204, 0, 0, // switch off dot colour
        0, 0, 0,
        0, 0, 0,
        0, 0, 0 
    };
    memcpy(parent.colours, coloursPrep, 42 * sizeof(double));
    parent.loadingSegments = 100; // 100 loading segments

    /* data */
    parent.genreList = list_init();
    list_append(parent.genreList, (unitype) "Drama", 's');
    list_append(parent.genreList, (unitype) "Documentary", 's');
    list_append(parent.genreList, (unitype) "Comedy", 's');
    list_append(parent.genreList, (unitype) "\\N", 's');
    list_append(parent.genreList, (unitype) "Action", 's');
    list_append(parent.genreList, (unitype) "Romance", 's');
    list_append(parent.genreList, (unitype) "Thriller", 's');
    list_append(parent.genreList, (unitype) "Horror", 's');
    list_append(parent.genreList, (unitype) "Crime", 's');
    list_append(parent.genreList, (unitype) "Adventure", 's');
    list_append(parent.genreList, (unitype) "Family", 's');
    list_append(parent.genreList, (unitype) "Mystery", 's');
    list_append(parent.genreList, (unitype) "Biography", 's');
    list_append(parent.genreList, (unitype) "Fantasy", 's');
    list_append(parent.genreList, (unitype) "History", 's');
    list_append(parent.genreList, (unitype) "Sci-Fi", 's');
    list_append(parent.genreList, (unitype) "Music", 's');
    list_append(parent.genreList, (unitype) "Musical", 's');
    list_append(parent.genreList, (unitype) "War", 's');
    list_append(parent.genreList, (unitype) "Animation", 's');
    list_append(parent.genreList, (unitype) "Adult", 's');
    list_append(parent.genreList, (unitype) "Western", 's');
    list_append(parent.genreList, (unitype) "Sport", 's');
    list_append(parent.genreList, (unitype) "News", 's');
    list_append(parent.genreList, (unitype) "Film-Noir", 's');
    list_append(parent.genreList, (unitype) "Reality-TV", 's');
    list_append(parent.genreList, (unitype) "Talk-Show", 's');
    list_append(parent.genreList, (unitype) "Game-Show", 's');
    list_append(parent.genreList, (unitype) "Short", 's');
    parent.genreColour = list_init(); // sourced from https://lospec.com/palette-list/30-color
    list_append(parent.genreColour, (unitype) 242, 'i'); // Drama #f22020
    list_append(parent.genreColour, (unitype) 32, 'i');
    list_append(parent.genreColour, (unitype) 32, 'i');

    list_append(parent.genreColour, (unitype) 57, 'i'); // Documentary #3998f5
    list_append(parent.genreColour, (unitype) 152, 'i');
    list_append(parent.genreColour, (unitype) 245, 'i');

    list_append(parent.genreColour, (unitype) 138, 'i'); // Comedy #8ad8e8
    list_append(parent.genreColour, (unitype) 216, 'i');
    list_append(parent.genreColour, (unitype) 232, 'i');

    list_append(parent.genreColour, (unitype) 255, 'i'); // \N #ffffff
    list_append(parent.genreColour, (unitype) 255, 'i');
    list_append(parent.genreColour, (unitype) 255, 'i');

    list_append(parent.genreColour, (unitype) 47, 'i'); // Action #2f2aa0
    list_append(parent.genreColour, (unitype) 42, 'i');
    list_append(parent.genreColour, (unitype) 160, 'i');

    list_append(parent.genreColour, (unitype) 211, 'i'); // Romance #d30b94
    list_append(parent.genreColour, (unitype) 11, 'i');
    list_append(parent.genreColour, (unitype) 148, 'i');

    list_append(parent.genreColour, (unitype) 35, 'i'); // Thriller #235b54
    list_append(parent.genreColour, (unitype) 91, 'i');
    list_append(parent.genreColour, (unitype) 84, 'i');

    list_append(parent.genreColour, (unitype) 55, 'i'); // Horror #37294f
    list_append(parent.genreColour, (unitype) 41, 'i');
    list_append(parent.genreColour, (unitype) 79, 'i');

    list_append(parent.genreColour, (unitype) 72, 'i'); // Crime #201923
    list_append(parent.genreColour, (unitype) 65, 'i');
    list_append(parent.genreColour, (unitype) 75, 'i');

    list_append(parent.genreColour, (unitype) 14, 'i'); // Adventure #0ec434
    list_append(parent.genreColour, (unitype) 196, 'i');
    list_append(parent.genreColour, (unitype) 52, 'i');

    list_append(parent.genreColour, (unitype) 252, 'i'); // Family #fcff5d
    list_append(parent.genreColour, (unitype) 255, 'i');
    list_append(parent.genreColour, (unitype) 93, 'i');

    list_append(parent.genreColour, (unitype) 100, 'i'); // Mystery #645648
    list_append(parent.genreColour, (unitype) 86, 'i');
    list_append(parent.genreColour, (unitype) 72, 'i');

    list_append(parent.genreColour, (unitype) 255, 'i'); // Biography #ffcba5
    list_append(parent.genreColour, (unitype) 203, 'i');
    list_append(parent.genreColour, (unitype) 165, 'i');

    list_append(parent.genreColour, (unitype) 39, 'i'); // Fantasy #277da7
    list_append(parent.genreColour, (unitype) 125, 'i');
    list_append(parent.genreColour, (unitype) 167, 'i');

    list_append(parent.genreColour, (unitype) 197, 'i'); // History #c56133
    list_append(parent.genreColour, (unitype) 97, 'i');
    list_append(parent.genreColour, (unitype) 51, 'i');

    list_append(parent.genreColour, (unitype) 125, 'i'); // Sci-Fi #7dfc00
    list_append(parent.genreColour, (unitype) 252, 'i');
    list_append(parent.genreColour, (unitype) 0, 'i');

    list_append(parent.genreColour, (unitype) 93, 'i'); // Music #5d4c86
    list_append(parent.genreColour, (unitype) 76, 'i');
    list_append(parent.genreColour, (unitype) 134, 'i');

    list_append(parent.genreColour, (unitype) 148, 'i'); // Musical #946aa2
    list_append(parent.genreColour, (unitype) 106, 'i');
    list_append(parent.genreColour, (unitype) 162, 'i');

    list_append(parent.genreColour, (unitype) 34, 'i'); // War #228c68
    list_append(parent.genreColour, (unitype) 140, 'i');
    list_append(parent.genreColour, (unitype) 104, 'i');

    list_append(parent.genreColour, (unitype) 119, 'i'); // Animation #772b9d
    list_append(parent.genreColour, (unitype) 43, 'i');
    list_append(parent.genreColour, (unitype) 157, 'i');

    list_append(parent.genreColour, (unitype) 230, 'i'); // Adult #e68f66
    list_append(parent.genreColour, (unitype) 143, 'i');
    list_append(parent.genreColour, (unitype) 102, 'i');

    list_append(parent.genreColour, (unitype) 255, 'i'); // Western #ffc413
    list_append(parent.genreColour, (unitype) 196, 'i');
    list_append(parent.genreColour, (unitype) 19, 'i');

    list_append(parent.genreColour, (unitype) 244, 'i'); // Sport #f47a22
    list_append(parent.genreColour, (unitype) 122, 'i');
    list_append(parent.genreColour, (unitype) 34, 'i');

    list_append(parent.genreColour, (unitype) 6, 'i'); // Film-Noir #060606
    list_append(parent.genreColour, (unitype) 6, 'i');
    list_append(parent.genreColour, (unitype) 6, 'i');

    list_append(parent.genreColour, (unitype) 195, 'i'); // Reality-TV #c3a5b4
    list_append(parent.genreColour, (unitype) 165, 'i');
    list_append(parent.genreColour, (unitype) 180, 'i');

    list_append(parent.genreColour, (unitype) 240, 'i'); // Talk-Show #f07cab
    list_append(parent.genreColour, (unitype) 124, 'i');
    list_append(parent.genreColour, (unitype) 171, 'i');

    list_append(parent.genreColour, (unitype) 183, 'i'); // Game-Show #b732cc
    list_append(parent.genreColour, (unitype) 50, 'i');
    list_append(parent.genreColour, (unitype) 204, 'i');

    list_append(parent.genreColour, (unitype) 237, 'i'); // Short #edeff3
    list_append(parent.genreColour, (unitype) 239, 'i');
    list_append(parent.genreColour, (unitype) 243, 'i');

    *parentp = parent;
}

void generateNodes(visual *parentp) {
    visual parent = *parentp;

    /* set up loading bar */
    long long nextThresh = parent.columns -> data[0].r -> length / parent.loadingSegments;
    long long threshLoaded = 1;
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

    printf("number of columns: %d\n", parent.columns -> length);
    /* qualify columns */
    list_t *nconst = parent.columns -> data[0].r;
    list_t *names = parent.columns -> data[1].r;
    list_t *genre = parent.columns -> data[2].r; // convert to genreID
    

    for (int i = 1; i < genre -> length; i++) {
        list_append(parent.selectedIDs, (unitype) i, 'i');
        char *toFree = genre -> data[i].s;
        int calc = list_find(parent.genreList, (unitype) toFree, 's');
        genre -> type[i] = 'i';
        genre -> data[i].i = calc;
        free(toFree);
    }
    list_t *birthYear = parent.columns -> data[3].r; // convert to integers
    for (int i = 1; i < birthYear -> length; i++) {
        char *toFree = birthYear -> data[i].s;
        int calc;
        sscanf(toFree, "%d", &calc);
        birthYear -> type[i] = 'i';
        birthYear -> data[i].i = calc;
        free(toFree);
    }
    double averageRecognisability = 0;
    list_t *recognisability = parent.columns -> data[4].r; // convert to doubles
    for (int i = 1; i < recognisability -> length; i++) {
        char *toFree = recognisability -> data[i].s;
        double calc;
        sscanf(toFree, "%lf", &calc);
        recognisability -> type[i] = 'd';
        recognisability -> data[i].d = calc;
        averageRecognisability += calc;
        free(toFree);
    }
    averageRecognisability /= recognisability -> length;
    list_t *connections = parent.columns -> data[5].r; // convert to list
    int maxConnections = 0; // maximum number of connections
    for (int i = 1; i < connections -> length; i++) {
        if (iters > nextThresh) {
            printf("processed %lld/%i lines\n", iters, connections -> length); // this is just an estimation, it is not 100% accurate
            if (threshLoaded <= parent.loadingSegments) {
                turtleGoto(620 / parent.loadingSegments * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += connections -> length / parent.loadingSegments;
            threshLoaded++;
            turtleUpdate();
        }
        iters++;
        char *toFree = connections -> data[i].s;
        list_t *runlist = list_init();
        int j = -1;
        while (1) {
            int connectionStrength;
            if (j == -1 || toFree[j] == ',') {
                if (strlen(toFree) < 2) {
                    printf("problem in dataset: line %d\n", i);
                }
                sscanf(toFree + j + 1, "%d", &connectionStrength);
                // printf("connection strength: %d\n", connectionStrength);
                // list_append(runlist, (unitype) connectionStrength, 'i'); // hold off on adding it yet until we know if the person exists
                j += 3;
            } else {
                break;
            }
            if (toFree[j] == ',') {
                int nameConst;
                sscanf(toFree + j + 3, "%d", &nameConst);
                // printf("nameConst: %d\n", nameConst);
                int nameIndex = -1;
                int target = nameConst;
                int lowerBound = 0;
                int upperBound = nconst -> length - 1;
                int check = upperBound / 2;
                int numAt = atoi(nconst -> data[check].s + 2);
                while (upperBound >= lowerBound) {
                    if (numAt == target) {
                        nameIndex = check;
                        break;
                    } else if (numAt > target) {
                        upperBound = check - 1;
                    } else {
                        lowerBound = check + 1;
                    }
                    check = lowerBound + (upperBound - lowerBound) / 2;
                    if (check > nconst -> length - 1 || check < 0) {
                        // printf("heyo, check is %d\n", check);
                        // fclose(fp);
                        // exit(-1);
                    } else {
                        numAt = atoi(nconst -> data[check].s + 2);
                    }
                }
                if (nameIndex == -1) {
                    // we didn't find this nameConst
                    // printf("did not find nameConst %d\n", nameConst);
                    /* this means that this connection was to someone who was culled for having 0 recognisability
                    We will simply not include them */
                    // list_append(runlist, (unitype) -1, 'i');
                } else {
                    list_append(runlist, (unitype) connectionStrength, 'i'); // add strength
                    list_append(runlist, (unitype) (nameIndex - 1), 'i'); // add ID
                }
                j += 10;
            } else {
                printf("this should never happen %d %d %s\n", i, j, toFree);
                break;
            }
        }
        if (runlist -> length > maxConnections) {
            maxConnections = runlist -> length;
        }
        /* sort runlist */
        // list_print(runlist);
        for (int k = 1; k < runlist -> length; k += 2) {
            int min = 0x7fffffff;
            int minInd = k;
            for (int h = k; h < runlist -> length; h += 2) {
                if (runlist -> data[h].i < min) {
                    min = runlist -> data[h].i;
                    minInd = h;
                }
            }
            int temp = runlist -> data[minInd].i;
            int temp2 = runlist -> data[minInd - 1].i;
            runlist -> data[minInd] = runlist -> data[k];
            runlist -> data[minInd - 1] = runlist -> data[k - 1];
            runlist -> data[k].i = temp;
            runlist -> data[k - 1].i = temp2;
        }
        // list_print(runlist);
        connections -> type[i] = 'r';
        connections -> data[i].r = runlist;
        free(toFree);
    }

    // double alphaTune = -0.005;
    graph_node_t *canvas = ((graph_node_t *) parent.graphs -> data[0].p);
    list_t *nodeGraph = canvas -> nodes;
    // printf("max connections: %d\n", maxConnections);
    for (int i = 1; i < nconst -> length; i++) {
        node_t *newNode = malloc(sizeof(node_t));
        newNode -> name = names -> data[i].s;
        newNode -> ID = i;
        // newNode -> size = 0.5 / (1 + pow(2.718281, alphaTune * (recognisability -> data[i].d - averageRecognisability)));
        newNode -> size = recognisability -> data[i].d / (averageRecognisability * sqrt(nconst -> length) * 0.1);
        newNode -> xpos = (birthYear -> data[i].i - canvas -> leftYear) * ((double) (canvas -> bounds[2] - canvas -> bounds[0]) / (canvas -> rightYear - canvas -> leftYear)) + canvas -> bounds[0] + ((rand() % 500) / 100.0);
        newNode -> ypos = (canvas -> bounds[3] - canvas -> bounds[1]) * ((double) (connections -> data[i].r -> length) / maxConnections) + canvas -> bounds[1] + ((rand() % 100) / (maxConnections / 3.0));

        /* colour based on genre */
        newNode -> genre = genre -> data[i].i;
        if (genre -> data[i].i == -1) {
            newNode -> colour[0] = 250;
            newNode -> colour[1] = 250;
            newNode -> colour[2] = 250;
        } else {
            newNode -> colour[0] = parent.genreColour -> data[genre -> data[i].i * 3].i;
            newNode -> colour[1] = parent.genreColour -> data[genre -> data[i].i * 3 + 1].i;
            newNode -> colour[2] = parent.genreColour -> data[genre -> data[i].i * 3 + 2].i;
        }
        
        newNode -> connections = connections -> data[i].r;
        list_append(nodeGraph, (unitype) (void *) newNode, 'p');
    }
    *parentp = parent;
}

int import(visual *parentp, char *filename) {
    visual parent = *parentp;
    list_clear(parent.columns);
    list_clear(parent.selectedIDs);
    list_clear(((graph_node_t *) parent.graphs -> data[0].p) -> nodes);
    list_clear(parent.fileList);
    // list_clear(parent.graphs);
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
    list_append(parent.fileList, (unitype) filename, 's');
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
    long long threshLoaded = 1;
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
            list_append(parent.columns, (unitype) list_init(), 'r');
            tempStr = list_to_string(stringify);
            list_append(parent.columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            stringify -> length = 0; // this should work, reduces malloc and realloc calls, but it could cause a memory leak
            index++;
        } else {
            list_append(stringify, (unitype) read, 'c');
        }
    }
    list_pop(stringify); // delete newline
    list_append(parent.columns, (unitype) list_init(), 'r');
    tempStr = list_to_string(stringify);
    list_append(parent.columns -> data[index].r, (unitype) tempStr, 's');
    free(tempStr);
    stringify -> length = 0;
    index = 0;
    read = '\0';


    // estimate line length
    while (read != '\n' && fscanf(fp, "%c", &read) != EOF) {
        if (read == delimeter) {
            tempStr = list_to_string(stringify);
            list_append(parent.columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            stringify -> length = 0;
            index++;
        } else {
            list_append(stringify, (unitype) read, 'c');
        }
    }
    list_pop(stringify); // delete newline
    tempStr = list_to_string(stringify);
    list_append(parent.columns -> data[index].r, (unitype) tempStr, 's');
    free(tempStr);
    stringify -> length = 0;
    index = 0;
    lineLength = ftello(fp) / 1; /*  we use the first row to estimate the number of lines, but this is not optimal.
    It's really just dependends on how representative the first row is of the lengths of all the others */
    lineLength = 45278400 / 185146; // hardcoded value
    nextThresh = fileSize / parent.loadingSegments / lineLength;
    read = '\0';
    
    // read the rest
    while (fscanf(fp, "%c", &read) != EOF) {
        if (read == '\n') {
            // end of line
            char *tempStr = list_to_string(stringify);
            list_append(parent.columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            stringify -> length = 0;
            index = 0;
            iters++;
        } else if (read == delimeter) {
            // end of column
            char *tempStr = list_to_string(stringify);
            list_append(parent.columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            list_clear(stringify);
            index++;
        } else {
            list_append(stringify, (unitype) read, 'c');
        }
        if (iters > nextThresh) {
            printf("read %lld/%lld lines\n", iters, fileSize / lineLength); // this is just an estimation, it is not 100% accurate
            if (threshLoaded <= parent.loadingSegments) {
                turtleGoto(620 / parent.loadingSegments * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += fileSize / parent.loadingSegments / lineLength;
            threshLoaded++;
            turtleUpdate();
        }
    }
    // list_print(self.columns -> data[1].r);
    fclose(fp);
    *parentp = parent;
    /* add nodes to graph */
    generateNodes(parentp);
    return 0;
}

void export(visual *selfp, char *filename) {

}

void renderConnections(visual *parentp) {
    visual parent = *parentp;
    graph_node_t self = *((graph_node_t *) (parent.graphs -> data[0].p));
    if (parent.keys[0] < 2 && parent.keys[1] == 0 && parent.renderConnections) {
        turtlePenShape("none");
        turtlePenSize(1);
        for (int i = 0; i < self.nodes -> length; i++) {
            // printf("iter: %d\n", i);
            if (i == self.highlight || self.highlight == -1) {
                node_t node = *((node_t *) (self.nodes -> data[i].p));
                if (self.genreHighlight == -1 || self.genreHighlight == node.genre || i == self.highlight) {
                    double anchorX;
                    double anchorY;
                    if (parent.parallax) {
                        anchorX = (node.xpos + self.screenX) * self.globalsize * node.size;;
                        anchorY = (node.ypos + self.screenY) * self.globalsize * node.size;; 
                    } else {
                        anchorX = (node.xpos + self.screenX) * self.globalsize;
                        anchorY = (node.ypos + self.screenY) * self.globalsize; 
                    }
                    if (self.highlight != -1 || (anchorX > -330 && anchorX < 330 && anchorY > -190 && anchorY < 190)) {
                        turtlePenColor(node.colour[0], node.colour[1], node.colour[2]);
                        for (int j = 1; j < node.connections -> length; j += 2) {
                            node_t *connect = ((node_t *) (self.nodes -> data[node.connections -> data[j].i].p));
                            if (self.genreHighlight == -1 || self.genreHighlight == connect -> genre) {
                                double destX;
                                double destY;
                                if (parent.parallax) {
                                    destX = (connect -> xpos + self.screenX) * self.globalsize * connect -> size;
                                    destY = (connect -> ypos + self.screenY) * self.globalsize * connect -> size;
                                } else {
                                    destX = (connect -> xpos + self.screenX) * self.globalsize;
                                    destY = (connect -> ypos + self.screenY) * self.globalsize;
                                }
                                if (i == self.highlight) {
                                    turtlePenColor(connect -> colour[0], connect -> colour[1], connect -> colour[2]);
                                }
                                if (self.highlight == -1) {
                                    turtle.pena = (100 - node.connections -> data[j - 1].i) / 100.0; // scale alpha according to strength of connection
                                } else {
                                    turtle.pena = (100 - node.connections -> data[j - 1].i) / 120.0; // more visible for small networks
                                }
                                turtleGoto(anchorX, anchorY);
                                turtlePenDown();
                                turtleGoto(destX, destY);
                                turtlePenUp();
                            }
                        }
                    }
                }
            }
        }
        turtlePenShape("circle");
    }
}

void renderGraph(visual *parentp) {
    visual parent = *parentp;
    for (int i = 0; i < parent.graphs -> length; i++) {
        graph_t *graphptr = (graph_t *) parent.graphs -> data[i].p; // um so it's like polymorphism
        if (graphptr -> type == GRAPH_NORMAL) {
            graph_t graph = *((graph_t *) (parent.graphs -> data[i].p)); // cursed
            turtlePenColor(parent.colours[3], parent.colours[4], parent.colours[5]);
            turtlePenSize(3);
            turtleGoto(graph.minX, graph.maxY);
            turtlePenDown();
            turtleGoto(graph.minX, graph.minY);
            turtleGoto(graph.maxX, graph.minY);
            turtlePenUp();
        } else if (graphptr -> type == GRAPH_NODE) {
            graph_node_t graph = *((graph_node_t *) (parent.graphs -> data[i].p)); // cursed
            turtlePenColor(parent.colours[3], parent.colours[4], parent.colours[5]);
            turtlePenSize(2);
            turtleGoto(graph.minX, graph.minY);
            turtlePenDown();
            turtleGoto(graph.minX, graph.maxY);
            turtleGoto(graph.maxX, graph.maxY);
            turtleGoto(graph.maxX, graph.minY);
            turtleGoto(graph.minX, graph.minY);
            turtlePenUp();
            /* render hover */
            // printf("hover: %d\n", graph.hover);
            if (graph.hover != -1) {
                if (parent.parallax) {
                    turtleGoto((((node_t *) (graph.nodes -> data[graph.hover].p)) -> xpos + graph.screenX) * graph.globalsize * ((node_t *) (graph.nodes -> data[graph.hover].p)) -> size, 
                               (((node_t *) (graph.nodes -> data[graph.hover].p)) -> ypos + graph.screenY) * graph.globalsize * ((node_t *) (graph.nodes -> data[graph.hover].p)) -> size);
                } else {
                    turtleGoto((((node_t *) (graph.nodes -> data[graph.hover].p)) -> xpos + graph.screenX) * graph.globalsize, (((node_t *) (graph.nodes -> data[graph.hover].p)) -> ypos + graph.screenY) * graph.globalsize);
                }
                turtlePenColor(255, 255, 255);
                turtlePenSize(((node_t *) (graph.nodes -> data[graph.hover].p)) -> size * 4.2 * graph.globalsize);
                turtlePenDown();
                turtlePenUp();
            }
            for (int j = 0; j < 30; j++) { // reset genreHistogram
                parent.genreHistogram -> data[j].i = 0;
            }
            int k = 0;
            graph.hover = -1;
            for (int j = 0; j < graph.nodes -> length; j++) {
                if (j == parent.selectedIDs -> data[k].i) {
                    node_t node = *((node_t *) (graph.nodes -> data[j].p));
                    
                    /* gather genre histogram */
                    if (node.genre != -1) {
                        parent.genreHistogram -> data[node.genre].i++;
                    }

                    /* rendering */
                    double calcX = 0;
                    double calcY = 0;
                    if (parent.parallax) {
                        calcX = (node.xpos + graph.screenX) * graph.globalsize * node.size;
                        calcY = (node.ypos + graph.screenY) * graph.globalsize * node.size;
                    } else {
                        calcX = (node.xpos + graph.screenX) * graph.globalsize;
                        calcY = (node.ypos + graph.screenY) * graph.globalsize;
                    }
                    double realSize = node.size * 4 * graph.globalsize;
                    /* get hover*/
                    if (graph.genreHighlight == -1 || graph.genreHighlight == node.genre || i == graph.highlight) {
                        if (parent.mouseX < graph.legendBounds[0] && ((calcX - parent.mouseX) * (calcX - parent.mouseX) + (calcY - parent.mouseY) * (calcY - parent.mouseY) < realSize * realSize * 0.25)) {
                            graph.hover = j;
                        }
                        if (calcX + realSize > -330 && calcX - realSize < 330 && calcY + realSize > -190 && calcY - realSize < 190) {
                            turtleGoto(calcX, calcY);
                            turtlePenColorAlpha(node.colour[0], node.colour[1], node.colour[2], 80);
                            turtlePenSize(realSize);
                            turtlePenDown();
                            turtlePenUp();
                        }
                        if (realSize > 15 || (graph.highlight != -1 && parent.renderNamesForConnections) || j == graph.hover || graph.highlight == j) {
                            if (calcX > -350 && calcX < 350 && calcY > -190 && calcY < 190) {
                                turtlePenColor(parent.colours[6], parent.colours[7], parent.colours[8]);
                                textGLWriteUnicode(node.name, calcX, calcY, 5, 50);
                            }
                        }
                    }
                    k++;
                }
            }
            *((graph_node_t *) (parent.graphs -> data[0].p)) = graph;
        }
    }
    *parentp = parent;
}

void renderTimeline(visual *parentp) {
    visual parent = *parentp;
    graph_node_t self = *((graph_node_t *) (parent.graphs -> data[0].p));
    if (1) { // bottom
        turtleQuad(-320, -185, 320, -185, 320, -160, -320, -160, parent.colours[12], parent.colours[13], parent.colours[14], 150);
        turtlePenColor(parent.colours[15], parent.colours[16], parent.colours[17]);
        /* render ticks */
        double midYear = (self.leftYear + self.rightYear) / 2.0;
        double timeScale = (double) (self.rightYear - self.leftYear) / (self.bounds[2] - self.bounds[0]);
        double leftEdge = midYear + ((-self.screenX - 320 / self.globalsize) * timeScale);
        double rightEdge = midYear + ((-self.screenX + 320 / self.globalsize) * timeScale);
        double xpos = (ceil(leftEdge) - leftEdge) * (1 / timeScale) * self.globalsize - 320;
        int currentTickYear = ceil(leftEdge);
        double topY;
        double tickWidth = 0.2;
        int dist;
        if (self.globalsize < 0.8) {
            dist = 100;
        } else if (self.globalsize < 16) {
            dist = 5;
        } else {
            dist = 1;
        }
        while (xpos < self.legendBounds[0]) {
            if (currentTickYear % dist == 0) {
                if (dist == 1) {
                    if (currentTickYear % 5 == 0) {
                        topY = -172;
                    } else {
                        topY = -174;
                    }
                } else {
                    topY = -172;
                }
                char year[12];
                sprintf(year, "%d", currentTickYear);
                textGLWriteString(year, xpos, topY + 5, 6, 50);
            } else {
                if (dist == 100 && currentTickYear % 5 == 0) {
                    topY = -174;
                } else {
                    topY = -176;
                }
            }
            turtleQuad(xpos, -180, xpos + tickWidth, -180, xpos + tickWidth, topY, xpos, topY, parent.colours[15], parent.colours[16], parent.colours[17], 0);
            xpos += 1 / timeScale * self.globalsize;
            currentTickYear++;
        }
    }
}

void renderLegend(visual *parentp) {
    visual parent = *parentp;
    graph_node_t self = *((graph_node_t *) (parent.graphs -> data[0].p));
    if (1) { // right side
        turtleQuad(self.legendBounds[0], -180, self.legendBounds[1], -180, self.legendBounds[1], 180, self.legendBounds[0], 180, parent.colours[9], parent.colours[10], parent.colours[11], 0);
        turtlePenColor(parent.colours[6], parent.colours[7], parent.colours[8]);
        textGLWriteString("Legend", (self.legendBounds[0] + self.legendBounds[1]) / 2, 160, 10, 50);
        textGLWriteString("<- Birth Year", 270, -170, 7, 50);

        /* render stacked genre bars */
        double scale = (100.0 / parent.selectedIDs -> length);
        double leftX = self.legendBounds[0] + 20;
        double rightX = 300;
        double topY = 140;
        double localMin;
        /* selection sort the genreHistogram so the bars are sorted */
        list_t *tempList = list_init();
        list_copy(parent.genreHistogram, tempList);
        list_t *genreSort = list_init();
        for (int i = 0; i < tempList -> length; i++) {
            int max = -1;
            int maxInd = 0;
            for (int j = 0; j < tempList -> length; j++) {
                if (tempList -> data[j].i > max) {
                    max = tempList -> data[j].i;
                    maxInd = j;
                }
            }
            tempList -> data[maxInd].i = -1;
            list_append(genreSort, (unitype) maxInd, 'i');
        }
        int textQue = -1;
        double midY;
        for (int i = 0; i < parent.genreHistogram -> length; i++) {
            localMin = topY - parent.genreHistogram -> data[genreSort -> data[i].i].i * scale;
            turtleQuad(leftX, topY, leftX, localMin, rightX, localMin, rightX, topY, 
            parent.genreColour -> data[genreSort -> data[i].i * 3].i, parent.genreColour -> data[genreSort -> data[i].i * 3 + 1].i, parent.genreColour -> data[genreSort -> data[i].i * 3 + 2].i, 0);
            if (topY - localMin > 8) {
                if (genreSort -> data[i].i == 3) {
                    turtlePenColor(10, 10, 10);
                } else {
                    turtlePenColor(parent.colours[6], parent.colours[7], parent.colours[8]);
                }
                textGLWriteString(parent.genreList -> data[genreSort -> data[i].i].s, leftX + (rightX - leftX) / 2, localMin + (topY - localMin) / 2, 6, 50);
                
            }
            if (parent.mouseX >= leftX && parent.mouseX <= rightX && parent.mouseY >= localMin && parent.mouseY < topY) {
                textQue = genreSort -> data[i].i;
                midY = localMin + (topY - localMin) / 2;
            }
            if (self.genreLocked == 1 && self.genreHighlight == genreSort -> data[i].i) {
                turtlePenColor(parent.colours[6], parent.colours[7], parent.colours[8]);
                textGLWriteString(parent.genreList -> data[genreSort -> data[i].i].s, leftX - 4, localMin + (topY - localMin) / 2, 6, 100);
                turtleTriangle(rightX, localMin + (topY - localMin) / 2, rightX + 5, localMin + (topY - localMin) / 2 - 5, rightX + 5, localMin + (topY - localMin) / 2 + 5,
                parent.genreColour -> data[genreSort -> data[i].i * 3].i, parent.genreColour -> data[genreSort -> data[i].i * 3 + 1].i, parent.genreColour -> data[genreSort -> data[i].i * 3 + 2].i, 0);
            }
            topY = localMin;
        }
        if (textQue > -1) {
            turtlePenColor(parent.colours[6], parent.colours[7], parent.colours[8]);
            textGLWriteString(parent.genreList -> data[textQue].s, leftX - 4, midY, 6, 100);
            turtleTriangle(rightX, midY, rightX + 5, midY - 5, rightX + 5, midY + 5,
            parent.genreColour -> data[textQue * 3].i, parent.genreColour -> data[textQue * 3 + 1].i, parent.genreColour -> data[textQue * 3 + 2].i, 0);
            if (self.genreLocked == 0) {
                self.genreHighlight = textQue;
            }
            if (turtleMouseDown()) {
                self.genreLocked = 1;
            }
        } else {
            if (self.genreLocked == 0) {
                self.genreHighlight = -1;
            }
        }
        list_free(genreSort);
        list_free(tempList);


        turtlePenColor(parent.colours[6], parent.colours[7], parent.colours[8]);
        textGLWriteString("Render Connections", (self.legendBounds[0] + self.legendBounds[1]) / 2, 0, 5, 50);
        textGLWriteString("Render Connection Names", (self.legendBounds[0] + self.legendBounds[1]) / 2, -30, 5, 50);
        textGLWriteString("Starfield", (self.legendBounds[0] + self.legendBounds[1]) / 2, -60, 5, 50);

        double saveprez = turtle.circleprez;
        turtle.circleprez = 10;
        if (parent.renderConnections) {
            turtlePenColor(parent.colours[18], parent.colours[19], parent.colours[20]);
        } else {
            turtlePenColor(parent.colours[21], parent.colours[22], parent.colours[23]);
        }
        turtlePenSize(10);
        turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 - 5, -15);
        turtlePenDown();
        turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 + 5, -15);
        turtlePenUp();
        turtlePenSize(9);
        if (parent.renderConnections) {
            turtlePenColor(parent.colours[24], parent.colours[25], parent.colours[26]);
            turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 + 5, -15);
        } else {
            turtlePenColor(parent.colours[27], parent.colours[28], parent.colours[29]);
            turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 - 5, -15);
        }
        turtlePenDown();
        turtlePenUp();

        if (parent.renderNamesForConnections) {
            turtlePenColor(parent.colours[18], parent.colours[19], parent.colours[20]);
        } else {
            turtlePenColor(parent.colours[21], parent.colours[22], parent.colours[23]);
        }
        turtlePenSize(10);
        turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 - 5, -45);
        turtlePenDown();
        turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 + 5, -45);
        turtlePenUp();
        turtlePenSize(9);
        if (parent.renderNamesForConnections) {
            turtlePenColor(parent.colours[24], parent.colours[25], parent.colours[26]);
            turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 + 5, -45);
        } else {
            turtlePenColor(parent.colours[27], parent.colours[28], parent.colours[29]);
            turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 - 5, -45);
        }
        turtlePenDown();
        turtlePenUp();

        if (parent.parallax) {
            turtlePenColor(parent.colours[18], parent.colours[19], parent.colours[20]);
        } else {
            turtlePenColor(parent.colours[21], parent.colours[22], parent.colours[23]);
        }
        turtlePenSize(10);
        turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 - 5, -75);
        turtlePenDown();
        turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 + 5, -75);
        turtlePenUp();
        turtlePenSize(9);
        if (parent.parallax) {
            turtlePenColor(parent.colours[24], parent.colours[25], parent.colours[26]);
            turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 + 5, -75);
        } else {
            turtlePenColor(parent.colours[27], parent.colours[28], parent.colours[29]);
            turtleGoto((self.legendBounds[0] + self.legendBounds[1]) / 2 - 5, -75);
        }
        turtlePenDown();
        turtlePenUp();
        turtle.circleprez = saveprez;
       
    }
    *((graph_node_t *) (parent.graphs -> data[0].p)) = self;
    *parentp = parent;
}

void highlightNode(visual *parentp) {
    visual parent = *parentp;
    graph_node_t self = *((graph_node_t *) (parent.graphs -> data[0].p));
    /* gather list */
    list_clear(parent.selectedIDs);
    if (self.hover == -1) {
        for (int i = 1; i < self.nodes -> length; i++) {
            list_append(parent.selectedIDs, (unitype) i, 'i');
        }
    } else {
        /* add all connections to selected list */
        node_t node = *((node_t *) (self.nodes -> data[self.hover].p));
        char addedSelf = 0;
        if (list_find(node.connections, (unitype) self.hover, 'i') != -1) { // sometimes people are listed multiple times on one movie for multiple roles, thus becoming their own connection
            addedSelf = 1;
        }
        for (int i = 1; i < node.connections -> length; i += 2) {
            if (node.connections -> data[i].i > self.hover && addedSelf == 0) {
                list_append(parent.selectedIDs, (unitype) self.hover, 'i');
                addedSelf = 1;
            }
            list_append(parent.selectedIDs, node.connections -> data[i], 'i');
        }
        if (addedSelf == 0) {
            list_append(parent.selectedIDs, (unitype) self.hover, 'i');
        }
        // list_print(parent.selectedIDs);
    }
    *parentp = parent;
}

void mouseTick(visual *parentp) {
    visual parent = *parentp;
    /* bind mouse coordinates to window coordinates */
    if (turtle.mouseX > 320) {
        parent.mouseX = 320;
    } else {
        if (turtle.mouseX < -320) {
            parent.mouseX = -320;
        } else {
            parent.mouseX = turtle.mouseX;
        }
    }
    if (turtle.mouseY > 180) {
        parent.mouseY = 180;
    } else {
        if (turtle.mouseY < -180) {
            parent.mouseY = -180;
        } else {
            parent.mouseY = turtle.mouseY;
        }
    }
    graph_node_t self = *((graph_node_t *) (parent.graphs -> data[0].p));
    if (turtleMouseDown()) {
        if (parent.keys[0] == 0) {
            /* first tick */
            if (parent.mouseX < self.legendBounds[0]) {
                parent.keys[0] = 1;
                self.focalX = parent.mouseX;
                self.focalY = parent.mouseY;
                self.focalCSX = self.screenX;
                self.focalCSY = self.screenY;
            } else {
                self.genreLocked = 0;
                parent.keys[0] = 3;

                /* check switches */
                if (parent.mouseX >= 260 && parent.mouseX <= 280) {
                    if (parent.mouseY >= -20 && parent.mouseY <= -10) {
                        if (parent.renderConnections) {
                            parent.renderConnections = 0;
                        } else {
                            parent.renderConnections = 1;
                        }
                    }
                    if (parent.mouseY >= -50 && parent.mouseY <= -40) {
                        if (parent.renderNamesForConnections) {
                            parent.renderNamesForConnections = 0;
                        } else {
                            parent.renderNamesForConnections = 1;
                        }
                    }
                    if (parent.mouseY >= -80 && parent.mouseY <= -70) {
                        if (parent.parallax) {
                            parent.parallax = 0;
                        } else {
                            parent.parallax = 1;
                        }
                    }
                }
            }
        } else if (parent.keys[0] < 3) {
            self.screenX = (parent.mouseX - self.focalX) / self.globalsize + self.focalCSX;
            self.screenY = (parent.mouseY - self.focalY) / self.globalsize + self.focalCSY;
            if (fabs(parent.mouseX - self.focalX) > 0.1 && fabs(parent.mouseY - self.focalY) > 0.1) {
                parent.keys[0] = 2;
            }
        }
    } else {
        if (parent.keys[0]) {
            parent.keys[0] = 0;
            if (self.focalX == parent.mouseX && self.focalY == parent.mouseY) {
                /* highlight */
                highlightNode(&parent);
                if (self.hover == -1 && self.highlight != -1) { // crash protection
                    parent.renderConnections = 0;
                } else if (self.hover != -1) {
                    parent.renderConnections = 1;
                }
                self.highlight = self.hover;
                self.genreHighlight = -1;
                self.genreLocked = 0;
                // if (parent.selectedIDs -> length > 10000) {
                //     parent.renderConnections = 0;
                // }
            }
        }
    }
    *((graph_node_t *) parent.graphs -> data[0].p) = self;
    *parentp = parent;
}

void hotkeyTick(visual *parentp) {
    visual parent = *parentp;
    graph_node_t self = *((graph_node_t *) (parent.graphs -> data[0].p));
    if (turtleKeyPressed(GLFW_KEY_SPACE)) {
        if (parent.keys[2] == 0) {
            parent.keys[2] = 1;
            if (parent.parallax) {
                parent.parallax = 0;
            } else {
                parent.parallax = 1;
            }
        }
    } else {
        parent.keys[2] = 0;
    }
    if (turtleKeyPressed(GLFW_KEY_W)) {
        if (parent.keys[3] == 0) {
            parent.keys[3] = 1;
            if (parent.renderConnections) {
                parent.renderConnections = 0;
            } else {
                parent.renderConnections = 1;
            }
        }
    } else {
        parent.keys[3] = 0;
    }
    if (turtleKeyPressed(GLFW_KEY_A)) {
        if (parent.keys[4] == 0) {
            parent.keys[4] = 1;
            self.screenX = 0;
            self.globalsize = 1;
        }
    } else {
        parent.keys[4] = 0;
    }
    if (turtleKeyPressed(GLFW_KEY_C)) {
        if (parent.keys[5] == 0) {
            parent.keys[5] = 1;
            if (parent.renderNamesForConnections) {
                parent.renderNamesForConnections = 0;
            } else {
                parent.renderNamesForConnections = 1;
            }
        }
    } else {
        parent.keys[5] = 0;
    }
    *((graph_node_t *) parent.graphs -> data[0].p) = self;
    *parentp = parent;
}

void scrollTick(visual *parentp) {
    visual parent = *parentp;
    parent.mouseW = turtleMouseWheel();
    graph_node_t self = *((graph_node_t *) parent.graphs -> data[0].p);
    if (parent.mouseW > 0) {
        self.screenX -= (turtle.mouseX * (-1 / self.scrollSpeed + 1)) / self.globalsize;
        self.screenY -= (turtle.mouseY * (-1 / self.scrollSpeed + 1)) / self.globalsize;
        self.globalsize *= self.scrollSpeed;
        parent.keys[1] = 1;
    }
    if (parent.mouseW < 0) {
        self.globalsize /= self.scrollSpeed;
        self.screenX += (turtle.mouseX * (-1 / self.scrollSpeed + 1)) / self.globalsize;
        self.screenY += (turtle.mouseY * (-1 / self.scrollSpeed + 1)) / self.globalsize;
        parent.keys[1] = 1;
    }
    if (parent.keys[1] > 0) {
        parent.keys[1]++;
        if (parent.keys[1] > 15) {
            parent.keys[1] = 0;
        }
    }
    *((graph_node_t *) parent.graphs -> data[0].p) = self;
    *parentp = parent;
}

void parseRibbonOutput(visual *selfp) {
    #ifdef OS_WINDOWS
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
    #endif
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
    window = glfwCreateWindow(windowHeight * 16 / 9, windowHeight, "IMDb Actor Graph", NULL, NULL);
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
    turtle.circleprez = 3;
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
    visual parent;
    init(&parent); // initialise the logicgates

    turtleBgColor(parent.colours[0], parent.colours[1], parent.colours[2]);
    ribbonDarkTheme();

    if (argc > 1) {
        import(&parent, argv[1]);
        #ifdef OS_WINDOWS
        strcpy(win32FileDialog.selectedFilename, argv[1]);
        #endif
        #ifdef OS_LINUX
        strcpy(zenityFileDialog.selectedFilename, argv[1]);
        #endif
    } else {
        import(&parent, "customSetTruncated.tsv");
    }
    int frame = 0;
    while (turtle.close == 0) {
        start = clock(); // for frame syncing
        turtleClear(); // clear the screen
        turtleGetMouseCoords(); // get the mouse coordinates (turtle.mouseX, turtle.mouseY)
        renderConnections(&parent);
        renderGraph(&parent);
        renderTimeline(&parent);
        renderLegend(&parent);
        ribbonUpdate(); // do ribbon before mouseTick
        parseRibbonOutput(&parent);
        mouseTick(&parent);
        hotkeyTick(&parent);
        scrollTick(&parent);
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