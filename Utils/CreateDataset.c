/* select OS */
// #define OS_WINDOWS
#define OS_LINUX

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

// shared state
typedef struct {
    list_t *fileList; // list of filenames
    double colours[42];
    /* do I want rows or columns? Columns.
    Here's the spec:
    
    
    */
    list_t *datasets; // a list of columns lists, which itself is a list of column lists. There is one entry here for every new file imported
    int loadingSegmentsI; // number of segments to display on import file loading bar
    int loadingSegmentsE; // number of segments to display on export file loading bar
} visual;

/*
Number of movies: 672270
    Drama: 242665
    Documentary: 129106
    Comedy: 112741
    \N: 74541
    Action: 55886
    Romance: 48935
    Thriller: 46596
    Horror: 39628
    Crime: 38708
    Adventure: 29100
    Family: 18310
    Mystery: 18141
    Biography: 17892
    Fantasy: 15957
    History: 14857
    Sci-Fi: 14538
    Music: 13319
    Musical: 10505
    War: 9506
    Animation: 9488
    Adult: 8978
    Western: 8279
    Sport: 7688
    News: 1457
    Film-Noir: 881
    Reality-TV: 536
    Talk-Show: 180
    Game-Show: 27
    Short: 2
*/

enum M_GENRE {
    DRAMA,
    DOCUMENTARY,
    COMEDY,
    NOGENRE,
    ACTION,
    ROMANCE,
    THRILLER,
    HORROR,
    CRIME,
    ADVENTURE,
    FAMILY,
    MYSTERY,
    BIOGRAPHY,
    FANTASY,
    HISTORY,
    SCIFI,
    MUSIC,
    MUSICAL,
    WAR,
    ANIMATION,
    ADULT,
    WESTERN,
    SPORT,
    NEWS,
    FILMNOIR,
    REALITYTV,
    TALKSHOW,
    GAMESHOW,
    SHORT
};

