#include <algorithm>
#include <csignal>
#include <cstdio>
#include <random>
#include <sstream>
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

int getTermWidth() {
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  int width = ws.ws_col;
  return width;
}

std::string generateLine(std::mt19937 &rng, int terminalWidth) {
  std::uniform_int_distribution<int> dist(0, wordList.size() - 1);
  std::string line;
  int lineLength = 0;
  while (lineLength < terminalWidth) {
    std::string_view word = wordList[dist(rng)];
    int newLength = lineLength + word.size();
    if (newLength >= static_cast<int>(terminalWidth)) {
      break;
    }
    if (lineLength > 0) {
      line += " ";
      newLength += 1;
    }
    line += word;
    lineLength = newLength;
  }
  return line;
}

void initialRender(std::vector<std::string> &lines) {
  printf("\033[2J\033[H");
  printf("\033[90m");
  for (const std::string &line : lines) {
    printf("%s", line.c_str());
    printf("\r\n");
  }
  printf("\033[H");
  fflush(stdout);
}

void fillLines(std::vector<std::string> &lines, std::mt19937 &rng,
               int terminalWidth) {
  lines.push_back(generateLine(rng, terminalWidth));
  lines.push_back(generateLine(rng, terminalWidth));
  lines.push_back(generateLine(rng, terminalWidth));
}

int main() {
  std::signal(SIGABRT, handleSignal);
  std::signal(SIGSEGV, handleSignal);
  std::signal(SIGINT, handleSignal);
  std::signal(SIGTERM, handleSignal);
  RawTerminal term;
  std::vector<std::string> lines;
  std::random_device rd;
  std::mt19937 rng(rd());
  int width = getTermWidth();
  fillLines(lines, rng, width);
  initialRender(lines);
  // 1 based indexing for terminal col and row
  int cursorRow = 0;
  int cursorCol = 0;
  std::vector<std::string> usrInput;
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 27) {
    if (c == 127) {
      // handle backspace
    }
    char currChar = lines[cursorRow][cursorCol];
    if (c == currChar) {
      printf("\033[%i;%iH\033[32m%c", cursorRow + 1, cursorCol + 1, currChar);
      cursorCol += 1;
      if (cursorCol > width) {
        // resize
      }
      printf("\033[%i;%iH\033[4m", cursorRow + 1, cursorCol + 1);
      fflush(stdout);
      continue;
    } else {
      printf("\033[%i;%iH\033[31m%c", cursorRow + 1, cursorCol + 1, currChar);
      if (cursorCol == 0) {
        continue;
      }
      cursorCol += 1;
      printf("\033[%i;%iH\033[4m", cursorRow + 1, cursorCol + 1);
      fflush(stdout);
      continue;
    }
  }
  return 0;
}
