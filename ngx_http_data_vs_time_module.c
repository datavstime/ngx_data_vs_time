#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

//typedef ngx_keyvalue_t[] dvt_ts_identity_t

//typedef struct dvt_key_value_pair_s {
//  ngx_string key;
//  ngx_value key;
//} dvt_ts_identity_t;

/*
ping_time_ms{level="320"}
sin{period="1",period="4",scale="medium"}
random_wave{period="4",scale="medium"}
mixed_wave{scale="long"}

timeseries {}
{
  ping_ms: {
    level: {
      4: 1
    }
  }
}
*/

#define MAX_N_VALUES 10000

typedef struct functionObject_s
{
  void *data;
  double (*fn)(struct functionObject_s* d, int64_t t, int64_t step);
} functionObject_t;


// The murmur3 32bit hash function by Austin Appleby which is used as a
// really fast pseudo random number generator.

#define FORCE_INLINE __attribute__((always_inline))

uint32_t fmix ( uint32_t h )
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

uint32_t getblock ( const uint32_t * p, int i )
{
  return p[i];
}

inline uint32_t rotl32 ( uint32_t x, int8_t r )
{
  return (x << r) | (x >> (32 - r));
}
#define ROTL32(x,y)     rotl32(x,y)

uint32_t MurmurHash3_x86_32 ( const void * key, int len, uint32_t seed )
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 4;

  uint32_t h1 = seed;

  uint32_t c1 = 0xcc9e2d51;
  uint32_t c2 = 0x1b873593;

  const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);

  int i;
  for(i = -nblocks; i; i++)
  {
    uint32_t k1 = getblock(blocks,i);

    k1 *= c1;
    k1 = ROTL32(k1,15);
    k1 *= c2;

    h1 ^= k1;
    h1 = ROTL32(h1,13);
    h1 = h1*5+0xe6546b64;
  }

  const uint8_t * tail = (const uint8_t*)(data + nblocks*4);

  uint32_t k1 = 0;

  switch(len & 3)
  {
  case 3: k1 ^= tail[2] << 16;
  case 2: k1 ^= tail[1] << 8;
  case 1: k1 ^= tail[0];
          k1 *= c1; k1 = ROTL32(k1,15); k1 *= c2; h1 ^= k1;
  };

  h1 ^= len;

  h1 = fmix(h1);

  return h1;
}

double uniform_rand_01(int64_t t)
{
  return ((double)MurmurHash3_x86_32(&t,sizeof(int64_t),0) / (double)UINT_MAX);
}

// --- END Murmur3


static double pingValueCreator(functionObject_t* fo, int64_t t, int64_t step)
{
  double lvl = *((double *)(fo->data));
  double rnd = uniform_rand_01(t/15000 + (int)lvl);
  if (rnd < 0.95) {
    return rnd * (1 + lvl / 10) + lvl;
  }
  if (rnd < 0.98) {
    return (rnd - 0.95) * 20.0 * 200 + 800;
  }
  return 5000;
}

//sin{period="2", scale="small"}
static double sinValueCreator(functionObject_t* fo, int64_t t, int64_t step)
{
  double period_seconds = *((double *)(fo->data));
  return sin((double)t/1000.0*M_2_PI/period_seconds);
}


static double rsValueCreator(functionObject_t* fo, int64_t t, int64_t step)
{
  double period_seconds = *((double *)(fo->data));
  double rnd = uniform_rand_01(t + (int)period_seconds);
  return sin((double)t/1000.0*M_2_PI/period_seconds) + rnd * 0.1 - 0.05;
}

static double mixValueCreator(functionObject_t* fo, int64_t t, int64_t step)
{
  return
    10.0 +
    sin((double)t/1000.0*M_2_PI/100.0) +
    sin((double)t/1000.0*M_2_PI/802.0)*2.0 +
    sin((double)t/1000.0*M_2_PI/7)*0.02 +
    sin((double)t/1000.0*M_2_PI/9)*0.05;
}

static double wtValueCreator(functionObject_t* fo, int64_t t, int64_t step)
{
  uint32_t level = ((uint32_t *)(fo->data))[0];
  uint32_t period_10m = ((uint32_t *)(fo->data))[1] * 60 * 10 * 1000;

  int64_t p1 = (int64_t) ((uniform_rand_01(t/(int64_t)period_10m) / 2.0) * period_10m + (t/(int64_t)period_10m)*period_10m);
  int64_t p1_a = p1 - 1000*20;

  int64_t p2 = (int64_t) ((uniform_rand_01(t/(int64_t)period_10m+1) / 2.0 + 0.5) * period_10m + (t/(int64_t)period_10m)*period_10m);
  int64_t p2_b = p2 + 1000*20;

  if (t >= p1 && t <= p2) {
    // implement a bit of (resolution dependent!) smoothing to make it look better.
    double v1 = (1 + uniform_rand_01(t) * 0.04);
    double v2 = (1 + uniform_rand_01(t-step) * 0.04);
    double v3 = (1 + uniform_rand_01(t-2*step) * 0.04);
    double v4 = (1 + uniform_rand_01(t-3*step) * 0.04);
    return (v1+v2+v3+v4) * level / 4;
  }

  if (t > p1_a && t < p1) {
    return (double)(t - p1_a) / (double)(p1 - p1_a) * level;
  }

  if (t > p2 && t < p2_b) {
    return (double)(p2_b - t) / (double)(p2_b - p2) * level;
  }

  return 0.0;
}


