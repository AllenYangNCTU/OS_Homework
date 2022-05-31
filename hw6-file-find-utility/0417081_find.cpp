#include <iostream>
#include <getopt.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>

using namespace std;


bool isInodeSet = false;
bool isNameSet = false;
bool isMinSet = false;
bool isMaxSet = false;

int targetInodeNum = 0;
string targetName = "";
int sizeMin = 0;
int sizeMax = 0;

void printFileInfo(const string path, const string name) {

  if ( isNameSet && (name.compare(targetName) != 0) ) return;

  struct stat buf;
  stat((path + "/" + name).c_str(), &buf);

  if ( isInodeSet && buf.st_ino != targetInodeNum ) return;

  double sizeInMiB = buf.st_size / 1048576.0; // convert from bytes to MiB
  if ( isMinSet && sizeInMiB < sizeMin ) return;
  if ( isMaxSet && sizeInMiB > sizeMax ) return;

  // Should print
  cout << path << "/" << name << " " << buf.st_ino << " " << fixed << setprecision(2) << sizeInMiB << " MB" << endl;
}


void listdir(const char *name) {
  DIR *dir;
  struct dirent *entry;

  if (!(dir = opendir(name)))
    return;

  while ((entry = readdir(dir)) != NULL) {

    // Skit '.' and '..'
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;
    // Print infomations if is target
    printFileInfo(name, entry->d_name);

    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);

    struct stat buf;
    stat(path, &buf);

    if (S_ISDIR(buf.st_mode)) {
      // is directory, recursivly list
      listdir(path);
    }
  }

  closedir(dir);
}

int main ( int argc, char* argv[] ) {

  struct option opts[] = {{"inode", required_argument, NULL, 'i'},
                          {"name", required_argument, NULL, 'n'},
                          {"size_min", required_argument, NULL, 'b'},
                          {"size_max", required_argument, NULL, 'a'},
                          {NULL, 0, NULL, 0}};

  const char* optstring = "i:n:b:a:";
  int opt, index;
  
  stringstream ss;

  char* path = argv[1];

  while ((opt = getopt_long_only(argc, argv, optstring, opts, &index)) != -1 ) {

    ss.clear();
    ss.str(optarg);
    switch (opt) {
      case 'i':
        isInodeSet = true;
        ss >> targetInodeNum;
        break;
      case 'n':
        isNameSet = true;
        ss >> targetName;
        break;
      case 'b':
        isMinSet = true;
        ss >> sizeMin;
        break;
      case 'a':
        isMaxSet = true;
        ss >> sizeMax;
        break;
    }
  }

  listdir(path);

  return 0;
}
