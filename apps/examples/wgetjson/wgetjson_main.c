/****************************************************************************
 * examples/wgetjson/wgetjson_main.c
 *
 *   Copyright (C) 2012 Gregory Nutt. All rights reserved.
 *   Author: Darcy Gong
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <apps/netutils/uiplib.h>
#include <apps/netutils/webclient.h>
#include <apps/netutils/cJSON.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_WGETJSON_MAXSIZE
# define  CONFIG_EXAMPLES_WGETJSON_MAXSIZE 1024
#endif

#ifndef CONFIG_EXAMPLES_EXAMPLES_WGETJSON_URL
# define  CONFIG_EXAMPLES_EXAMPLES_WGETJSON_URL "http://10.0.0.1/wgetjson/json_cmd.php"
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char *g_json_buff    = NULL;
static int   g_json_bufflen = 0;
static bool  g_has_json     = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wgetjson_callback
 ****************************************************************************/

static void wgetjson_callback(FAR char **buffer, int offset, int datend,
                              FAR int *buflen, FAR void *arg)
{
  int len = datend - offset,org=len;

  if (len <= 0)
    {
      return;
    }

  if (!g_json_buff)
    {
      g_json_buff = malloc(len + 1);
      memcpy(g_json_buff, &((*buffer)[offset]), len);
      g_json_buff[len] = 0;
      g_json_bufflen = len;
    }
  else
    {
      if (g_json_bufflen >= CONFIG_EXAMPLES_WGETJSON_MAXSIZE)
        {
          g_json_bufflen += org;
          return;
        }

      if (g_json_bufflen+len > CONFIG_EXAMPLES_WGETJSON_MAXSIZE)
        {
          len = CONFIG_EXAMPLES_WGETJSON_MAXSIZE - g_json_bufflen;
        }

      g_json_buff = realloc(g_json_buff, g_json_bufflen + len + 1);
      memcpy(&g_json_buff[g_json_bufflen-1], &((*buffer)[offset]), len);
      g_json_buff[g_json_bufflen + len] = 0;
      g_json_bufflen += org;
    }
}

/****************************************************************************
 * Name: wgetjson_json_release
 ****************************************************************************/

static void wgetjson_json_release(void)
{
  if (g_json_buff)
    {
      free(g_json_buff);
      g_json_buff = NULL;
    }

  g_json_bufflen = 0;
}

/****************************************************************************
 * Name: wgetjson_doit
 ****************************************************************************/

#if 0 /* Not used */
static void wgetjson_doit(char *text)
{
  char *out;
  cJSON *json;

  json = cJSON_Parse(text);
  if (!json)
    {
      printf("Error before: [%s]\n",cJSON_GetErrorPtr());
    }
  else
    {
      out = cJSON_Print(json);
      cJSON_Delete(json);
      printf("%s\n", out);
      free(out);
    }
}
#endif

/****************************************************************************
 * Name: wgetjson_json_item_callback
 ****************************************************************************/

static int wgetjson_json_item_callback(const char *name,int type,cJSON *item)
{
  if (strlen(name) > 8 && !memcmp(name, "/(null)", 7))
    {
      name += 8;
      g_has_json = true;
    }

  if (!strcmp(name, "name"))
    {
      printf("name:\t\t\t%s \n", item->valuestring);
      // todo something....
    }
  else if (strcmp(name, "format/type")==0)
    {
      printf("format/type:\t\t%s \n", item->valuestring);
      // todo something....
    }
  else if (!strcmp(name, "format/width"))
    {
      printf("format/width:\t\t%d \n", item->valueint);
      // todo something....
    }
  else if (!strcmp(name, "format/height"))
    {
      printf("format/height:\t\t%d \n", item->valueint);
      // todo something....
    }
  else if (!strcmp(name, "format/interlace"))
    {
      printf("format/interlace:\t%s \n", (item->valueint) ? "true" : "false");
      // todo something....
    }
  else if (!strcmp(name, "format/frame rate"))
    {
      printf("format/frame rate:\t%d \n", item->valueint);
      // todo something....
    }

  return 1;
}

/****************************************************************************
 * Name: wgetjson_json_item_scan
 ****************************************************************************/

static void wgetjson_json_item_scan(cJSON *item, const char *prefix)
{
  char *newprefix;
  int dorecurse;

  while (item)
    {
      newprefix = malloc(strlen(prefix) + strlen(item->string) + 2);
      sprintf(newprefix, "%s/%s", prefix, item->string);

      dorecurse = wgetjson_json_item_callback(newprefix, item->type, item);
      if (item->child && dorecurse)
        {
          wgetjson_json_item_scan(item->child, newprefix);
        }

      item = item->next;
      free(newprefix);
    }
}

/****************************************************************************
 * Name: wgetjson_json_parse
 ****************************************************************************/

static int wgetjson_json_parse(char *text)
{
  cJSON *json;
  char *path = "";

  json=cJSON_Parse(text);
  if (!json)
    {
      printf("Error before: [%s]\n", cJSON_GetErrorPtr());
      return ERROR;
    }
  else
    {
      wgetjson_json_item_scan(json, path);
      cJSON_Delete(json);
      return OK;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wgetjson_main
 ****************************************************************************/

int wgetjson_main(int argc, char *argv[])
{
  char *buffer = NULL;
  char *url = CONFIG_EXAMPLES_EXAMPLES_WGETJSON_URL;
  int ret;

  buffer = malloc(512);
  wgetjson_json_release();

  printf("URL: %s\n", url);

  ret = wget(url, buffer, 512, wgetjson_callback, NULL);
  if (ret < 0)
    {
      printf("get json size: %d\n",g_json_bufflen);
    }
  else
    {
      g_has_json = false;
      if (wgetjson_json_parse(g_json_buff) == OK && g_has_json)
        {
          printf("Parse OK\n");
        }
      else
        {
          printf("Parse error\n");
        }

      g_has_json = false;
    }

  wgetjson_json_release();
  free(buffer);
  return 0;
}
