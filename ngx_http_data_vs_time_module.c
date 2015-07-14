#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "handler_series.h"
#include "handler_values.h"

#define MAX_N_VALUES 10000

static ngx_str_t handle_values(ngx_http_request_t *r)
{
  ngx_str_t result_body;
  char* args;
  char* arg;
  char* vname;
  char* vvalue;

  char tmp_str[32];
  char* series;
  int64_t start;
  int64_t stop;
  int step;

  double period_seconds;
  double d;
  int64_t current;
  int n;

  series = NULL;
  start = -1;
  stop = -1;
  step = -1;

  result_body.data = NULL;
  result_body.len = 0;

  args = ngx_palloc(r->pool, r->args.len + 1);
  if (!args) {
    return result_body;
  }
  strncpy(args, r->args.data, r->args.len);
  args[r->args.len] = '\0';

  while (args != NULL)
  {
     arg = strsep(&args, "&");

     vname = strsep(&arg, "=");
     vvalue = arg;

     if (strncmp(vname, "start", sizeof("start") - 1) == 0)
     {
       start = strtoull(vvalue, (char **)NULL, 10);
     }
     else if (strncmp(vname, "stop", sizeof("stop") - 1) == 0)
     {
       stop = strtoull(vvalue, (char **)NULL, 10);
     }
     else if (strncmp(vname, "step", sizeof("step") - 1) == 0)
     {
       step = atoi(vvalue);
     }
     else if (strncmp(vname, "series", sizeof("series") - 1) == 0)
     {
       series = vvalue;
     }
     else
     {
       return result_body;
     }
  }

  if (series == NULL || start < 0 || stop < 0 || step <= 0)
  {
    return result_body;
  }

  n = (int)(stop - start) / step;
  if (n < 1 || n > MAX_N_VALUES)
  {
    return result_body;
  }

  if (strncmp(series, "SIN", sizeof("SIN")-1) != 0)
  {
    return result_body;
  }

  series = series + 3;

  period_seconds = atoi(series);

  result_body.data = ngx_palloc(r->pool, 10 * n + 20); // oodles.
  strcpy(result_body.data, "[");

  for (current = start; current<stop; current += step)
  {
    if (current != start)
    {
      strcat(result_body.data, ",");
    }

    d = sin( (double)current/1000.0 * M_2_PI / period_seconds );
    snprintf(tmp_str, sizeof(tmp_str), "%.4f", d);
    strcat(result_body.data, tmp_str);
  }

  strcat(result_body.data, "]");

  //result_body.data = ngx_palloc(r->pool, 1024);
  //sprintf(result_body.data, "s:%s:%d:%d:%d:\n", series, start, stop, step);
  result_body.len = strlen(result_body.data);

  return result_body;
}


static char *ngx_http_data_vs_time(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static ngx_command_t  ngx_http_data_vs_time_commands[] = {

  { ngx_string("data_vs_time"),
    NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
    ngx_http_data_vs_time,
    0,
    0,
    NULL },

    ngx_null_command
};


static ngx_http_module_t  ngx_http_data_vs_time_module_ctx = {
  NULL,                          /* preconfiguration */
  NULL,                          /* postconfiguration */
  NULL,                          /* create main configuration */
  NULL,                          /* init main configuration */
  NULL,                          /* create server configuration */
  NULL,                          /* merge server configuration */
  NULL,                          /* create location configuration */
  NULL                           /* merge location configuration */
};


ngx_module_t ngx_http_data_vs_time_module = {
  NGX_MODULE_V1,
  &ngx_http_data_vs_time_module_ctx, /* module context */
  ngx_http_data_vs_time_commands,   /* module directives */
  NGX_HTTP_MODULE,               /* module type */
  NULL,                          /* init master */
  NULL,                          /* init module */
  NULL,                          /* init process */
  NULL,                          /* init thread */
  NULL,                          /* exit thread */
  NULL,                          /* exit process */
  NULL,                          /* exit master */
  NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_data_vs_time_handler(ngx_http_request_t *r)
{
  ngx_buf_t    *b;
  ngx_chain_t   out;
  ngx_str_t    result_body;

  if (strncmp(r->uri.data, "/api/v1/series", sizeof("/api/v1/series")-1) == 0)
  {
    result_body = handle_series(r);
  }
  else if (strncmp(r->uri.data, "/api/v1/values", sizeof("/api/v1/values")-1) == 0)
  {
    result_body = handle_values(r);
  }
  else
  {
    return NGX_DECLINED;
  }

  if (result_body.len == 0)
  {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  r->headers_out.content_type.len = sizeof("application/json; charset=utf-8") - 1;
  r->headers_out.content_type.data = (u_char *) "application/json; charset=utf-8";

  b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

  out.buf = b;
  out.next = NULL;

  b->pos = result_body.data;
  b->last = result_body.data + result_body.len;
  b->memory = 1;
  b->last_buf = 1;

  r->headers_out.status = NGX_HTTP_OK;
  r->headers_out.content_length_n = result_body.len;
  ngx_http_send_header(r);

  return ngx_http_output_filter(r, &out);
}


static char *ngx_http_data_vs_time(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
  ngx_http_core_loc_conf_t  *clcf;

  clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  clcf->handler = ngx_http_data_vs_time_handler;

  return NGX_CONF_OK;
}
