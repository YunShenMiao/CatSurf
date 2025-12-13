#ifndef LOCATION_HPP
#define LOCATION_HPP

#include "configParser.hpp"

class Location
{
    private:
    LocationConfig config;

    public:
    Location();
    Location(const LocationConfig& serv);
    Location(const Location& other);
    Location& operator=(const Location& other);
    ~ConfigParser();


}