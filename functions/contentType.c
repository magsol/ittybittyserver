#ifndef _CONTENTTYPE_
#define _CONTENTTYPE_

#include <stdlib.h>
#include <string.h>

/* Given the name of the file requested, this function performs simple
 * string operations in order to determine the file type, specifically
 * regarding the file's extension.  Simply changing the extension of
 * the file will fool this function!
 *
 * @param file The name of the file whose type is to be determined.
 * @return The HTML-compliant file type.  Example: "text/html"
 */
char* contentType(char* file) {
  char* dot;
  dot = strrchr(file, '.');

  /* begin the ifs */
  if ( dot == (char*) 0 ) {
    return "text/plain; charset=iso-8859-1";
  } else if ( strcmp( dot, ".html" ) == 0 || strcmp( dot, ".htm" ) == 0 ) {
    return "text/html; charset=iso-8859-1";
  } else if ( strcmp( dot, ".jpg" ) == 0 || strcmp( dot, ".jpeg" ) == 0 ) {
    return "image/jpeg";
  } else if ( strcmp( dot, ".gif" ) == 0 ) {
    return "image/gif";
  } else if ( strcmp( dot, ".png" ) == 0 ) {
    return "image/png";
  } else if ( strcmp( dot, ".css" ) == 0 ) {
    return "text/css";
  } else if ( strcmp( dot, ".au" ) == 0 ) {
    return "audio/basic";
  } else if ( strcmp( dot, ".wav" ) == 0 ) {
    return "audio/wav";
  } else if ( strcmp( dot, ".avi" ) == 0 ) {
    return "video/x-msvideo";
  } else if ( strcmp( dot, ".mov" ) == 0 || strcmp( dot, ".qt" ) == 0 ) {
    return "video/quicktime";
  } else if ( strcmp( dot, ".mpeg" ) == 0 || strcmp( dot, ".mpe" ) == 0 ) {
    return "video/mpeg";
  } else if ( strcmp( dot, ".vrml" ) == 0 || strcmp( dot, ".wrl" ) == 0 ) {
    return "model/vrml";
  } else if ( strcmp( dot, ".midi" ) == 0 || strcmp( dot, ".mid" ) == 0 ) {
    return "audio/midi";
  } else if ( strcmp( dot, ".mp3" ) == 0 ) {
    return "audio/mpeg";
  } else if ( strcmp( dot, ".ogg" ) == 0 ) {
    return "application/ogg";
  } else if ( strcmp( dot, ".pac" ) == 0 ) {
    return "application/x-ns-proxy-autoconfig";
  } else { /* no idea what it is */
    return "text/plain; charset=iso-8859-1";
  }
}

#endif /* _CONTENTTYPE_ */
