#ifndef LIBNETTHERMPRINT_H
#define LIBNETTHERMPRINT_H
#include <cstdint>
#include <ctime>
#include <string>

using namespace std;

class NetThermPrint {
private:
  time_t reqLast = 0;     // time when last REQUEST received
  bool busy_flag = false; // printer busyness flag
  time_t busyLast = 0;    // time when last BUSY received

  struct task_print_t {
    uint16_t id = 0;
    uint16_t special = 0;
    time_t epoch = 0;
    string title;
    string content;
  } task_print;

  void sendReqAll();
  void sendBusy(uint16_t id);
  void sendStatus(uint16_t id, uint8_t status);
  bool notBusy() { return !busy_flag; }
  bool busy() { return busy_flag; }
  void setBusy() { busy_flag = true; }
  void clearBusy() { busy_flag = false; }
  void printTask();
  char *unescapeNewl(const char *input, const size_t length);

public:
  void timerLoop();
  void processData(void *payload);
};

time_t NetThermPrint_getTime() __attribute__((weak));
void NetThermPrint_systemReset() __attribute__((weak));
void NetThermPrint_sendUdp(const char *data, size_t len) __attribute__((weak));
void NetThermPrint_sendUdpResponse(const char *data, size_t len)
    __attribute__((weak));

#endif