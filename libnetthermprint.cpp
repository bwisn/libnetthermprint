#include "libnetthermprint.h"
#include "../libescpos/libescpos.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>

using namespace std;

void NetThermPrint::sendReqAll() {
  string buf = "REQ_ALL";
  NetThermPrint_sendUdp(buf.c_str(), buf.length());
}

void NetThermPrint::sendBusy(uint16_t id) {
  char outstr[16];
  sprintf(outstr, "BUSY %u", id);
  NetThermPrint_sendUdpResponse(outstr, strlen(outstr));
  busyLast = NetThermPrint_getTime();
}

void NetThermPrint::sendStatus(uint16_t id, uint8_t status) {
  char outstr[16];

  if (status == 0) {
    sprintf(outstr, "OK %u", id);
  } else {
    sprintf(outstr, "ERROR %u", id);
  }
  NetThermPrint_sendUdpResponse(outstr, strlen(outstr));
}

void NetThermPrint::timerLoop() {
  time_t currentSecond = NetThermPrint_getTime();
  if (currentSecond - reqLast > 10000) {
    reqLast = currentSecond;

    if (notBusy()) {
      sendReqAll(); // REQ_ALL every 10 seconds if not busy
    }
  }

  if (currentSecond - busyLast > 45000) {
    if (busy()) {
      clearBusy(); // clear BUSY every 45 seconds if busy and stuck
    }
  }

  if (currentSecond > 1800000) {
    if (notBusy()) {
      NetThermPrint_systemReset(); // restart every 1/2h
    }
  }
}

/**
 * @brief Replaces "\" and "n" with '\n', skips "\" and "r"
 *
 * @param input char*
 * @return char* pointer to allocated variable with modified data
 */
char *NetThermPrint::unescapeNewl(const char *input, const size_t length) {

  size_t j = 0;
  char *out = new char[length + 1]();

  for (size_t i = 0; i < length; i++) {
    if (i + 1 < length) {
      if ((input[i] == '\\') && (input[i + 1] == 'n')) {
        // if found "\n" at the input
        out[j++] = '\n'; // put newline at the output
        i++;             // skip two if found
      } else if ((input[i] == '\\') && (input[i + 1] == 'r')) {
        // if found "\r" at the input
        i++; // skip two if found
      } else {
        out[j++] = input[i]; // otherwise rewrite
      }
    }
  }
  out[j++] = '\0'; // NUL on end
  return out;
}

void NetThermPrint::printTask() {
  EscPos printer;

  // Timestamp formatting
  char timebuf[80];
  struct tm ts;
  ts = *localtime(&task_print.epoch);
  strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S %Z", &ts);

  printer.init();
  printer.bold();
  printer.big();
  printer.center();
  printer.println("_______");
  if (task_print.special == 1) {
    printer.bold();
    printer.big();
    printer.center();
    printer.println("ALERT");
  }
  printer.small();
  printer.center();
  printer.println(timebuf);
  printer.println(task_print.title.c_str());
  printer.left();
  printer.bold(0);

  char *modified_content =
      unescapeNewl(task_print.content.c_str(), task_print.content.length() + 1);
  uint16_t linecount = 0;

  for (size_t i = 0; i < task_print.content.length(); i++) {
    if (modified_content[i] == '\n') {
      linecount++;
    }
  }

  char *saveptr;
  printer.println(strtok_r(modified_content, "\n", &saveptr));
  for (size_t i = 0; i < linecount; i++) {
    printer.println(strtok_r(NULL, "\n", &saveptr));
  }

  printer.end();
  delete[] modified_content;
}

void NetThermPrint::processData(void *payload) {
  // if first 5 characters of payload is PRINT, this is a print task
  if (strncmp("PRINT", (char *)payload, 5) == 0) {
    if (notBusy()) {
      char *saveptr; // saving ptr for strtok_r

      string str_buf = strtok_r((char *)payload + 6, ";", &saveptr);
      task_print.id = (uint16_t)strtol(str_buf.c_str(), nullptr, 10);

      str_buf = strtok_r(NULL, ";", &saveptr);
      task_print.epoch = (time_t)strtol(str_buf.c_str(), nullptr, 10);

      task_print.title = strtok_r(NULL, ";", &saveptr);

      task_print.content = strtok_r(NULL, ";", &saveptr);

      str_buf = strtok_r(NULL, ";", &saveptr);
      task_print.special = (uint16_t)strtol(str_buf.c_str(), nullptr, 10);

      setBusy();
      sendBusy(task_print.id);
    }
  }

  if (strncmp("WAITING", (char *)payload, 7) == 0) { // Printing
    char *saveptr;
    uint16_t waitid = strtol((char *)payload + 8, &saveptr, 10);

    if (busy()) {
      if (waitid == task_print.id) {
        // Print command
        printTask();
        sendStatus(task_print.id, 0);
        clearBusy();
      } else {
        sendStatus(task_print.id, 1);
        clearBusy();
      }
    } else {
      sendStatus(task_print.id, 0);
    }
  }
}