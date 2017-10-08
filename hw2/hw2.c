/*
----------------------------
Sajiel Chishti
CSCI4210
Homework 2 - Fork Calculator
----------------------------
Implementation of a LISP type expression
handling calculator.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_LENGTH 128

// Helper method to detect invalid operators
int is_invalid_op(char ch)
{
  return !(ch == '+' || ch == '-' || ch == '/' || ch == '*' || isdigit(ch) || ch == '(' || ch == ')' || ch == ' ');
}

// Helper method to see if a character is an operator
int is_operator(char ch)
{
  return ch == '+' || ch == '-' || ch == '/' || ch == '*';
}

// Helper method for divison checking
int unsafe_division(char* expr)
{
  int i = 0;
  while (i < strlen(expr))
  {
    if (expr[i] == '0' && expr[i+1] == ')' && expr[i-1] == ' ') {
      return 1;
    }
    i++;
  }
  return 0;
}
// Get the number of subexpressions and operands
// ex. + 5 7 (* 7 1 (* 8 2)) should return 2
int num_sub_ops(char* expr)
{
  int i, num_of_ops = 0;
  // Count the number of distinct operations
  // in each expression
  for (i = 0; i < strlen(expr); i++)
  {
    if (isdigit(expr[i]))
    {
        num_of_ops++;
    }
    // Count a sub expression as one operand
    // Make sure that it isn't the first paren
    else if (expr[i] == '(' && i > 0)
    {
      int count_paren = 0;
      // Make sure expression is balanced
      while (count_paren != 0 && i < strlen(expr))
      {
        if (expr[i] == '(')
          count_paren++;
        else if (expr[i] == ')')
          count_paren--;
        i++;
      }

      if (!count_paren) // = 0
        num_of_ops++;
    }
  }
  return num_of_ops;
}

int process_expr(char* expr, int pid_initial, int index_left_paren, int index_right_paren)
{
  int i, sum = 0;
  char operator;
  printf("PID %d: My expression is \"%s\"\n", getpid(), expr);
  if (num_sub_ops(expr) < 2) {
    fprintf(stderr, "PID %d: ERROR: not enough operands; exiting\n", getpid());
    fflush(NULL);
    exit(EXIT_FAILURE);
  }
  for (i = index_left_paren; i < index_right_paren; i++)
   {
    if (is_invalid_op(expr[i]))
    {
      int j = 0;
      char inval_op[MAX_LENGTH];
      // Extract the invalid operator
      while (is_invalid_op(expr[i]))
      {
        inval_op[j] = expr[i];
        i++;
        j++;
      }
      fprintf(stderr, "PID %d: ERROR: unknown \"%s\" operator; exiting\n", getpid(), inval_op);
      fflush(NULL);
      exit(EXIT_FAILURE);
    }
    else if (is_operator(expr[i]) && expr[i+1] == ' ')
    {
        // Keep track of operator
        operator = expr[i];
        printf("PID %d: Starting operation \"%c\"\n", getpid(), expr[i]);
        fflush(NULL);
        if (operator == '/')
        {
          if (unsafe_division(expr))
          {
            fprintf(stderr, "PID %d: ERROR: division by zero is not allowed; exiting\n", getpid());
            fflush(NULL);
            exit(EXIT_FAILURE);
          }
        }
    }
    else if (isdigit(expr[i]))
    {
      int j = 0;
      char temp[MAX_LENGTH];
      if (expr[i-1] == '-')
      {
        temp[j] = '-';
        j++;
      }
      while (isdigit(expr[i]) && i < strlen(expr))
      {
        temp[j] = expr[i];
        i++;
        j++;
      }

      // Null terminate the string
      temp[j] = '\0';
      // Open parent-child communication
      int p[2];
      int rc = pipe(p);
      if (rc == -1) {
        perror("pipe() failed!\n");
        fflush(NULL);
        exit(EXIT_FAILURE);
      }

      // Create a child
      int pid = fork();
      if (pid < 0)
      {
        perror("fork() failed!\n");
        fflush(NULL);
        exit(EXIT_FAILURE);
      }

      if (pid == 0)
      {
        // Close the read end
        close(p[0]);
        // Ensure child is illiterate
        p[0] = -1;
        printf("PID %d: My expression is \"%s\"\n", getpid(), temp);
        fflush(NULL);
        printf("PID %d: Sending \"%s\" on pipe to parent\n", getpid(), temp);
        fflush(NULL);
        write(p[1], temp, strlen(temp));
        close(p[1]);
        exit(EXIT_SUCCESS);
      }
      else if (pid > 0)
      {
        // Wait for the child
        wait(0);
        // Close write end
        close(p[1]);
        p[1] = -1;

        // Create temporary read buffer
        char temp[MAX_LENGTH];
        int bytes_read = read(p[0], temp, MAX_LENGTH);
        temp[bytes_read] = '\0';

        // Increase the running sum
        if (sum == 0)
          sum += atoi(temp);

        else
        {
          // Do a specific operation
          if (operator == '+')
            sum += atoi(temp);
          if (operator == '-')
            sum -= atoi(temp);
          if (operator == '*')
            sum *= atoi(temp);
          if (operator == '/')
            sum /= atoi(temp);
        }
      }
    }
    else if (expr[i] == '(' && i != 0) {
      int start = i;
      char temp[MAX_LENGTH];
      // Get the subexpression
      int count_paren = 0;
      while ((expr[i] != ')')) {
        temp[i-start] = expr[i];
        if (expr[i] == '(')
          count_paren++;
        i++;
      }
      int j = 0;
      while (j < count_paren) {
        temp[i-start+j] = ')';
        j++;
      }
      temp[i-start+j] = '\0';

      // Open parent-child communication
      int p[2];
      int rc = pipe(p);
      if (rc == -1)
      {
        perror("pipe() failed!\n");
        fflush(NULL);
        return EXIT_FAILURE;
      }

      // Create a child
      int pid = fork();
      if (pid < 0)
      {
        perror("fork() failed!\n");
        fflush(NULL);
        return EXIT_FAILURE;
      }
      if (pid == 0) // Child
      {
        // Close p[0]
        close(p[0]);
        // Ensure the child is illiterate
        p[0] = -1;
        // Process the subexpression from start '(' to the final one at location i
        int sub_sum = process_expr(temp, pid_initial, 0, i-start);
        // Don't need to say this if it's a subexpression
        //printf("PID %d: My expression is \"%s\"\n", getpid(), temp);
        printf("PID %d: Processed \"%s\"; sending \"%d\" to parent\n", getpid(), temp, sub_sum);
        fflush(NULL);
        char temp[MAX_LENGTH];
        sprintf(temp, "%d", sub_sum);
        write(p[1], temp, strlen(temp));
        exit(EXIT_SUCCESS);
      }
      if (pid > 0) // Parent
      {
        wait(0);
        // Close write end
        close(p[1]);
        p[1] = -1;
        char temp[MAX_LENGTH];
        int bytes_read = read(p[0], temp, MAX_LENGTH);
        temp[bytes_read] = '\0';

        // Increase the running sum
        if (sum == 0)
          sum += atoi(temp);

        else
        {
          // Do a specific operation
          if (operator == '+')
            sum += atoi(temp);
          if (operator == '-')
            sum -= atoi(temp);
          if (operator == '*')
            sum *= atoi(temp);
          if (operator == '/')
            sum /= atoi(temp);
        }
      }
    }
  }
  return sum;
}

int main(int argc, char** argv)
{
  if (argc == 2)
  {
    int invalid = 1;
    char expr[MAX_LENGTH];
    strcpy(expr, argv[1]);
    FILE* fp = fopen(expr, "r");

    while (fgets(expr, MAX_LENGTH, fp))
    {
      if (expr[0] != '#' || expr[0] == '(')
      {
        invalid = 0;
        break;
      }
    }

    if (invalid)
    {
      fprintf(stderr, "ERROR: No valid expressions parsed from file.\nEXITING\n");
      fflush(NULL);
      return(EXIT_FAILURE);
    }
    // Remove any newline characters
    expr[strcspn(expr, "\n")] = 0;
    int result = process_expr(expr, getpid(), 0, strlen(expr));
    printf("PID %d: Processed \"%s\"; final answer \"%d\"\n", getpid(), expr, result);
    fflush(NULL);
  }
  else
    return EXIT_FAILURE;
}
