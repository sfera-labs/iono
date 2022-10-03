/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil;  c-file-style: "k&r"; c-basic-offset: 2; -*-

   Webduino, a simple Arduino web server
   Copyright 2009-2014 Ben Combee, Ran Talbott, Christopher Lee, Martin Lormes
   Francisco M Cuenca-Acuna

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include <WebServer.h>

WebServer::WebServer(const char *urlPrefix, uint16_t port) :
  m_server(port),
  m_client(),
  m_urlPrefix(urlPrefix),
  m_pushbackDepth(0),
  m_contentLength(0),
  m_failureCmd(&defaultFailCmd),
  m_defaultCmd(&defaultFailCmd),
  m_cmdCount(0),
  m_urlPathCmd(NULL),
  m_bufFill(0)
{
}

P(webServerHeader) = "Server: Iono" CRLF;

void WebServer::begin()
{
  m_server.begin();
}

void WebServer::setDefaultCommand(Command *cmd)
{
  m_defaultCmd = cmd;
}

void WebServer::setFailureCommand(Command *cmd)
{
  m_failureCmd = cmd;
}

void WebServer::addCommand(const char *verb, Command *cmd)
{
  if (m_cmdCount < SIZE(m_commands))
  {
    m_commands[m_cmdCount].verb = verb;
    m_commands[m_cmdCount++].cmd = cmd;
  }
}

void WebServer::setUrlPathCommand(UrlPathCommand *cmd)
{
  m_urlPathCmd = cmd;
}

size_t WebServer::write(uint8_t ch)
{
  m_buffer[m_bufFill++] = ch;

  if(m_bufFill == sizeof(m_buffer))
  {
    m_client.write(m_buffer, sizeof(m_buffer));
    m_bufFill = 0;
  }

  return sizeof(ch);
}

size_t WebServer::write(const uint8_t *buffer, size_t size)
{
  flushBuf(); //Flush any buffered output
  return m_client.write(buffer, size);
}

void WebServer::flushBuf()
{
  if(m_bufFill > 0)
  {
    m_client.write(m_buffer, m_bufFill);
    m_bufFill = 0;
  }
}

void WebServer::writeP(const unsigned char *data, size_t length)
{
  // copy data out of program memory into local storage

  while (length--)
  {
    write(pgm_read_byte(data++));
  }
}

void WebServer::printP(const unsigned char *str)
{
  // copy data out of program memory into local storage

  while (uint8_t value = pgm_read_byte(str++))
  {
    write(value);
  }
}

void WebServer::printCRLF()
{
  print(CRLF);
}

// void WebServer::printf(char *fmt, ... )
// {
//   char tmp[128]; // resulting string limited to 128 chars
//   va_list args;
//   va_start (args, fmt );
//   vsnprintf(tmp, 128, fmt, args);
//   va_end (args);
//   print(tmp);
// }
//
// #ifdef F
// void WebServer::printf(const __FlashStringHelper *format, ... )
// {
//   char buf[128]; // resulting string limited to 128 chars
//   va_list ap;
//   va_start(ap, format);
// #ifdef __AVR__
//   vsnprintf_P(buf, sizeof(buf), (const char *)format, ap); // progmem for AVR
// #else
//   vsnprintf(buf, sizeof(buf), (const char *)format, ap); // for the rest of the world
// #endif
//   va_end(ap);
//   print(buf);
// }
// #endif

