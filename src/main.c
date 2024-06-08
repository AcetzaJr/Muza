#include <alsa/asoundlib.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>

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
    mzpnc(read != -EAGAIN, 1, "snd_rawmidi_read");
    mzpnc(pthread_mutex_lock(&mzg.mtx) != 0, 1, "pthread_mutex_lock");
    if (!mzg.work) {
      mzpnc(pthread_mutex_unlock(&mzg.mtx) != 0, 1, "pthread_mutex_unlock");
      break;
    }
    mzpnc(pthread_mutex_unlock(&mzg.mtx) != 0, 1, "pthread_mutex_unlock");
    mzpnc(nanosleep(&time, NULL) == -1, 1, "nanosleep");
  }
  return NULL;
}

int main(void) {
  pthread_mutex_init(&mzg.mtx, NULL);
  mzpnc(snd_rawmidi_open(&mzg.hnd, NULL, mzg.dev, 0) < 0, 1,
        "snd_rawmidi_open");
  mzpnc(snd_rawmidi_nonblock(mzg.hnd, 1) < 0, 1, "snd_rawmidi_nonblock");
  pthread_t midithrd;
  mzpnc(pthread_create(&midithrd, NULL, mzmidi, NULL) != 0, 1,
        "pthread_create");
  struct timespec time;
  time.tv_sec = 3;
  time.tv_nsec = 0;
  mzpnc(nanosleep(&time, NULL) == -1, 1, "nanosleep");
  mzpnc(pthread_mutex_lock(&mzg.mtx) != 0, 1, "pthread_mutex_lock");
  mzg.work = false;
  mzpnc(pthread_mutex_unlock(&mzg.mtx) != 0, 1, "pthread_mutex_unlock");
  mzpnc(pthread_join(midithrd, NULL) != 0, 1, "pthread_join");
  mzpnc(snd_rawmidi_close(mzg.hnd) < 0, 1, "snd_rawmidi_close");
  return 0;
}