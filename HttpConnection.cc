/*
 * Copyright ©2023 Justin Hsia.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Winter Quarter 2023 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
 */

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>
#include <iostream>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;
using std::vector;
using boost::algorithm::split;
using boost::algorithm::token_compress_on;
using boost::trim;
using boost::is_any_of;
using boost::to_lower;

#define BUFFER_LEN 256

namespace hw4 {

static const char* kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;

bool HttpConnection::GetNextRequest(HttpRequest* const request) {
  // Use WrappedRead from HttpUtils.cc to read bytes from the files into
  // private buffer_ variable. Keep reading until:
  // 1. The connection drops
  // 2. You see a "\r\n\r\n" indicating the end of the request header.
  //
  // Hint: Try and read in a large amount of bytes each time you call
  // WrappedRead.
  //
  // After reading complete request header, use ParseRequest() to parse into
  // an HttpRequest and save to the output parameter request.
  //
  // Important note: Clients may send back-to-back requests on the same socket.
  // This means WrappedRead may also end up reading more than one request.
  // Make sure to save anything you read after "\r\n\r\n" in buffer_ for the
  // next time the caller invokes GetNextRequest()!

  // STEP 1:
  size_t pos = buffer_.find(kHeaderEnd);
  if (pos == string::npos) {
    int bytes_read = -1;
    char buf[BUFFER_LEN];
    while (bytes_read != 0 && buffer_.find(kHeaderEnd) == string::npos) {
      bytes_read = WrappedRead(
        fd_, reinterpret_cast<unsigned char*>(buf), BUFFER_LEN);
      if (bytes_read == -1) {
        return false;
      }
      buffer_ += string(buf, bytes_read);
    }
  }

  // unable to find kHeaderEnd
  pos = buffer_.find(kHeaderEnd);
  if (pos == string::npos) {
    return false;
  }

  *request = ParseRequest(buffer_.substr(0, pos));

  buffer_.erase(0, pos + kHeaderEndLen);

  return true;  // You may want to change this.
}

bool HttpConnection::WriteResponse(const HttpResponse& response) const {
  string str = response.GenerateResponseString();
  int res = WrappedWrite(fd_,
                         reinterpret_cast<const unsigned char*>(str.c_str()),
                         str.length());
  if (res != static_cast<int>(str.length()))
    return false;
  return true;
}

HttpRequest HttpConnection::ParseRequest(const string& request) const {
  HttpRequest req("/");  // by default, get "/".

  // Plan for STEP 2:
  // 1. Split the request into different lines (split on "\r\n").
  // 2. Extract the URI from the first line and store it in req.URI.
  // 3. For the rest of the lines in the request, track the header name and
  //    value and store them in req.headers_ (e.g. HttpRequest::AddHeader).
  //
  // Hint: Take a look at HttpRequest.h for details about the HTTP header
  // format that you need to parse.
  //
  // You'll probably want to look up boost functions for:
  // - Splitting a string into lines on a "\r\n" delimiter
  // - Trimming whitespace from the end of a string
  // - Converting a string to lowercase.
  //
  // Note: If a header is malformed, skip that line.

  // STEP 2:
  vector<string> results;
  vector<string> first;
  vector<string> body;

  split(results, request, is_any_of("\r\n"), token_compress_on);

  for (size_t i = 0; i < results.size(); i++) {
    trim(results[i]);
  }

  split(first, results[0], is_any_of(" "), token_compress_on);
  if (first.size() == 3) {
    req.set_uri(first[1]);
    results.erase(results.begin());
  }

  for (size_t i = 0; i < results.size(); i++) {
    split(body, results[i], is_any_of(": "), token_compress_on);
    if (body.size() == 2) {
      string header_name = body[0];
      to_lower(header_name);
      req.AddHeader(header_name, body[1]);
    }
  }

  return req;
}

}  // namespace hw4
