#include <csignal>
#include <cstdio>
#include <random>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "words.h"

class RawTerminal {
  inline static struct termios defaultTerm;
  inline static bool isRawModeActive = false;

public:
  static void restore() {
    printf("\033[0m");
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

std::string generateWords() {
  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<int> dist(0, wordList.size() - 1);
  std::string words;
  for (int i = 0; i < 80; i++) {
    int idx = dist(rng);
    std::string_view word = wordList[idx];
    if (i > 0) {
      words += ' ';
    }
    words += word;
  }
  return words;
}

int getTermWidth() {
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  int width = ws.ws_col;
  return width;
}

void render(const std::string &words, int maxWidth) {
  // clear the screen and move cursor to position (1, 1)
  printf("\033[2J\033[H");
  // use sstream to iterate through the string and keep track of currwidth, make
  // sure it doesnt exceed maxWidth
}

int main() {
  std::signal(SIGABRT, handleSignal);
  std::signal(SIGSEGV, handleSignal);
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);
  RawTerminal term;

  std::string randomWords = generateWords();
  int termWidth = getTermWidth();
  render(randomWords, termWidth);
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 27) {
    printf("%d\r\n", c);
  }
  return 0;
}
