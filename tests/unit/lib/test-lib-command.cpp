/*
 * test-lib-command.cpp - test commands
 *
 * Copyright (C) 2019 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat Relay.
 *
 * WeeChat Relay is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat Relay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat Relay.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

extern "C"
{
#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif
#include <unistd.h>
#include <string.h>
#include "tests/tests.h"
#include "lib/command.h"
#include "lib/weechat-relay.h"
}

#define RELAY_CMD_EQUAL(__command)                                      \
    num_read = read (fd_pipe[0], read_buffer, sizeof (read_buffer));    \
    read_buffer[num_read] = '\0';                                       \
    LONGS_EQUAL(strlen (__command), num_read);                          \
    STRCMP_EQUAL(__command, read_buffer);

struct t_weechat_relay_session relay_session;
int fd_pipe[2];
char read_buffer[4096];
ssize_t num_read;

TEST_GROUP(LibCommand)
{
    void setup()
    {
        pipe (fd_pipe);
        relay_session.sock = fd_pipe[1];
        relay_session.ssl = 0;
        relay_session.gnutls_session = NULL;
    }

    void teardown()
    {
        close (fd_pipe[0]);
        close (fd_pipe[1]);
    }
};

/*
 * Tests functions:
 *   weechat_relay_cmd_escape
 */

TEST(LibCommand, CmdEscape)
{
    char *result;

    result = weechat_relay_cmd_escape (NULL, NULL);
    POINTERS_EQUAL(NULL, result);

    result = weechat_relay_cmd_escape (NULL, "");
    POINTERS_EQUAL(NULL, result);

    result = weechat_relay_cmd_escape ("", NULL);
    POINTERS_EQUAL(NULL, result);

    result = weechat_relay_cmd_escape ("", "");
    STRCMP_EQUAL("", result);
    free (result);

    result = weechat_relay_cmd_escape ("abc", "");
    STRCMP_EQUAL("abc", result);
    free (result);

    result = weechat_relay_cmd_escape ("abc", ".,;");
    STRCMP_EQUAL("abc", result);
    free (result);

    result = weechat_relay_cmd_escape ("abc.def,ghi;jkl/mno", ",");
    STRCMP_EQUAL("abc.def\\,ghi;jkl/mno", result);
    free (result);

    result = weechat_relay_cmd_escape ("abc.def,ghi;jkl/mno", ".,;");
    STRCMP_EQUAL("abc\\.def\\,ghi\\;jkl/mno", result);
    free (result);
}

/*
 * Tests functions:
 *   weechat_relay_cmd_raw
 */

