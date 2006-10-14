/* Herve Fache

20061008 Creation
*/

#include "metadata.c"

int main(void) {
  metadata_t metadata;

  printf("metadata_get\n");
  metadata_get("test/testfile", &metadata);
  printf(" * type: 0x%08x\n", metadata.type);
  printf(" * size: %u\n", (unsigned int) metadata.size);
  printf(" * mtime: %u\n", (unsigned int) metadata.mtime);
  printf(" * uid: %u\n", metadata.uid);
  printf(" * gid: %u\n", metadata.gid);
  printf(" * mode: 0x%08x\n", metadata.mode);

  return 0;
}
