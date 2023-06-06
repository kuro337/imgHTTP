#include "ImageCache.h"
#include <iostream>
#include <sqlite3.h>
#include <string>
ImageCache::ImageCache(const std::string &dbPath) {
  int rc = sqlite3_open(dbPath.c_str(), &db);
  if (rc != SQLITE_OK) {
    std::cerr << "Cannot open database: " << sqlite3_errmsg(db);
    // Exception 
  }
}

ImageCache::~ImageCache() { sqlite3_close(db); }

void ImageCache::createTable() {
  char *err_msg = 0;
  std::string sql =
      "CREATE TABLE IF NOT EXISTS images(link TEXT PRIMARY KEY, data BLOB);";
  int rc = sqlite3_exec(db, sql.c_str(), 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to create table: " << err_msg;
    sqlite3_free(err_msg);
    // Exception 
  }
}

bool ImageCache::checkImage(const std::string &imageLink) {
  std::string sql = "SELECT COUNT(*) FROM images WHERE link = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db);
    return false;
  }

  rc = sqlite3_bind_text(stmt, 1, imageLink.c_str(), -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to bind value: " << sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    return false;
  }

  bool exists = false;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    int count = sqlite3_column_int(stmt, 0);
    exists = (count > 0);
  }

  sqlite3_finalize(stmt);
  return exists;
}

void ImageCache::addImage(const std::string &imageLink,
                          const std::string &imageData) {
  std::string sql = "INSERT INTO images (link, data) VALUES (?, ?)";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db);
    return;
  }

  rc = sqlite3_bind_text(stmt, 1, imageLink.c_str(), -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to bind value: " << sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    return;
  }

  rc = sqlite3_bind_blob(stmt, 2, imageData.data(), imageData.size(),
                         SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to bind value: " << sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    return;
  }

  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db);
  }

  sqlite3_finalize(stmt);
}

std::string ImageCache::getImage(const std::string &imageLink) {
  std::string sql = "SELECT data FROM images WHERE link = ?";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db);
    return "";
  }

  rc = sqlite3_bind_text(stmt, 1, imageLink.c_str(), -1, SQLITE_STATIC);
  if (rc != SQLITE_OK) {
    std::cerr << "Failed to bind value: " << sqlite3_errmsg(db);
    sqlite3_finalize(stmt);
    return "";
  }

  std::string imageData;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const void *data = sqlite3_column_blob(stmt, 0);
    int dataSize = sqlite3_column_bytes(stmt, 0);
    imageData = std::string(static_cast<const char *>(data), dataSize);
  }

  sqlite3_finalize(stmt);
  return imageData;
}