bool WebServer::dispatchCommand(ConnectionType requestType, char *verb,
        bool tail_complete)
{
  // if there is no URL, i.e. we have a prefix and it's requested without a
  // trailing slash or if the URL is just the slash
  if ((verb[0] == 0) || ((verb[0] == '/') && (verb[1] == 0)))
  {
    m_defaultCmd(*this, requestType, (char*)"", tail_complete);
    return true;
  }
  // if the URL is just a slash followed by a question mark
  // we're looking at the default command with GET parameters passed
  if ((verb[0] == '/') && (verb[1] == '?'))
  {
    verb+=2; // skip over the "/?" part of the url
    m_defaultCmd(*this, requestType, verb, tail_complete);
    return true;
  }
  // We now know that the URL contains at least one character.  And,
  // if the first character is a slash,  there's more after it.
  if (verb[0] == '/')
  {
    uint8_t i;
    char *qm_loc;
    uint16_t verb_len;
    uint8_t qm_offset;
    // Skip over the leading "/",  because it makes the code more
    // efficient and easier to understand.
    verb++;
    // Look for a "?" separating the filename part of the URL from the
    // parameters.  If it's not there, compare to the whole URL.
    qm_loc = strchr(verb, '?');
    verb_len = (qm_loc == NULL) ? strlen(verb) : (qm_loc - verb);
    qm_offset = (qm_loc == NULL) ? 0 : 1;
    for (i = 0; i < m_cmdCount; ++i)
    {
      if ((verb_len == strlen(m_commands[i].verb))
          && (strncmp(verb, m_commands[i].verb, verb_len) == 0))
      {
        // Skip over the "verb" part of the URL (and the question
        // mark, if present) when passing it to the "action" routine
        m_commands[i].cmd(*this, requestType,
        verb + verb_len + qm_offset,
        tail_complete);
        return true;
      }
    }
    // Check if UrlPathCommand is assigned.
    if (m_urlPathCmd != NULL)
    {
      // Initialize with null bytes, so number of parts can be determined.
      char *url_path[WEBDUINO_URL_PATH_COMMAND_LENGTH] = {0};
      uint8_t part = 0;

      // URL path should be terminated with null byte.
      *(verb + verb_len) = 0;

      // First URL path part is at the start of verb.
      url_path[part++] = verb;
      // Replace all slashes ('/') with a null byte so every part of the URL
      // path is a seperate string. Add every char following a '/' as a new
      // part of the URL, even if that char is a '/' (which will be replaced
      // with a null byte).
      for (char * p = verb; p < verb + verb_len; p++)
      {
        if (*p == '/')
        {
          *p = 0;
          url_path[part++] = p + 1;
          // Don't try to assign out of array bounds.
          if (part == WEBDUINO_URL_PATH_COMMAND_LENGTH) break;
        }
      }
      m_urlPathCmd(*this, requestType, url_path,
                   verb + verb_len + qm_offset, tail_complete);
      return true;
    }
  }
  return false;
}

// processConnection with a default buffer
void WebServer::processConnection()
{
  char request[WEBDUINO_DEFAULT_REQUEST_LENGTH];
  int  request_len = WEBDUINO_DEFAULT_REQUEST_LENGTH;
  processConnection(request, &request_len);
}

void WebServer::processConnection(char *buff, int *bufflen)
{
  int urlPrefixLen = strlen(m_urlPrefix);

  m_client = m_server.available();

  if (m_client) {
    m_readingContent = false;
    buff[0] = 0;
    ConnectionType requestType = INVALID;
#if WEBDUINO_SERIAL_DEBUGGING > 1
    Serial.println("*** checking request ***");
#endif
    getRequest(requestType, buff, bufflen);
#if WEBDUINO_SERIAL_DEBUGGING > 1
    Serial.print("*** requestType = ");
    Serial.print((int)requestType);
    Serial.print(", request = \"");
    Serial.print(buff);
    Serial.println("\" ***");
#endif

    // don't even look further at invalid requests.
    // this is done to prevent Webduino from hanging
    // - when there are illegal requests,
    // - when someone contacts it through telnet rather than proper HTTP,
    // - etc.
    if (requestType != INVALID)
    {
      processHeaders();
#if WEBDUINO_SERIAL_DEBUGGING > 1
      Serial.println("*** headers complete ***");
#endif

      if (strcmp(buff, "/robots.txt") == 0)
      {
        noRobots(requestType);
      }
      else if (strcmp(buff, "/favicon.ico") == 0)
      {
        favicon(requestType);
      }
    }
    // Only try to dispatch command if request type and prefix are correct.
    // Fix by quarencia.
    if (requestType == INVALID ||
        strncmp(buff, m_urlPrefix, urlPrefixLen) != 0)
    {
      m_failureCmd(*this, requestType, buff, (*bufflen) >= 0);
    }
    else if (!dispatchCommand(requestType, buff + urlPrefixLen,
             (*bufflen) >= 0))
    {
      m_failureCmd(*this, requestType, buff, (*bufflen) >= 0);
    }

    flushBuf();

#if WEBDUINO_SERIAL_DEBUGGING > 1
    Serial.println("*** stopping connection ***");
#endif
    reset();
  }
}

