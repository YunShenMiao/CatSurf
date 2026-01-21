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

enum SuccessCode
{
    Ok = 200,
    Created = 201,
/*     Accepted = 202, */
    NoContent = 204
};

enum RedirectCode
{
    MovedPermanently = 301,
    Found = 302,
    SeeOther = 303
};

#endif