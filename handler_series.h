#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_str_t handle_series(ngx_http_request_t *r)
{
  ngx_str_t result_body;

  result_body.data = ngx_pcalloc(r->pool, 256);
  strcpy(result_body.data, "['sin4', 'sin5', 'sin6', 'sin7', 'sin8', 'sin9', 'sin10']");
  result_body.len = strlen(result_body.data);

  return result_body;
}