bool WebServer::checkCredentials(const char authCredentials[45])
{
  char basic[7] = "Basic ";
  if((0 == strncmp(m_authCredentials,basic,6)) &&
     (0 == strcmp(authCredentials, m_authCredentials + 6))) return true;
  return false;
}

void WebServer::httpFail()
{
  P(failMsg1) = "HTTP/1.0 400 Bad Request" CRLF;
  printP(failMsg1);

#ifndef WEBDUINO_SUPRESS_SERVER_HEADER
  printP(webServerHeader);
#endif

  P(failMsg2) =
    "Content-Type: text/html" CRLF
    CRLF
    WEBDUINO_FAIL_MESSAGE;

  printP(failMsg2);
}

void WebServer::defaultFailCmd(WebServer &server,
                               WebServer::ConnectionType type,
                               char *url_tail,
                               bool tail_complete)
{
  server.httpFail();
}

void WebServer::noRobots(ConnectionType type)
{
  httpSuccess("text/plain");
  if (type != HEAD)
  {
    P(allowNoneMsg) = "User-agent: *" CRLF "Disallow: /" CRLF;
    printP(allowNoneMsg);
  }
}

void WebServer::favicon(ConnectionType type)
{
  httpSuccess("image/x-icon","Cache-Control: max-age=31536000\r\n");
  if (type != HEAD)
  {
    P(faviconIco) = WEBDUINO_FAVICON_DATA;
    writeP(faviconIco, sizeof(faviconIco));
  }
}

// void WebServer::httpUnauthorized()
// {
//   P(unauthMsg1) = "HTTP/1.0 401 Authorization Required" CRLF;
//   printP(unauthMsg1);

// #ifndef WEBDUINO_SUPRESS_SERVER_HEADER
//   printP(webServerHeader);
// #endif

//   P(unauthMsg2) =
//     "Content-Type: text/html" CRLF
//     "WWW-Authenticate: Basic realm=\"" WEBDUINO_AUTH_REALM "\"" CRLF
//     CRLF
//     WEBDUINO_AUTH_MESSAGE;

//   printP(unauthMsg2);
// }

// void WebServer::httpServerError()
// {
//   P(servErrMsg1) = "HTTP/1.0 500 Internal Server Error" CRLF;
//   printP(servErrMsg1);

// #ifndef WEBDUINO_SUPRESS_SERVER_HEADER
//   printP(webServerHeader);
// #endif

//   P(servErrMsg2) =
//     "Content-Type: text/html" CRLF
//     CRLF
//     WEBDUINO_SERVER_ERROR_MESSAGE;

//   printP(servErrMsg2);
// }

// void WebServer::httpNoContent()
// {
//   P(noContentMsg1) = "HTTP/1.0 204 NO CONTENT" CRLF;
//   printP(noContentMsg1);

// #ifndef WEBDUINO_SUPRESS_SERVER_HEADER
//   printP(webServerHeader);
// #endif

//   P(noContentMsg2) =
//     CRLF
//     CRLF;

//   printP(noContentMsg2);
// }

void WebServer::httpSuccess(const char *contentType,
                            const char *extraHeaders)
{
  P(successMsg1) = "HTTP/1.0 200 OK" CRLF;
  printP(successMsg1);

#ifndef WEBDUINO_SUPRESS_SERVER_HEADER
  printP(webServerHeader);
#endif

  P(successMsg2) =
    "Access-Control-Allow-Origin: *" CRLF
    "Content-Type: ";

  printP(successMsg2);
  print(contentType);
  printCRLF();
  if (extraHeaders)
    print(extraHeaders);
  printCRLF();
}

void WebServer::httpSeeOther(const char *otherURL)
{
  P(seeOtherMsg1) = "HTTP/1.0 303 See Other" CRLF;
  printP(seeOtherMsg1);

#ifndef WEBDUINO_SUPRESS_SERVER_HEADER
  printP(webServerHeader);
#endif

  P(seeOtherMsg2) = "Location: ";
  printP(seeOtherMsg2);
  print(otherURL);
  printCRLF();
  printCRLF();
}

