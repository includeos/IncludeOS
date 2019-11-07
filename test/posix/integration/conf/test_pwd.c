
#include <pwd.h>
#include <stdio.h>

void print_pwd_entry(struct passwd* pwd_ent);

void test_pwd() {
  struct passwd* pwd_ent;

  printf("All entries:\n");
  while((pwd_ent = getpwent()) != NULL) {
    print_pwd_entry(pwd_ent);
  }

  printf("Back to start!\n");
  setpwent();

  printf("First entry again:\n");
  pwd_ent = getpwent();
  print_pwd_entry(pwd_ent);
  endpwent();

  printf("Entry for user 'root':\n");
  pwd_ent = getpwnam("root");
  if (pwd_ent != NULL) {
    print_pwd_entry(pwd_ent);
  }

  printf("Entry for user 'alfred'\n");
  pwd_ent = getpwnam("alfred");
  if (pwd_ent != NULL) {
    print_pwd_entry(pwd_ent);
  }

  printf("Entry for user with uid 0\n");
  pwd_ent = getpwuid(0);
  if (pwd_ent != NULL) {
    print_pwd_entry(pwd_ent);
  }

  printf("Entry for user with uid 65534\n");
  pwd_ent = getpwuid(65534);
  if (pwd_ent != NULL) {
    print_pwd_entry(pwd_ent);
  }

  printf("Entry for user with uid 666\n");
  pwd_ent = getpwuid(666);
  if (pwd_ent != NULL) {
    print_pwd_entry(pwd_ent);
  }
}

void print_pwd_entry(struct passwd* pwd_ent) {
  printf("Name: %s\n", pwd_ent->pw_name);
  printf("Uid: %ld\n", (long)pwd_ent->pw_uid);
  printf("Gid: %d\n", pwd_ent->pw_gid);
  printf("Path: %s\n", pwd_ent->pw_dir);
  printf("Shell: %s\n\n", pwd_ent->pw_shell);
}
