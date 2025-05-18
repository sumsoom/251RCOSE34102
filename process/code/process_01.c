#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>


static bool young = true;

int main(int argc, char* argv[]){
    // Allocated in stack segment.
    bool wild = true;
    
    pid_t pid;

    switch(pid = fork()){
        case 0: //child
            // HINT: The child process should fall into this scope.
            break;
        case -1:
            printf("Something went awfully wrong");
            return -1;
        default:
            // HINT: The parent process should fall into this scope.
            young = false;
            wild = false;
            break;
    }

    printf("[%ld] (%s) I am %s and %s.\n", (long)getpid(), pid == 0 ? "Child" : "Parent", young == true ? "young" : "old", wild == true ? "wild" : "calm");

    return 0;
}


/*
Expected output:

[xxxx] (Parent) I am old and calm.
[yyyy] (Child) I am young and wild.

*/
