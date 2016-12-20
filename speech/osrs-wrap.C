#include <string>
#include <fstream>
#include <iostream>
using namespace std;

#include <stdint.h>

int main(int argc, char **argv) {
  if (argc<2)
    return 1;

  size_t repeat = 5;

  int firstsleep  = 10;
  int repeatsleep =  3;
  int intersleep  =  0;
  int lastsleep   = 10;

  if (firstsleep)
    sleep(firstsleep);

  for (int i=1; i<argc; i++) {
    ifstream in(argv[i]);
    string data;
    for (;;)
      try {
	char tmp[1024*1024];
	in.read(tmp, sizeof tmp);
	streamsize n = in.gcount();
	data.append(tmp, n);
	if (!n || !in)
	  break;
      }
      catch (...) { return 2; }

    uint32_t len = 6+data.size();

    // cout << sizeof(len) << endl;  return 0;

    char m_audio = 0; // M_AUDIO
    char urgent  = 0;
    string buf;
    buf.insert(0, (char*)&len, 4);
    buf.insert(4, 1, m_audio);
    buf.insert(5, 1, urgent);
    buf.insert(buf.size(), data);

    for (size_t j=0; j<repeat; j++) {
      cout << buf << flush;
      if (repeatsleep && j+1<repeat)
	sleep(repeatsleep);
    }

    if (intersleep)
      sleep(intersleep);
  }

  sleep(lastsleep);

  return 0;
}
  
