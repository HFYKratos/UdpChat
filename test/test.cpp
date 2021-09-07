#include <stdio.h>
#include <ncurses.h>

int main()
{
  //newwin(int lines, int cols, int start_y, int start_x);
  WINDOW* header = newwin(5, 5, 0, 0);

  endwin(header);
  return 0; 
}
