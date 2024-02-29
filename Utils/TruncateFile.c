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

/* truncate file */
int truncate(visual *selfp, char *filename) {
    visual self = *selfp;
    /* identify file type
    Supported types:
     - tsv
    Structure:
    */
    int filenameLen = strlen(filename);
    if (filename[filenameLen - 1] == 'v' && filename[filenameLen - 2] == 's' && filename[filenameLen - 3] == 'c' && filename[filenameLen - 4] == '.') {
        printf("not implemented yet\n");
    } else if (filename[filenameLen - 1] == 'v' && filename[filenameLen - 2] == 's' && filename[filenameLen - 3] == 't' && filename[filenameLen - 4] == '.') {
        list_append(self.fileList, (unitype) filename, 's');
        FILE *fp = fopen(filename, "r");
        FILE *fpwr = fopen("truncated.tsv", "w");
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


        // read headers
        list_t *columns = list_init();
        list_t *stringify = list_init();
        int index = 0;
        char read = '\0';

        // read
        while (fscanf(fp, "%c", &read) != EOF) {
            if (read == '\n') {
                // printf("iter %d\n", iters);
                // end of line
                char *tempStr = list_to_string(stringify);
                fprintf(fpwr, "%s%c", tempStr, '\n');
                free(tempStr);
                list_clear(stringify);
                index = 0;
                iters++;
            } else if (read == '\t') {
                // printf("made it\n");
                // end of column
                char *tempStr = list_to_string(stringify);
                fprintf(fpwr, "%s%c", tempStr, '\t');
                free(tempStr);
                list_clear(stringify);
                index++;
            } else {
                list_append(stringify, (unitype) read, 'c');
            }
            if (iters > 10000) {
                printf("truncated file\n");
                fclose(fpwr);
                fclose(fp);
                return 0;
            }
        }
        fclose(fp);
        fclose(fpwr);
    }
    *selfp = self;
    return 0;
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
        /* enable to truncate */
        // if (iters > 100000) {
        //     printf("truncated file\n");
        //     fclose(fpwr);
        //     fclose(fp);
        //     return 0;
        // }
    }
    // list_print(columns -> data[1].r);
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
            strncpy(out + len, input -> data[i].s, strlen(input -> data[i].s));
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
    /* The Spec:
    I would like a custom set of data:
    I want it to be actors/actresses, numAct, associatedActors/Actresses, numFilm, associatedFilms (separated with ' for multiple films with the same two actors and , for the next actor)
    Example (inside {} is a column):

    {nm0018935} {04} {nm0062649,nm0063078,nm0026012,nm0090093} {01,02,01,01} {tt0016309,tt0021857'tt0007657,tt0012764,tt0019735}

    This means nm0018935 has 4 associated actors/actresses - nm0062649, nm0063078, nm0026012, nm0090093
    nm0018935 and nm0062649 played together in tt0016309
    nm0018935 and nm0063078 played together in tt0021857 AND tt0007657
    nm0018935 and nm0026012 played together in tt0012764
    nm0018935 and nm0090093 played together in tt0019735

    I can use the fact that each person and film ID is exactly 9 OR 10 characters long (thats annoying why are some of them different)
    I can normalise each one to 10 characters long by adding a leading 0 on ones that are 9 characters long
    and I think I will do that, hopefully that doesn't end up biting me

    One last thing: I want the numbers in the 4th column to all be 2 digit numbers, for the same reason.
    But this hinges on no two actors being assocated with each other for more than 99 films
    I will do a check while gathering this data to make sure this doesn't happen. If it does I'll be surprised
    */

    /* The Method:
    We'll be loading in two datasets:
    nameBasics
    titlePrincipals

    We go down the nameBasics list.
    If they have actor/actress in their primaryProfession:
        We go through every "known for" title:
            If they are listed in that title as an actor/actress (this will cull small actors somewhat):
                We get everyone else listed in that title as an actor/actress
                If they are already in the assocA list
                    We add the film to the correct index of the associatedFilms entry
                otherwise
                    We add them to the assocA list
                    We append to the "associatedFilms" list to add that film
                Insert length of assocA list as a string to index 1 of rowList
                Turn each length of assocF lists into 2 digit strings and put a list of that into index 3 of rowList
                Add rowList to file

    Stringify everything, tab delimeters for rowList, commas for assocA, commas for numFilms, commas for assocF, apostrophes for assocF children lists
    Structure:
    rowList - list with the data for a single row it has {str, str, list, list, list of lists} - replaced to {str, str, str, str, str} before the final gauntlet
    assocA list - list with the data for all associated actors/actresses with a given one {str, str, ..., n} (where n is the number of associated actors/actresses with the given one)
    numFilms - generated last, has 2 digit numbers for how many associated films each actor/actress has with the given one {str, str, ..., n}
    assocF - has lists of associated films with each actor/actresses association with the given one {list, list, ..., n}
    children of assocF - the child lists of assocF have the associated films for the actor/actress associated with the given one {str, str, ..., x} (where x is the number in numFilms[i] where is the index of this child in assocF)
    
    Situation: mismatch
     - What will happen is this situation.
      - An actor will be listed as associated with another, but that other will not be associated with the first. In other words, association mismatch
      - This is because small actors are "known for" titles that big actors are not, or some play a more major role
      - This should be fine, it just means that the connections go one way in many cases
      - Small actors will be mostly culled because they are not even listed as actors for the titles that they are "known for" which is kind of tragic but hollywood is ruthless

    The expected usecase of this custom set:
    We want to graph

    This dataset can be expanded upon later - adding average rating for all associated films

    This is one of the most complex things I've ever done
    */
    if (selfp -> datasets -> length != 2) {
        printf("expecting datasets nameBasics and titlePrincipals. Got %d datasets\n", selfp -> datasets -> length);
        return;
    }
    list_t *rowList = list_init();
    list_t *assocA = list_init();
    list_t *numFilms = list_init();
    list_t *assocF = list_init();

    list_t *nameBasics = selfp -> datasets -> data[0].r;
    list_t *titlePrincipals;
    if (strcmp(nameBasics -> data[0].r -> data[0].s, "nconst") == 0) {
        /* this is expected */
        titlePrincipals = selfp -> datasets -> data[1].r;
        if (strcmp(selfp -> datasets -> data[1].r -> data[0].r -> data[0].s, "tconst") != 0) {
            printf("expecting datasets nameBasics and titlePrincipals. Got unrecognised %s as first column header for list 2\n", selfp -> datasets -> data[1].r -> data[0].r -> data[0].s);
            return;
        }
    } else {
        if (strcmp(nameBasics -> data[0].r -> data[0].s, "tconst") == 0) {
            /* switch them */
            titlePrincipals = nameBasics;
            nameBasics = selfp -> datasets -> data[1].r;
            if (strcmp(selfp -> datasets -> data[1].r -> data[0].r -> data[0].s, "nconst") != 0) {
                printf("expecting datasets nameBasics and titlePrincipals. Got unrecognised %s as first column header for list 2\n", selfp -> datasets -> data[1].r -> data[0].r -> data[0].s);
                return;
            }
        } else {
            printf("expecting datasets nameBasics and titlePrincipals. Got unrecognised %s as first column header for list 1\n", selfp -> datasets -> data[0].r -> data[0].r -> data[0].s);
            return;
        }
    }
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("could not open file customSet.tsv\n");
        return;
    }
    /* set up loading bar */
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

    char *tempStr; // we're going to be using this a lot
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
        list_clear(rowList);
        list_clear(assocA);
        list_clear(numFilms);
        list_clear(assocF);
        /* determine whether this person is an actor/actress */
        char isAct = 0;
        tempStr = strtok(nameBasics -> data[4].r -> data[i].s, ",");
        while (tempStr != NULL) {
            if (strcmp(tempStr, "actor") == 0 || strcmp(tempStr, "actress") == 0) {
                /* i dont exactly know why it doesn't make tempStr the whole rest of it... Okay the answer is it modifies the original. Bro they have to mention this stuff */
                isAct = 1;
                break;
            }
            tempStr = strtok(NULL, ",");
        }
        if (isAct) {
            /* add name const to rowList (normalised to 10 characters) */
            if (strlen(nameBasics -> data[0].r -> data[i].s) == 9) {
                char nameConst[11];
                memcpy(nameConst, nameBasics -> data[0].r -> data[i].s, 2);
                nameConst[2] = '0';
                memcpy(nameConst + 3, nameBasics -> data[0].r -> data[i].s + 2, 8);
                // printf("sanity %s\n", nameConst);
                list_append(rowList, (unitype) nameConst, 's');
            } else {
                list_append(rowList, nameBasics -> data[0].r -> data[i], 's');
            }
            /* loop through every "known for" title */
            tempStr = strtok(nameBasics -> data[5].r -> data[i].s, ","); // current title string
            char tempStrNormal[11];
            /* normalise to 10 characters */
            if (strlen(tempStr) == 9) {
                memcpy(tempStrNormal, tempStr, 2);
                tempStrNormal[2] = '0';
                memcpy(tempStrNormal + 3, tempStr + 2, 8);
            } else {
                strcpy(tempStrNormal, tempStr);
            }
            if (strcmp(tempStr, "\\N") == 0) {
                // skip NULL
                tempStr = strtok(NULL, ",");
                continue;
            }
            int rollbackAssocA = 0;
            while (tempStr != NULL) {
                // binary search
                int titleIndex = -1;
                int target = atoi(tempStr + 2); // integer position (probably)
                int lowerBound = 0;
                int upperBound = titlePrincipals -> data[0].r -> length - 1;
                int check = upperBound / 2;
                int numAt = atoi(titlePrincipals -> data[0].r -> data[check].s + 2);
                while (upperBound >= lowerBound) {
                    if (numAt == target) {
                        titleIndex = check;
                        while (titleIndex > -1 && strcmp(titlePrincipals -> data[0].r -> data[titleIndex].s, tempStr) == 0) {
                            titleIndex--;
                        }
                        titleIndex++;
                        break;
                    } else if (numAt > target) {
                        upperBound = check - 1;
                    } else {
                        lowerBound = check + 1;
                    }
                    check = lowerBound + (upperBound - lowerBound) / 2;
                    numAt = atoi(titlePrincipals -> data[0].r -> data[check].s + 2);
                    // printf("checking %d %d %d\n", lowerBound, upperBound, check);
                }
                if (titleIndex == -1) {
                    // skip if not found
                    tempStr = strtok(NULL, ",");
                    continue;
                }
                char isListed = 0;
                while (titleIndex < titlePrincipals -> data[0].r -> length && strcmp(titlePrincipals -> data[0].r -> data[titleIndex].s, tempStr) == 0) {
                    if (strcmp(titlePrincipals -> data[3].r -> data[titleIndex].s, "actor") == 0 || strcmp(titlePrincipals -> data[3].r -> data[titleIndex].s, "actress") == 0) {
                        if (strcmp(titlePrincipals -> data[2].r -> data[titleIndex].s, nameBasics -> data[0].r -> data[i].s) == 0) {
                            /* they are listed as an actor/actress for this title */
                            isListed = 1;
                        } else {
                            /* screen for already associated */
                            char alreadyAssoc = 0;
                            for (int j = 0; j < assocA -> length; j++) {
                                if (strcmp(assocA -> data[j].s, titlePrincipals -> data[2].r -> data[titleIndex].s) == 0) {
                                    alreadyAssoc = 1;
                                    break;
                                }
                            }
                            if (alreadyAssoc) {
                                /* it's possible that we need to add this film to the list of associated films, but we can't know for sure until we know that this actor is listed as an actor for this title */
                                list_append(assocA, (unitype) "pending", 's'); // very dubious
                            }
                            /* add the person as an associated actor */
                            /* normalise to 10 characters */
                            if (strlen(titlePrincipals -> data[2].r -> data[titleIndex].s) == 9) {
                                char nameConst[11];
                                memcpy(nameConst, titlePrincipals -> data[2].r -> data[titleIndex].s, 2);
                                nameConst[2] = '0';
                                memcpy(nameConst + 3, titlePrincipals -> data[2].r -> data[titleIndex].s + 2, 8);
                                list_append(assocA, (unitype) nameConst, 's');
                            } else {
                                list_append(assocA, titlePrincipals -> data[2].r -> data[titleIndex], 's');
                            }
                        }
                    }
                    titleIndex++;
                }
                if (isListed) {
                    /* add actors as associated, add film to associated film with that actor */
                    for (int j = rollbackAssocA; j < assocA -> length; j++) {
                        if (strcmp(assocA -> data[j].s, "pending") == 0) {
                            list_append(assocF -> data[list_find(assocA, assocA -> data[j + 1], 's')].r, (unitype) tempStrNormal, 's');
                            list_delete(assocA, j); // might cause problems :|
                            list_delete(assocA, j);
                            j--;
                        } else {
                            list_append(assocF, (unitype) list_init(), 'r');
                            list_append(assocF -> data[assocF -> length - 1].r, (unitype) tempStrNormal, 's');
                        }
                    }
                    rollbackAssocA = assocA -> length;
                } else {
                    /* roll back actors list */
                    assocA -> length = rollbackAssocA;
                }
                tempStr = strtok(NULL, ",");
            }
            /* build numFilms */
            for (int j = 0; j < assocF -> length; j++) {
                /* convert to 2 digit string */
                char lenAssocFilms[3];
                /* check if there are more than 99 associated films */
                if (assocF -> data[j].r -> length > 99) {
                    printf("more than 99 associated films for actors %s and %s\n", nameBasics -> data[0].r -> data[i].s, assocA -> data[j].s);
                    return;
                }
                sprintf(lenAssocFilms, "%02d", assocF -> data[j].r -> length);
                list_append(numFilms, (unitype) lenAssocFilms, 's');
            }
            /* build rowList */
            /* convert number of associated actors to 2 digit string */
            char numAssocA[3];
            /* check if there are more than 99 associated films */
            if (assocA -> length > 99) {
                printf("more than 99 associated actors with %s\n", nameBasics -> data[0].r -> data[i].s);
                return;
            }
            sprintf(numAssocA, "%02d", assocA -> length);
            list_append(rowList, (unitype) numAssocA, 's');

            tempStr = list_to_string_advanced(assocA, ",", 0);
            list_append(rowList, (unitype) tempStr, 's');
            free(tempStr);

            tempStr = list_to_string_advanced(numFilms, ",", 0);
            list_append(rowList, (unitype) tempStr, 's');

            free(tempStr);
            for (int j = 0; j < assocF -> length; j++) {
                tempStr = list_to_string_advanced(assocF -> data[j].r, "'", 0);
                list_free(assocF -> data[j].r);
                /* replace assocF with strings */
                assocF -> type[j] = 's';
                assocF -> data[j].s = strdup(tempStr);
                free(tempStr);
            }
            tempStr = list_to_string_advanced(assocF, ",", 0);
            // if (assocF -> length > 0) {
            //     printf("assocF: ");
            //     list_print(assocF);
            //     printf("tempStr: %s\n", tempStr);
            // }
            list_append(rowList, (unitype) tempStr, 's');
            free(tempStr);
            
            char *toAdd = list_to_string_advanced(rowList, "\t", 1);
            // if (assocF -> length > 0) {
            //     printf("rowList: ");
            //     list_print(rowList);
            //     printf("toAdd: %s\n", toAdd);
            //     exit(-1);
            // }
            /* add row to output file */
            fprintf(fp, "%s", toAdd);
        }
    }
    /* free memory */
    list_free(rowList);
    list_free(assocA);
    list_free(numFilms);
    list_free(assocF);
    fclose(fp);
}

