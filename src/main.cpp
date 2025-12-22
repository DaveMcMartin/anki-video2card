#include "Application.h"

int main(int argc, char* argv[])
{
  Video2Card::Application app("Anki Video2Card", 1280, 720);
  app.Run();

  return 0;
}
