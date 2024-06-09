#include <alsa/asoundlib.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

const char *mzspncstr = "%s:%d In function '%s'";

#define MZSPNC(ARG) mzpnc(ARG, 1, mzspncstr, __FILE__, __LINE__, __func__)

void mzpnc(bool cnd, int code, const char *msg, ...) {
  if (!cnd) {
    return;
  }
  printf("> Panic: ");
  va_list args;
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
  printf("\n");
  fflush(stdout);
  exit(code);
}

typedef struct {
  const char *dev;
  snd_rawmidi_t *hnd;
  pthread_mutex_t mtx;
  bool work;
} mzglobal_t;

mzglobal_t mzg = {.dev = "hw:CARD=USBMIDI", .work = true};

void *mzmidi(void *) {
  struct timespec time;
  time.tv_sec = 0;
  time.tv_nsec = 1'000'000;
  unsigned char buf[3];
  ssize_t read;
  while (true) {
    while ((read = snd_rawmidi_read(mzg.hnd, buf, sizeof(buf))) > 0) {
      printf("> Message read with %li bytes\n", read);
      for (ssize_t i = 0; i < read; i++) {
        printf("[%02x]", buf[i]);
      }
      printf("\n");
    }
    MZSPNC(read != -EAGAIN);
    MZSPNC(pthread_mutex_lock(&mzg.mtx) != 0);
    if (!mzg.work) {
      MZSPNC(pthread_mutex_unlock(&mzg.mtx) != 0);
      break;
    }
    MZSPNC(pthread_mutex_unlock(&mzg.mtx) != 0);
    MZSPNC(nanosleep(&time, NULL) == -1);
  }
  return NULL;
}

int main(void) {
  MZSPNC(true);
  pthread_mutex_init(&mzg.mtx, NULL);
  MZSPNC(snd_rawmidi_open(&mzg.hnd, NULL, mzg.dev, 0) < 0);
  MZSPNC(snd_rawmidi_nonblock(mzg.hnd, 1) < 0);
  pthread_t midithrd;
  MZSPNC(pthread_create(&midithrd, NULL, mzmidi, NULL) != 0);
  struct timespec time;
  time.tv_sec = 3;
  time.tv_nsec = 0;
  MZSPNC(nanosleep(&time, NULL) == -1);
  MZSPNC(pthread_mutex_lock(&mzg.mtx) != 0);
  mzg.work = false;
  MZSPNC(pthread_mutex_unlock(&mzg.mtx) != 0);
  MZSPNC(pthread_join(midithrd, NULL) != 0);
  MZSPNC(snd_rawmidi_close(mzg.hnd) < 0);
  return 0;
}