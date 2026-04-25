#include <csignal>
#include <cstdio>
#include <termios.h>
#include <unistd.h>

#include "words.h"

class RawTerminal {
  inline static struct termios defaultTerm;
  inline static bool isRawModeActive = false;

public:
  static void restore() {
    if (!isRawModeActive)
      return;
    isRawModeActive = false;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &defaultTerm);
  }

  RawTerminal() {
    tcgetattr(STDIN_FILENO, &defaultTerm);

    struct termios rawTerm = defaultTerm;

    rawTerm.c_oflag &= ~(OPOST);
    rawTerm.c_iflag &= ~(ICRNL | IXON);
    rawTerm.c_lflag &= ~(ECHO | ICANON | IEXTEN);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawTerm);
    isRawModeActive = true;
  }
  ~RawTerminal() { restore(); }
};

void handleSignal(int sig) {
  RawTerminal::restore();
  std::signal(sig, SIG_DFL);
  raise(sig);
}

int main() {
  std::signal(SIGABRT, handleSignal);
  std::signal(SIGSEGV, handleSignal);
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);
  RawTerminal term;
  char c;

  while (read(STDIN_FILENO, &c, 1) == 1 && c != 27) {
    printf("%d\r\n", c);
  }
  return 0;
}
