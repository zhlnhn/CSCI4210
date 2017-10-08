/*
Sajiel Chishti
---------------------------
Homework 1 - Files, Strings,
and Memory Allocation in C
---------------------------
CSCI4210
09/08/2017
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>

int main(int argc, char *argv[]) {
  // Word counters
  int max_words = -1;
  int c = 0;
  int max_word_size = 16;
  int max_stored_words = 16;
  int total_words = 0;
  int current_unique_word = 1;
  int current_index = 0;

  // File/directory variables
  FILE *fp = NULL;
  struct stat file_type_buf;
  struct stat dir_or_not_buf;
  struct dirent *next_file;

  // Temporary word holder
  char* word;

  // Flags and counters
  int exists_flag = 0;
  int i;
  int rc = 0;

  // Check to make sure a directory was specified
  if (argc == 1) {
    fprintf(stderr, "ERROR: INVALID DIRECTORY\nProgram Terminated\n");
    return EXIT_FAILURE;
  }

  // Check to see if a third argument exists
  // Convert it to an integer regardless of the argument
  if (argc == 3) {
    max_words = atoi(argv[2]);
  }

  // Open the directory specified in the command line
  DIR *directory = opendir(argv[1]);
  fflush(stdout);

  // Get the directory info
  rc = lstat(argv[1], &dir_or_not_buf);

  // Check if lstat failed
  if (rc == -1) {
    fprintf(stderr, "ERROR: lstat() failed.\n");
    return EXIT_FAILURE;
  }

  // Check if this is a directory
  if (S_ISDIR(dir_or_not_buf.st_mode)) {
    // Allocate initial parallel arrays
    char** words_in_files = calloc(max_stored_words, sizeof(char*));
    int* counts = calloc(max_stored_words, sizeof(int));

    printf("Allocated initial parallel arrays of size %d.\n", max_stored_words);
    fflush(stdout);

    // Read next file in directory
    next_file = readdir(directory);
    word = calloc(max_word_size, sizeof(char));

    // Change back to the current working directory
    chdir(argv[1]);

    // Get the file info
    rc = lstat(next_file->d_name, &file_type_buf);

    // Check if lstat worked
    if (rc == -1) {
      fprintf(stderr, "ERROR: lstat() failed.\n");
      return EXIT_FAILURE;
    }

    // Continue until all files are exhausted
    while (next_file != NULL) {
      // Get the file info
      rc = lstat(next_file->d_name, &file_type_buf);

      // Check if lstat failed
      if (rc == -1) {
        fprintf(stderr, "ERROR: lstat() failed.\n");
        return EXIT_FAILURE;
      }

      // Check if it's a regular file
      if (S_ISREG(file_type_buf.st_mode)) {
        if (fp != NULL) {
          // Close the current file
          fclose(fp);
        }
        // Open the new file
        fp = fopen(next_file->d_name, "r");
        if (fp == NULL) {
          fprintf(stderr, "ERROR: Unable to open file.\n");
          fclose(fp);
        }

        // Initial character in the file
        c = fgetc(fp);
        while (c != EOF) {
          while (isalnum((char)c)) {

            // Increase space if word exceeds max_word_size
            if (current_index+1 >= max_word_size) {
              max_word_size += 16;
              word = realloc(word, max_word_size*sizeof(char)+1);
              if (word == NULL) {
                fprintf(stderr, "ERROR: not enough space to reallocate.\n");
                return EXIT_FAILURE;
              }
            }
            word[current_index] = (char)c;
            c = fgetc(fp);
            current_index++;
          }
          c = fgetc(fp);

          // Words of length >= 2
          if (current_index >= 2) {
            i = 0;
            while (i+1 < current_unique_word) {
              if (strcmp(word,words_in_files[i]) == 0) {
                counts[i]++;
                exists_flag = 1;
                total_words++;
              }
              i++;
            }
            if (!exists_flag) {

              // Increase if unique word count exceeds
              // capacity of words array
              if (current_unique_word+1 >= max_stored_words) {
                max_stored_words += 16;
                words_in_files = realloc(words_in_files, max_stored_words*sizeof(char*));
                counts = realloc(counts, max_stored_words*sizeof(int));

                // One check since they're parallel arrays
                // if one fails, both fail.
                if (words_in_files == NULL || counts == NULL) {
                  fprintf(stderr, "ERROR: not enough space to reallocate.\n");
                  return EXIT_FAILURE;
                }
                printf("Re-allocated parallel arrays to be size %d.\n",max_stored_words);
                fflush(stdout);
              }

              // Allocate enough space to store the word
              // Store the word + null terminating character
              // current_index = word length + 1 so it accounts
              // for the null terminating character.
              words_in_files[current_unique_word-1] = calloc(current_index+1, sizeof(char));
              strcpy(words_in_files[current_unique_word-1], word);

              // Update counts parallel array
              counts[current_unique_word-1] = 1;

              // Increase the unique word count and total word count
              current_unique_word++;
              total_words++;
            }
          }

          // Clear previous word, allocate clean memory
          // for the new word.
          free(word);
          word = NULL;
          word = calloc(16, sizeof(char));
          max_word_size = 16;
          current_index = 0;

          // Reset exists_flag
          exists_flag = 0;
        }
      }
      // Clear out buffers
      fflush(NULL);
      // Read the next file
      next_file = readdir(directory);
    }

    // Print words/counts
    printf("All done (successfully read %d words; %d unique words).\n",total_words, current_unique_word-1);
    fflush(stdout);
    int j = 0;
    if (max_words != -1) {

      // Check if max requested > max stored
      if (max_words > max_stored_words) {
        printf("First %d words (and corresponding counts) are:\n", max_stored_words);
      }
      else {
        printf("First %d words (and corresponding counts) are:\n", max_words);
      }
      fflush(stdout);
      for(j = 0; j < max_words; j++) {
        if (words_in_files[j] != NULL) {
          printf("%s -- %d\n", words_in_files[j], counts[j]);
          fflush(stdout);
          // Free the memory for the char*
          free(words_in_files[j]);
          words_in_files[j] = NULL;
        }
      }
      // Free all other dynamically allocated memory at locations
      // greater than max_words
      int k = 0;
      for (k = 0; k < current_unique_word-1; k++) {
        free(words_in_files[j]);
      }
    }
    else {
      printf("All words (and corresponding counts) are:\n");
      fflush(stdout);
      for(j = 0; j < current_unique_word-1; j++) {
        printf("%s -- %d\n", words_in_files[j], counts[j]);
        fflush(stdout);

        // Free the memory for the char*
        free(words_in_files[j]);
        words_in_files[j] = NULL;
      }
    }

    // Free pointers / Set them to NULL
    free(word);
    word = NULL;
    free(words_in_files);
    words_in_files = NULL;
    free(counts);
    counts = NULL;
    closedir(directory);
    directory = NULL;
    fclose(fp);
    fp = NULL;

    return EXIT_SUCCESS;
  }
  else if (directory == NULL){
    fprintf(stderr, "ERROR: Not a directory");
    return EXIT_FAILURE;
  }
}
