#pragma once

#include <jio/xplatform.hpp>
#include <jio/exception.hpp>
#include <jio/transports/common.hpp>
#include <jio/transports/named_pipe.hpp>
#include <jio/transports/basic_udp.hpp>

#define BEGIN_TEST_TEXT(x) #x
#define BEGIN_TEST_IMPL(x) { printf("* %s: Testing\n", BEGIN_TEST_TEXT(x) ); x (); }
#define BEGIN_TEST(x) BEGIN_TEST_IMPL(x)

void named_pipe(void);
void basic_udp(void);
void dispatcher(void);