void init(visual *selfp) {
    visual self = *selfp;
    self.fileList = list_init();
    self.datasets = list_init();
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
    self.loadingSegmentsI = 20; // 20 loading segments
    self.loadingSegmentsE = 100; // 100 loading segments
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
    list_t *columns = list_init();
    list_t *stringify = list_init();
    int index = 0;
    char read = '\0';
    char *tempStr;
    while (read != '\n' && fscanf(fp, "%c", &read) != EOF) {
        if (read == delimeter) {
            list_append(columns, (unitype) list_init(), 'r');
            tempStr = list_to_string(stringify);
            list_append(columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            stringify -> length = 0; // this should work, reduces malloc and realloc calls, but it could cause a memory leak
            index++;
        } else {
            list_append(stringify, (unitype) read, 'c');
        }
    }
    list_pop(stringify); // delete newline
    list_append(columns, (unitype) list_init(), 'r');
    tempStr = list_to_string(stringify);
    list_append(columns -> data[index].r, (unitype) tempStr, 's');
    free(tempStr);
    stringify -> length = 0;
    index = 0;
    lineLength = ftello(fp); // record data from after first line
    read = '\0';


    // estimate line length
    while (read != '\n' && fscanf(fp, "%c", &read) != EOF) {
        if (read == delimeter) {
            tempStr = list_to_string(stringify);
            list_append(columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            stringify -> length = 0;
            index++;
        } else {
            list_append(stringify, (unitype) read, 'c');
        }
    }
    list_pop(stringify); // delete newline
    tempStr = list_to_string(stringify);
    list_append(columns -> data[index].r, (unitype) tempStr, 's');
    free(tempStr);
    stringify -> length = 0;
    index = 0;
    lineLength = ftello(fp) - lineLength; /*  we use the first row to estimate the number of lines, but this is not optimal.
    It's really just dependends on how representative the first row is of the lengths of all the others */
    nextThresh = fileSize / self.loadingSegmentsI / lineLength;
    read = '\0';
    
    // read the rest
    while (fscanf(fp, "%c", &read) != EOF) {
        if (read == '\n') {
            // end of line
            char *tempStr = list_to_string(stringify);
            list_append(columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            stringify -> length = 0;
            index = 0;
            iters++;
        } else if (read == delimeter) {
            // end of column
            char *tempStr = list_to_string(stringify);
            list_append(columns -> data[index].r, (unitype) tempStr, 's');
            free(tempStr);
            list_clear(stringify);
            index++;
        } else {
            list_append(stringify, (unitype) read, 'c');
        }
        if (iters > nextThresh) {
            printf("read %lld/%lld lines\n", iters, fileSize / lineLength); // this is just an estimation, it is not 100% accurate
            if (threshLoaded <= self.loadingSegmentsI) {
                turtleGoto(620 / self.loadingSegmentsI * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += fileSize / self.loadingSegmentsI / lineLength;
            threshLoaded++;
            turtleUpdate();
        }
    }
    list_append(self.datasets, (unitype) columns, 'r');
    fclose(fp);
    *selfp = self;
    return 0;
}

/* this method turns a list of STRINGS (ONLY!) to a singular string where each string is separated by the delimeter string (or char) */
char *list_to_string_advanced(list_t *input, char *delimeter, char addNewline) {
    // printf("list: ");
    // list_print(input);
    if (strlen(delimeter) == 1) {
        char deli = delimeter[0];
        char *out = malloc(1);
        int newLen = 1;
        out[0] = '\0';
        for (int i = 0; i < (int) input -> length; i++) { // holy shit unsigned int was a mistake
            int len = strlen(out);
            newLen += sizeof(char) * (strlen(input -> data[i].s) + 1);
            // printf("out: %s %d\n", out, len);
            out = realloc(out, newLen);
            strcpy(out + len, input -> data[i].s);
            if (i == input -> length - 1) {
                out[newLen - 2] = '\0';
            } else {
                out[newLen - 2] = deli;
                out[newLen - 1] = '\0';
            }
        }
        // out[newLen - 2] = '\0'; // dont put delimeter after last one
        if (addNewline) {
            out[newLen - 2] = '\n';
            out[newLen - 1] = '\0';
        }
        // printf("the list became %c%s%c\n", '[', out, ']');
        return out;
    } else {
        printf("this should never happen\n");
        // don't care/not implemented
    }
    return NULL;
}

void export(visual *selfp, char *filename) {
    printf("opening %s\n", filename);
    visual self = *selfp;
    FILE *fp = fopen(filename, "w");
    /* get rid of all titles that aren't movies
    datasets[0] - titlePrincipals
    datasets[1] - titleBasics
    */
    list_t *titlePrincipals = self.datasets -> data[0].r;
    printf("length: %d\n", titlePrincipals -> data[0].r -> length);
    list_t *titleBasics = self.datasets -> data[1].r;
    list_t *rowlist = list_init();

    long long numLines = titlePrincipals -> data[0].r -> length;
    long long nextThresh = numLines / selfp -> loadingSegmentsE;
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
    for (int i = 1; i < titlePrincipals -> data[0].r -> length; i++) {
        if (iters > nextThresh) {
            printf("wrote %lld/%lld lines\n", iters, numLines); // this is accurate
            if (threshLoaded <= selfp -> loadingSegmentsE) {
                turtleGoto(620 / selfp -> loadingSegmentsE * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += numLines / selfp -> loadingSegmentsE;
            threshLoaded++;
            turtleUpdate();
        }
        iters++;
        
        char *tempStr = titlePrincipals -> data[0].r -> data[i].s;
        if (strlen(tempStr) < 9) {
            printf("error: tempStr less than 9 at %d\n", i);
            fclose(fp);
            exit(-1);
        }
        if (strlen(tempStr) > 9) {
            // printf("tempStr is more than 9, thats cool %s\n", tempStr);
        }
        // binary search
        int titleIndex = -1;
        int target = atoi(tempStr + 2); // integer position (probably)
        if (strlen(tempStr) > 9) {
            // printf("target: %d\n", target);
        }
        int lowerBound = 0;
        int upperBound = titleBasics -> data[0].r -> length - 1;
        int check = upperBound / 2;
        if (check > titleBasics -> data[0].r -> length - 1 || check < 0) {
            printf("heyo, check is %d\n", check);
            fclose(fp);
            exit(-1);
        }
        int numAt = atoi(titleBasics -> data[0].r -> data[check].s + 2);
        while (upperBound >= lowerBound) {
            if (numAt == target) {
                titleIndex = check;
                break;
            } else if (numAt > target) {
                upperBound = check - 1;
            } else {
                lowerBound = check + 1;
            }
            check = lowerBound + (upperBound - lowerBound) / 2;
            if (check > titleBasics -> data[0].r -> length - 1 || check < 0) {
                // printf("heyo, check is %d\n", check);
                // fclose(fp);
                // exit(-1);
            } else {
                numAt = atoi(titleBasics -> data[0].r -> data[check].s + 2);
            }
            
            // printf("checking %d %d %d\n", lowerBound, upperBound, check);
        }
        // int titleIndex = list_find(titleBasics -> data[0].r, (unitype) tempStr, 's');
        if (titleIndex == -1) {
            // we didn't find this title in titleBasics, skip
            // printf("did not find\n");
        } else {
            if (titleIndex > titleBasics -> data[0].r -> length - 1 || titleIndex < 0) {
                printf("heyo, this title index is %d\n", titleIndex);
                fclose(fp);
                exit(-1);
            }
            if (strcmp(titleBasics -> data[1].r -> data[titleIndex].s, "movie") != 0) {
                // not a movie, skip
                // i++;
                // iters++;
                // while (i < titlePrincipals -> data[0].r -> length && strcmp(titlePrincipals -> data[0].r -> data[i].s, tempStr) == 0) {
                //     i++;
                //     iters++;
                // }
                // iters--;
                // i--;
            } else {
                list_clear(rowlist);
                for (int j = 0; j < titlePrincipals -> length; j++) {
                    list_append(rowlist, titlePrincipals -> data[j].r -> data[i], 's');
                }
                char *toAdd = list_to_string_advanced(rowlist, "\t", 1);
                fprintf(fp, "%s", toAdd);
            }
        }
    }

    fclose(fp);
    *selfp = self;
}

// cull all non actors/actresses
void exportOnlyActors(visual *selfp, char *filename) {
    printf("opening %s\n", filename);
    visual self = *selfp;
    FILE *fp = fopen(filename, "w");
    /* get rid of all titles that aren't movies
    datasets[0] - titlePrincipals
    datasets[1] - titleBasics
    */
    list_t *titlePrincipals = self.datasets -> data[0].r;
    printf("length: %d\n", titlePrincipals -> data[0].r -> length);
    list_t *rowlist = list_init();

    long long numLines = titlePrincipals -> data[0].r -> length;
    long long nextThresh = numLines / selfp -> loadingSegmentsE;
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
    for (int i = 1; i < titlePrincipals -> data[0].r -> length; i++) {
        if (iters > nextThresh) {
            printf("wrote %lld/%lld lines\n", iters, numLines); // this is accurate
            if (threshLoaded <= selfp -> loadingSegmentsE) {
                turtleGoto(620 / selfp -> loadingSegmentsE * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += numLines / selfp -> loadingSegmentsE;
            threshLoaded++;
            turtleUpdate();
        }
        iters++;
        
        char *tempStr = titlePrincipals -> data[3].r -> data[i].s;
        if (strcmp(tempStr, "actor") == 0 || strcmp(tempStr, "actress") == 0) {
            list_clear(rowlist);
            for (int j = 0; j < titlePrincipals -> length; j++) {
                list_append(rowlist, titlePrincipals -> data[j].r -> data[i], 's');
            }
            char *toAdd = list_to_string_advanced(rowlist, "\t", 1);
            fprintf(fp, "%s", toAdd);
        }
    }
    fclose(fp);
    *selfp = self;
}

// cull all 10 digit names and titles
void exportNineDigits(visual *selfp, char *filename) {
    printf("opening %s\n", filename);
    visual self = *selfp;
    FILE *fp = fopen(filename, "w");
    /* get rid of all titles that aren't movies
    datasets[0] - titlePrincipals
    datasets[1] - titleBasics
    */
    list_t *titlePrincipals = self.datasets -> data[0].r;
    printf("length: %d\n", titlePrincipals -> data[0].r -> length);
    list_t *rowlist = list_init();

    long long numLines = titlePrincipals -> data[0].r -> length;
    long long nextThresh = numLines / selfp -> loadingSegmentsE;
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
    for (int i = 1; i < titlePrincipals -> data[0].r -> length; i++) {
        if (iters > nextThresh) {
            printf("wrote %lld/%lld lines\n", iters, numLines); // this is accurate
            if (threshLoaded <= selfp -> loadingSegmentsE) {
                turtleGoto(620 / selfp -> loadingSegmentsE * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += numLines / selfp -> loadingSegmentsE;
            threshLoaded++;
            turtleUpdate();
        }
        iters++;
        
        char *tempStr = titlePrincipals -> data[2].r -> data[i].s;
        if (strlen(tempStr) == 9 && strlen(titlePrincipals -> data[0].r -> data[i].s) == 9) {
            list_clear(rowlist);
            for (int j = 0; j < titlePrincipals -> length; j++) {
                list_append(rowlist, titlePrincipals -> data[j].r -> data[i], 's');
            }
            char *toAdd = list_to_string_advanced(rowlist, "\t", 1);
            fprintf(fp, "%s", toAdd);
        }
    }
    fclose(fp);
    *selfp = self;
}

// cull all 10 digit names
void exportNineDigitNames(visual *selfp, char *filename) {
    printf("opening %s\n", filename);
    visual self = *selfp;
    FILE *fp = fopen(filename, "w");
    /* get rid of all titles that aren't movies
    datasets[0] - titlePrincipals
    datasets[1] - titleBasics
    */
    list_t *nameBasics = self.datasets -> data[0].r;
    printf("length: %d\n", nameBasics -> data[0].r -> length);
    list_t *rowlist = list_init();

    long long numLines = nameBasics -> data[0].r -> length;
    long long nextThresh = numLines / selfp -> loadingSegmentsE;
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
    for (int i = 1; i < nameBasics -> data[0].r -> length; i++) {
        if (iters > nextThresh) {
            printf("wrote %lld/%lld lines\n", iters, numLines); // this is accurate
            if (threshLoaded <= selfp -> loadingSegmentsE) {
                turtleGoto(620 / selfp -> loadingSegmentsE * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += numLines / selfp -> loadingSegmentsE;
            threshLoaded++;
            turtleUpdate();
        }
        iters++;
        
        char *tempStr = nameBasics -> data[0].r -> data[i].s;
        if (strlen(tempStr) == 9) {
            list_clear(rowlist);
            for (int j = 0; j < nameBasics -> length; j++) {
                list_append(rowlist, nameBasics -> data[j].r -> data[i], 's');
            }
            char *toAdd = list_to_string_advanced(rowlist, "\t", 1);
            fprintf(fp, "%s", toAdd);
        }
    }
    fclose(fp);
    *selfp = self;
}

// get all name associations
void exportNames(visual *selfp, char *filename) {
    printf("opening %s\n", filename);
    visual self = *selfp;
    FILE *fp = fopen(filename, "w");
    /* get rid of all titles that aren't movies
    datasets[0] - titlePrincipals
    datasets[1] - titleBasics
    */
    list_t *titlePrincipals = self.datasets -> data[0].r;
    printf("length: %d\n", titlePrincipals -> data[0].r -> length);
    list_t *output = list_init();

    long long numLines = titlePrincipals -> data[0].r -> length;
    long long nextThresh = numLines / selfp -> loadingSegmentsE;
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
    for (int i = 0; i < 10000000; i++) {
        list_append(output, (unitype) list_init(), 'r');
    }
    
    int j = 1;
    list_t *runList = list_init();
    while (j < titlePrincipals -> data[0].r -> length) {
        if (iters > nextThresh) {
            printf("wrote %lld/%lld lines\n", iters, numLines); // this is accurate
            if (threshLoaded <= selfp -> loadingSegmentsE) {
                turtleGoto(620 / selfp -> loadingSegmentsE * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += numLines / selfp -> loadingSegmentsE;
            threshLoaded++;
            turtleUpdate();
        }
        // iters++;
        
        char *currentMovie = titlePrincipals -> data[0].r -> data[j].s;
        list_clear(runList);
        while (j < titlePrincipals -> data[0].r -> length && strcmp(currentMovie, titlePrincipals -> data[0].r -> data[j].s) == 0) {
            char *tempStr = titlePrincipals -> data[2].r -> data[j].s;
            list_append(runList, (unitype) tempStr, 's');
            j++;
            iters++;
        }
        for (int k = 0; k < runList -> length; k++) {
            int index = atoi(runList -> data[k].s + 2);
            for (int h = 0; h < runList -> length; h++) {
                if (h != k) {
                    int found = list_find(output -> data[index].r, runList -> data[h], 's');
                    if (found == -1) {
                        list_append(output -> data[index].r, (unitype) 1, 'i');
                        list_append(output -> data[index].r, (unitype) runList -> data[h], 's');
                    } else {
                        output -> data[index].r -> data[found - 1].i++;
                    }
                }
            }
        }
    }
    for (int i = 0; i < 10000000; i++) {
        if (output -> data[i].r -> length > 0) {
            for (int k = 0; k < output -> data[i].r -> length; k += 2) {
                char *convert = malloc(3);
                sprintf(convert, "%02d", output -> data[i].r -> data[k].i);
                output -> data[i].r -> type[k] = 's';
                output -> data[i].r -> data[k].s = convert;
            }
            char *sweep = list_to_string_advanced(output -> data[i].r, ",", 1);
            char *toAdd = malloc(sizeof(char) * (strlen(sweep) + 1 + 10));
            char *nameID = malloc(10);
            nameID[0] = 'n';
            nameID[1] = 'm';
            sprintf(nameID + 2, "%07d", i);
            strcpy(toAdd, nameID);
            strcpy(toAdd + 9, "\t");
            strcpy(toAdd + 10, sweep);
            // printf("%d\n", i);
            fprintf(fp, "%s", toAdd);
            free(nameID);
            free(sweep);
            free(toAdd);
        }
    }
    fclose(fp);
    *selfp = self;
}

char *getGenres(visual *selfp, char *movieTitle) {
    visual self = *selfp;
    list_t *titleBasics = self.datasets -> data[1].r;
    // binary search
    int titleIndex = -1;
    int target = atoi(movieTitle + 2); // integer position (probably)
    int lowerBound = 0;
    int upperBound = titleBasics -> data[0].r -> length - 1;
    int check = upperBound / 2;
    int numAt = atoi(titleBasics -> data[0].r -> data[check].s + 2);
    while (upperBound >= lowerBound) {
        if (numAt == target) {
            titleIndex = check;
            break;
        } else if (numAt > target) {
            upperBound = check - 1;
        } else {
            lowerBound = check + 1;
        }
        check = lowerBound + (upperBound - lowerBound) / 2;
        if (check > titleBasics -> data[0].r -> length - 1 || check < 0) {
            // printf("heyo, check is %d\n", check);
            // fclose(fp);
            // exit(-1);
        } else {
            numAt = atoi(titleBasics -> data[0].r -> data[check].s + 2);
        }
        // printf("checking %d %d %d\n", lowerBound, upperBound, check);
    }
    if (titleIndex == -1) {
        // we didn't find this title in titleBasics, skip
        // printf("did not find\n");
        return NULL;
    } else {
        return titleBasics -> data[8].r -> data[titleIndex].s;
    }
    *selfp = self;
}

void gatherPrimaryGenre(visual *selfp, char *filename) {
    printf("opening %s\n", filename);

    list_t *genreList = list_init();
    list_append(genreList, (unitype) "Drama", 's');
    list_append(genreList, (unitype) "Documentary", 's');
    list_append(genreList, (unitype) "Comedy", 's');
    list_append(genreList, (unitype) "\\N", 's');
    list_append(genreList, (unitype) "Action", 's');
    list_append(genreList, (unitype) "Romance", 's');
    list_append(genreList, (unitype) "Thriller", 's');
    list_append(genreList, (unitype) "Horror", 's');
    list_append(genreList, (unitype) "Crime", 's');
    list_append(genreList, (unitype) "Adventure", 's');
    list_append(genreList, (unitype) "Family", 's');
    list_append(genreList, (unitype) "Mystery", 's');
    list_append(genreList, (unitype) "Biography", 's');
    list_append(genreList, (unitype) "Fantasy", 's');
    list_append(genreList, (unitype) "History", 's');
    list_append(genreList, (unitype) "Sci-Fi", 's');
    list_append(genreList, (unitype) "Music", 's');
    list_append(genreList, (unitype) "Musical", 's');
    list_append(genreList, (unitype) "War", 's');
    list_append(genreList, (unitype) "Animation", 's');
    list_append(genreList, (unitype) "Adult", 's');
    list_append(genreList, (unitype) "Western", 's');
    list_append(genreList, (unitype) "Sport", 's');
    list_append(genreList, (unitype) "News", 's');
    list_append(genreList, (unitype) "Film-Noir", 's');
    list_append(genreList, (unitype) "Reality-TV", 's');
    list_append(genreList, (unitype) "Talk-Show", 's');
    list_append(genreList, (unitype) "Game-Show", 's');
    list_append(genreList, (unitype) "Short", 's');

    visual self = *selfp;
    FILE *fp = fopen(filename, "w");
    /* get rid of all titles that aren't movies
    datasets[0] - titlePrincipals
    datasets[1] - titleBasics
    datasets[2] - actorActor
    */
    list_t *titlePrincipals = self.datasets -> data[0].r;
    printf("length: %d\n", titlePrincipals -> data[0].r -> length);
    // list_t *titleBasics = self.datasets -> data[1].r;
    list_t *actorActor = self.datasets -> data[2].r;
    list_t *output = list_init();

    long long numLines = titlePrincipals -> data[0].r -> length;
    long long nextThresh = numLines / selfp -> loadingSegmentsE;
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
    for (int i = 0; i < 10000000; i++) {
        list_append(output, (unitype) list_init(), 'r');
        for (int j = 0; j < 30; j++) {
            list_append(output -> data[i].r, (unitype) 0, 'i');
        }
    }
    int j = 1;
    list_t *runList = list_init();
    while (j < titlePrincipals -> data[0].r -> length) {
        if (iters > nextThresh) {
            printf("wrote %lld/%lld lines\n", iters, numLines); // this is accurate
            if (threshLoaded <= selfp -> loadingSegmentsE) {
                turtleGoto(620 / selfp -> loadingSegmentsE * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += numLines / selfp -> loadingSegmentsE;
            threshLoaded++;
            turtleUpdate();
        }
        // iters++;
        
        char *currentMovie = titlePrincipals -> data[0].r -> data[j].s;
        char *genres = getGenres(selfp, currentMovie);
        list_clear(runList);
        while (j < titlePrincipals -> data[0].r -> length && strcmp(currentMovie, titlePrincipals -> data[0].r -> data[j].s) == 0) {
            char *tempStr = titlePrincipals -> data[2].r -> data[j].s;
            list_append(runList, (unitype) tempStr, 's');
            j++;
            iters++;
        }
        if (genres != NULL) {
            char *gen = strtok(genres, ",");
            while (gen != NULL) {
                int genreIndex = list_find(genreList, (unitype) gen, 's');
                if (genreIndex == -1) {
                    printf("ruh roh, could not find genre %s\n", gen);
                    genreIndex = 29;
                }
                for (int k = 0; k < runList -> length; k++) {
                    int nameIndex = atoi(runList -> data[k].s + 2);
                    output -> data[nameIndex].r -> data[genreIndex].i++;
                }
                gen = strtok(NULL, ",");
            }
        }
    }
    j = 1;
    list_t *rowlist = list_init();
    for (int i = 0; i < 10000000; i++) {
        if (atoi(actorActor -> data[0].r -> data[j].s + 2) == i) {
            list_clear(rowlist);
            list_append(rowlist, actorActor -> data[0].r -> data[j], 's');
            int max = 0;
            int maxInd = 3; // start at null
            for (int k = 0; k < output -> data[i].r -> length; k++) {
                if (output -> data[i].r -> data[k].i > max) {
                    max = output -> data[i].r -> data[k].i;
                    maxInd = k;
                }
            }
            list_append(rowlist, genreList -> data[maxInd], 's'); // genre
            list_append(rowlist, actorActor -> data[1].r -> data[j], 's');

            char *toAdd = list_to_string_advanced(rowlist, "\t", 1);
            fprintf(fp, "%s", toAdd);
            j++;
            if (j >= actorActor -> data[0].r -> length) {
                break;
            }
        }
    }
    fclose(fp);
    *selfp = self;
}

void gatherDoB(visual *selfp, char *filename) {
    printf("opening %s\n", filename);
    visual self = *selfp;
    FILE *fp = fopen(filename, "w");
    /* 
    datasets[0] - actorActor
    datasets[1] - nameBasics
    */
    list_t *nameBasics = self.datasets -> data[1].r;
    list_t *actorActor  = self.datasets -> data[0].r;
    printf("length: %d\n", actorActor -> data[0].r -> length);
    list_t *rowlist = list_init();

    long long numLines = actorActor -> data[0].r -> length;
    long long nextThresh = numLines / selfp -> loadingSegmentsE;
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
    for (int i = 1; i < actorActor -> data[0].r -> length; i++) {
        list_clear(rowlist);
        if (iters > nextThresh) {
            printf("wrote %lld/%lld lines\n", iters, numLines); // this is accurate
            if (threshLoaded <= selfp -> loadingSegmentsE) {
                turtleGoto(620 / selfp -> loadingSegmentsE * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += numLines / selfp -> loadingSegmentsE;
            threshLoaded++;
            turtleUpdate();
        }
        iters++;
        
        list_append(rowlist, actorActor -> data[0].r -> data[i], 's');
        list_append(rowlist, actorActor -> data[1].r -> data[i], 's');
        char *tempStr = actorActor -> data[0].r -> data[i].s;
        // binary search
        int titleIndex = -1;
        int target = atoi(tempStr + 2); // integer position (probably)
        int lowerBound = 0;
        int upperBound = nameBasics -> data[0].r -> length - 1;
        int check = upperBound / 2;
        int numAt = atoi(nameBasics -> data[0].r -> data[check].s + 2);
        while (upperBound >= lowerBound) {
            if (numAt == target) {
                titleIndex = check;
                break;
            } else if (numAt > target) {
                upperBound = check - 1;
            } else {
                lowerBound = check + 1;
            }
            check = lowerBound + (upperBound - lowerBound) / 2;
            if (check > nameBasics -> data[0].r -> length - 1 || check < 0) {
                // printf("heyo, check is %d\n", check);
                // fclose(fp);
                // exit(-1);
            } else {
                numAt = atoi(nameBasics -> data[0].r -> data[check].s + 2);
            }
            // printf("checking %d %d %d\n", lowerBound, upperBound, check);
        }
        if (titleIndex == -1) {
            // we didn't find this title in titleBasics, skip
            // printf("did not find\n");
            list_append(rowlist, (unitype) "\\N", 's');
        } else {
            list_append(rowlist, nameBasics -> data[2].r -> data[titleIndex], 's');
        }
        list_append(rowlist, actorActor -> data[2].r -> data[i], 's');
        char *toAdd = list_to_string_advanced(rowlist, "\t", 1);
        fprintf(fp, "%s", toAdd);
    }
    fclose(fp);
    *selfp = self;
}

void getRatings(double *rating, int *numVotes, visual *selfp, char *movieTitle) {
    visual self = *selfp;
    list_t *titleRatings = self.datasets -> data[2].r;
    // binary search
    int titleIndex = -1;
    int target = atoi(movieTitle + 2); // integer position (probably)
    int lowerBound = 0;
    int upperBound = titleRatings -> data[0].r -> length - 1;
    int check = upperBound / 2;
    int numAt = atoi(titleRatings -> data[0].r -> data[check].s + 2);
    while (upperBound >= lowerBound) {
        if (numAt == target) {
            titleIndex = check;
            break;
        } else if (numAt > target) {
            upperBound = check - 1;
        } else {
            lowerBound = check + 1;
        }
        check = lowerBound + (upperBound - lowerBound) / 2;
        if (check > titleRatings -> data[0].r -> length - 1 || check < 0) {
            // printf("heyo, check is %d\n", check);
            // fclose(fp);
            // exit(-1);
        } else {
            numAt = atoi(titleRatings -> data[0].r -> data[check].s + 2);
        }
        // printf("checking %d %d %d\n", lowerBound, upperBound, check);
    }
    if (titleIndex == -1) {
        // printf("did not find\n");
        *rating = 0;
        *numVotes = 0;
        // printf("couldn't find rating for %s\n", movieTitle);
    } else {
        sscanf(titleRatings -> data[1].r -> data[titleIndex].s, "%lf", rating);
        *numVotes = atoi(titleRatings -> data[2].r -> data[titleIndex].s);
    }
    *selfp = self;
}

void addRecognisability(visual *selfp, char *filename) {
    printf("opening %s\n", filename);

    visual self = *selfp;
    FILE *fp = fopen(filename, "w");
    /* get rid of all titles that aren't movies
    datasets[0] - actorActor
    datasets[1] - titlePrincipals
    datasets[2] - titleRatings
    */
    list_t *actorActor = self.datasets -> data[0].r;
    list_t *titlePrincipals = self.datasets -> data[1].r;
    // list_t *titleRatings = self.datasets -> data[2].r;
    printf("length: %d\n", titlePrincipals -> data[0].r -> length);
    
    list_t *output = list_init();

    long long numLines = titlePrincipals -> data[0].r -> length;
    long long nextThresh = numLines / selfp -> loadingSegmentsE;
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
    for (int i = 0; i < 10000000; i++) {
        list_append(output, (unitype) list_init(), 'r');
    }
    int j = 1;
    list_t *runList = list_init();
    while (j < titlePrincipals -> data[0].r -> length) {
        if (iters > nextThresh) {
            printf("wrote %lld/%lld lines\n", iters, numLines); // this is accurate
            if (threshLoaded <= selfp -> loadingSegmentsE) {
                turtleGoto(620 / selfp -> loadingSegmentsE * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += numLines / selfp -> loadingSegmentsE;
            threshLoaded++;
            turtleUpdate();
        }
        
        char *currentMovie = titlePrincipals -> data[0].r -> data[j].s;
        double rating = 0;
        int numVotes = 0;
        getRatings(&rating, &numVotes, selfp, currentMovie);
        list_clear(runList);
        while (j < titlePrincipals -> data[0].r -> length && strcmp(currentMovie, titlePrincipals -> data[0].r -> data[j].s) == 0) {
            char *tempStr = titlePrincipals -> data[2].r -> data[j].s;
            list_append(runList, (unitype) tempStr, 's');
            j++;
            iters++;
        }
        if (numVotes > 0) {
            for (int k = 0; k < runList -> length; k++) {
                int nameIndex = atoi(runList -> data[k].s + 2);
                list_append(output -> data[nameIndex].r, (unitype) rating, 'd');
                list_append(output -> data[nameIndex].r, (unitype) numVotes, 'i');
            }
        }
    }
    j = 1;
    list_t *rowlist = list_init();
    for (int i = 0; i < 10000000; i++) {
        if (atoi(actorActor -> data[0].r -> data[j].s + 2) == i) {
            list_clear(rowlist);
            list_append(rowlist, actorActor -> data[0].r -> data[j], 's');
            list_append(rowlist, actorActor -> data[1].r -> data[j], 's');
            list_append(rowlist, actorActor -> data[2].r -> data[j], 's');
            /* calculate recognisability */
            double calculatedR = 0;
            for (int k = 0; k < output -> data[i].r -> length; k += 2) {
                double rat = output -> data[i].r -> data[k].d;
                double vot = (double) output -> data[i].r -> data[k + 1].i;
                calculatedR += (log(vot) / log(2)) * rat;
            }
            char *recog = malloc(15);
            sprintf(recog, "%4.3lf", calculatedR);
            list_append(rowlist, (unitype) recog, 's');
            free(recog);
            list_append(rowlist, actorActor -> data[3].r -> data[j], 's');

            char *toAdd = list_to_string_advanced(rowlist, "\t", 1);
            fprintf(fp, "%s", toAdd);
            j++;
            if (j >= actorActor -> data[0].r -> length) {
                break;
            }
        }
    }
    fclose(fp);
    *selfp = self;
}


void addNameStrings(visual *selfp, char *filename) {
    printf("opening %s\n", filename);
    visual self = *selfp;
    FILE *fp = fopen(filename, "w");
    /* get rid of all titles that aren't movies
    datasets[0] - customSet
    */
    list_t *customSet = self.datasets -> data[0].r;
    list_t *nameBasics = self.datasets -> data[1].r;
    printf("length: %d\n", customSet -> data[0].r -> length);
    list_t *rowlist = list_init();

    long long numLines = customSet -> data[0].r -> length;
    long long nextThresh = numLines / selfp -> loadingSegmentsE;
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
    for (int i = 1; i < customSet -> data[0].r -> length; i++) {
        if (iters > nextThresh) {
            printf("wrote %lld/%lld lines\n", iters, numLines); // this is accurate
            if (threshLoaded <= selfp -> loadingSegmentsE) {
                turtleGoto(620 / selfp -> loadingSegmentsE * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += numLines / selfp -> loadingSegmentsE;
            threshLoaded++;
            turtleUpdate();
        }
        iters++;
        
        char *tempStr = customSet -> data[0].r -> data[i].s;
        list_clear(rowlist);
        list_append(rowlist, customSet -> data[0].r -> data[i], 's');
        // binary search
        int titleIndex = -1;
        int target = atoi(tempStr + 2); // integer position (probably)
        int lowerBound = 0;
        int upperBound = nameBasics -> data[0].r -> length - 1;
        int check = upperBound / 2;
        int numAt = atoi(nameBasics -> data[0].r -> data[check].s + 2);
        while (upperBound >= lowerBound) {
            if (numAt == target) {
                titleIndex = check;
                break;
            } else if (numAt > target) {
                upperBound = check - 1;
            } else {
                lowerBound = check + 1;
            }
            check = lowerBound + (upperBound - lowerBound) / 2;
            if (check > nameBasics -> data[0].r -> length - 1 || check < 0) {
                // printf("heyo, check is %d\n", check);
                // fclose(fp);
                // exit(-1);
            } else {
                numAt = atoi(nameBasics -> data[0].r -> data[check].s + 2);
            }
            // printf("checking %d %d %d\n", lowerBound, upperBound, check);
        }
        if (titleIndex == -1) {
            // printf("did not find\n");
            // printf("couldn't find rating for %s\n", movieTitle);
            list_append(rowlist, (unitype) "\\N", 's');
        } else {
            list_append(rowlist, nameBasics -> data[1].r -> data[titleIndex], 's');
        }
        for (int j = 1; j < customSet -> length; j++) {
            list_append(rowlist, customSet -> data[j].r -> data[i], 's');
        }
        char *toAdd = list_to_string_advanced(rowlist, "\t", 1);
        fprintf(fp, "%s", toAdd);
    }
    fclose(fp);
    *selfp = self;
}

// cull all actors with 0 recognisibility score
void cullNonZero(visual *selfp, char *filename) {
    printf("opening %s\n", filename);
    visual self = *selfp;
    FILE *fp = fopen(filename, "w");
    /* get rid of all titles that aren't movies
    datasets[0] - customSet
    datasets[1] - nameBasics
    */
    list_t *customSet = self.datasets -> data[0].r;
    printf("length: %d\n", customSet -> data[0].r -> length);
    list_t *rowlist = list_init();

    long long numLines = customSet -> data[0].r -> length;
    long long nextThresh = numLines / selfp -> loadingSegmentsE;
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
    for (int i = 1; i < customSet -> data[0].r -> length; i++) {
        if (iters > nextThresh) {
            printf("wrote %lld/%lld lines\n", iters, numLines); // this is accurate
            if (threshLoaded <= selfp -> loadingSegmentsE) {
                turtleGoto(620 / selfp -> loadingSegmentsE * threshLoaded - 310, 160);
                turtlePenUp();
                turtlePenDown();
            }
            nextThresh += numLines / selfp -> loadingSegmentsE;
            threshLoaded++;
            turtleUpdate();
        }
        iters++;
        
        char *tempStr = customSet -> data[3].r -> data[i].s;
        if (strcmp(tempStr, "0.000") != 0) {
            list_clear(rowlist);
            for (int j = 0; j < customSet -> length; j++) {
                list_append(rowlist, customSet -> data[j].r -> data[i], 's');
            }
            char *toAdd = list_to_string_advanced(rowlist, "\t", 1);
            fprintf(fp, "%s", toAdd);
        }
    }
    fclose(fp);
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
    // if (ribbonRender.output[0] == 1) {
    //     ribbonRender.output[0] = 0; // untoggle
    //     if (ribbonRender.output[1] == 0) { // file
    //         if (ribbonRender.output[2] == 1) { // new
    //             printf("Custom command\n");
    //             // printAllGenres(&self);
    //             // customCommand(&self);
    //         }
    //         if (ribbonRender.output[2] == 2) { // save
    //             if (strcmp(win32FileDialog.selectedFilename, "null") == 0) {
    //                 if (win32FileDialogPrompt(1, "") != -1) {
    //                     printf("Saved to: %s\n", win32FileDialog.selectedFilename);
    //                     export(&self, win32FileDialog.selectedFilename);
    //                 }
    //             } else {
    //                 printf("Saved to: %s\n", win32FileDialog.selectedFilename);
    //                 export(&self, win32FileDialog.selectedFilename);
    //             }
    //         }
    //         if (ribbonRender.output[2] == 3) { // save as
    //             if (win32FileDialogPrompt(1, "") != -1) {
    //                 printf("Saved to: %s\n", win32FileDialog.selectedFilename);
    //                 export(&self, win32FileDialog.selectedFilename);
    //             }
    //         }
    //         if (ribbonRender.output[2] == 4) { // load
    //             if (win32FileDialogPrompt(0, "") != -1) {
    //                 // printf("Loaded data from: %s\n", win32FileDialog.selectedFilename);
    //                 // clearAll(&self);
    //                 import(&self, win32FileDialog.selectedFilename);
    //                 // truncate(&self, win32FileDialog.selectedFilename);
    //             }
    //         }
    //     }
    // }
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
    /* import nameBasics and titlePrincipals */
    // printf("importing\n");
    // import(&self, "nameBasicsTruncated.tsv");
    // import(&self, "titlePrincipalsTruncated.tsv");
    // printf("datasets: %d\n", self.datasets -> length);
    // export(&self, "custom2.tsv");
    // printf("completed custom set\n");

    // import(&self, "/mnt/d/Characters/Information/College/GeeVis/titlePrincipals.tsv");
    // import(&self, "/mnt/d/Characters/Information/College/GeeVis/titleBasics.tsv");
    // import(&self, "D:\\Characters\\Information\\College\\GeeVis\\titlePrincipals.tsv");
    // import(&self, "D:\\Characters\\Information\\College\\GeeVis\\titleBasics.tsv");
    // import(&self, "titlePrincipalsTruncated.tsv");
    // import(&self, "titleBasicsTruncated.tsv");
    // import(&self, "titlePrincipalsOnlyMovieActorsNineDigits.tsv");
    // import(&self, "actorActorGenre.tsv");
    // import(&self, "titlePrincipalsOnlyMovieActorsNineDigits.tsv");
    // import(&self, "D:\\Characters\\Information\\College\\GeeVis\\nameBasics.tsv");
    // exportNineDigits(&self, "titlePrincipalsOnlyMovieActorsNineDigits.tsv");
    // gatherDoB(&self, "actorActorGenreDoB2.tsv");


    // import(&self, "actorActorGenreDoB2.tsv");
    // import(&self, "titlePrincipalsOnlyMovieActorsNineDigits.tsv");
    // import(&self, "D:\\Characters\\Information\\College\\GeeVis\\titleRatings.tsv");
    // addRecognisability(&self, "customSet.tsv");

    import(&self, "customSetMini.tsv");
    import(&self, "D:\\Characters\\Information\\College\\GeeVis\\nameBasicsNineDigits.tsv");
    addNameStrings(&self, "customSetMiniNames.tsv");
    // exportNineDigitNames(&self, "nameBasicsNineDigits");


    int frame = 0;
    while (turtle.close == 0) {
        start = clock(); // for frame syncing
        turtleClear(); // clear the screen
        turtleGetMouseCoords(); // get the mouse coordinates (turtle.mouseX, turtle.mouseY)
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