/* CS111 Winter 2016 Lab1a
See README for further information
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>

#define _GNU_SOURCE

/*********************************************************************************
TO DO LIST

difficult options:
- abort         // crash the shell. The shell itself should immediately dump core
                  via a segmentation violation
- catch N       // catch signal N, where N is a decimal integer, with a handler
                  that outputs the diagnostic "N caught" to stderr, and exits with
                  status N. This exits the entire shell. N uses the same numbering as
                  your system; for example, on GNU/Linux, a segmentation violation is
                  signal 11.
- ignore N      // ignore signal N
- default N     // use default behavior for signal N
- pause         // waiting for signal to arrive

ERROR CHECKING
- test close
- pipe ends called in wrong order? runs correctly, should it print an error?
- creat rdonly and wronly both create rdonlys
******************************************************************************/

// holds the indices for start and end of a command in argv,
  // so they can be printed out after wait. 
struct cmd_info {
  int cmd_start;
  int cmd_end;
  pid_t pid;
};


// Check if a file descriptor is valid
int validFd(int fd, int fd_array_cur){
	if( fd >= fd_array_cur){	
  		fprintf(stderr, "Error: Invalid use of file descriptor %d before initiation.\n", fd);
  		return 0;
  	}
  	return 1;
}

// Check if the open system call had an error
int checkOpenError(int fd) {
	if (fd == -1) {
		fprintf(stderr, "Error: open returned unsuccessfully\n");
    return -1;
  }
  return 0;
}

int isDigit(char* str) {
  int i = 0;
  while (str != NULL && *(str+i) != '\0') {
    if (!isdigit(*(str+i))) {
      return 0;
    }
    i++;
  }
  return 1;
}

// Checks for --command arguments
int passChecks(char* str, int index, int num_args) {
  // checks if is a digit
  if (!isDigit(str)) {
    fprintf(stderr, "Error: Incorrect usage of --command. Requires integer argument.\n");
    return 0;
  }
  // checks if is within number of arguments
  if (index >= num_args) {
    fprintf(stderr, "Error: Invalid number of arguments for --command\n");
    return 0;
  }
  return 1;
}

// Stores all arguments for a command in args_array, until the next "--" flag
// Doesn't include optarg which is accepted automatically.
// Keeps the current index updated and returns the updated argv index (optind)
int findArgs(char** args_array, size_t args_array_size, 
              int index, int* args_current,
              int argc, char** argv) {
  
  int args_array_cur = *args_current;
  //store arguments of the command into an array of char**
  while(index < argc){
    //break the loop if the index reaches the next "--"option
    if(argv[index][0] == '-' && argv[index][1] == '-') {
      break;
    }
      
    //now this must be an argument for the command. Store it into args array
    //realloc: same mechanics as fd_array
    if(args_array_cur == args_array_size){
      args_array_size *= 2;
      args_array = (char**)realloc((void*)args_array, args_array_size*sizeof(char*)); 
    }
    args_array[args_array_cur] = argv[index];
    args_array_cur++;
    index++;
  }
  *args_current = args_array_cur;
  return index;
}

// checks if a logical file descriptor is a pipe
int isPipe(int fd, int * pipes, int size_of_pipes_arr) {
  for (int j = 0; j < size_of_pipes_arr; j++) {
    if (fd == pipes[j]) return 1;
  }
  return 0;
}

