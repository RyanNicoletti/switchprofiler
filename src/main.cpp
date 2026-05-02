#include <algorithm>
#include <csignal>
#include <cstdio>
#include <iterator>
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

void render(std::vector<std::string> &lines, std::size_t topLineIdx) {
  printf("\033[2J\033[H");
  printf("\033[90m");
  for (size_t i = topLineIdx; i < lines.size(); i++) {
    printf("%s", lines[i].c_str());
    printf("\r\n");
  }
  printf("\033[H");
  fflush(stdout);
}

void rerender(std::vector<std::string> &lines,
              const std::vector<std::string> &usrInput, size_t topLineIdx) {
  printf("\033[2J\033[H");
  printf("\033[90m");
  std::string topLine = lines[topLineIdx];
  for (size_t i = 0; i < topLine.size() && i < usrInput[topLineIdx].size();
       i++) {
    if (topLine[i] == usrInput[topLineIdx][i]) {
      printf("\033[%i;%zuH\033[32m%c", 1, i + 1, lines[topLineIdx][i]);
    } else {
      printf("\033[%i;%zuH\033[31m%c", 1, i + 1, lines[topLineIdx][i]);
    }
  }
  printf("\r\n");
  printf("\033[90m");
  for (size_t i = topLineIdx + 1; i < lines.size(); i++) {
    printf("\033[%zu;1H%s", i - topLineIdx + 1, lines[i].c_str());
  }
  printf("\033[%i;%iH", 2, 1);
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
  size_t topLineIdx = 0;
  render(lines, topLineIdx);
  size_t cursorRow = 0;
  int cursorCol = 0;
  std::vector<std::string> usrInput(1);
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 27) {
    char currChar = lines[cursorRow][cursorCol];
    if (c == 127) {
      if (cursorCol == 0 && cursorRow == 0) {
        continue;
      }
      // handle condition where back space happens at beginning of new line
      printf("\033[%zu;%iH\033[90m%c", cursorRow - topLineIdx + 1,
             cursorCol + 1, currChar);
      printf("\033[%zu;%iH\033[90m%c", cursorRow - topLineIdx + 1, cursorCol,
             lines[cursorRow][cursorCol - 1]);
      printf("\033[%zu;%iH\033[4m", cursorRow - topLineIdx + 1, cursorCol);
      fflush(stdout);
      cursorCol -= 1;
      continue;
    }

    if (c == currChar) {
      printf("\033[%zu;%iH\033[32m%c", cursorRow - topLineIdx + 1,
             cursorCol + 1, currChar);
      fflush(stdout);
    } else {
      printf("\033[%zu;%iH\033[31m%c", cursorRow - topLineIdx + 1,
             cursorCol + 1, currChar);
      fflush(stdout);
    }
    usrInput[cursorRow] += c;

    cursorCol += 1;
    if (cursorCol > static_cast<int>(lines[cursorRow].size())) {
      cursorCol = 0;
      cursorRow += 1;
      usrInput.emplace_back();
      if (cursorRow > topLineIdx + 1) {
        topLineIdx += 1;
        currChar = lines[cursorRow][cursorCol];
        lines.push_back(generateLine(rng, width));
        rerender(lines, usrInput, topLineIdx);
        continue;
      }
    }
    printf("\033[%zu;%iH\033[4m", cursorRow - topLineIdx + 1, cursorCol + 1);
    fflush(stdout);
  }
  return 0;
}