TEST(LibCommand, CmdRaw)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_raw (NULL, NULL, 0));
    LONGS_EQUAL(-1, weechat_relay_cmd_raw (&relay_session, NULL, 0));
    LONGS_EQUAL(-1, weechat_relay_cmd_raw (&relay_session, "abc", 0));

    LONGS_EQUAL(3, weechat_relay_cmd_raw (&relay_session, "abc", 3));
    RELAY_CMD_EQUAL("abc");

    LONGS_EQUAL(8, weechat_relay_cmd_raw (&relay_session, "abc def\n", 8));
    RELAY_CMD_EQUAL("abc def\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd
 */

TEST(LibCommand, Cmd)
{
    const char *one_arg[2] = { "arg1", NULL };
    const char *two_args[3] = { "arg1", "arg2", NULL };

    LONGS_EQUAL(-1, weechat_relay_cmd (NULL, NULL, NULL, NULL));
    LONGS_EQUAL(-1, weechat_relay_cmd (&relay_session, NULL, NULL, NULL));
    LONGS_EQUAL(-1, weechat_relay_cmd (NULL, "id", NULL, NULL));
    LONGS_EQUAL(-1, weechat_relay_cmd (&relay_session, "id", NULL, NULL));

    LONGS_EQUAL(5, weechat_relay_cmd (&relay_session, NULL, "test", NULL));
    RELAY_CMD_EQUAL("test\n");

    LONGS_EQUAL(10, weechat_relay_cmd (&relay_session, "id", "test", NULL));
    RELAY_CMD_EQUAL("(id) test\n");

    LONGS_EQUAL(15, weechat_relay_cmd (&relay_session, "id", "test",
                                       one_arg));
    RELAY_CMD_EQUAL("(id) test arg1\n");

    LONGS_EQUAL(20, weechat_relay_cmd (&relay_session, "id", "test",
                                       two_args));
    RELAY_CMD_EQUAL("(id) test arg1 arg2\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_init
 */

TEST(LibCommand, CmdInit)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_init (NULL, NULL, NULL));

    LONGS_EQUAL(18, weechat_relay_cmd_init (&relay_session, NULL, ""));
    RELAY_CMD_EQUAL("init compression=\n");

    LONGS_EQUAL(21, weechat_relay_cmd_init (&relay_session, NULL,
                                            WEECHAT_RELAY_COMPRESSION_OFF));
    RELAY_CMD_EQUAL("init compression=" WEECHAT_RELAY_COMPRESSION_OFF "\n");

    LONGS_EQUAL(22, weechat_relay_cmd_init (&relay_session, NULL,
                                            WEECHAT_RELAY_COMPRESSION_ZLIB));
    RELAY_CMD_EQUAL("init compression=" WEECHAT_RELAY_COMPRESSION_ZLIB "\n");

    LONGS_EQUAL(38, weechat_relay_cmd_init (&relay_session, "secret", NULL));
    RELAY_CMD_EQUAL("init password=secret,"
                    "compression=" WEECHAT_RELAY_COMPRESSION_ZLIB "\n");

    LONGS_EQUAL(34, weechat_relay_cmd_init (&relay_session, "secret", ""));
    RELAY_CMD_EQUAL("init password=secret,compression=\n");

    LONGS_EQUAL(38, weechat_relay_cmd_init (&relay_session, "secret",
                                            WEECHAT_RELAY_COMPRESSION_ZLIB));
    RELAY_CMD_EQUAL("init password=secret,"
                    "compression=" WEECHAT_RELAY_COMPRESSION_ZLIB "\n");

    LONGS_EQUAL(40, weechat_relay_cmd_init (&relay_session, "sec,ret",
                                            WEECHAT_RELAY_COMPRESSION_ZLIB));
    RELAY_CMD_EQUAL("init password=sec\\,ret,"
                    "compression=" WEECHAT_RELAY_COMPRESSION_ZLIB "\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_hdata
 */

TEST(LibCommand, CmdHdata)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_hdata (NULL, NULL, NULL, NULL));

    LONGS_EQUAL(6, weechat_relay_cmd_hdata (&relay_session, NULL, NULL, NULL));
    RELAY_CMD_EQUAL("hdata\n");

    LONGS_EQUAL(11, weechat_relay_cmd_hdata (&relay_session, "id",
                                             NULL, NULL));
    RELAY_CMD_EQUAL("(id) hdata\n");

    LONGS_EQUAL(16, weechat_relay_cmd_hdata (&relay_session, "id",
                                             "path", NULL));
    RELAY_CMD_EQUAL("(id) hdata path\n");

    LONGS_EQUAL(11, weechat_relay_cmd_hdata (&relay_session, "id",
                                             NULL, "keys"));
    RELAY_CMD_EQUAL("(id) hdata\n");

    LONGS_EQUAL(21, weechat_relay_cmd_hdata (&relay_session, "id",
                                             "path", "keys"));
    RELAY_CMD_EQUAL("(id) hdata path keys\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_info
 */

TEST(LibCommand, CmdInfo)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_info (NULL, NULL, NULL));

    LONGS_EQUAL(5, weechat_relay_cmd_info (&relay_session, NULL, NULL));
    RELAY_CMD_EQUAL("info\n");

    LONGS_EQUAL(10, weechat_relay_cmd_info (&relay_session, "id", NULL));
    RELAY_CMD_EQUAL("(id) info\n");

    LONGS_EQUAL(15, weechat_relay_cmd_info (&relay_session, "id", "name"));
    RELAY_CMD_EQUAL("(id) info name\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_infolist
 */

TEST(LibCommand, CmdInfolist)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_infolist (NULL, NULL, NULL, NULL, NULL));

    LONGS_EQUAL(9, weechat_relay_cmd_infolist (&relay_session, NULL,
                                               NULL, NULL, NULL));
    RELAY_CMD_EQUAL("infolist\n");

    LONGS_EQUAL(14, weechat_relay_cmd_infolist (&relay_session, "id",
                                                NULL, NULL, NULL));
    RELAY_CMD_EQUAL("(id) infolist\n");

    LONGS_EQUAL(19, weechat_relay_cmd_infolist (&relay_session, "id",
                                                "name", NULL, NULL));
    RELAY_CMD_EQUAL("(id) infolist name\n");

    LONGS_EQUAL(28, weechat_relay_cmd_infolist (&relay_session, "id",
                                                "name", "0x123abc", NULL));
    RELAY_CMD_EQUAL("(id) infolist name 0x123abc\n");

    LONGS_EQUAL(33, weechat_relay_cmd_infolist (&relay_session, "id",
                                                "name", "0x123abc", "args"));
    RELAY_CMD_EQUAL("(id) infolist name 0x123abc args\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_nicklist
 */

TEST(LibCommand, CmdNicklist)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_nicklist (NULL, NULL, NULL));

    LONGS_EQUAL(9, weechat_relay_cmd_nicklist (&relay_session, NULL, NULL));
    RELAY_CMD_EQUAL("nicklist\n");

    LONGS_EQUAL(14, weechat_relay_cmd_nicklist (&relay_session, "id", NULL));
    RELAY_CMD_EQUAL("(id) nicklist\n");

    LONGS_EQUAL(27, weechat_relay_cmd_nicklist (&relay_session, "id",
                                                "core.weechat"));
    RELAY_CMD_EQUAL("(id) nicklist core.weechat\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_input
 */

TEST(LibCommand, CmdInput)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_input (NULL, NULL, NULL));

    LONGS_EQUAL(6, weechat_relay_cmd_input (&relay_session, NULL, NULL));
    RELAY_CMD_EQUAL("input\n");

    LONGS_EQUAL(19, weechat_relay_cmd_input (&relay_session,
                                             "core.weechat", NULL));
    RELAY_CMD_EQUAL("input core.weechat\n");

    LONGS_EQUAL(32, weechat_relay_cmd_input (&relay_session,
                                             "core.weechat", "/help filter"));
    RELAY_CMD_EQUAL("input core.weechat /help filter\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_sync
 */

TEST(LibCommand, CmdSync)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_sync (NULL, NULL, NULL));

    LONGS_EQUAL(5, weechat_relay_cmd_sync (&relay_session, NULL, NULL));
    RELAY_CMD_EQUAL("sync\n");

    LONGS_EQUAL(27, weechat_relay_cmd_sync (&relay_session,
                                            "irc.freenode.#weechat", NULL));
    RELAY_CMD_EQUAL("sync irc.freenode.#weechat\n");

    LONGS_EQUAL(34, weechat_relay_cmd_sync (&relay_session,
                                            "irc.freenode.#weechat",
                                            "buffer"));
    RELAY_CMD_EQUAL("sync irc.freenode.#weechat buffer\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_desync
 */

TEST(LibCommand, CmdDesync)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_desync (NULL, NULL, NULL));

    LONGS_EQUAL(7, weechat_relay_cmd_desync (&relay_session, NULL, NULL));
    RELAY_CMD_EQUAL("desync\n");

    LONGS_EQUAL(29, weechat_relay_cmd_desync (&relay_session,
                                              "irc.freenode.#weechat", NULL));
    RELAY_CMD_EQUAL("desync irc.freenode.#weechat\n");

    LONGS_EQUAL(36, weechat_relay_cmd_desync (&relay_session,
                                              "irc.freenode.#weechat",
                                              "buffer"));
    RELAY_CMD_EQUAL("desync irc.freenode.#weechat buffer\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_test
 */

TEST(LibCommand, CmdTest)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_test (NULL));

    LONGS_EQUAL(5, weechat_relay_cmd_test (&relay_session));
    RELAY_CMD_EQUAL("test\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_ping
 */

TEST(LibCommand, CmdPing)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_ping (NULL, NULL));

    LONGS_EQUAL(5, weechat_relay_cmd_ping (&relay_session, NULL));
    RELAY_CMD_EQUAL("ping\n");

    LONGS_EQUAL(15, weechat_relay_cmd_ping (&relay_session, "test args"));
    RELAY_CMD_EQUAL("ping test args\n");
}

/*
 * Tests functions:
 *   weechat_relay_cmd_quit
 */

TEST(LibCommand, CmdQuit)
{
    LONGS_EQUAL(-1, weechat_relay_cmd_quit (NULL));

    LONGS_EQUAL(5, weechat_relay_cmd_quit (&relay_session));
    RELAY_CMD_EQUAL("quit\n");
}
