//EYAL STYSKIN 206264749
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

#define SYSERR "Error in system call"
#define LINENUM 3
#define LINESIZE 150
#define NO_C "NO_C_FILE\n"
#define COMP_ERR "COMPILATION_ERROR\n"
#define TO "TIMEOUT\n"
#define GJ "GREAT_JOB\n"
#define BO "BAD_OUTPUT\n"
#define SO "SIMILAR_OUTPUT\n"

//given file descriptor, the function reads its content
char *readAllFile(int fd) {
    long size;
    if ((size = lseek(fd, 0, SEEK_END)) == -1) { //search for end of file
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    }

    char *content = (char *) malloc((size_t) size + 1);
    if ((lseek(fd, 0, SEEK_SET)) == -1) { //set the file pointer back to the beginning
        free(content);
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    }

    if (read(fd, content, (size_t) size) == -1) { //read 'size'(all file) chars when each char 1 byte from f;
        free(content);
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    }
    return content;
}
/* given file descriptor, the function reads the 3 line configuration file,
and feels "content" with them*/
void getLines(int fd, char content[LINENUM][LINESIZE]) {
    char *allFile = readAllFile(fd);
    char delim = '\n';
    strcpy(content[0], strtok(allFile, &delim));
    int i;
    for (i = 1; i < 3; ++i) {
        strcpy(content[i], strtok(NULL, &delim));
    }
    free(allFile); //free the dynamically allocated content
}
// check if the given path is a directory (".." and "." does not count)
bool isDir(const char *target) {
    if ((!strcmp(target, "..")) || (!strcmp(target, "."))) {
        return false;
    }
    struct stat statbuf;
    stat(target, &statbuf);
    return S_ISDIR(statbuf.st_mode);
}

// check if the given file name ends with ".c"
bool isCFile(const char *name) {
    int len = (int) strlen(name);
    char subset[2];
    memcpy(subset, &name[len - 2], 2);
    if (!strcmp(subset, ".c"))
        return true;
    return false;
}

/*Compiling the .c File, in case of fork() err function returns -1,
 * otherwise it returns 1 */
int compileFile(char *fName) {
    pid_t pid;
    char *args[3];
    args[0] = "gcc";
    args[1] = fName;
    args[2] = NULL;
    if ((pid = fork()) < 0) { //fork new process
        return -1; //if failed forking
    } else if (pid == 0) { // if in child
        execvp(args[0], args);
        exit(0); //exit child - shouldn't reach here;
    }
    waitpid(pid, NULL, 0); //wait for son process to finish
    return 1;
}

/* function to return the name of the C file, if error occured - NULL is returned,
 * if no C file found, NO_C will be returned, OW, the name. this function is a
 * recursive function, if inside the directory there are more dirs, it will search
 * it also with DFS*/
char *getCFile(DIR *loc) {
    struct dirent *item = NULL;
    DIR *newDir = NULL;
    //LOOP ALL ITEMS in current Directory
    while ((item = readdir(loc)) != NULL) {
        if (isCFile(item->d_name)) { //if this is a C file return its name
            return item->d_name;
        } else if (isDir(item->d_name)) { //if it is a directory
            if ((newDir = opendir(item->d_name)) == NULL) {  //OPEN the NEW directory;
                return NULL;
            }
            if ((chdir(item->d_name)) == -1) { //ENTER the NEW directory
                closedir(newDir);
                return NULL;
            }
            char *cName;
            if ((cName = getCFile(newDir)) == NULL) { //if error occurred
                closedir(newDir);
                return NULL;
            }
            if (strcmp(cName, NO_C) != 0) { // if found c file
                closedir(newDir);
                return cName;
            } else {     //if didnt found a c file in the directory
                closedir(newDir);
                if ((chdir("..")) == -1) { //return to last directory
                    closedir(newDir);
                    return NULL;
                }
            }
        }
    }
    return NO_C;
}

/*build the string for the results file, by a
 * given grade and student's file name.
 * IF THE "WRITE" SYSCALL FAILS = RETURN -1*/
