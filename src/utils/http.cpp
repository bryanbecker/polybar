#include <curl/curl.h>
#include <curl/curlbuild.h>
#include <curl/easy.h>
#include <iostream>
#include <sstream>

#include "errors.hpp"
#include "utils/http.hpp"

POLYBAR_NS

http_downloader::http_downloader(int connection_timeout) {
  m_curl = curl_easy_init();
  curl_easy_setopt(m_curl, CURLOPT_ACCEPT_ENCODING, "deflate");
  curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, connection_timeout);
  curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, true);
  curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, true);
  curl_easy_setopt(m_curl, CURLOPT_USERAGENT, "polybar/" GIT_TAG);
  curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, http_downloader::write);
}

http_downloader::~http_downloader() {
  curl_easy_cleanup(m_curl);
}

string http_downloader::get(const string& url) {
  stringstream out{};
  curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &out);

  auto res = curl_easy_perform(m_curl);
  if (res != CURLE_OK) {
    throw application_error(curl_easy_strerror(res), res);
  }

  return out.str();
}

string http_downloader::post(const string& url, const string& post_fields) {
  stringstream out{};
  curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, post_fields.c_str());
  curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &out);

  auto res = curl_easy_perform(m_curl);
  if (res != CURLE_OK) {
    throw application_error(curl_easy_strerror(res), res);
  }

  return out.str();
}

string http_downloader::post(const string& url, const string& post_fields, const string& user_auth) {
  curl_easy_setopt(m_curl, CURLOPT_USERPWD, user_auth.c_str());
  return http_downloader(url, post_fields);
}

long http_downloader::response_code() {
  long code{0};
  curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &code);
  return code;
}

size_t http_downloader::write(void* p, size_t size, size_t bytes, void* stream) {
  string data{static_cast<const char*>(p), size * bytes};
  *(static_cast<stringstream*>(stream)) << data << '\n';
  return size * bytes;
}

POLYBAR_NS_END
