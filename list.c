void MzListDevices() {
  void **hints;
  int result = snd_device_name_hint(-1, "ctl", &hints);
  if (result < 0) {
    MzPanicF(1, "snd_device_name_hint");
  }
  for (void **hint = hints; *hint != NULL; hint++) {
    char *nameL = snd_device_name_get_hint(*hint, "NAME");
    if (nameL == NULL) {
      MzPanicF(1, "NAME");
    }
    char *descL = snd_device_name_get_hint(*hint, "DESC");
    if (descL == NULL) {
      MzPanicF(1, "DESC");
    }
    char *ioidL = snd_device_name_get_hint(*hint, "IOID");
    printf("> Device\n");
    printf("* NAME: %s\n", nameL);
    printf("* DESC: %s\n", descL);
    printf("* IOID: %s\n", ioidL);
    printf("\n");
    free(nameL);
    free(descL);
    if (ioidL != NULL) {
      free(ioidL);
    }
  }
  snd_device_name_free_hint(hints);
}