int writeRes(char *fName, int fd, int grade) {
    char buffer[200] = "";
    strcpy(buffer, fName);
    switch (grade) {
        case 0:
            strcat(buffer, ",0,");
            strcat(buffer, NO_C);
            if ((write(fd, buffer, strlen(buffer))) < 0) {
                return -1;
            }
            break;
        case 20:
            strcat(buffer, ",20,");
            strcat(buffer, COMP_ERR);
            if ((write(fd, buffer, strlen(buffer))) < 0) {
                return -1;
            }
            break;
        case 40:
            strcat(buffer, ",40,");
            strcat(buffer, TO);
            if ((write(fd, buffer, strlen(buffer))) < 0) {
                return -1;
            }
            break;
        case 60:
            strcat(buffer, ",60,");
            strcat(buffer, BO);
            if ((write(fd, buffer, strlen(buffer))) < 0) {
                return -1;
            }
            break;
        case 80:
            strcat(buffer, ",80,");
            strcat(buffer, SO);
            if ((write(fd, buffer, strlen(buffer))) < 0) {
                return -1;
            }
            break;
        default:
            strcat(buffer, ",100,");
            strcat(buffer, GJ);
            if ((write(fd, buffer, strlen(buffer))) < 0) {
                return -1;
            }
    }
    return 1;
}

/*function to run the a.out file created from the C file of the student.
 * inputF = the given input file in configuration
 * outputF = the temporary file created to store the output of the student
 * program. the function returns 0 in case of a timeout, -1 in case of fork()
 * ERROR and 1 OW */
int runProgram(int inputF, int outputF) {
    pid_t pid;
    char *args[2];
    args[0] = "./a.out";
    args[1] = NULL;
    if ((pid = fork()) < 0) { //fork new process
        return -1;
    } else if (pid == 0) { // if in child
        dup2(inputF, 0); //set std input to be the input file
        dup2(outputF, 1); //set std output to be the output file

        execvp(args[0], args);
        exit(0); //exit child - shouldn't reach here;
    } else { //for main process
        int i;
        for(i=0;i<5; ++i) {
            sleep(1);
            int status = waitpid(pid, NULL, WNOHANG);
            if (status == pid) {    //check if process finished
                return 1;
            }
        }
        return 0;   //process did not finish during 5 seconds = TIMEOUT
    }
}

/* this function, runs the Comp.out program that located in the starting
 * program directory (the path given as parameter), and gives the comp.out
 * two paths as parameters to be compared. if 1 returned - grade of 100 given,
 * if 2- returned - grade of 60 is given, if 3 returned - grade of 80 given*/
int compareResults(char *programFolder, char *test, char *correct) {
    pid_t pid;
    int stat;
    int retVal;
    char *args[4];

    char *prev = NULL;
    prev = getcwd(prev, 0); //save last location
    if (prev == NULL)
        return -1;
    //return to starting folder (where comp.out located)
    if ((chdir(programFolder)) == -1) {
        return -1;
    }
    args[0] = "./comp.out";
    args[1] = test;
    args[2] = correct;
    args[3] = NULL;

    if ((pid = fork()) < 0) { //fork new process
        return -1;
    } else if (pid == 0) { // if in child
        execvp(args[0], args); //run the comp program
        exit(7); //exit child - shouldn't reach here;
    } else { //for main process
        waitpid(pid,&stat,0);
        retVal = WEXITSTATUS(stat);
    }
    if ((chdir(prev)) == -1) {
        free(prev);
        return -1;
    }
    chdir(prev);    //return to last located directory
    switch (retVal) {
        case 1:
            return 100;
        case 2:
            return 60;
        case 3:
            return 80;
        default:
            return -1;
    }
}
//free opened files and allocated resources, and write err to STDERR
void freeAndClose(int resFD, DIR *currDir, DIR *mainDir) {
    close(resFD);   //results file
    closedir(currDir);   //CLOSE current Directory
    closedir(mainDir); // CLOSE main Directory
    write(2, SYSERR, sizeof(SYSERR) - 1);
    exit(-1);
}


