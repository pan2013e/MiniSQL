#pragma once

#include "pch.h"
#include "server_http.hpp"
#include <boost/filesystem.hpp>
#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

void init_web_interface();