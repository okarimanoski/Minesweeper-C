//---------------------------------------------------------------------------------------------------------------------
// a4.c
//
//
// The goal of this assignment is to implement the well-known 2D-game Minesweeper.
//
// Group: 4
//
// Author: 12231663
//---------------------------------------------------------------------------------------------------------------------
//

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>

// ANSI color codes
#define FLAGERRED_FIELD_COLOR "\033[31m"
#define MINE_NORMAL_COLOR "\033[33m"
#define MINE_HIGHLITED_COLOR "\033[33m\033[41m"
#define RESET_TEXT "\033[0m"

// ASCII values for field items
#define VERTICAL_BORDER '|'                // ''
#define HORIZONTAL_BORDER '='              // ''
#define CLOSED_FIELD 176                   // '▒'
#define OPENED_FIELD_NO_ADJACENT_MINES 250 // '·'
#define FLAGGED_FIELD 244                  // '¶'
#define MINE 64                            // '@'
#define EMPTY_SPACE " "
#define MAGIC_NUMBER "ESP\0"
#define MAX_SIZE 18446744073709551615ULL

// Error messages
#define OUT_OF_MEMORY "Out of memory!\n"
#define INVALID_NUMBER_OF_PARAMETERS "Invalid number of parameters given!\n"
#define UNEXPECTED_ARGUMENT "Unexpected argument provided!\n"
#define INVALID_TYPE_FOR_ARGUMENT "Invalid type for argument!\n"
#define INVALID_VALUE_FOR_ARGUMENT "Invalid value for argument!\n"
#define UNKNOWN_COMMAND "Error: Unknown command!\n"
#define COMMAND_MISSING_ARGUMENTS "Error: Command is missing arguments!\n"
#define TOO_MANY_ARGUMENTS "Error: Too many arguments given for command!\n"
#define INVALID_ARGUMENTS "Error: Invalid arguments given!\n"
#define INVALID_COORDINATES "Error: Coordinates are invalid for this game board!\n"
#define FAILED_TO_OPEN_FILE "Error: Failed to open file!\n"
#define INVALID_FILE_CONTENT "Error: Invalid file content!\n"

typedef struct _field_
{
  bool is_bomb;
  bool is_opened;
  int adjacent_bombs;
  bool is_flagged;
} myField;

typedef struct _block_
{
  uint8_t valid_bits; // 8 bits
  uint8_t mine_bits;  // 8 bits
  uint8_t open_bits;  // 8 bits
  uint8_t flag_bits;  // 8 bits
} myBlockField;

typedef struct _bit_field_
{
  char magic_number[4];           // 32 bits
  int board_height;               // 64 bits/8bytes
  int board_width;                // 64 bits/8bytes
  myBlockField *blocks_of_fields; // will point to 8 blocks because if each block is 4, 8x4 = 32
} myBitField;

void openAdjacentFields(myField **board, int x, int y, unsigned long long height, unsigned long long width, int *remaining_flags);
int countAdjacentBombs(myField **board, int i, int j, unsigned long long height, unsigned long long width);