static ngx_str_t values_handler(ngx_http_request_t *r)
{
  ngx_str_t result_body;
  char tmp_str[32];

  char* series = NULL;
  int64_t start = -1;
  int64_t stop = -1;
  int64_t step = -1;

  ngx_keyval_t identity;

  ngx_str_set(identity.key, "test");
  ngx_str_set(identity.value, "world");

  fprintf(stderr, "key: %s\n", identity.key.data);
  fprintf(stderr, "val: %s\n", identity.value.data);
  fflush(stderr);

  result_body.data = NULL;
  result_body.len = 0;

  char* args = ngx_palloc(r->pool, r->args.len + 1);
  if (!args) {
    return result_body;
  }
  strncpy(args, (char *)r->args.data, r->args.len);
  args[r->args.len] = '\0';

  while (args != NULL)
  {
     char* arg = strsep(&args, "&");

     char* vname = strsep(&arg, "=");
     char* vvalue = arg;

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
       step = strtoull(vvalue, (char **)NULL, 10);
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

  int n = (int)((stop - start) / step);
  if (n < 1 || n > MAX_N_VALUES)
  {
    return result_body;
  }

  functionObject_t fo;
  if (strncmp(series, "SIN", sizeof("SIN")-1) == 0)
  {
    series = series + 3;
    double period_seconds = (double)strtol(series, (char **)NULL, 10);
    fo.data = &period_seconds;
    fo.fn = &sinValueCreator;
  }
  else if (strncmp(series, "ping", sizeof("ping")-1) == 0)
  {
    series = series + 4;
    double d = (double)strtol(series, (char **)NULL, 10);
    fo.data = &d;
    fo.fn = &pingValueCreator;
  }
  else if (strncmp(series, "rs", sizeof("rs")-1) == 0)
  {
    series = series + 2;
    double period_seconds = (double)strtol(series, (char **)NULL, 10);
    fo.data = &period_seconds;
    fo.fn = &rsValueCreator;
  }
  else if (strncmp(series, "mix", sizeof("mix")-1) == 0)
  {
    series = series + 3;
    int num = (int)strtol(series, (char **)NULL, 10);
    fo.data = &num;
    fo.fn = &mixValueCreator;
  }
  else if (strncmp(series, "wt", sizeof("wt")-1) == 0)
  {
    series = series + 2;

    char* period = strsep(&series, "_");
    char* rate = series;

    uint32_t p_ = (uint32_t)strtol(period, (char **)NULL, 10);
    uint32_t r_ = (uint32_t)strtol(rate, (char **)NULL, 10);

    uint32_t* ars = (uint32_t *)ngx_palloc(r->pool, sizeof(uint32_t) * 2);
    ars[0] = p_;
    ars[1] = r_;

    fo.data = ars;
    fo.fn = &wtValueCreator;
  }
  else
  {
    return result_body;
  }

  result_body.data = ngx_palloc(r->pool, 10 * n + 20); // oodles of room.
  strcpy((char *)result_body.data, "[");

  int64_t current;
  for (current = start; current<stop; current += step)
  {
    if (current != start)
    {
      strcat((char *)result_body.data, ",");
    }

    double d = fo.fn(&fo, current, step);
    snprintf(tmp_str, sizeof(tmp_str), "%.4f", d);
    strcat((char *)result_body.data, tmp_str);
  }

  strcat((char *)result_body.data, "]");

  result_body.len = strlen((char *)result_body.data);

  return result_body;
}

static ngx_str_t series_handler(ngx_http_request_t *r)
{
  ngx_str_t result_body;

  result_body.data = ngx_pcalloc(r->pool, 2048);
  strcpy((char *)result_body.data, "[\"SIN4\",\"SIN5\",\"SIN7\",\"SIN9\",\"SIN17\",\"SIN23\",\"SIN29\",\"SIN36\",\"SIN95\",\"SIN113\",\"SIN198\",\"SIN207\",\"ping4\",\"ping27\",\"ping120\",\"ping130\",\"ping180\",\"ping220\",\"ping320\",\"ping500\",\"rs5\",\"rs7\",\"rs10\",\"rs25\",\"rs42\",\"rs67\",\"rs133\",\"rs145\",\"rs168\",\"rs220\",\"rs265\",\"rs310\",\"rs340\",\"rs387\",\"rs412\",\"rs444\",\"rs502\",\"rs550\",\"rs599\",\"rs680\",\"rs850\",\"mix1\",\"wt300_4\",\"wt100_10\",\"wt237_7\",\"wt310_5\",\"wt290_11\",\"wt276_6\",\"wt302_3\",\"wt_288_8\"]");
  result_body.len = strlen((char *)result_body.data);

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

  if (strncmp((char *)r->uri.data, "/api/v1/series", sizeof("/api/v1/series")-1) == 0)
  {
    result_body = series_handler(r);
  }
  else if (strncmp((char *)r->uri.data, "/api/v1/values", sizeof("/api/v1/values")-1) == 0)
  {
    result_body = values_handler(r);
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
