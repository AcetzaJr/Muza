#include <alsa/asoundlib.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define MZBLOCKS 3
#define MZFRATE 48'000
#define MZCHAN 2
#define MZLATENCY 25'000
#define MZBUFSIZE (MZCHAN) * ((MZFRATE) / 1'000) * ((MZLATENCY) / 1'000)
#define MZSPNC(ARG) mzpnc(ARG, 1, mzspncstr, __FILE__, __LINE__, __func__)

const char *mzspncstr = "%s:%d In function '%s'";

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
  float buffer[MZBUFSIZE];
  const char *mididev;
  const char *pcmdev;
  snd_rawmidi_t *midihnd;
  snd_pcm_t *pcmhnd;
  pthread_mutex_t mtx;
  pthread_t midithrd;
  pthread_t pcmthrd;
  int channels;
  int frate;
  bool work;
} mzg_t;

mzg_t mzg = {.pcmdev = "default",
             .mididev = "hw:CARD=USBMIDI",
             .work = true,
             .channels = 2,
             .frate = MZFRATE};

bool mzworking() {
  bool working;
  MZSPNC(pthread_mutex_lock(&mzg.mtx) != 0);
  working = mzg.work;
  MZSPNC(pthread_mutex_unlock(&mzg.mtx) != 0);
  return working;
}

void *mzpcm(void *) {
  ssize_t bufsize = sizeof(mzg.buffer);
  printf("bufsize %li\n", bufsize);
  while (true) {
    MZSPNC(snd_pcm_writei(mzg.pcmhnd, mzg.buffer, bufsize) < 0);
    if (!mzworking()) {
      break;
    }
  }
  printf("exit\n");
  MZSPNC(snd_pcm_drain(mzg.pcmhnd) < 0);
  return NULL;
}

// 48'000 - 1'000'000
// x - 25'000

void *mzmidi(void *) {
  struct timespec time;
  time.tv_sec = 0;
  time.tv_nsec = 1'000'000;
  unsigned char buf[3];
  ssize_t read;
  while (true) {
    while ((read = snd_rawmidi_read(mzg.midihnd, buf, sizeof(buf))) > 0) {
      printf("> Message read with %li bytes\n", read);
      for (ssize_t i = 0; i < read; i++) {
        printf("[%02x]", buf[i]);
      }
      printf("\n");
    }
    MZSPNC(read != -EAGAIN);
    MZSPNC(pthread_mutex_lock(&mzg.mtx) != 0);
    if (!mzworking()) {
      break;
    }
    MZSPNC(nanosleep(&time, NULL) == -1);
  }
  return NULL;
}

void mzmidiinit() {
  MZSPNC(snd_rawmidi_open(&mzg.midihnd, NULL, mzg.mididev, 0) < 0);
  MZSPNC(snd_rawmidi_nonblock(mzg.midihnd, 1) < 0);
  MZSPNC(pthread_create(&mzg.midithrd, NULL, mzmidi, NULL) != 0);
}

void mzmidiend() {
  MZSPNC(pthread_join(mzg.midithrd, NULL) != 0);
  MZSPNC(snd_rawmidi_close(mzg.midihnd) < 0);
}

void mzinit() {
  pthread_mutex_init(&mzg.mtx, NULL);
  for (int s = 0; s < MZBUFSIZE; s++) {
    mzg.buffer[s] = 0;
  }
}

void mzpcminit() {
  MZSPNC(snd_pcm_open(&mzg.pcmhnd, mzg.pcmdev, SND_PCM_STREAM_PLAYBACK, 0) < 0);
  MZSPNC(snd_pcm_set_params(mzg.pcmhnd, SND_PCM_FORMAT_FLOAT,
                            SND_PCM_ACCESS_RW_INTERLEAVED, mzg.channels,
                            mzg.frate, 1, 25'000) < 0);
  MZSPNC(pthread_create(&mzg.pcmthrd, NULL, mzpcm, NULL) != 0);
}

void mzpcmend() {
  MZSPNC(pthread_join(mzg.pcmthrd, NULL) != 0);
  MZSPNC(snd_pcm_close(mzg.pcmhnd) < 0);
}

void mzwaitenter() {
  printf("> Press enter to exit\n");
  while (getchar() != '\n') {
  }
  MZSPNC(pthread_mutex_lock(&mzg.mtx) != 0);
  mzg.work = false;
  MZSPNC(pthread_mutex_unlock(&mzg.mtx) != 0);
}

int main(void) {
  mzinit();
  mzpcminit();
  mzwaitenter();
  mzpcmend();
  return 0;
}