//---------------------------------------------------------------------------------------------------------------------
///
/// This function checks if the given string represents a valid integer. A valid integer may start with an optional
/// '+' or '-' sign followed by one or more digits. It returns 1 if the string is a valid integer, and 0 otherwise.
///
/// @param str The string to be checked for integer representation.
///
/// @return 1 if the string is a valid integer, 0 otherwise.
//
int isInteger(const char *str)
{
  if (*str == '-' || *str == '+')
    str++;

  if (*str == '\0')
    return 0;

  while (*str != '\0')
  {
    if (!isdigit((unsigned char)*str))
      return 0; //
    str++;
  }
  return 1;
}
//---------------------------------------------------------------------------------------------------------------------
/// Prints a horizontal border.
/// @param width Width of the border.
//---------------------------------------------------------------------------------------------------------------------
void printHorizontalBorder(unsigned long long width)
{
  printf(" ");
  printf(" ");
  for (unsigned long long j = 0; j < width; j++)
  {
    printf("%c", HORIZONTAL_BORDER);
  }
  printf(" ");
  // printf(EMPTY_SPACE);

  printf("\n");
}
//---------------------------------------------------------------------------------------------------------------------
/// Prints a representation of a closed field in a game, typically used to indicate an unrevealed or unselected area.
//---------------------------------------------------------------------------------------------------------------------
void printClosedField()
{
  printf("░");
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints the game board when the player loses, revealing all bombs and the state of each field.
/// @param board The game board represented as a 2D array of myField structures.
/// @param bomb_x The x-coordinate of the bomb that caused the loss.
/// @param bomb_y The y-coordinate of the bomb that caused the loss.
/// @param height The height of the game board.
/// @param width The width of the game board.
//---------------------------------------------------------------------------------------------------------------------
void printLostField(myField **board, int bomb_x, int bomb_y, unsigned long long height, unsigned long long width)
{
  for (unsigned long long i = 0; i < height; i++)
  {
    printf("%s", EMPTY_SPACE);
    printf("%c", VERTICAL_BORDER);
    for (unsigned long long j = 0; j < width; j++)
    {
      if (board[i][j].is_bomb == true)
      {
        if (i == bomb_x && j == bomb_y)
        {
          printf(MINE_HIGHLITED_COLOR "@" RESET_TEXT);
        }
        else
        {
          printf(MINE_NORMAL_COLOR "@" RESET_TEXT);
        }
      }
      else if (board[i][j].is_opened == true)
      {
        board[i][j].adjacent_bombs = countAdjacentBombs(board, i, j, height, width);
        if (board[i][j].adjacent_bombs == 0)
        {
          printf("·");
        }
        else
        {
          printf("%d", board[i][j].adjacent_bombs);
        }
      }
      else
      {
        if (board[i][j].is_flagged)
        {
          printf(FLAGERRED_FIELD_COLOR "¶" RESET_TEXT);
        }
        else
        {
          printClosedField();
        }
      }
    }
    printf("%c", VERTICAL_BORDER);
    printf("\n");
  }
}
//---------------------------------------------------------------------------------------------------------------------
/// Prints the number of flags left for the player to use.
/// @param flags_left The number of flags remaining.
//---------------------------------------------------------------------------------------------------------------------
void printFlagsLeft(int flags_left)
{
  printf("%s", EMPTY_SPACE);
  printf("%s", EMPTY_SPACE);
  printf(FLAGERRED_FIELD_COLOR "¶" RESET_TEXT);
  printf(": %d\n", flags_left);
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints the game board in the state when the player loses, including remaining flags and a special marker for the
/// bomb that was triggered.
/// @param board The game board, a 2D array of myField structures.
/// @param x The x-coordinate of the triggered bomb.
/// @param y The y-coordinate of the triggered bomb.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param remaining_flags The number of flags left for the player to use.
//---------------------------------------------------------------------------------------------------------------------
void printLostMap(myField **board, int x, int y, unsigned long long height, unsigned long long width, int remaining_flags)
{
  printFlagsLeft(remaining_flags);
  printHorizontalBorder(width);
  printLostField(board, x, y, height, width);
  printHorizontalBorder(width);
}

//---------------------------------------------------------------------------------------------------------------------
/// Validates a command line argument to ensure it is an integer and converts it to an integer value.
/// @param arg The command line argument to be validated.
/// @param value Pointer to an integer where the converted value will be stored if validation is successful.
/// @return Returns 0 if the argument is successfully validated and converted, or 4 if the argument is not a valid integer.
//---------------------------------------------------------------------------------------------------------------------
int validateArgument(char *arg, int *value)
{
  if (!isInteger(arg))
  {
    printf(INVALID_TYPE_FOR_ARGUMENT);
    return 4;
  }
  *value = atoi(arg);
  return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints a mine character with normal color formatting.
//---------------------------------------------------------------------------------------------------------------------
void printMineNormal()
{
  printf(MINE_NORMAL_COLOR "%c" RESET_TEXT, 64);
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints a mine character with highlighted color formatting.
//---------------------------------------------------------------------------------------------------------------------
void printMineHighlited()
{
  printf(MINE_HIGHLITED_COLOR "%c" RESET_TEXT, 64);
}

//---------------------------------------------------------------------------------------------------------------------
/// Determines if a string represents a negative number.
/// @param string The string to check.
/// @return Returns 1 if the string starts with a '-', indicating a negative number, otherwise returns 0.
//---------------------------------------------------------------------------------------------------------------------
int isNegative(char *string)
{
  if (string[0] == '-')
  {
    return 1;
  }
  return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/// Validates the command line arguments for height and width, ensuring they are positive integers and within allowed limits.
/// @param index The starting index in argv for height and width arguments.
/// @param argc The total number of command line arguments.
/// @param argv The array of command line arguments.
/// @param height Pointer to store the validated height value.
/// @param width Pointer to store the validated width value.
/// @return Returns 0 if validation is successful, or an error code indicating the type of validation failure.
//---------------------------------------------------------------------------------------------------------------------
int validateSizeArguments(int index, int argc, char *argv[], unsigned long long *height, unsigned long long *width)
{
  unsigned long long max_value = MAX_SIZE;

  if (index + 2 >= argc)
  {
    printf(INVALID_NUMBER_OF_PARAMETERS);
    return 2;
  }

  if (!isInteger(argv[index + 1]) || !isInteger(argv[index + 2]))
  {
    printf(INVALID_TYPE_FOR_ARGUMENT);
    return 4;
  }

  if (isNegative(argv[index + 1]) || isNegative(argv[index + 2]))
  {
    printf(INVALID_VALUE_FOR_ARGUMENT);
    return 5;
  }

  unsigned long long temp_height = strtoull(argv[index + 1], NULL, 10);
  unsigned long long temp_width = strtoull(argv[index + 2], NULL, 10);

  if (temp_height >= max_value || temp_width >= max_value)
  {
    printf(OUT_OF_MEMORY);
    return 1;
  }

  *height = temp_height;
  *width = temp_width;

  if (*height <= 1 || *width <= 1)
  {
    printf(INVALID_VALUE_FOR_ARGUMENT);
    return 5;
  }

  return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/// Validates the command line arguments for the number of mines, ensuring the count is a positive integer and does not exceed the total number of fields.
/// @param index The index in argv where the mine count argument is located.
/// @param argc The total number of command line arguments.
/// @param argv The array of command line arguments.
/// @param count Pointer to store the validated mine count.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @return Returns 0 if validation is successful, or an error code indicating the type of validation failure.
//---------------------------------------------------------------------------------------------------------------------
int validateMinesArguments(int index, int argc, char *argv[], int *count, unsigned long long height, unsigned long long width)
{
  if (index + 1 >= argc)
  {
    printf(INVALID_NUMBER_OF_PARAMETERS);
    return 2;
  }

  if (argv[index + 1][0] == '-' && !isInteger(argv[index + 1]))
  {
    printf(INVALID_NUMBER_OF_PARAMETERS);
    return 2;
  }

  int exit_code = validateArgument(argv[index + 1], count);
  if (exit_code != 0)
  {
    return exit_code;
  }

  if (*count <= 0 || *count >= (height) * (width))
  {
    printf(INVALID_VALUE_FOR_ARGUMENT);
    return 5;
  }

  return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/// Validates the command line argument for the seed value, ensuring it is a positive integer.
/// @param index The index in argv where the seed argument is located.
/// @param argc The total number of command line arguments.
/// @param argv The array of command line arguments.
/// @param seed Pointer to a pointer to store the validated seed value.
/// @return Returns 0 if validation is successful, or an error code indicating the type of validation failure.
//---------------------------------------------------------------------------------------------------------------------
int validateSeedArguments(int index, int argc, char *argv[], int **seed)
{
  if (index + 1 >= argc)
  {
    printf(INVALID_NUMBER_OF_PARAMETERS);
    return 2;
  }

  if (argv[index + 1][0] == '-' && !isInteger(argv[index + 1]))
  {
    printf(INVALID_NUMBER_OF_PARAMETERS);
    return 2;
  }

  int exit_code = validateArgument(argv[index + 1], *seed);
  if (exit_code != 0)
  {
    return exit_code;
  }

  if (**seed < 0)
  {
    printf(INVALID_VALUE_FOR_ARGUMENT);
    return 5;
  }

  (**seed) = atoi(argv[index + 1]);
  index += 1;

  return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/// Handles and validates all command line arguments for the game, including board size, number of mines, and seed value.
/// @param argc The total number of command line arguments.
/// @param argv The array of command line arguments.
/// @param height Pointer to store the validated height of the game board.
/// @param width Pointer to store the validated width of the game board.
/// @param count Pointer to store the validated number of mines.
/// @param seed Pointer to store the validated seed value for random number generation.
/// @return Returns 0 if all arguments are successfully validated, or an error code indicating the type of validation failure.
//---------------------------------------------------------------------------------------------------------------------
int handleCommandLineArguments(int argc, char *argv[], unsigned long long *height, unsigned long long *width, int *count, int *seed)
{
  for (int index = 1; index < argc; index++)
  {
    if (strcmp(argv[index], "--size") == 0)
    {
      int exit_code = validateSizeArguments(index, argc, argv, height, width);
      if (exit_code != 0)
      {
        return exit_code;
      }
      index += 2;
    }
    else if (strcmp(argv[index], "--mines") == 0)
    {
      int exit_code = validateMinesArguments(index, argc, argv, count, *height, *width);
      if (exit_code != 0)
      {
        return exit_code;
      }
      index += 1;
    }
    else if (strcmp(argv[index], "--seed") == 0)
    {
      int exit_code = validateSeedArguments(index, argc, argv, &seed);
      if (exit_code != 0)
      {
        printf(INVALID_TYPE_FOR_ARGUMENT);
        return exit_code;
      }
      index += 1;
    }
    else
    {
      printf(UNEXPECTED_ARGUMENT);
      return 3;
    }
  }
  return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/// Allocates memory for a game board of a specified size and initializes each field.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @return Returns a pointer to the allocated game board or NULL if memory allocation fails.
//---------------------------------------------------------------------------------------------------------------------
myField **allocateMemoryBoard(unsigned long long height, unsigned long long width)
{
  myField **board = malloc(height * sizeof(myField *));
  if (board == NULL)
  {
    printf(OUT_OF_MEMORY);
    return NULL;
  }

  for (unsigned long long i = 0; i < height; i++)
  {
    board[i] = malloc(width * sizeof(myField));
    if (board[i] == NULL)
    {
      printf(OUT_OF_MEMORY);
      for (unsigned long long j = 0; j < i; j++)
      {
        free(board[j]);
      }
      free(board);
      return NULL;
    }
    for (unsigned long long j = 0; j < width; j++)
    {
      board[i][j].is_bomb = false;
      board[i][j].is_opened = false;
      board[i][j].adjacent_bombs = 0;
      board[i][j].is_flagged = false;
    }
  }

  return board;
}

//---------------------------------------------------------------------------------------------------------------------
/// Frees the allocated memory for the game board.
/// @param board A pointer to the game board, a 2D array of myField structures.
/// @param height The height of the game board, used to iterate through the first dimension.
//---------------------------------------------------------------------------------------------------------------------
void freeMemoryBoard(myField **board, unsigned long long height)
{
  for (unsigned long long i = 0; i < height; i++)
  {
    free(board[i]);
  }
  free(board);
}

//---------------------------------------------------------------------------------------------------------------------
/// Generates a 64-bit random number by combining the results of two calls to rand().
/// This function shifts the result of the first rand() call to the upper 32 bits and combines it with the result of
/// a second rand() call to fill the lower 32 bits, creating a 64-bit random number.
/// @return A 64-bit random number.
//---------------------------------------------------------------------------------------------------------------------
long long generate64BitRandomNumber()
{
  long long upper_bits = rand();
  long long lower_bits = rand();

  upper_bits = upper_bits << 32;

  long long fullNumber = upper_bits | lower_bits;

  return fullNumber;
}

//---------------------------------------------------------------------------------------------------------------------
/// Generates the game map by randomly placing mines on the board, except for the starting field.
/// @param board A pointer to the game board, a 2D array of myField structures.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param count The total number of mines to place on the board.
/// @param starting_field The index of the starting field, which will not contain a mine.
/// @param seed A pointer to the seed value used for random number generation; if zero, the current time is used.
//---------------------------------------------------------------------------------------------------------------------
void generateMap(myField **board, unsigned long long height, unsigned long long width, int count, int starting_field, int *seed)
{
  unsigned long long fields_left = height * width - 1;
  int mines_left = count;
  int random_number;

  if (seed != 0)
  {
    srand(*seed);
  }
  else
  {
    srand(time(NULL));
  }

  for (unsigned long long row = 0; row < height; row++)
  {
    for (unsigned long long col = 0; col < width; col++)
    {
      if (row * width + col == starting_field)
      {
        board[row][col].is_bomb = false;
        continue;
      }

      random_number = generate64BitRandomNumber() % fields_left;
      if (random_number < mines_left)
      {
        board[row][col].is_bomb = true;
        mines_left = mines_left - 1;
      }
      else
      {
        board[row][col].is_bomb = false;
      }
      fields_left = fields_left - 1;
    }
  }
}

//---------------------------------------------------------------------------------------------------------------------
/// Counts the number of bombs adjacent to a given field on the game board.
/// @param board A 2D array of myField structures representing the game board.
/// @param i The row index of the field.
/// @param j The column index of the field.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @return The number of adjacent bombs.
//---------------------------------------------------------------------------------------------------------------------
int countAdjacentBombs(myField **board, int i, int j, unsigned long long height, unsigned long long width)
{
  int adjacent_bombs = 0;
  for (int x = -1; x <= 1; x++)
  {
    for (int y = -1; y <= 1; y++)
    {
      unsigned long long new_x = x + i;
      unsigned long long new_y = y + j;
      if (new_x >= 0 && new_x < height && new_y >= 0 && new_y < width && board[new_x][new_y].is_bomb == true)
      {
        adjacent_bombs++;
      }
    }
  }
  return adjacent_bombs;
}

//---------------------------------------------------------------------------------------------------------------------
/// Opens a field on the game board. If the field is a bomb, the game ends. Otherwise, it reveals the number of adjacent bombs.
/// If a field with no adjacent bombs is opened, adjacent fields are recursively opened.
///
/// @param board A 2D array of myField structures representing the game board.
/// @param x The row index of the field to open.
/// @param y The column index of the field to open.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param remaining_flags A pointer to the number of flags remaining for the player to use.
///
/// @return Returns 0 if the field was successfully opened, 1 if the coordinates are invalid, 2 if a bomb was opened,
///         indicating the game is over.
//---------------------------------------------------------------------------------------------------------------------
int openField(myField **board, int x, int y, unsigned long long height, unsigned long long width, int *remaining_flags)
{
  if (x < 0 || x >= height || y < 0 || y >= width)
  {
    printf(INVALID_COORDINATES);
    return 1;
  }

  if (board[x][y].is_opened == true)
  {
    return 0;
  }

  if (board[x][y].is_flagged)
  {
    board[x][y].is_flagged = false;
    (*remaining_flags)++;
  }

  if (board[x][y].is_bomb == true)
  {
    board[x][y].is_opened = true;
    return 2;
  }

  int adjacent_bombs = countAdjacentBombs(board, x, y, height, width);
  board[x][y].is_opened = true;
  board[x][y].adjacent_bombs = adjacent_bombs;

  if (adjacent_bombs == 0)
  {
    openAdjacentFields(board, x, y, height, width, remaining_flags);
  }

  return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/// Recursively opens adjacent fields of a given field if they are not bombs and not already opened.
/// @param board A 2D array of myField structures representing the game board.
/// @param x The row index of the current field.
/// @param y The column index of the current field.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param remaining_flags A pointer to the number of flags remaining for the player to use.
//---------------------------------------------------------------------------------------------------------------------
void openAdjacentFields(myField **board, int x, int y, unsigned long long height, unsigned long long width, int *remaining_flags)
{
  for (int i = -1; i <= 1; i++)
  {
    for (int j = -1; j <= 1; j++)
    {
      unsigned long long new_x = i + x;
      unsigned long long new_y = j + y;

      if (new_x < 0 || new_x >= height || new_y < 0 || new_y >= width)
      {
        continue;
      }

      if (board[new_x][new_y].is_opened || board[new_x][new_y].is_bomb)
      {
        continue;
      }
      openField(board, new_x, new_y, height, width, remaining_flags);
    }
  }
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints the current state of the game board to the console, showing opened fields, flagged fields, and closed fields.
/// @param board A 2D array of myField structures representing the game board.
/// @param height The height of the game board.
/// @param width The width of the game board.
//---------------------------------------------------------------------------------------------------------------------
void printField(myField **board, unsigned long long height, unsigned long long width)
{
  for (unsigned long long i = 0; i < height; i++)
  {
    printf("%s", EMPTY_SPACE);
    printf("%c", VERTICAL_BORDER);
    for (unsigned long long j = 0; j < width; j++)
    {
      if (board[i][j].is_flagged == true)
      {
        printf(FLAGERRED_FIELD_COLOR "¶" RESET_TEXT);
      }
      else if (board[i][j].is_opened)
      {
        if (board[i][j].is_bomb)
        {
          printf(MINE_NORMAL_COLOR "@" RESET_TEXT);
        }
        else
        {
          board[i][j].adjacent_bombs = countAdjacentBombs(board, i, j, height, width);
          if (board[i][j].adjacent_bombs == 0)
          {
            printf("·");
          }
          else
          {
            printf("%d", board[i][j].adjacent_bombs);
          }
        }
      }
      else
      {
        printClosedField();
      }
    }
    printf("%c", VERTICAL_BORDER);
    printf("\n");
  }
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints only the opened fields of the game board, showing bombs and the number of adjacent bombs for each field.
/// @param board A 2D array of myField structures representing the game board.
/// @param height The height of the game board.
/// @param width The width of the game board.
//---------------------------------------------------------------------------------------------------------------------
void printOpenedField(myField **board, unsigned long long height, unsigned long long width)
{
  for (unsigned long long i = 0; i < height; i++)
  {
    printf("%s", EMPTY_SPACE);
    printf("%c", VERTICAL_BORDER);
    for (unsigned long long j = 0; j < width; j++)
    {
      if (board[i][j].is_bomb)
      {
        printf(MINE_NORMAL_COLOR "@" RESET_TEXT);
      }
      else
      {
        board[i][j].adjacent_bombs = countAdjacentBombs(board, i, j, height, width);
        if (board[i][j].adjacent_bombs == 0)
        {
          printf("·");
        }
        else
        {
          printf("%d", board[i][j].adjacent_bombs);
        }
      }
    }
    printf("%c", VERTICAL_BORDER);
    printf("\n");
  }
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints the entire game map including the number of flags left, a horizontal border, and the current state of the game board.
/// @param board A 2D array of myField structures representing the game board.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param flags_left The number of flags left for the player to use.
//---------------------------------------------------------------------------------------------------------------------
void printMap(myField **board, unsigned long long height, unsigned long long width, int flags_left)
{
  printFlagsLeft(flags_left);
  printHorizontalBorder(width);
  printField(board, height, width);
  printHorizontalBorder(width);
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints the map with only opened fields visible, including the number of flags left and a horizontal border.
/// @param board A 2D array of myField structures representing the game board.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param flags_left The number of flags left for the player to use.
//---------------------------------------------------------------------------------------------------------------------
void printOpenedMap(myField **board, unsigned long long height, unsigned long long width, int flags_left)
{
  printFlagsLeft(flags_left);
  printHorizontalBorder(width);
  printOpenedField(board, height, width);
  printHorizontalBorder(width);
}

//---------------------------------------------------------------------------------------------------------------------
/// Removes the newline character from a string by replacing it with a null terminator.
/// @param string The string from which the newline character will be removed.
//---------------------------------------------------------------------------------------------------------------------
void removeNewLine(char string[])
{
  while (*string != '\0')
  {
    if (*string == '\n')
    {
      *string = '\0';
    }
    string++;
  }
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints the initial welcome message and game settings including field size and number of mines.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param count The number of mines to be placed on the game board.
//---------------------------------------------------------------------------------------------------------------------
void printInitialMessage(unsigned long long height, unsigned long long width, int count)
{
  printf("Welcome to ESP Minesweeper!\n");
  printf("Chosen field size: %llu x %llu.\n", height, width);
  printf("After map generation %d mines will be hidden in the playing field.\n", count);
}

//---------------------------------------------------------------------------------------------------------------------
/// Clears the input buffer to prevent unwanted characters from affecting subsequent inputs.
//---------------------------------------------------------------------------------------------------------------------
void clearInputBuffer()
{
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
  {
  }
}

//---------------------------------------------------------------------------------------------------------------------
/// Toggles the flag status of a field on the game board, either adding or removing a flag based on its current state.
/// @param board A 2D array of myField structures representing the game board.
/// @param i The row index of the field to toggle.
/// @param j The column index of the field to toggle.
/// @param remaining_flags A pointer to the number of flags remaining for the player to use.
//---------------------------------------------------------------------------------------------------------------------
void fieldFlag(myField **board, int i, int j, int *remaining_flags)
{
  if (board[i][j].is_flagged == true)
  {
    board[i][j].is_flagged = false;
    (*remaining_flags)++;
  }
  else if (board[i][j].is_flagged == false && board[i][j].is_opened == false)
  {
    board[i][j].is_flagged = true;
    (*remaining_flags)--;
  }
}

//---------------------------------------------------------------------------------------------------------------------
/// Checks if the win condition is met by verifying all non-bomb fields are opened.
/// @param board A 2D array of myField structures representing the game board.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @return Returns 0 if the win condition is met, otherwise returns 1.
//---------------------------------------------------------------------------------------------------------------------
int checkWinCondition(myField **board, unsigned long long height, unsigned long long width)
{
  for (unsigned long long i = 0; i < height; i++)
  {
    for (unsigned long long j = 0; j < width; j++)
    {
      if (board[i][j].is_opened == false && board[i][j].is_bomb == false)
      {
        return 1;
      }
    }
  }
  return 0;
}

//---------------------------------------------------------------------------------------------------------------------
/// Prints the game board in a special format when the player wins, showing all bombs and opened fields.
/// @param board A 2D array of myField structures representing the game board.
/// @param height The height of the game board.
/// @param width The width of the game board.
//---------------------------------------------------------------------------------------------------------------------
void printWonField(myField **board, unsigned long long height, unsigned long long width)
{
  for (unsigned long long i = 0; i < height; i++)
  {
    printf("%s", EMPTY_SPACE);
    printf("%c", VERTICAL_BORDER);
    for (unsigned long long j = 0; j < width; j++)
    {
      if (board[i][j].is_bomb == true)
      {
        printf(MINE_NORMAL_COLOR "@" RESET_TEXT);
      }
      else if (board[i][j].is_opened == true && board[i][j].is_bomb == false)
      {
        board[i][j].adjacent_bombs = countAdjacentBombs(board, i, j, height, width);
        if (board[i][j].adjacent_bombs == 0)
        {
          printf("·");
        }
        else
        {
          printf("%d", board[i][j].adjacent_bombs);
        }
      }
    }
    printf("%c", VERTICAL_BORDER);
    printf("\n");
  }
}
//---------------------------------------------------------------------------------------------------------------------
/// Prints the game board in a special format when the player wins, showing all bombs and opened fields.
/// @param board A 2D array of myField structures representing the game board.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param remaining_flags The number of flags left for the player to use.
//---------------------------------------------------------------------------------------------------------------------
void printWonMap(myField **board, unsigned long long height, unsigned long long width, int remaining_flags)
{
  printFlagsLeft(remaining_flags);
  printHorizontalBorder(width);
  printWonField(board, height, width);
  printHorizontalBorder(width);
}

//---------------------------------------------------------------------------------------------------------------------
/// Handles the "start" command by initializing the game board, placing mines, and opening the starting field.
/// @param board A pointer to the game board, a 2D array of myField structures.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param count The total number of mines to place on the board.
/// @param i The number of arguments passed to the command.
/// @param remaining_flags A pointer to the number of flags remaining for the player to use.
/// @param seed The seed value used for random number generation.
/// @param words An array of strings containing the command arguments.
/// @return Returns 0 if the game continues, 1 for invalid command usage, and 2 if the player loses by opening a bomb.
//---------------------------------------------------------------------------------------------------------------------
int handleStartCommand(myField **board, unsigned long long height, unsigned long long width, int count, int i, int *remaining_flags, int seed, char **words)
{
  if (i < 3)
  {
    printf(COMMAND_MISSING_ARGUMENTS);
    return 1;
  }
  else if (i > 3)
  {
    printf(TOO_MANY_ARGUMENTS);
    return 1;
  }
  else
  {
    if (!isInteger(words[1]) || !isInteger(words[2]))
    {
      printf(INVALID_ARGUMENTS);
      return 1;
    }
    int x = atoi(words[1]);
    int y = atoi(words[2]);

    if (x >= height || y >= width)
    {
      printf(INVALID_COORDINATES);
      return 1;
    }

    *remaining_flags = count;
    int starting_field = x * width + y;

    generateMap(board, height, width, count, starting_field, &seed);

    int flag_bombica = openField(board, x, y, height, width, remaining_flags);

    if (flag_bombica != 0)
    {
      printf("=== You lost! ===\n");
      printLostMap(board, x, y, height, width, *remaining_flags);
      freeMemoryBoard(board, height);
      return 2;
    }

    printMap(board, height, width, *remaining_flags);
  }
  return 1;
}

//---------------------------------------------------------------------------------------------------------------------
/// Handles the "open" command by opening a specified field on the game board. It checks for command validity, parses
/// coordinates, and manages game state changes such as flag removal, bomb opening, win condition, and map printing.
/// @param board A pointer to the game board, a 2D array of myField structures.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param i The number of arguments passed to the command.
/// @param remaining_flags A pointer to the number of flags remaining for the player to use.
/// @param words An array of strings containing the command arguments.
/// @return Returns 1 for continued gameplay, 0 for game over, and 1 for invalid command usage.
//---------------------------------------------------------------------------------------------------------------------
int handleOpenCommand(myField **board, unsigned long long height, unsigned long long width, int i, int *remaining_flags, char **words)
{
  if (i < 3)
  {
    printf(COMMAND_MISSING_ARGUMENTS);
    return 1;
  }
  else if (i > 3)
  {
    printf(TOO_MANY_ARGUMENTS);
    return 1;
  }
  else
  {
    if (!isInteger(words[1]) || !isInteger(words[2]))
    {
      printf(INVALID_ARGUMENTS);
      return 1;
    }
    int x = atoi(words[1]);
    int y = atoi(words[2]);

    if (x >= height || y >= width)
    {
      printf(INVALID_COORDINATES);
      return 1;
    }

    if (board[x][y].is_flagged)
    {
      if (!board[x][y].is_bomb)
      {
        (*remaining_flags)++;
      }
      board[x][y].is_flagged = false;
    }

    int bomb = openField(board, x, y, height, width, remaining_flags);
    if (bomb != 0)
    {
      printf("=== You lost! ===\n\n");
      printLostMap(board, x, y, height, width, *remaining_flags);
      freeMemoryBoard(board, height);
      return 0;
    }

    int win = checkWinCondition(board, height, width);
    if (win == 0)
    {
      printf("=== You won! ===\n\n");
      printWonMap(board, height, width, *remaining_flags);
      freeMemoryBoard(board, height);
      return 0;
    }
    printMap(board, height, width, *remaining_flags);
  }
  return 1;
}

//---------------------------------------------------------------------------------------------------------------------
/// Handles the "flag" command by toggling the flag status of a specified field on the game board. It validates the
/// command arguments, checks for valid coordinates, and updates the game state accordingly.
/// @param board A pointer to the game board, a 2D array of myField structures.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @param i The number of arguments passed to the command.
/// @param remaining_flags A pointer to the number of flags remaining for the player to use.
/// @param words An array of strings containing the command arguments.
//---------------------------------------------------------------------------------------------------------------------
void handleFlagCommand(myField **board, unsigned long long height, unsigned long long width, int i, int *remaining_flags, char **words)
{
  if (i < 3)
  {
    printf(COMMAND_MISSING_ARGUMENTS);
    return;
  }
  else if (i > 3)
  {
    printf(TOO_MANY_ARGUMENTS);
    return;
  }
  else
  {
    if (!isInteger(words[1]) || !isInteger(words[2]))
    {
      printf(INVALID_ARGUMENTS);
      return;
    }
    int row = atoi(words[1]);
    int col = atoi(words[2]);

    if (row >= height || col >= width)
    {
      printf(INVALID_COORDINATES);
      return;
    }
    fieldFlag(board, row, col, remaining_flags);
    printMap(board, height, width, *remaining_flags);
  }
}

//---------------------------------------------------------------------------------------------------------------------
/// Saves the current game state to a file, including the board dimensions and the status of each field.
/// @param filename The name of the file where the game state will be saved.
/// @param board A 2D array of myField structures representing the game board.
/// @param height The height of the game board.
/// @param width The width of the game board.
//---------------------------------------------------------------------------------------------------------------------
void saveGameStateToFile(char *filename, myField **board, unsigned long long height, unsigned long long width)
{
  FILE *file_pointer = fopen(filename, "wb");
  if (file_pointer == NULL)
  {
    printf(FAILED_TO_OPEN_FILE);
    return;
  }

  fwrite(MAGIC_NUMBER, sizeof(char), 4, file_pointer);

  uint64_t height64 = (uint64_t)height;
  uint64_t width64 = (uint64_t)width;
  fwrite(&height64, sizeof(height64), 1, file_pointer);
  fwrite(&width64, sizeof(width64), 1, file_pointer);

  unsigned long long total_fields = height * width;
  unsigned long long num_blocks = (total_fields + 7) / 8;

  myBlockField *blocks = (myBlockField *)calloc(num_blocks, sizeof(myBlockField));
  if (blocks == NULL)
  {
    printf(OUT_OF_MEMORY);
    fclose(file_pointer);
    return;
  }

  for (unsigned long long i = 0; i < total_fields; i++)
  {
    unsigned long long block_index = i / 8;
    unsigned long long bit_position = i % 8;

    if (board[i / width][i % width].is_flagged)
    {
      blocks[block_index].flag_bits |= 1 << bit_position;
    }
    if (board[i / width][i % width].is_opened)
    {
      blocks[block_index].open_bits |= 1 << bit_position;
    }
    if (board[i / width][i % width].is_bomb)
    {
      blocks[block_index].mine_bits |= 1 << bit_position;
    }
    blocks[block_index].valid_bits |= 1 << bit_position;
  }

  fwrite(blocks, sizeof(myBlockField), num_blocks, file_pointer);

  free(blocks);
  fclose(file_pointer);
}

//---------------------------------------------------------------------------------------------------------------------
/// Loads the game state from a file, reconstructing the game board with its dimensions and the status of each field.
/// @param filename The name of the file from which the game state is loaded.
/// @param height A pointer to an integer where the height of the board will be stored.
/// @param width A pointer to an integer where the width of the board will be stored.
/// @return Returns a pointer to a 2D array of myField structures representing the loaded game board, or NULL on failure.
//---------------------------------------------------------------------------------------------------------------------
myField **loadGameStateFromFile(char *filename, unsigned long long *height, unsigned long long *width)
{
  FILE *file_pointer = fopen(filename, "rb");
  if (file_pointer == NULL)
  {
    printf(FAILED_TO_OPEN_FILE);
    return NULL;
  }

  char magic[4];
  fread(magic, sizeof(char), 4, file_pointer);
  if (strncmp(magic, MAGIC_NUMBER, 4) != 0)
  {
    printf(INVALID_FILE_CONTENT);
    fclose(file_pointer);
    return NULL;
  }

  uint64_t height64;
  uint64_t width64;
  fread(&height64, sizeof(height64), 1, file_pointer);
  fread(&width64, sizeof(width64), 1, file_pointer);
  *height = (unsigned long long)height64;
  *width = (unsigned long long)width64;

  myField **board = allocateMemoryBoard(*height, *width);
  if (board == NULL)
  {
    fclose(file_pointer);
    return NULL;
  }

  unsigned long long total_fields = (*height) * (*width);
  unsigned long long num_blocks = (total_fields + 7) / 8;

  myBlockField *blocks = (myBlockField *)malloc(num_blocks * sizeof(myBlockField));
  if (blocks == NULL)
  {
    printf(OUT_OF_MEMORY);
    freeMemoryBoard(board, *height);
    fclose(file_pointer);
    return NULL;
  }

  fread(blocks, sizeof(myBlockField), num_blocks, file_pointer);

  for (unsigned long long i = 0; i < total_fields; i++)
  {
    unsigned long long block_index = i / 8;
    unsigned long long bit_position = i % 8;

    if (blocks[block_index].valid_bits & (1 << bit_position))
    {
      board[i / (*width)][i % (*width)].is_bomb = (blocks[block_index].mine_bits & (1 << bit_position)) != 0;
      board[i / (*width)][i % (*width)].is_opened = (blocks[block_index].open_bits & (1 << bit_position)) != 0;
      board[i / (*width)][i % (*width)].is_flagged = (blocks[block_index].flag_bits & (1 << bit_position)) != 0;
    }
  }

  free(blocks);
  fclose(file_pointer);
  return board;
}

//---------------------------------------------------------------------------------------------------------------------
/// Validates a given command against a list of known commands for the game.
/// @param command The command string to validate.
//---------------------------------------------------------------------------------------------------------------------
void validateCommand(char *command)
{
  if (strcmp(command, "start") != 0 && strcmp(command, "open") != 0 && strcmp(command, "flag") != 0 &&
      strcmp(command, "dump") != 0 && strcmp(command, "save") != 0 && strcmp(command, "load") != 0 &&
      strcmp(command, "quit") != 0)
  {
    printf(UNKNOWN_COMMAND);
  }
}

//---------------------------------------------------------------------------------------------------------------------
/// Counts the total number of bombs on the game board.
/// @param board A 2D array of myField structures representing the game board.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @return The total number of bombs on the board.
//---------------------------------------------------------------------------------------------------------------------
int countBombs(myField **board, unsigned long long height, unsigned long long width)
{
  int total_bombs = 0;
  for (unsigned long long i = 0; i < height; i++)
  {
    for (unsigned long long j = 0; j < width; j++)
    {
      if (board[i][j].is_bomb == true)
      {
        total_bombs++;
      }
    }
  }
  return total_bombs;
}
//---------------------------------------------------------------------------------------------------------------------
/// Counts the total number of flags placed on the game board.
/// @param board A 2D array of myField structures representing the game board.
/// @param height The height of the game board.
/// @param width The width of the game board.
/// @return The total number of flags placed on the board.
//---------------------------------------------------------------------------------------------------------------------
int countFlags(myField **board, unsigned long long height, unsigned long long width)
{
  int total_flags_places = 0;
  for (unsigned long long i = 0; i < height; i++)
  {
    for (unsigned long long j = 0; j < width; j++)
    {
      if (board[i][j].is_flagged == true)
      {
        total_flags_places++;
      }
    }
  }
  return total_flags_places;
}

//---------------------------------------------------------------------------------------------------------------------
/// The main function of the minesweeper game. It initializes the game, processes user commands, and manages the game state.
/// @param argc The number of command-line arguments.
/// @param argv The array of command-line arguments.
/// @return Returns 0 on successful execution and termination of the game, and 1 on encountering an error.
//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  unsigned long long height = 9;
  unsigned long long width = 9;
  int count = 10; // number of mines
  int seed = 0;
  char line[101];
  char command[11];
  char *words[5];
  myField **board = NULL;
  int remaining_flags = 0;

  int exit_code = handleCommandLineArguments(argc, argv, &height, &width, &count, &seed);

  if (exit_code != 0)
  {
    return exit_code;
  }

  unsigned long long max_size = MAX_SIZE;
  unsigned long long size_check = (unsigned long long)height * (unsigned long long)width;

  if (size_check >= max_size)
  {
    printf(OUT_OF_MEMORY);
    return 1;
  }

  board = allocateMemoryBoard(height, width);
  if (board == NULL)
  {
    return 1;
  }

  printInitialMessage(height, width, count);

  while (1)
  {
    printf(" > ");
    fgets(line, 100, stdin);
    removeNewLine(line);

    if (strlen(line) == 0)
    {
      printf(UNKNOWN_COMMAND);
      continue;
    }

    char *token = strtok(line, " \n");
    int i = 0;
    while (token != NULL)
    {
      words[i] = token;
      i++;
      token = strtok(NULL, " \n");
    }

    if (i == 0)
    {
      printf(UNKNOWN_COMMAND);
      continue;
    }

    strcpy(command, words[0]);
    removeNewLine(command);
    validateCommand(command);

    if (strcmp(command, "start") == 0)
    {
      printf("\n");
      int exit_code = handleStartCommand(board, height, width, count, i, &remaining_flags, seed, words);
      if (exit_code == 1)
      {
        continue;
      }
      else
      {
        return 0;
      }
    }
    else if (strcmp(command, "open") == 0)
    {
      printf("\n");
      int exit_code = handleOpenCommand(board, height, width, i, &remaining_flags, words);
      if (exit_code == 1)
      {
        continue;
      }
      else
      {
        return 0;
      }
    }
    else if (strcmp(command, "flag") == 0)
    {
      printf("\n");
      handleFlagCommand(board, height, width, i, &remaining_flags, words);
    }
    else if (strcmp(command, "quit") == 0)
    {
      printf("\n");
      if (board != NULL)
      {
        printMap(board, height, width, remaining_flags);
        freeMemoryBoard(board, height);
        board = NULL;
      }
      return 0;
    }
    else if (strcmp(command, "dump") == 0)
    {
      printf("\n");
      printOpenedMap(board, height, width, remaining_flags);
      printf("\n");
      printMap(board, height, width, remaining_flags);
    }
    else if (strcmp(command, "save") == 0)
    {
      if (i < 2)
      {
        printf("Error: Command is missing arguments!\n");
      }
      else
      {
        char *filename = words[1];
        FILE *file_pointer = fopen(filename, "wb");
        if (file_pointer == NULL)
        {
          printf(FAILED_TO_OPEN_FILE);
        }
        else
        {
          fclose(file_pointer);
          saveGameStateToFile(filename, board, height, width);
          printf("\n");
        }
      }
      printMap(board, height, width, remaining_flags);
    }
    else if (strcmp(command, "load") == 0)
    {
      if (i < 2)
      {
        printf("Error: Command is missing arguments!\n");
      }
      else
      {
        char *filename = words[1];
        unsigned long long new_height, new_width;
        myField **new_board = loadGameStateFromFile(filename, &new_height, &new_width);
        if (new_board != NULL)
        {
          printf("\n");
          freeMemoryBoard(board, height);
          board = new_board;
          height = new_height;
          width = new_width;

          int total_bombs = countBombs(board, new_height, new_width);
          int flags_placed = countFlags(board, new_height, new_width);

          remaining_flags = total_bombs - flags_placed;
          printMap(board, height, width, remaining_flags);
        }
      }
    }
  }

  if (board != NULL)
  {
    freeMemoryBoard(board, height);
  }
  return 0;
}