int WebServer::read()
{
  if (!m_client)
    return -1;

  if (m_pushbackDepth == 0)
  {
    unsigned long timeoutTime = millis() + WEBDUINO_READ_TIMEOUT_IN_MS;

    while (m_client.connected())
    {
      // stop reading the socket early if we get to content-length
      // characters in the POST.  This is because some clients leave
      // the socket open because they assume HTTP keep-alive.
      if (m_readingContent)
      {
        if (m_contentLength == 0)
        {
#if WEBDUINO_SERIAL_DEBUGGING > 1
          Serial.println("\n*** End of content, terminating connection");
#endif
          return -1;
        }
      }

      int ch = m_client.read();

      // if we get a character, return it, otherwise continue in while
      // loop, checking connection status
      if (ch != -1)
      {
        // count character against content-length
        if (m_readingContent)
        {
          --m_contentLength;
        }

#if WEBDUINO_SERIAL_DEBUGGING
        if (ch == '\r')
          Serial.print("<CR>");
        else if (ch == '\n')
          Serial.println("<LF>");
        else
          Serial.print((char)ch);
#endif
        return ch;
      }
      else
      {
        unsigned long now = millis();
        if (now > timeoutTime)
        {
          // connection timed out, destroy client, return EOF
#if WEBDUINO_SERIAL_DEBUGGING
          Serial.println("*** Connection timed out");
#endif
          reset();
          return -1;
        }
      }
    }

    // connection lost, return EOF
#if WEBDUINO_SERIAL_DEBUGGING
    Serial.println("*** Connection lost");
#endif
    return -1;
  }
  else
    return m_pushback[--m_pushbackDepth];
}

void WebServer::push(int ch)
{
  // don't allow pushing EOF
  if (ch == -1)
    return;

  m_pushback[m_pushbackDepth++] = ch;
  // can't raise error here, so just replace last char over and over
  if (m_pushbackDepth == SIZE(m_pushback))
    m_pushbackDepth = SIZE(m_pushback) - 1;
}

void WebServer::reset()
{
  m_pushbackDepth = 0;
  m_client.flush();
  m_client.stop();
}

bool WebServer::expect(const char *str)
{
  const char *curr = str;
  while (*curr != 0)
  {
    int ch = read();
    if (ch != *curr++)
    {
      // push back ch and the characters we accepted
      push(ch);
      while (--curr != str)
        push(curr[-1]);
      return false;
    }
  }
  return true;
}

bool WebServer::readInt(int &number)
{
  bool negate = false;
  bool gotNumber = false;
  int ch;
  number = 0;

  // absorb whitespace
  do
  {
    ch = read();
  } while (ch == ' ' || ch == '\t');

  // check for leading minus sign
  if (ch == '-')
  {
    negate = true;
    ch = read();
  }

  // read digits to update number, exit when we find non-digit
  while (ch >= '0' && ch <= '9')
  {
    gotNumber = true;
    number = number * 10 + ch - '0';
    ch = read();
  }

  push(ch);
  if (negate)
    number = -number;
  return gotNumber;
}

void WebServer::readHeader(char *value, int valueLen)
{
  int ch;
  memset(value, 0, valueLen);
  --valueLen;

  // absorb whitespace
  do
  {
    ch = read();
  } while (ch == ' ' || ch == '\t');

  // read rest of line
  do
  {
    if (valueLen > 1)
    {
      *value++=ch;
      --valueLen;
    }
    ch = read();
  } while (ch != '\r');
  push(ch);
}

