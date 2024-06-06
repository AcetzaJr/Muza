#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct Estructura;

int main(void) {
  char *device = "/dev/midi2";
  unsigned char data[3] = {0x90, 60, 127};

  // step 1: open the OSS device for writing
  int fd = open(device, O_WRONLY, 0);
  if (fd < 0) {
    printf("Error: cannot open %s\n", device);
    exit(1);
  }

  // step 2: write the MIDI information to the OSS device
  write(fd, data, sizeof(data));

  // step 3: (optional) close the OSS device
  close(fd);

  return 0;
}
