//EYAL STYSKIN 206264749
#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#define SYSERR "Error in system call"

/*
 * read full content file
 */
char *readAllFile(int fd) {
    long size;
    if ((size = lseek(fd, 0, SEEK_END)) == -1) //search for end of file
        return NULL;

    char *content = (char *) malloc((size_t) size + 1);
    if ((lseek(fd, 0, SEEK_SET)) == -1)  //set the file pointer back to the beginning
        return NULL;

    if (read(fd, content, (size_t) size) == -1) { //read 'size'(all file) chars when each char 1 byte from f;
        return NULL;
    }
    return content;
}

//a function to remove all whitespaces from a string
void removeSpaces(char *s) {
    char *i = s;
    char *j = s;
    while (*j != 0) {
        *i = *j++;
        if ((*i != ' ') && (*i != '\n'))
            i++;
    }
    *i = 0;
}
//set char to lower case
void lowerCase(char *s) {
    char *letter = s;
    while (*letter != 0) {
        *letter = (char) tolower(*letter);
        letter++;
    }
}
//free cllocated resources and close given files
void finish(int fA,int fB, char* a, char* b) {
    free(a);
    free(b);
    close(fA);
    close(fB);
}


int main(int argc, char **argv) {
    char *pathA = argv[1];
    char *pathB = argv[2];
    int fA, fB; //open first given file
    if ((fA = open(pathA, O_RDONLY)) == -1) {
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    }   //open the 2nd given file
    if ((fB = open(pathB, O_RDONLY)) == -1) {
        close(fA);
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    }
    //Read the whole file content
    char *contentA = NULL;
    char *contentB = NULL;
    if ((contentA = readAllFile(fA)) == NULL) {
        free(contentA);
        close(fA);
        close(fB);
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    };
    if ((contentB = readAllFile(fB)) == NULL) {
        free(contentA);
        free(contentB);
        close(fA);
        close(fB);
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    };

    if (!strcmp(contentA, contentB)) { //check if both file content EQUALS
        finish(fA,fB,contentA,contentB);

        return 1;
    }
    removeSpaces(contentA);
    removeSpaces(contentB);
    lowerCase(contentA);
    lowerCase(contentB);

    if (!strcmp(contentA, contentB)) {//check if both file content SIMILAR
        finish(fA,fB,contentA,contentB);

        return 3;
    }
    finish(fA,fB,contentA,contentB);
    return 2;
}
