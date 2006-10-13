/* Herve Fache

20061008 Creation
*/

#include "metadata.c"

int main(void) {
  metadata_t file_data;

  printf("metadata_get\n");
  metadata_get("test/testfile", &file_data);
  printf(" * path: %s\n", file_data.path);
  printf(" * type: 0x%08x\n", file_data.type);
  printf(" * size: %u\n", (unsigned int) file_data.size);
  printf(" * mtime: %u\n", (unsigned int) file_data.mtime);
  printf(" * uid: %u\n", file_data.uid);
  printf(" * gid: %u\n", file_data.gid);
  printf(" * mode: 0x%08x\n", file_data.mode);

  return 0;
}