void customCommand(visual *selfp) {
    visual self = *selfp;
    FILE *fp = fopen("customCommandOut.tsv", "w");
    if (fp == NULL) {
        printf("could not open file customSet.tsv\n");
        return;
    }
    /* 
    Needed:
     - actorActors (dataset[0])
     - titleBasics (dataset[1])
     - nameBasics (dataset[2])
     - titlePrincipals (dataset[3])
    add:
    birth and death years
    genre information to each person
    recognisability

    Because I cant load in all of that at once, I will do them one at a time
    */

    /* append birth and death year to actorActors
    Components:
     - actorActors (dataset[0])
     - nameBasics (dataset[1])
    */

    list_t *actorActors = self.datasets -> data[0].r;
    // list_t *nameBasics = self.datasets -> data[1].r;
    list_t *titlePrincipals = self.datasets -> data[1].r;
    list_t *rowList = list_init();
    char *tempStr;


    long long numLines = actorActors -> data[0].r -> length;
    // printf("length: %d\n", actorActors -> data[0].r -> length);
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
    for (int i = 1; i < actorActors -> data[0].r -> length; i++) {
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
        list_clear(rowList);
        // printf("length: %d\n", actorActors -> length);
        for (int j = 0; j < actorActors -> length; j++) {
            // printf("adding: %s\n", actorActors -> data[j].r -> data[i].s);
            list_append(rowList, actorActors -> data[j].r -> data[i], 's'); // 
        }
        // tempStr = actorActors -> data[0].r -> data[i].s; // name constant
        // // printf("name: %s\n", tempStr);
        // // binary search
        // int titleIndex = -1;
        // int target = atoi(tempStr + 2); // integer position (probably)
        // int lowerBound = 0;
        // int upperBound = nameBasics -> data[0].r -> length - 1;
        // int check = upperBound / 2;
        // int numAt = atoi(nameBasics -> data[0].r -> data[check].s + 2);
        // while (upperBound >= lowerBound) {
        //     if (numAt == target) {
        //         titleIndex = check;
        //         break;
        //     } else if (numAt > target) {
        //         upperBound = check - 1;
        //     } else {
        //         lowerBound = check + 1;
        //     }
        //     check = lowerBound + (upperBound - lowerBound) / 2;
        //     numAt = atoi(nameBasics -> data[0].r -> data[check].s + 2);
        //     // printf("checking %d %d %d\n", lowerBound, upperBound, check);
        // }
        // if (titleIndex == -1) {
        //     // if not found, use "\N"
        //     list_append(rowList, (unitype) "\\N", 's');
        //     list_append(rowList, (unitype) "\\N", 's');
        // } else {
        //     /* gather birth and death years */
        //     // printf("%s %s\n", nameBasics -> data[2].r -> data[titleIndex].s, nameBasics -> data[3].r -> data[titleIndex].s);
        //     list_append(rowList, nameBasics -> data[2].r -> data[titleIndex], 's'); // birth year
        //     list_append(rowList, nameBasics -> data[3].r -> data[titleIndex], 's'); // death year
        // }




        char *toAdd = list_to_string_advanced(rowList, "\t", 1);
        fprintf(fp, "%s", toAdd);
    }
    fclose(fp);
}


