#include "protocol.h"
#include "dictionary.h"
#include "format.h"

const char* http_status_to_string(http_status status) {
    switch ( status ) {
        case STATUS_OK:
            return "OK";
        case STATUS_CREATED:
            return "Created";
        case STATUS_ACCEPTED:
            return "Accepted";
        case STATUS_NO_CONTENT:
            return "No Content";
        case STATUS_MOVED_PERMANENTLY:
            return "Moved Permanently";
        case STATUS_FOUND:
            return "Found";
        case STATUS_SEE_OTHER:
            return "See Other";
        case STATUS_NOT_MODIFIED:
            return "Not Modified";
        case STATUS_TEMPORARY_REDIRECT:
            return "Temporary Redirect";
        case STATUS_PERMANENT_REDIRECT:
            return "Permanent Redirect";
        case STATUS_BAD_REQUEST:
            return "Bad Request";
        case STATUS_UNAUTHORIZED:
            return "Unauthorized";
        case STATUS_FORBIDDEN:
            return "Forbidden";
        case STATUS_NOT_FOUND:
            return "Not Found";
        case STATUS_METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case STATUS_REQUEST_TIMEOUT:
            return "Request Timeout";
        case STATUS_LENGTH_REQUIRED:
            return "Length Required";
        case STATUS_PAYLOAD_TOO_LARGE:
            return "Payload Too Large";
        case STATUS_URI_TOO_LONG:
            return "URI Too Long";
        case STATUS_UNSUPPORTED_MEDIA_TYPE:
            return "Unsupported Media Type";
        case STATUS_IM_A_TEAPOT:
            return "I'm a teapot";
        case STATUS_TOO_MANY_REQUESTS:
            return "Too Many Requests";
        case STATUS_INTERNAL_SERVER_ERROR:
            return "Internal Server Error";
        case STATUS_NOT_IMPLEMENTED:
            return "Not Implemented";
        case STATUS_BAD_GATEWAY:
            return "Bad Gateway";
        case STATUS_SERVICE_UNAVAILABLE:
            return "Service Unavailable";
        case STATUS_GATEWAY_TIMEOUT:
            return "Gateway Timeout";
        case STATUS_HTTP_VERSION_NOT_SUPPORTED:
            return "HTTP Version Not Supported";
        case STATUS_INSUFFICIENT_STORAGE:
            return "Insufficient Storage";
        default:
            WARN("Unknown http_status enum variant: %d", status);
            return "";
    }    
}

const char* http_method_to_string(http_method method) {
    switch ( method ) {
        case HTTP_GET:
            return "GET";
        case HTTP_HEAD:
            return "HEAD";
        case HTTP_POST:
            return "POST";
        case HTTP_PUT:
            return "PUT";
        case HTTP_DELETE:
            return "DELETE";
        case HTTP_CONNECT:
            return "CONNECT";
        case HTTP_OPTIONS:
            return "OPTIONS";
        case HTTP_TRACE:
            return "TRACE";
        default:
            WARN("Unknown http_method enum variant: %d", method);
            return "";
    }
}

// Adapted from https://stackoverflow.com/a/30895866
int url_decode(char* out, const char* in) {
    static const char tbl[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
         0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    char c, v1, v2, *beg=out;
    if(in != NULL) {
        while((c=*in++) != '\0') {
            if(c == '%') {
                if((v1=tbl[(unsigned char)*in++])<0 || 
                   (v2=tbl[(unsigned char)*in++])<0) {
                    *beg = '\0';
                    return -1;
                }
                c = (v1<<4)|v2;
            }
            *out++ = c;
        }
    }
    *out = '\0';
    return 0;
}