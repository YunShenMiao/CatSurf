#ifndef STATUSCODES_HPP
#define STATUSCODES_HPP

enum ErrorCode
{
    BadRequest = 400,
    Forbidden = 403,
    NotFound = 404,
    MethodNotAllowed = 405,
    PayloadTooLarge = 413,
    InternalServerError = 500,
    NotImplemented = 501,
    HTTPVersionNotSupported = 505
};

#endif