bool WebServer::readPOSTparam(char *name, int nameLen,
                              char *value, int valueLen)
{
  // assume name is at current place in stream
  int ch;
  // to not to miss the last parameter
  bool foundSomething = false;

  // clear out name and value so they'll be NUL terminated
  memset(name, 0, nameLen);
  memset(value, 0, valueLen);

  // decrement length so we don't write into NUL terminator
  --nameLen;
  --valueLen;

  while ((ch = read()) != -1)
  {
    foundSomething = true;
    if (ch == '+')
    {
      ch = ' ';
    }
    else if (ch == '=')
    {
      /* that's end of name, so switch to storing in value */
      nameLen = 0;
      continue;
    }
    else if (ch == '&')
    {
      /* that's end of pair, go away */
      return true;
    }
    else if (ch == '%')
    {
      /* handle URL encoded characters by converting back to original form */
      int ch1 = read();
      int ch2 = read();
      if (ch1 == -1 || ch2 == -1)
        return false;
      char hex[3] = { (char)ch1, (char)ch2, '\0' };
      ch = strtoul(hex, NULL, 16);
    }

    // output the new character into the appropriate buffer or drop it if
    // there's no room in either one.  This code will malfunction in the
    // case where the parameter name is too long to fit into the name buffer,
    // but in that case, it will just overflow into the value buffer so
    // there's no harm.
    if (nameLen > 0)
    {
      *name++ = ch;
      --nameLen;
    }
    else if (valueLen > 0)
    {
      *value++ = ch;
      --valueLen;
    }
  }

  if (foundSomething)
  {
    // if we get here, we have one last parameter to serve
    return true;
  }
  else
  {
    // if we get here, we hit the end-of-file, so POST is over and there
    // are no more parameters
    return false;
  }
}

/* Retrieve a parameter that was encoded as part of the URL, stored in
 * the buffer pointed to by *tail.  tail is updated to point just past
 * the last character read from the buffer. */
URLPARAM_RESULT WebServer::nextURLparam(char **tail, char *name, int nameLen,
                                        char *value, int valueLen)
{
  // assume name is at current place in stream
  char ch, hex[3];
  URLPARAM_RESULT result = URLPARAM_OK;
  char *s = *tail;
  bool keep_scanning = true;
  bool need_value = true;

  // clear out name and value so they'll be NUL terminated
  memset(name, 0, nameLen);
  memset(value, 0, valueLen);

  if (*s == 0)
    return URLPARAM_EOS;
  // Read the keyword name
  while (keep_scanning)
  {
    ch = *s++;
    switch (ch)
    {
    case 0:
      s--;  // Back up to point to terminating NUL
      // Fall through to "stop the scan" code
    case '&':
      /* that's end of pair, go away */
      keep_scanning = false;
      need_value = false;
      break;
    case '+':
      ch = ' ';
      break;
    case '%':
      /* handle URL encoded characters by converting back
       * to original form */
      if ((hex[0] = *s++) == 0)
      {
        s--;        // Back up to NUL
        keep_scanning = false;
        need_value = false;
      }
      else
      {
        if ((hex[1] = *s++) == 0)
        {
          s--;  // Back up to NUL
          keep_scanning = false;
          need_value = false;
        }
        else
        {
          hex[2] = 0;
          ch = strtoul(hex, NULL, 16);
        }
      }
      break;
    case '=':
      /* that's end of name, so switch to storing in value */
      keep_scanning = false;
      break;
    }


    // check against 1 so we don't overwrite the final NUL
    if (keep_scanning && (nameLen > 1))
    {
      *name++ = ch;
      --nameLen;
    }
    else if(keep_scanning)
      result = URLPARAM_NAME_OFLO;
  }

  if (need_value && (*s != 0))
  {
    keep_scanning = true;
    while (keep_scanning)
    {
      ch = *s++;
      switch (ch)
      {
      case 0:
        s--;  // Back up to point to terminating NUL
              // Fall through to "stop the scan" code
      case '&':
        /* that's end of pair, go away */
        keep_scanning = false;
        need_value = false;
        break;
      case '+':
        ch = ' ';
        break;
      case '%':
        /* handle URL encoded characters by converting back to original form */
        if ((hex[0] = *s++) == 0)
        {
          s--;  // Back up to NUL
          keep_scanning = false;
          need_value = false;
        }
        else
        {
          if ((hex[1] = *s++) == 0)
          {
            s--;  // Back up to NUL
            keep_scanning = false;
            need_value = false;
          }
          else
          {
            hex[2] = 0;
            ch = strtoul(hex, NULL, 16);
          }

        }
        break;
      }


      // check against 1 so we don't overwrite the final NUL
      if (keep_scanning && (valueLen > 1))
      {
        *value++ = ch;
        --valueLen;
      }
      else if(keep_scanning)
        result = (result == URLPARAM_OK) ?
          URLPARAM_VALUE_OFLO :
          URLPARAM_BOTH_OFLO;
    }
  }
  *tail = s;
  return result;
}



