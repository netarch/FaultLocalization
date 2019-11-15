#include "utils.h"

#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {
  string filter = "port 5001";
  string interface = "eth1";
  string local_ip = "130.126.136.66";

  if (argc >= 2) {
    interface = argv[1];
    cout << "Reset capture interface to: " << interface << endl;
  }
  if (argc >= 3) {
    local_ip = argv[2];
  }
  cout << "Local IP address: " << local_ip << endl;

  string tcpdump_cmd(
      "sudo tcpdump -i " + interface + " -n -s 80 -l -tt " + filter);
  StartTcpdump(tcpdump_cmd, local_ip);

  return 0;
}