void customCommand2(visual *selfp) {
    visual self = *selfp;
    FILE *fp = fopen("customCommandOut.tsv", "w");
    if (fp == NULL) {
        printf("could not open file customSet.tsv\n");
        return;
    }
    /* 
    I want all the movies that this person is listed as an actor for
    */

    list_t *actorActors = self.datasets -> data[0].r;
    // list_t *nameBasics = self.datasets -> data[1].r;
    list_t *titlePrincipals = self.datasets -> data[1].r;
    list_t *rowList = list_init();
    char *tempStr;


    long long numLines = titlePrincipals -> data[0].r -> length;
    // printf("length: %d\n", actorActors -> data[0].r -> length);
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
            printf("read %lld/%lld lines\n", iters, numLines); // this is accurate
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
        tempStr = titlePrincipals -> data[0].r -> data[i].s; // name constant
        // printf("name: %s\n", tempStr);
        // binary search
        int titleIndex = -1;
        int target = atoi(tempStr + 2); // integer position (probably)
        int lowerBound = 0;
        int upperBound = actorActors -> data[0].r -> length - 1;
        int check = upperBound / 2;
        int numAt = atoi(actorActors -> data[0].r -> data[check].s + 2);
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
            numAt = atoi(actorActors -> data[0].r -> data[check].s + 2);
            // printf("checking %d %d %d\n", lowerBound, upperBound, check);
        }
        if (titleIndex == -1) {
            // if not found, use "\N"
            list_append(rowList, (unitype) "\\N", 's');
            list_append(rowList, (unitype) "\\N", 's');
        } else {
            /* gather birth and death years */
            // printf("%s %s\n", nameBasics -> data[2].r -> data[titleIndex].s, nameBasics -> data[3].r -> data[titleIndex].s);
            list_append(rowList, actorActors -> data[2].r -> data[titleIndex], 's'); // birth year
            list_append(rowList, actorActors -> data[3].r -> data[titleIndex], 's'); // death year
        }
        
    }
    fclose(fp);
}