// Read and parse the first line of the request header.
// The "command" (GET/HEAD/POST) is translated into a numeric value in type.
// The URL is stored in request,  up to the length passed in length
// NOTE 1: length must include one byte for the terminating NUL.
// NOTE 2: request is NOT checked for NULL,  nor length for a value < 1.
// Reading stops when the code encounters a space, CR, or LF.  If the HTTP
// version was supplied by the client,  it will still be waiting in the input
// stream when we exit.
//
// On return, length contains the amount of space left in request.  If it's
// less than 0,  the URL was longer than the buffer,  and part of it had to
// be discarded.

void WebServer::getRequest(WebServer::ConnectionType &type,
                           char *request, int *length)
{
  --*length; // save room for NUL

  type = INVALID;

  // store the HTTP method line of the request
  if (expect("GET "))
    type = GET;
  else if (expect("HEAD "))
    type = HEAD;
  else if (expect("POST "))
    type = POST;
  else if (expect("PUT "))
    type = PUT;
  else if (expect("DELETE "))
    type = DELETE;
  else if (expect("PATCH "))
    type = PATCH;

  // if it doesn't start with any of those, we have an unknown method
  // so just get out of here
  else
    return;

  int ch;
  while ((ch = read()) != -1)
  {
    // stop storing at first space or end of line
    if (ch == ' ' || ch == '\n' || ch == '\r')
    {
      break;
    }
    if (*length > 0)
    {
      *request = ch;
      ++request;
    }
    --*length;
  }
  // NUL terminate
  *request = 0;
}

void WebServer::processHeaders()
{
  // look for three things: the Content-Length header, the Authorization
  // header, and the double-CRLF that ends the headers.

  // empty the m_authCredentials before every run of this function.
  // otherwise users who don't send an Authorization header would be treated
  // like the last user who tried to authenticate (possibly successful)
  m_authCredentials[0]=0;

  while (1)
  {
    if (expect("Content-Length:"))
    {
      readInt(m_contentLength);
#if WEBDUINO_SERIAL_DEBUGGING > 1
      Serial.print("\n*** got Content-Length of ");
      Serial.print(m_contentLength);
      Serial.print(" ***");
#endif
      continue;
    }

    if (expect("Authorization:"))
    {
      readHeader(m_authCredentials,51);
#if WEBDUINO_SERIAL_DEBUGGING > 1
      Serial.print("\n*** got Authorization: of ");
      Serial.print(m_authCredentials);
      Serial.print(" ***");
#endif
      continue;
    }

    if (expect(CRLF CRLF))
    {
      m_readingContent = true;
      return;
    }

    // no expect checks hit, so just absorb a character and try again
    if (read() == -1)
    {
      return;
    }
  }
}

// void WebServer::outputCheckboxOrRadio(const char *element, const char *name,
//                                       const char *val, const char *label,
//                                       bool selected)
// {
//   P(cbPart1a) = "<label><input type='";
//   P(cbPart1b) = "' name='";
//   P(cbPart2) = "' value='";
//   P(cbPart3) = "' ";
//   P(cbChecked) = "checked ";
//   P(cbPart4) = "/> ";
//   P(cbPart5) = "</label>";

//   printP(cbPart1a);
//   print(element);
//   printP(cbPart1b);
//   print(name);
//   printP(cbPart2);
//   print(val);
//   printP(cbPart3);
//   if (selected)
//     printP(cbChecked);
//   printP(cbPart4);
//   print(label);
//   printP(cbPart5);
// }

// void WebServer::checkBox(const char *name, const char *val,
//                          const char *label, bool selected)
// {
//   outputCheckboxOrRadio("checkbox", name, val, label, selected);
// }

// void WebServer::radioButton(const char *name, const char *val,
//                             const char *label, bool selected)
// {
//   outputCheckboxOrRadio("radio", name, val, label, selected);
// }

uint8_t WebServer::available(){
  return m_server.available();
}
