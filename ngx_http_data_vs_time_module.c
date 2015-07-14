#include <string.h>
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include "handler_series.h"
#include "handler_values.h"


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
