#include "ChatServer.hpp"

int main()
{
  ChatServer cs;
  cs.InitSerber();
  cs.Start();
  while(1)
  {
      sleep(1);
  }
  return 0;
}
