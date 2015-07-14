#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

static ngx_str_t handle_values(ngx_http_request_t *r)
{
  ngx_str_t result_body;

  result_body.data = ngx_pcalloc(r->pool, 1024);
  strcpy(result_body.data, "handling values\n");
  result_body.len = strlen(result_body.data);

  return result_body;
}
