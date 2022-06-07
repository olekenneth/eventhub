#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/ossl_typ.h>
#include <spdlog/logger.h>
#include <algorithm>
#include <chrono>
#include <sstream>
#include <string>
#include <stdexcept>
#include <cctype>

#include "Util.hpp"
#include "Logger.hpp"

namespace eventhub {
const std::string Util::base64Encode(const unsigned char* buffer, size_t length) {
  BIO *bio, *b64;
  BUF_MEM* bufferPtr;
  std::string s;

  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);

  BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
  BIO_write(bio, buffer, length);
  BIO_flush(bio);
  BIO_get_mem_ptr(bio, &bufferPtr);
  s.assign(bufferPtr->data, bufferPtr->length);
  BIO_set_close(bio, BIO_CLOSE);
  BIO_free_all(bio);

  return s;
}

const std::string Util::uriDecode(const std::string& str) {
  std::ostringstream unescaped;
  for (std::string::const_iterator i = str.begin(), n = str.end(); i != n; ++i) {
    std::string::value_type c = (*i);
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '*' || c == '/' || c == '+') {
      unescaped << c;
    } else if (c == '%') {
      // throw error if string is invalid and doesn't have 2 char after,
      // or if it has non-hex chars here (courtesy GitHub @scinart)
      if (i + 2 >= n || !isxdigit(*(i + 1)) || !isxdigit(*(i + 2))) {
        LOG->trace("urlDecode: Invalid percent-encoding in string: '{}'.", str);
        return "";
      }

      // decode a URL-encoded ASCII character, e.g. %40 => &
      char ch1        = *(i + 1);
      char ch2        = *(i + 2);
      int hex1        = (isdigit(ch1) ? (ch1 - '0') : (toupper(ch1) - 'A' + 10));
      int hex2        = (isdigit(ch2) ? (ch2 - '0') : (toupper(ch2) - 'A' + 10));
      int decodedChar = (hex1 << 4) + hex2;
      unescaped << static_cast<char>(decodedChar);
      i += 2;
    } else {
      std::ostringstream msg;
      LOG->trace("urlDecode: Unexpected character in string: {} ({}).", static_cast<int>(c), c);
      return "";
    }
  }

  return unescaped.str();
}

std::string& Util::strToLower(std::string& s) {
  std::transform(s.begin(), s.end(), s.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  return s;
}

int64_t Util::getTimeSinceEpoch() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string Util::getSSLErrorString(unsigned long e) {
  char buf[512] = {'\0'};
  ERR_error_string_n(e, buf, 512);
  ERR_clear_error();
  return buf;
}

std::string Util::getFileMD5Sum(const std::string& filePath) {
  char buf[EVP_MAX_MD_SIZE] = { '\0' };
  std::stringstream out;

  auto bio = BIO_new(BIO_s_null());
  auto sha1 = BIO_new(BIO_f_md());
  BIO_set_md(sha1, EVP_md5());
  bio = BIO_push(sha1, bio);

  auto fd = open(filePath.c_str(), O_RDONLY);
  if (!fd) {
    throw new std::runtime_error("getFileMD5Sum: no such file");
  }

  int rdLen = 0;

  for(;;) {
    rdLen = read(fd, &buf, sizeof(buf));

    if (rdLen > 0)
      BIO_write(bio, buf, rdLen);
    else
      break;
  }

  close(fd);

  auto mdLen = BIO_gets(bio, buf, sizeof(buf));
  for (int i = 0; i < mdLen; i++) {
    char hex[3];
    sprintf(hex, "%02x", buf[i] & 0xFF);
    out << hex;
  }

  BIO_free_all(bio);

  return out.str();
}
} // namespace eventhub