int main(int argc, char **argv) {
    int conFD;  //open the configuration file
    if ((conFD = open(argv[1], O_RDONLY)) == -1) {
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    }
    char config[3][150];
    getLines(conFD, config);    //get the three lines from the conf file
    close(conFD);
    int resFD;//open new file for writing results and get it file descriptor
    if ((resFD = open("results.csv", O_WRONLY | O_APPEND | O_CREAT, 0777)) == -1) {
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    }

    char programFolder[150];    //save the starting dir location of the program
    getcwd(programFolder, sizeof(programFolder));

    //OPEN MAIN DIRECTORY
    DIR *mainDir;
    if ((mainDir = opendir(config[0])) == NULL) {
        close(resFD);
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    }
    //ENTER MAIN DIRECTORY
    if ((chdir(config[0])) == -1) {
        close(resFD);
        closedir(mainDir);
        write(2, SYSERR, sizeof(SYSERR) - 1);
        exit(-1);
    };

    struct dirent *newDirent = NULL;
    // looping through the MAIN directory
    while ((newDirent = readdir(mainDir)) != NULL) {
        if ((!strcmp(newDirent->d_name, "..")) || (!strcmp(newDirent->d_name, "."))) {
            continue;
        }
        DIR *currDir = NULL;
         if ((currDir = opendir(newDirent->d_name)) == NULL) { //OPEN student's directory;
            close(resFD);
            closedir(mainDir); //close main dir
            write(2, SYSERR, sizeof(SYSERR) - 1);
            exit(-1);
        }
        if ((chdir(newDirent->d_name)) == -1) { //ENTER the NEW directory
            freeAndClose(resFD, currDir, mainDir);
        };

        char *cName; //will be: NULL if err, NO_C if not found, otherwise found
        if ((cName = getCFile(currDir)) == NULL) {  //IN CASE FOUND ERROR
            freeAndClose(resFD, currDir, mainDir);
        }

        else if (!strcmp(cName, NO_C)) { //if NO C File FOUND
            int status = writeRes(newDirent->d_name, resFD, 0); //write 0 result
            if (status == -1) { //in case "write()" syscall Failed
                freeAndClose(resFD, currDir, mainDir);
            }
        }

        else {    //Case that : NO ERRORS- and C FILE WAS FOUND
            //First - compile the file
            if ((compileFile(cName)) == -1) { //-1 in case of "fork()" syscall err
                freeAndClose(resFD, currDir, mainDir);
            }

            //Second - check if compilation worked
            if ((access("a.out", F_OK)) == -1) { //if Compilation failed (no a.out file)
                int status = writeRes(newDirent->d_name, resFD, 20); //COMPILE ERROR = grade 20
                if (status == -1) {
                    freeAndClose(resFD, currDir, mainDir);
                }
            } else {    //Compilation worked ! Calculate Grade:
                int inputFD, outputFD;
                if ((inputFD = open(config[1], O_RDONLY)) == -1) { //open input file
                    unlink("a.out");
                    freeAndClose(resFD, currDir, mainDir);
                }//create NEW output file called: outputFD.txt
                if ((outputFD = open("outputFD.txt", O_WRONLY | O_CREAT, 0777)) == -1) {
                    unlink("a.out");
                    close(inputFD);
                    freeAndClose(resFD, currDir, mainDir);
                }

                int indicator;//will be 0 if Timeout occurred, -1 if fork() ERR, and 1 OW
                if ((indicator = runProgram(inputFD, outputFD)) == -1) {//-1 for "fork()" ERR
                    close(inputFD);
                    unlink("a.out");
                    unlink("outputFD.txt");
                    freeAndClose(resFD, currDir, mainDir);
                }
                if (!indicator) {  // 0 if Timeout occurred
                    int status = writeRes(newDirent->d_name, resFD, 40); //TIMEOUT = 40
                    if (status == -1) { //check if "Write()" SYSCALL FAILED
                        close(inputFD);
                        unlink("a.out");
                        unlink("outputFD.txt");
                        freeAndClose(resFD, currDir, mainDir);
                    }
                } else {
                    char *outputPath = NULL;   //get Absolute path of the new created output file
                    if ((outputPath = realpath("outputFD.txt", NULL)) == NULL) {
                        close(inputFD);
                        unlink("a.out");
                        unlink("outputFD.txt");
                        freeAndClose(resFD, currDir, mainDir);
                    }

                    int grade;  //Compare student's result with Correct result and get grade!
                    if ((grade= compareResults(programFolder, outputPath,config[2])) == -1) {
                        close(inputFD);
                        unlink("a.out");
                        unlink("outputFD.txt");
                        free(outputPath);
                        freeAndClose(resFD, currDir, mainDir);
                    }   //Write the grade to the results file
                    int status = writeRes(newDirent->d_name, resFD, grade);
                    if (status == -1) {
                        close(inputFD);
                        unlink("a.out");
                        unlink("outputFD.txt");
                        free(outputPath);
                        freeAndClose(resFD, currDir, mainDir);
                    }
                    free(outputPath);
                }

                close(inputFD); //close the file descriptor of the input
                unlink("outputFD.txt"); //delete the temporary created output file
                unlink("a.out");
            }
        }
        //Return to the main Dir - Normal to continue looping
        if ((chdir(config[0])) == -1) {
            closedir(mainDir);
            write(2, SYSERR, sizeof(SYSERR) - 1);
            exit(-1);
        }
        closedir(currDir);
    }
    close(resFD);
    closedir(mainDir);

    return 0;
}