int main(int argc, char **argv) {

  // Array to hold commands and info
  size_t cmd_info_size = 2;
  int cmd_info_cur = 0;
  struct cmd_info * commands = malloc(cmd_info_size*sizeof(struct cmd_info));

  // Array to hold file descriptors
  size_t fd_array_size = 2;
  int fd_array_cur = 0;
  int * fd_array = malloc(fd_array_size*sizeof(int));

  // pipe array of logical file descriptor numbers that are part of a pipe
  size_t num_pipe_fd = 2;
  int pipes_cur = 0;
  int * pipes = malloc(num_pipe_fd*sizeof(int));

  // array of flags when opening a file 
  int fileflags[11] = {0};

  // Verbose can be on or off, automatically set to off
  int verbose = 0;

  // will be updated as described in the spec
  int exit_status = 0;

  // Parse and handle options
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
    	// { "name",      has_arg,         *flag, val }
        {"rdonly",      required_argument,  0,  'r' }, 
        {"wronly",      required_argument,  0,  'w' }, 
        {"command",     required_argument,  0,  'c' },
        {"verbose",     no_argument,        0,  'v' },
        {"close",       required_argument,  0,  'o' },
      
        {"wait",        no_argument,        0,  'z' },
        {"append",      no_argument,        0,  'a' }, // fileflags[0]
        {"cloexec",     no_argument,        0,  'l' }, // fileflags[1]
        {"creat",       no_argument,        0,  't' }, // fileflags[2]
        {"directory",   no_argument,        0,  'd' }, // fileflags[3]
        {"dsync",       no_argument,        0,  'y' }, // fileflags[4]
        {"excl",        no_argument,        0,  'x' }, // fileflags[5]
        {"nofollow",    no_argument,        0,  'n' }, // fileflags[6]
        {"nonblock",    no_argument,        0,  'b' }, // fileflags[7]
        {"rsync",       no_argument,        0,  'e' }, // fileflags[8]
        {"sync",        no_argument,        0,  's' }, // fileflags[9]
        {"trunc",       no_argument,        0,  'u' },  // fileflags[10]
        {"rdwr",        no_argument,        0,  'g' },  
        {"pipe",        no_argument,        0,  'p'}

    };

    // get the next option
    int c = getopt_long(argc, argv, "", long_options, &option_index);
    
    // break when there are no further options to parse
    if (c == -1)
      break;

    // args_array will store flag and its arguments
    size_t args_array_size = 2; 
    char** args_array = malloc(args_array_size*sizeof(char*)); //command argument(s)
    int args_array_cur = 0;    //current index for the above array  

    switch (c) {
    
    case 'a': // append fileflags[0]
      if (verbose) { printf("--append "); }
      fileflags[0] = O_APPEND;
      break;
    case 'l': //cloexec fileflags[1]
      if (verbose) { printf("--cloexec "); }
      fileflags[1] = O_CLOEXEC;
      break;
    case 't': //creat fileflags[2]
      if (verbose) { printf("--creat "); }
      fileflags[2] = O_CREAT;
      break;
    case 'd': // directory fileflags[3]
      if (verbose) { printf("--directory "); }
      fileflags[3] = O_DIRECTORY;
      break;
    case 'y': // dsync fileflags[4]
      if (verbose) { printf("--dsync "); }
      fileflags[4] = O_DSYNC;
      break;
    case 'x': //excl fileflags[5]
      if (verbose) { printf("--excl "); }
      fileflags[5] = O_EXCL;
      break;
    case 'n': // nofollow fileflags[6]
      if (verbose) { printf("--nofollow "); }
      fileflags[6] = O_NOFOLLOW;
      break;
    case 'b': //nonblock fileflags[7]
      if (verbose) { printf("--nonblock "); }
      fileflags[7] = O_NONBLOCK;
      break;
    case 'e': //rsync fileflags[8]
      if (verbose) { printf("--rsync "); }
      fileflags[8] = O_RSYNC;
      break;
    case 's': //sync fileflags[9]
      if (verbose) { printf("--sync "); }
      fileflags[9] = O_SYNC;
      break;
    case 'u': //trunc fileflags[10]
      if (verbose) { printf("--trunc "); }
      fileflags[10] = O_TRUNC;
      break;

    case 'r': // read only 
    case 'w': // write only
    case 'g': { //read and write
      
      // open flag holds all open flags
      int oflag;
   		
      // assign read only, write only, or both
      if (c == 'r') 	oflag = O_RDONLY;
   		else if (c == 'w')		oflag = O_WRONLY;
      else  oflag = O_RDWR;

      //oflag argument of open is a bitwise "or" of all O_flags, which we've saved in fileflags.
      oflag = oflag | fileflags[0] | fileflags[1] | fileflags[2] | fileflags[3] | fileflags[4] | fileflags[5]
              | fileflags[6] | fileflags[7] | fileflags[8] | fileflags[9] | fileflags[10];

      // find all arguments before the next --. These will be printed if
              // verbose is enabled, and otherwise will be ignored.
      optind = findArgs(args_array, args_array_size, optind, &args_array_cur,
                        argc, argv);

      // print command if verbose is enabled
      if (verbose) {
        char * flags;
        if (c == 'r') flags = "--rdonly";
        else if (c == 'w') flags = "--wronly";
        else flags = "--rdwr";
        printf("%s ", flags);
        printf("%s ", optarg);
        int j;
        for (j = 0; j < args_array_cur; j++) {
          printf("%s ", args_array[j]);
        }
        printf("\n");
      }
      
      // open file into read write file descriptor
      int rw_fd = open(optarg, oflag, 777);
      if(checkOpenError(rw_fd) == -1) {
        exit_status = 1;
        continue;
      }
      
      // save file descriptor to array
      if (fd_array_cur == fd_array_size) {
      	fd_array_size *= 2;
      	fd_array = (int*)realloc((void*)fd_array, fd_array_size); 
      }
      fd_array[fd_array_cur] = rw_fd;
      fd_array_cur++;

      // reset fileflags
      int k;
      for (k = 0; k < 11; k++) { fileflags[k] = 0;}
      
      break;
    }

    case 'o': // close N (closes file descriptor N)
      if (verbose) { printf("--close %s\n", optarg); }
      if (isDigit(optarg)) {
        int i = atoi(optarg);
        if (i >= fd_array_cur)
          fprintf(stderr, "Error: Bad argument for --close. N must be a valid file descriptor.\n");
        if (fcntl(fd_array[i], F_GETFD) != -1 || errno != EBADF)
          close(fd_array[i]);
      }
      else {
        fprintf(stderr, "Error: Incorrect usage of --close. Requires integer argument.\n");
      }
      break;

    case 'c': { // command (format: --command i o e cmd args_array)
      int i, o, e; // stdin, stdout, stderr

      //store the file descripter numbers and check for errors
      if (!passChecks(optarg, optind, argc)) { break; }
      i = atoi(optarg);
      if (!passChecks(argv[optind], optind, argc)) { break; }
      o = atoi(argv[optind]); optind++;
      if (!passChecks(argv[optind], optind, argc)) { break; }
      e = atoi(argv[optind]); optind++;

      // check if there is the proper number of arguments
      if (optind >= argc) {
        fprintf(stderr, "Error: Invalid number of arguments for --command\n");
        break;
      }

      // save command in command_info struct
      if (cmd_info_cur == cmd_info_size) {
        cmd_info_size *= 2;
        commands = (struct cmd_info *)realloc((void *)commands, cmd_info_size*sizeof(struct cmd_info));
      }
      commands[cmd_info_cur].cmd_start = optind;

      // save command into args array
      args_array[0] = argv[optind]; optind++;
      args_array_cur++;

      // find arguments for command
      optind = findArgs(args_array, args_array_size, optind, &args_array_cur,
                        argc, argv);

      commands[cmd_info_cur].cmd_end = optind;
      
      //append NULL to args_array (necessary for execvp())
      if(args_array_cur == args_array_size){
          args_array_size++;
          args_array = (char**)realloc((void*)args_array, args_array_size*sizeof(char*)); 
      }
      args_array[args_array_cur] = NULL;
      args_array_cur++;

      // print if verbose
      if (verbose == 1) {
        printf("--command %d %d %d ", i,o,e);
        int j;
        for (j = 0; j < args_array_cur-1; j++) {
          printf("%s ", args_array[j]);
        }
        printf("\n");
      }

      //check if i,o,e fd are valid 
      if(!(validFd(i,fd_array_cur) && validFd(o,fd_array_cur) && validFd(e,fd_array_cur)))  
        continue;

      // fork to execute command
      pid_t pid = fork();

      if (pid == 0) {   //child process
        // close unused pipes
        if (isPipe(i, pipes, pipes_cur)) {
          if (isPipe(i+1, pipes, pipes_cur)) { 
            if (fcntl(fd_array[i+1], F_GETFD) != -1 || errno != EBADF)
              close(fd_array[i+1]); 
          } 
          // else error handling if input isn't from read end of pipe
        }
        if (isPipe(o, pipes, pipes_cur)) {
          if (isPipe(o-1, pipes, pipes_cur)) { 
            if (fcntl(fd_array[o-1], F_GETFD) != -1 || errno != EBADF)
              close(fd_array[o-1]); 
          } 
          // else error handling if output isn't from write end of pipe
        }
        if (isPipe(e, pipes, pipes_cur)) {
          if (isPipe(e-1, pipes, pipes_cur)) { 
            if (fcntl(fd_array[e-1], F_GETFD) != -1 || errno != EBADF)
              close(fd_array[e-1]); 
          } 
          // else error handling if output isn't from write end of pipe
        }

        //redirect stdin to i, stdout to o, stderr to e
        dup2(fd_array[i], 0);
        dup2(fd_array[o], 1);
        dup2(fd_array[e], 2);

        // Close all used file descriptors
        // fd_array_cur--;
        // while (fd_array_cur >= 0) {
        //   close(fd_array[fd_array_cur]);
        //   fd_array_cur--;
        // }
        
        // execute process
        execvp(args_array[0], args_array);
        //return to main program if execvp fails
        fprintf(stderr, "Error: Unknown command '%s'\n", args_array[0]);
        exit(255);  
      }
      // parent process moves to next command after saving this one
      commands[cmd_info_cur].pid = pid;
      cmd_info_cur++;
      break;
    }

    case 'p': { // pipe
      if (verbose) { printf("--pipe\n"); }

      int fd[2];
      int i;

      int val = pipe(fd);
      if (val < 0) {
        fprintf(stderr, "Error: pipe could not be opened\n");
        exit_status = 1;
        continue;
      }

      // save file descriptors to fd and pipe array
      for (i = 0; i < 2; i++) {
        if (fd_array_cur == fd_array_size) {
          fd_array_size *= 2;
          fd_array = (int*)realloc((void*)fd_array, fd_array_size); 
        }
        fd_array[fd_array_cur] = fd[i];
        if (pipes_cur == num_pipe_fd) {
          num_pipe_fd *= 2;
          pipes = (int *)realloc((void *) pipes, num_pipe_fd);
        }
        pipes[pipes_cur] = fd_array_cur;
        pipes_cur++;
        fd_array_cur++;
      }
      break;
    }
     
    case 'v': // verbose
      verbose = 1;
      break;
      
    case 'z':  { // wait
      if (verbose) { printf("--wait\n"); }
      int status;
      pid_t returnedPid;
      int waitStatus;

      // Close all used file descriptors
      fd_array_cur--;
      while (fd_array_cur >= 0) {
        if (fcntl(fd_array[fd_array_cur], F_GETFD) != -1 || errno != EBADF)
          close(fd_array[fd_array_cur]);
        fd_array_cur--;
      }

      while (1) {
        //wait for any child process to finish. 0 is for blocking.
        returnedPid = waitpid(-1, &status, 0);
        
        //WEXITSTATUS returns the exit status of the child.
        waitStatus = WEXITSTATUS(status);

        // break if no remaining processes to wait for
        if (returnedPid == -1) { break; }

        // sets exit status to the maximum of all exit statuses
        if (waitStatus > exit_status) {
          exit_status = waitStatus;
        }

        // print the exit status and arguments of exited process
        printf("%d ", waitStatus);
        int j = 0;
        int i;
        while(j < cmd_info_cur) {
          if (commands[j].pid == returnedPid) {
            for (i = commands[j].cmd_start; i < commands[j].cmd_end; i++) {
              printf("%s ", argv[i]);
            }
            break;              
          } else { j++; }
        }
        printf("\n");
      }
      break;
    }
     
    case '?': // ? returns when doesn't recognize option character
    default:
        fprintf(stderr, "Error: ?? getopt returned character code 0%o ??\n", c);
    }
    // Free arguments array for next command
    free(args_array);
  }

  // Prints out extra options that weren't parsed
 // if (optind < argc) {
 //      printf("non-option ARGV-elements: ");
 //      while (optind < argc)
 //          printf("%s ", argv[optind++]);
 //      printf("\n");
 //  }


  // Close all used file descriptors
  fd_array_cur--;
  while (fd_array_cur >= 0) {
    if (fcntl(fd_array[fd_array_cur], F_GETFD) != -1 || errno != EBADF)
      close(fd_array[fd_array_cur]);
  	fd_array_cur--;
  }

  // Free file descriptor, pipes, and command arrays
  free(fd_array);
  free(pipes);
  free(commands);

  // Exit with previously set status
  exit(exit_status);
}
