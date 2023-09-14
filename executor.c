/* Sahil Patel, 119012512 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sysexits.h>
#include <err.h>
#include <unistd.h>
#include "command.h"
#include "executor.h"

#define OPEN_FLAGS (O_WRONLY | O_TRUNC | O_CREAT)
#define DEF_MODE 0664

int execute_aux(struct tree *t, int input_fd, int output_fd) {
   /*Handing when conjunction is NONE*/
   if(t->conjunction == NONE) {
      if(strcmp(t->argv[0], "cd") == 0) {
         int success; 
         /*if a directory path is available as an argument*/
         if(t->argv[1]) {
            success = chdir(t->argv[1]);
            if(success == -1) {
               perror(t->argv[1]); 
               fflush(stdout);
               exit(EX_OSERR);
            }
         } else {
            /*by default changes to home directory*/
            success = chdir(getenv("HOME"));
            if(success == -1) {
               perror(getenv("HOME"));
               fflush(stdout);
               exit(EX_OSERR);
            }
         }
      /*Checks if the command is exit*/
      } else if(strcmp(t->argv[0], "exit") == 0) {
         exit(EXIT_SUCCESS);
      } else { /*Every other command will be forked*/
         pid_t result = fork();
         int status; 
         /*Checking if fork was successful*/
         if(result == -1) {
            perror("fork");
            exit(EX_OSERR);
         }
         /*Child code*/
         if(result == 0) {
            /*if using input redirection*/
            if(t->input) {
               /*then open that file and use dup2 */
               /*to redirect so it is used as std input*/
               if((input_fd = open(t->input, O_RDONLY)) < 0) {
		            perror("input error");  
		         }
               if(dup2(input_fd, STDIN_FILENO) < 0) {
                  perror("dup2 input");
               }
               /*closing redundant file descriptor*/
               close(input_fd);
            }
            /*if using output redirection*/
            if(t->output) {
               /*then open that file and use dup2 */
               /*to redirect so it is used as std output*/
               if((output_fd = open(t->output, OPEN_FLAGS, DEF_MODE)) < 0) {
                  perror("output error");
               }
               if(dup2(output_fd, STDOUT_FILENO) < 0) {
                  perror("dup2 output");
               }
               /*closing redundant file descriptor*/
               close(output_fd);
            }            
            /*Execute program from argv*/
            execvp(t->argv[0], t->argv);
            fprintf(stderr, "Failed to execute %s", t->argv[0]);
            fflush(stdout);
            exit(EX_OSERR);
         } else { /*Parent/Caller code*/
            wait(&status);
            /*Checking if the child exited normally*/
            if(WIFEXITED(status)) {
               /*Checking if the child was successful in its function*/
               if(WEXITSTATUS(status) == 0) {
                  return 0;
               }
            }
            /*Returns fail if child process failed or returned unsuccessfully*/
            return 1; 
         }
      }
   } else if(t->conjunction == PIPE) {
      int pipe_fd[2]; /*for the pipe*/
      pid_t result; /*using to identify between child & parent processes*/

      /*Checking if there is an ambiguous output/input which is not allowed*/
      if(t->left->output && t->right->input) {
         printf("Ambiguous output redirect.\n");
      } else if(t->left->output) {
         printf("Ambiguous output redirect.\n");
      } else if(t->right->input) {
         printf("Ambiguous input redirect.\n");
      } else {
         /*if using input redirection*/
         if(t->input) {
            /*then open that file and use dup2 */
            /*to redirect so it is used as std input*/   
            if((input_fd = open(t->input, O_RDONLY)) < 0) {
		         perror("input error");  
		      }
         }
         /*if using output redirection*/
         if(t->output) {
            /*then open that file and use dup2 */
            /*to redirect so it is used as std output*/   
            if((output_fd = open(t->output, OPEN_FLAGS, DEF_MODE)) < 0) {
               perror("output error");
            }
         }
         /*Checking if the pipe or the forked failed*/
         if(pipe(pipe_fd) == -1 || (result = fork()) == -1) {
            perror("pipe or fork");
            exit(EX_OSERR);
         }      
         /*Parent code*/
         if(result) {
            close(pipe_fd[1]); /*don't need the write fd for parent*/
            /*Checking if dup2 failed*/
            if(dup2(pipe_fd[0], STDIN_FILENO) < 0) {
               perror("dup2 in");
            }
            /*Executing right tree with pipe*/
            execute_aux(t->right, pipe_fd[0], output_fd);
            close(pipe_fd[0]); /*closing redundant fd*/
            wait(NULL); /*waiting & repeaing child*/
         } else if(!result) { /*Child code*/
            close(pipe_fd[0]); /*don't need the read fd for child*/
            /*Checking if dup2 failed*/
            if(dup2(pipe_fd[1], STDOUT_FILENO) < 0) {
               perror("dup2 out");
            }
            /*Executing left tree with pipe*/
            execute_aux(t->left, input_fd, pipe_fd[1]);
            close(pipe_fd[1]); /*closing redundant fd*/
            exit(EXIT_SUCCESS); 
         } 
         
      }
   } else if(t->conjunction == AND) {
      /*Checking if using input and/or output redirection*/
      if(t->input) {
         if((input_fd = open(t->input, O_RDONLY)) < 0) {
		      perror("input error");  
		   }
      }
      if(t->output) {
         if((output_fd = open(t->output, OPEN_FLAGS, DEF_MODE)) < 0) {
            perror("output error");
         }
      } 
      /*Recursively call the left tree and right tree for the AND conjunction*/
      /*Treats the left half and right half separtely */
      /*but only executes right tree if left is successful*/
      if(execute_aux(t->left, input_fd, output_fd) == 0) {
         if(execute_aux(t->right, input_fd, output_fd) != 0) {
            return EXIT_FAILURE;
         } 
      }
   } else if(t->conjunction == SUBSHELL) {
      int result;
      /*Checking if using input and/or output redirection*/
      if(t->input) {
         if((input_fd = open(t->input, O_RDONLY)) < 0) {
		      perror("input error");  
		   }
      }
      if(t->output) {
         if((output_fd = open(t->output, OPEN_FLAGS, DEF_MODE)) < 0) {
            perror("output error");
         }
      } 
      /*Checking if the fork was successful*/
      if((result = fork()) == -1) {
         perror("forkkkkkk");
      } else {
         if(result) { /*Parent code (if result > 0)*/
            wait(NULL);
         } else if(!result){ /*Child code (if result is 0)*/
            /*Executing left subtree in the subshell in its own enviroment/process*/
            execute_aux(t->left, input_fd, output_fd);
            exit(EXIT_SUCCESS);
         }
      }
   }
   return 0;
}

int execute(struct tree *t) {
   if(t == NULL) { /*Base case*/
      return 0;
   }
   return execute_aux(t, STDIN_FILENO, STDOUT_FILENO);
}