void printAllGenres(visual *selfp) {
    visual self = *selfp;
    /* print all (movie only) genres from most to least popular. 
    Step 1. load in titleBasics */

    list_t *genres = list_init(); // genre, number of movies with it
    list_t *typeColumn = self.datasets -> data[0].r -> data[1].r;
    list_t *genreColumn = self.datasets -> data[0].r -> data[8].r;
    char *genre;
    long long movies = 0;
    for (int i = 0; i < typeColumn -> length; i++) {
        if (strcmp(typeColumn -> data[i].s, "movie") == 0) {
            movies++;
            genre = strtok(genreColumn -> data[i].s, ",");
            while (genre != NULL) {
                int index = list_find(genres, (unitype) genre, 's');
                if (index == -1) {
                    list_append(genres, (unitype) genre, 's');
                    list_append(genres, (unitype) 1, 'i');
                } else {
                    genres -> data[index + 1].i++;
                }
                genre = strtok(NULL, ",");
            }
        }
    }
    printf("number of movies: %lld\n", movies);
    list_print(genres);
    int maxInd = 0;
    while (genres -> length > 0) {
        int max = 0;
        for (int i = 0; i < genres -> length; i += 2) {
            if (genres -> data[i + 1].i > max) {
                maxInd = i;
                max = genres -> data[i + 1].i;
            }
        }
        printf("%s: %d\n", genres -> data[maxInd].s, genres -> data[maxInd + 1].i);
        list_delete(genres, maxInd);
        list_delete(genres, maxInd);
    }
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
                printf("Custom command\n");
                // printAllGenres(&self);
                // customCommand(&self);
            }
            if (ribbonRender.output[2] == 2) { // save
                if (strcmp(win32FileDialog.selectedFilename, "null") == 0) {
                    if (win32FileDialogPrompt(1, "") != -1) {
                        printf("Saved to: %s\n", win32FileDialog.selectedFilename);
                        truncate(&self, win32FileDialog.selectedFilename);
                    }
                } else {
                    printf("Saved to: %s\n", win32FileDialog.selectedFilename);
                    truncate(&self, win32FileDialog.selectedFilename);
                }
            }
            if (ribbonRender.output[2] == 3) { // save as
                if (win32FileDialogPrompt(1, "") != -1) {
                    printf("Saved to: %s\n", win32FileDialog.selectedFilename);
                    truncate(&self, win32FileDialog.selectedFilename);
                }
            }
            if (ribbonRender.output[2] == 4) { // load
                if (win32FileDialogPrompt(0, "") != -1) {
                    // printf("Loaded data from: %s\n", win32FileDialog.selectedFilename);
                    // clearAll(&self);
                    import(&self, win32FileDialog.selectedFilename);
                    // truncate(&self, win32FileDialog.selectedFilename);
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
    /* import nameBasics and titlePrincipals */
    // printf("importing\n");
    // import(&self, "nameBasicsTruncated.tsv");
    // import(&self, "titlePrincipalsTruncated.tsv");
    // printf("datasets: %d\n", self.datasets -> length);
    // export(&self, "custom2.tsv");
    // printf("completed custom set\n");

    // import(&self, "D:\\Characters\\Information\\College\\GeeVis\\actorActor.tsv");
    // import(&self, "D:\\Characters\\Information\\College\\GeeVis\\nameBasics.tsv");
    // import(&self, "actorActorTruncated.tsv");
    // import(&self, "nameBasicsTruncated.tsv");
    // customCommand(&self);

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