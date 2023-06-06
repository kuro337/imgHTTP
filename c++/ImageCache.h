#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#include <sqlite3.h>
#include <string>

class ImageCache {
public:
  ImageCache(const std::string &dbPath);
  ~ImageCache();

  void createTable();
  bool checkImage(const std::string &imageLink);
  void addImage(const std::string &imageLink, const std::string &path);
  std::string getImage(const std::string &imageLink);

private:
  sqlite3 *db;
};

#endif
